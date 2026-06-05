//
// nan_font.cpp — NanFont 实现
//
// 所有 FreeType + HarfBuzz 类型隔离于此，不泄漏至模块接口。
//

module;

#include <thorvg-1/thorvg.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

module nandina.text.nan_font;

import nandina.log;
import nandina.foundation.color;

namespace nandina::text {

namespace {

    // ═══════════════════════════════════════════════════════════════
    // 工具函数
    // ═══════════════════════════════════════════════════════════════

    inline auto pt_to_pixel(float pt) noexcept -> float {
        return pt * 96.0f / 72.0f;
    }

    inline auto pixel_to_ft_26_6(float px) noexcept -> int {
        return static_cast<int>(px * 64.0f + 0.5f);
    }

    inline auto path_exists(std::string_view path) -> bool {
        std::error_code ec;
        return std::filesystem::exists(path, ec) && !ec;
    }

    // ═══════════════════════════════════════════════════════════════
    // FreeType 全局库句柄（懒加载，线程安全）
    // ═══════════════════════════════════════════════════════════════

    struct GlobalFT {
        FT_Library library{nullptr};
        std::mutex mutex;
        int ref_count{0};

        auto acquire() -> void {
            std::scoped_lock lock{mutex};
            if (ref_count == 0) {
                if (FT_Init_FreeType(&library) != FT_Err_Ok) {
                    throw std::runtime_error("FT_Init_FreeType failed");
                }
            }
            ++ref_count;
        }

        auto release() noexcept -> void {
            std::scoped_lock lock{mutex};
            --ref_count;
            if (ref_count == 0 && library != nullptr) {
                FT_Done_FreeType(library);
                library = nullptr;
            }
        }
    };

    auto global_ft() -> GlobalFT& {
        static GlobalFT instance;
        return instance;
    }

    // ═══════════════════════════════════════════════════════════════
    // 字体缓存
    // ═══════════════════════════════════════════════════════════════

    struct FontCacheKey {
        std::string path;
        int size_centipt{0};

        auto operator==(const FontCacheKey&) const noexcept -> bool = default;
    };

    struct FontCacheKeyHash {
        auto operator()(const FontCacheKey& key) const noexcept -> std::size_t {
            auto path_hash = std::hash<std::string>{}(key.path);
            auto size_hash = std::hash<int>{}(key.size_centipt);
            return path_hash ^ (size_hash + 0x9e3779b9u + (path_hash << 6u) + (path_hash >> 2u));
        }
    };

    struct FontCache {
        std::mutex mutex;
        std::unordered_map<FontCacheKey, std::weak_ptr<NanFont>, FontCacheKeyHash> entries;
    };

    auto font_cache() -> FontCache& {
        static FontCache cache;
        return cache;
    }

    auto font_cache_key(std::string_view path, float size_pt) -> FontCacheKey {
        return FontCacheKey{
            .path = std::string{path},
            .size_centipt = static_cast<int>(size_pt * 100.0f + (size_pt >= 0.0f ? 0.5f : -0.5f)),
        };
    }

    // ═══════════════════════════════════════════════════════════════
    // 系统字体查找
    // ═══════════════════════════════════════════════════════════════

    auto find_system_font_impl() -> std::string {
        static constexpr std::array<const char*, 14> preferred_fonts = {{
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/ubuntu/Ubuntu-R.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "/System/Library/Fonts/PingFang.ttc",
            "C:/Windows/Fonts/segoeui.ttf",
        }};

        for (const auto* path : preferred_fonts) {
            if (path_exists(path)) {
                return std::string{path};
            }
        }
        return {};
    }

    constexpr std::string_view k_ellipsis = "...";

    constexpr float k_layout_epsilon = 1e-4f;

} // namespace

// ═══════════════════════════════════════════════════════════════
// NanFont::Impl
// ═══════════════════════════════════════════════════════════════

struct FontStackEntry {
    FT_Face      ft_face{nullptr};
    hb_font_t*   hb_font{nullptr};
    std::string  path;
    float        size_pt{14.0f};
    float        pixel_height{0.0f};
    bool         colored{false};

    auto init_hb_font() -> void;
    auto cleanup() -> void;
};

void FontStackEntry::init_hb_font() {
    hb_font = hb_ft_font_create_referenced(ft_face);
    if (!hb_font) return;
    hb_font_set_scale(hb_font,
                      static_cast<int>(pixel_height * 64.0f + 0.5f),
                      static_cast<int>(pixel_height * 64.0f + 0.5f));
}

void FontStackEntry::cleanup() {
    if (hb_font) {
        hb_font_destroy(hb_font);
        hb_font = nullptr;
    }
    if (ft_face) {
        FT_Done_Face(ft_face);
        ft_face = nullptr;
    }
}

struct NanFont::Impl {
    std::vector<FontStackEntry> faces;
    std::vector<std::string> fallback_paths;
    float        size_pt{14.0f};
    float        pixel_height{0.0f};
    bool         ft_acquired{false};

    float ascent_cache{0.0f};
    float descent_cache{0.0f};
    float line_gap_cache{0.0f};

    mutable std::mutex load_mutex;
    bool loaded{false};

    ~Impl() {
        for (auto& entry : faces) {
            entry.cleanup();
        }
        if (ft_acquired) {
            global_ft().release();
            ft_acquired = false;
        }
    }

    auto init_from_face(FT_Face face, float size_pt_val) -> void;
    auto find_face_for_codepoint(unsigned int codepoint) -> std::size_t;
    auto ensure_fallbacks() -> void;

    static auto system_fallback_paths() -> std::vector<std::string>;
};

auto NanFont::Impl::system_fallback_paths() -> std::vector<std::string> {
    std::vector<std::string> paths;

    // Linux
    paths.push_back("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf");
    paths.push_back("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc");
    paths.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    paths.push_back("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf");
    paths.push_back("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    paths.push_back("/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc");
    paths.push_back("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf");
    paths.push_back("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");

    // macOS
    paths.push_back("/System/Library/Fonts/Apple Color Emoji.ttc");
    paths.push_back("/System/Library/Fonts/Supplemental/Arial Unicode.ttf");
    paths.push_back("/System/Library/Fonts/Helvetica.ttc");

    // Windows
    paths.push_back("C:/Windows/Fonts/seguiemj.ttf");
    paths.push_back("C:/Windows/Fonts/seguisym.ttf");
    paths.push_back("C:/Windows/Fonts/msyh.ttc");
    paths.push_back("C:/Windows/Fonts/simhei.ttf");

    return paths;
}

auto NanFont::Impl::init_from_face(FT_Face face, float size_pt_val) -> void {
    FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(size_pt_val * 64.0f + 0.5f), 0, 96);

    FontStackEntry entry;
    entry.ft_face    = face;
    entry.path       = {};
    entry.size_pt    = size_pt_val;
    entry.pixel_height = pt_to_pixel(size_pt_val);
    entry.init_hb_font();

    const auto& metrics = face->size->metrics;
    ascent_cache  = static_cast<float>(metrics.ascender)  / 64.0f;
    descent_cache = static_cast<float>(-metrics.descender) / 64.0f;
    const float raw_gap = static_cast<float>(metrics.height - metrics.ascender + metrics.descender) / 64.0f;
    line_gap_cache = (raw_gap > 0.0f) ? raw_gap : 0.0f;

    faces.push_back(std::move(entry));
}

auto NanFont::Impl::find_face_for_codepoint(unsigned int codepoint) -> std::size_t {
    if (faces.empty()) return 0;
    if (codepoint == ' ' || codepoint == '\t' || codepoint == 0) return 0;

    // 先检查已加载的面
    for (std::size_t i = 0; i < faces.size(); ++i) {
        if (FT_Get_Char_Index(faces[i].ft_face, codepoint) != 0) {
            return i;
        }
    }

    // 未找到，尝试惰性加载 fallback
    if (!fallback_paths.empty()) {
        ensure_fallbacks();
        for (std::size_t i = 0; i < faces.size(); ++i) {
            if (FT_Get_Char_Index(faces[i].ft_face, codepoint) != 0) {
                return i;
            }
        }
    }

    // 都没有，回退到主面
    return 0;
}

auto NanFont::Impl::ensure_fallbacks() -> void {
    for (const auto& fb_path : fallback_paths) {
        if (fb_path.empty()) continue;

        // 跳过已加载的路径
        bool already = false;
        for (const auto& entry : faces) {
            if (entry.path == fb_path) { already = true; break; }
        }
        if (already) continue;

        FT_Face fb_face = nullptr;
        if (FT_New_Face(global_ft().library, fb_path.c_str(), 0, &fb_face) != FT_Err_Ok || !fb_face) {
            continue;
        }

        FontStackEntry entry;
        entry.ft_face    = fb_face;
        entry.path       = fb_path;
        entry.size_pt    = size_pt;
        entry.pixel_height = pt_to_pixel(size_pt);
        entry.colored    = false;
        entry.init_hb_font();

        faces.push_back(std::move(entry));
    }
}

// ═══════════════════════════════════════════════════════════════
// NanFont — 构造 / 拷贝
// ═══════════════════════════════════════════════════════════════

NanFont::NanFont() : m_impl(std::make_shared<Impl>()) {}
NanFont::~NanFont() = default;

NanFont::NanFont(const NanFont& other)
    : m_impl(other.m_impl)
    , m_family(other.m_family)
    , m_size_pt(other.m_size_pt)
    , m_weight(other.m_weight)
    , m_color(other.m_color)
    , m_has_explicit_color(other.m_has_explicit_color)
    , m_line_height_override(other.m_line_height_override)
    , m_letter_spacing(other.m_letter_spacing)
    , m_overflow(other.m_overflow)
    , m_max_lines(other.m_max_lines)
    , m_single_line(other.m_single_line) {}

auto NanFont::operator=(const NanFont& other) -> NanFont& {
    if (this != &other) {
        m_impl                 = other.m_impl;
        m_family               = other.m_family;
        m_size_pt              = other.m_size_pt;
        m_weight               = other.m_weight;
        m_color                = other.m_color;
        m_has_explicit_color   = other.m_has_explicit_color;
        m_line_height_override = other.m_line_height_override;
        m_letter_spacing       = other.m_letter_spacing;
        m_overflow             = other.m_overflow;
        m_max_lines            = other.m_max_lines;
        m_single_line          = other.m_single_line;
    }
    return *this;
}

NanFont::NanFont(NanFont&&) noexcept = default;
auto NanFont::operator=(NanFont&&) noexcept -> NanFont& = default;

// ═══════════════════════════════════════════════════════════════
// NanFont — 工厂
// ═══════════════════════════════════════════════════════════════

auto NanFont::load_from_path(std::string_view file_path, float size_pt) -> NanFont {
    auto log = nandina::log::get("text.nan_font");

    if (size_pt <= 0.0f) {
        throw std::runtime_error(std::format("Invalid font size: {}", size_pt));
    }

    std::string path_str{file_path};
    std::error_code ec;
    if (!std::filesystem::exists(path_str, ec) || ec) {
        throw std::runtime_error(std::format("Font file not found: {}", path_str));
    }

    const auto cache_key = font_cache_key(path_str, size_pt);
    {
        auto& cache = font_cache();
        std::scoped_lock lock{cache.mutex};
        if (const auto it = cache.entries.find(cache_key); it != cache.entries.end()) {
            if (auto cached = it->second.lock()) {
                NanFont font;
                font.m_impl    = cached->m_impl;  // share Impl from cached font
                font.m_size_pt = size_pt;
                return font;
            }
            cache.entries.erase(it);
        }
    }

    log.debug("Loading font from \"{}\" size={:.1f}pt", path_str, size_pt);

    global_ft().acquire();

    FT_Face face = nullptr;
    const FT_Error error = FT_New_Face(global_ft().library, path_str.c_str(), 0, &face);
    if (error != FT_Err_Ok || !face) {
        global_ft().release();
        throw std::runtime_error(std::format("FT_New_Face failed for \"{}\": error={}", path_str, error));
    }

    NanFont font;
    font.m_size_pt = size_pt;
    font.m_impl->ft_acquired = true;

    try {
        font.m_impl->init_from_face(face, size_pt);
        font.m_impl->loaded = true;
    } catch (...) {
        FT_Done_Face(face);
        throw;
    }

    log.info("Font loaded: \"{}\" {:.1f}pt (pixel={:.2f})", path_str, size_pt, font.m_impl->pixel_height);

    {
        auto font_ptr = std::make_shared<NanFont>(std::move(font));
        auto& cache = font_cache();
        std::scoped_lock lock{cache.mutex};
        cache.entries[cache_key] = font_ptr;
        return *font_ptr;
    }
}

auto NanFont::load_from_memory(std::span<const std::byte> data, float size_pt) -> NanFont {
    auto log = nandina::log::get("text.nan_font");
    log.debug("Loading font from memory ({} bytes, {:.1f}pt)", data.size(), size_pt);

    if (size_pt <= 0.0f) {
        throw std::runtime_error(std::format("Invalid font size: {}", size_pt));
    }
    if (data.empty()) {
        throw std::runtime_error("Font data is empty");
    }

    global_ft().acquire();

    FT_Face face = nullptr;
    const FT_Error error = FT_New_Memory_Face(
        global_ft().library,
        reinterpret_cast<const FT_Byte*>(data.data()),
        static_cast<FT_Long>(data.size()),
        0, &face);
    if (error != FT_Err_Ok || !face) {
        global_ft().release();
        throw std::runtime_error(std::format("FT_New_Memory_Face failed: error={}", error));
    }

    NanFont font;
    font.m_size_pt = size_pt;
    font.m_impl->ft_acquired = true;

    try {
        font.m_impl->init_from_face(face, size_pt);
        font.m_impl->loaded = true;
    } catch (...) {
        FT_Done_Face(face);
        throw;
    }

    log.info("Font loaded from memory ({:.1f}pt)", size_pt);
    return font;
}

auto NanFont::load_system_default(float size_pt) -> Ptr {
    const auto path = find_system_font_impl();
    if (path.empty()) {
        throw std::runtime_error("No system font found");
    }
    auto font = load_from_path(path, size_pt);
    return std::make_shared<NanFont>(std::move(font));
}

auto NanFont::ensure_loaded() const -> void {
    if (m_impl->loaded) return;

    std::scoped_lock lock{m_impl->load_mutex};
    if (m_impl->loaded) return;

    auto default_font = load_system_default(m_size_pt);
    const_cast<NanFont*>(this)->m_impl = default_font->m_impl;
    m_impl->fallback_paths = Impl::system_fallback_paths();
}

auto NanFont::is_loaded() const noexcept -> bool {
    return m_impl && m_impl->loaded;
}

// ═══════════════════════════════════════════════════════════════
// NanFont — 链式配置
// ═══════════════════════════════════════════════════════════════

auto NanFont::family(std::string name) -> NanFont& {
    m_family = std::move(name);
    return *this;
}

auto NanFont::size(float pt) -> NanFont& {
    m_size_pt = pt;
    return *this;
}

auto NanFont::weight(NanFontWeight w) -> NanFont& {
    m_weight = w;
    return *this;
}

auto NanFont::color(NanColor c) -> NanFont& {
    m_has_explicit_color = true;
    m_color = std::move(c);
    return *this;
}

auto NanFont::line_height(float lh) -> NanFont& {
    m_line_height_override = lh;
    return *this;
}

auto NanFont::letter_spacing(float ls) -> NanFont& {
    m_letter_spacing = ls;
    return *this;
}

auto NanFont::overflow(TextOverflow o) -> NanFont& {
    m_overflow = o;
    return *this;
}

auto NanFont::max_lines(int n) -> NanFont& {
    m_max_lines = n;
    return *this;
}

auto NanFont::single_line(bool s) -> NanFont& {
    m_single_line = s;
    return *this;
}

// ═══════════════════════════════════════════════════════════════
// NanFont — 访问器
// ═══════════════════════════════════════════════════════════════

auto NanFont::family() const noexcept -> const std::string& { return m_family; }
auto NanFont::size() const noexcept -> float { return m_size_pt; }
auto NanFont::weight() const noexcept -> NanFontWeight { return m_weight; }
auto NanFont::color() const noexcept -> const NanColor& { return m_color; }
auto NanFont::has_explicit_color() const noexcept -> bool { return m_has_explicit_color; }
auto NanFont::letter_spacing() const noexcept -> float { return m_letter_spacing; }
auto NanFont::overflow() const noexcept -> TextOverflow { return m_overflow; }
auto NanFont::max_lines() const noexcept -> int { return m_max_lines; }
auto NanFont::single_line() const noexcept -> bool { return m_single_line; }

auto NanFont::line_height() const noexcept -> float {
    if (m_line_height_override > 0.0f) return m_line_height_override;
    if (m_impl && m_impl->loaded) {
        return m_impl->ascent_cache + m_impl->descent_cache + m_impl->line_gap_cache;
    }
    return m_size_pt * 1.4f;
}

// ═══════════════════════════════════════════════════════════════
// NanFont — 度量
// ═══════════════════════════════════════════════════════════════

auto NanFont::ascent() const noexcept -> float {
    ensure_loaded();
    return m_impl ? m_impl->ascent_cache : 0.0f;
}

auto NanFont::descent() const noexcept -> float {
    ensure_loaded();
    return m_impl ? m_impl->descent_cache : 0.0f;
}

auto NanFont::em_size() const noexcept -> float {
    ensure_loaded();
    return m_impl ? m_impl->pixel_height : 0.0f;
}

// ═══════════════════════════════════════════════════════════════
// NanFont — Glyph 度量
// ═══════════════════════════════════════════════════════════════

auto NanFont::glyph_advance(std::uint32_t codepoint) const noexcept -> float {
    ensure_loaded();
    if (!m_impl || m_impl->faces.empty()) return 0.0f;

    const auto face_idx = m_impl->find_face_for_codepoint(codepoint);
    auto& face = m_impl->faces[face_idx];
    const auto glyph_index = FT_Get_Char_Index(face.ft_face, codepoint);
    if (glyph_index == 0) return 0.0f;
    if (FT_Load_Glyph(face.ft_face, glyph_index, FT_LOAD_DEFAULT) != FT_Err_Ok) return 0.0f;

    return static_cast<float>(face.ft_face->glyph->advance.x) / 64.0f;
}

auto NanFont::estimate_text_width(std::string_view text) const noexcept -> float {
    if (text.empty()) {
        return 0.0f;
    }

    return shape(text, 0.0f).total_width;
}

// ═══════════════════════════════════════════════════════════════
// Shaping helpers
// ═══════════════════════════════════════════════════════════════

namespace {

auto shape_single_segment(hb_font_t* hb_font,
                          std::string_view segment) -> std::vector<GlyphInfo> {
    std::vector<GlyphInfo> result;
    if (segment.empty() || !hb_font) return result;

    auto* buffer = hb_buffer_create();
    if (!buffer) return result;

    hb_buffer_add_utf8(buffer, segment.data(), static_cast<int>(segment.size()), 0,
                       static_cast<int>(segment.size()));
    hb_buffer_guess_segment_properties(buffer);
    hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer, HB_SCRIPT_COMMON);
    hb_buffer_set_language(buffer, hb_language_from_string("en", -1));

    hb_shape(hb_font, buffer, nullptr, 0);

    unsigned int glyph_count = 0;
    auto* glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_count);
    auto* glyph_pos  = hb_buffer_get_glyph_positions(buffer, &glyph_count);

    result.reserve(glyph_count);
    for (unsigned int i = 0; i < glyph_count; ++i) {
        // HarfBuzz cluster 值 = 原始 UTF-8 字节偏移
        const auto cluster = glyph_info[i].cluster;
        std::uint32_t original_cp = 0;
        if (cluster < segment.size()) {
            // 从原始文本解码该位置的 Unicode 码点
            auto lead = static_cast<unsigned char>(segment[cluster]);
            if (lead < 0x80) {
                original_cp = lead;
            } else if ((lead & 0xE0) == 0xC0 && cluster + 1 < segment.size()) {
                original_cp = ((lead & 0x1Fu) << 6u) |
                              (static_cast<unsigned char>(segment[cluster + 1]) & 0x3Fu);
            } else if ((lead & 0xF0) == 0xE0 && cluster + 2 < segment.size()) {
                original_cp = ((lead & 0x0Fu) << 12u) |
                              ((static_cast<unsigned char>(segment[cluster + 1]) & 0x3Fu) << 6u) |
                              (static_cast<unsigned char>(segment[cluster + 2]) & 0x3Fu);
            } else if ((lead & 0xF8) == 0xF0 && cluster + 3 < segment.size()) {
                original_cp = ((lead & 0x07u) << 18u) |
                              ((static_cast<unsigned char>(segment[cluster + 1]) & 0x3Fu) << 12u) |
                              ((static_cast<unsigned char>(segment[cluster + 2]) & 0x3Fu) << 6u) |
                              (static_cast<unsigned char>(segment[cluster + 3]) & 0x3Fu);
            }
        }

        result.push_back(GlyphInfo{
            .glyph_index  = glyph_info[i].codepoint,
            .original_cp  = original_cp,
            .advance_x    = static_cast<float>(glyph_pos[i].x_advance) / 64.0f,
            .advance_y    = static_cast<float>(glyph_pos[i].y_advance) / 64.0f,
            .offset_x     = static_cast<float>(glyph_pos[i].x_offset)  / 64.0f,
            .offset_y     = static_cast<float>(glyph_pos[i].y_offset)  / 64.0f,
        });
    }

    hb_buffer_destroy(buffer);
    return result;
}

auto wrap_lines(std::vector<GlyphInfo>& glyphs,
                float max_width, float line_height,
                float ascent, int max_lines) -> std::vector<TextLine> {
    std::vector<TextLine> lines;

    std::size_t i = 0;
    while (i < glyphs.size()) {
        if (max_lines > 0 && static_cast<int>(lines.size()) >= max_lines) break;

        float line_w = 0.0f;
        std::size_t line_end = i;       // 本行结束位置（不含）
        std::size_t last_space = i;     // 最后一个空格之后的 glyph 索引
        float space_line_w = 0.0f;      // 到 last_space 为止的行宽

        // 扫描本行能容纳的 glyph
        for (std::size_t j = i; j < glyphs.size(); ++j) {
            float adv = glyphs[j].advance_x;

            if (j > i && line_w + adv > max_width + k_layout_epsilon) {
                if (last_space > i) {
                    line_end = last_space;
                    line_w   = space_line_w;
                } else {
                    line_end = j;
                }
                break;
            }

            line_w += adv;
            if (glyphs[j].original_cp == 0x0020) {
                // 空格后面的位置是下一个词的开始
                last_space   = j + 1;
                space_line_w = line_w;
            }
            line_end = j + 1;
        }

        // 提交当前行
        TextLine line;
        line.height   = line_height;
        line.baseline = ascent;
        for (std::size_t j = i; j < line_end && j < glyphs.size(); ++j) {
            line.glyphs.push_back(std::move(glyphs[j]));
        }
        line.width = line_w;
        lines.push_back(std::move(line));

        i = line_end;
    }

    return lines;
}

auto clip_glyphs(std::vector<GlyphInfo>& glyphs, float max_width) -> std::vector<GlyphInfo> {
    std::vector<GlyphInfo> result;
    float cursor_x = 0.0f;
    for (auto& g : glyphs) {
        if (cursor_x + g.advance_x > max_width + k_layout_epsilon) break;
        result.push_back(std::move(g));
        cursor_x += result.back().advance_x;
    }
    return result;
}

auto ellipsis_glyphs(std::vector<GlyphInfo>& glyphs,
                     float max_width, float line_height,
                     float ascent, hb_font_t* hb_font) -> TextLine {
    auto ellipsis_g = shape_single_segment(hb_font, k_ellipsis);
    float ellipsis_w = 0.0f;
    for (auto& g : ellipsis_g) ellipsis_w += g.advance_x;

    TextLine result;
    result.height   = line_height;
    result.baseline = ascent;
    float cursor_x = 0.0f;

    for (auto& g : glyphs) {
        if (cursor_x + g.advance_x + ellipsis_w > max_width + k_layout_epsilon) break;
        result.glyphs.push_back(std::move(g));
        cursor_x += result.glyphs.back().advance_x;
    }

    for (auto& g : ellipsis_g) {
        result.glyphs.push_back(std::move(g));
        cursor_x += result.glyphs.back().advance_x;
    }
    result.width = cursor_x;
    return result;
}

auto populate_line_ink_bounds(TextLine& line,
                              const std::vector<FontStackEntry>& faces) -> void {
    auto get_face_for_glyph = [&](const GlyphInfo& g) -> FT_Face {
        return g.face_index < faces.size() ? faces[g.face_index].ft_face : nullptr;
    };

    if (faces.empty()) {
        line.ink_left = 0.0f;
        line.ink_top = 0.0f;
        line.ink_right = line.width;
        line.ink_bottom = line.height;
        return;
    }

    float cursor_x = 0.0f;
    bool has_ink = false;
    float min_left = 0.0f;
    float min_top = 0.0f;
    float max_right = 0.0f;
    float max_bottom = 0.0f;

    for (const auto& glyph : line.glyphs) {
        const auto face = get_face_for_glyph(glyph);
        if (!face) { cursor_x += glyph.advance_x; continue; }
        if (FT_Load_Glyph(face, glyph.glyph_index, FT_LOAD_DEFAULT) != FT_Err_Ok) {
            cursor_x += glyph.advance_x;
            continue;
        }
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != FT_Err_Ok) {
            cursor_x += glyph.advance_x;
            continue;
        }

        const auto* bitmap = &face->glyph->bitmap;
        if (bitmap->buffer && bitmap->width > 0 && bitmap->rows > 0) {
            const float pen_x = cursor_x + glyph.offset_x + static_cast<float>(face->glyph->bitmap_left);
            const float pen_y = line.baseline - glyph.offset_y - static_cast<float>(face->glyph->bitmap_top);
            const float left = pen_x;
            const float top = pen_y;
            const float right = pen_x + static_cast<float>(bitmap->width);
            const float bottom = pen_y + static_cast<float>(bitmap->rows);

            if (!has_ink) {
                min_left = left;
                min_top = top;
                max_right = right;
                max_bottom = bottom;
                has_ink = true;
            } else {
                min_left = std::min(min_left, left);
                min_top = std::min(min_top, top);
                max_right = std::max(max_right, right);
                max_bottom = std::max(max_bottom, bottom);
            }
        }

        cursor_x += glyph.advance_x;
    }

    if (!has_ink) {
        line.ink_left = 0.0f;
        line.ink_top = 0.0f;
        line.ink_right = line.width;
        line.ink_bottom = line.height;
        return;
    }

    line.ink_left = min_left;
    line.ink_top = min_top;
    line.ink_right = max_right;
    line.ink_bottom = max_bottom;
}

} // namespace

// ═══════════════════════════════════════════════════════════════
// NanFont — Shaping
// ═══════════════════════════════════════════════════════════════

auto NanFont::shape_text(std::string_view text) const -> std::vector<std::vector<GlyphInfo>> {
    ensure_loaded();
    if (text.empty() || !m_impl || m_impl->faces.empty()) return {};

    const bool single = m_single_line;

    std::vector<std::string_view> segments;
    if (single) {
        segments.push_back(text);
    } else {
        std::size_t start = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\n') {
                segments.push_back(text.substr(start, i - start));
                start = i + 1;
            }
        }
        segments.push_back(text.substr(start));
    }

    auto decode_utf8_cp = [](const std::string_view& str, std::size_t& idx) -> unsigned int {
        const unsigned char lead = static_cast<unsigned char>(str[idx]);
        if (lead < 0x80) { ++idx; return lead; }
        if ((lead >> 5) == 0x6 && idx + 1 < str.size()) { idx += 2; return ((lead & 0x1F) << 6) | (static_cast<unsigned char>(str[idx - 1]) & 0x3F); }
        if ((lead >> 4) == 0xE && idx + 2 < str.size()) { idx += 3; return ((lead & 0x0F) << 12) | ((static_cast<unsigned char>(str[idx - 2]) & 0x3F) << 6) | (static_cast<unsigned char>(str[idx - 1]) & 0x3F); }
        if ((lead >> 3) == 0x1E && idx + 3 < str.size()) { idx += 4; return ((lead & 0x07) << 18) | ((static_cast<unsigned char>(str[idx - 3]) & 0x3F) << 12) | ((static_cast<unsigned char>(str[idx - 2]) & 0x3F) << 6) | (static_cast<unsigned char>(str[idx - 1]) & 0x3F); }
        ++idx; return '?';
    };

    std::vector<std::vector<GlyphInfo>> result;
    result.reserve(segments.size());

    for (const auto& segment : segments) {
        if (segment.empty()) {
            result.emplace_back();
            continue;
        }

        // Split segment into uniform-face runs, so each run is shaped
        // with the harfbuzz font that actually has the codepoints.
        struct TextRun { std::size_t face_idx; std::size_t byte_start; std::size_t byte_len; };
        std::vector<TextRun> runs;
        std::size_t cur_face = static_cast<std::size_t>(-1);
        std::size_t run_start = 0;
        std::size_t idx = 0;

        while (idx < segment.size()) {
            const std::size_t prev = idx;
            const unsigned int cp = decode_utf8_cp(segment, idx);
            const std::size_t face_idx = m_impl->find_face_for_codepoint(cp);

            if (cur_face == static_cast<std::size_t>(-1)) {
                cur_face = face_idx; run_start = prev;
            } else if (face_idx != cur_face) {
                runs.push_back({cur_face, run_start, prev - run_start});
                cur_face = face_idx; run_start = prev;
            }
        }
        if (cur_face != static_cast<std::size_t>(-1)) {
            runs.push_back({cur_face, run_start, idx - run_start});
        }

        std::vector<GlyphInfo> segment_glyphs;
        for (const auto& run : runs) {
            if (run.byte_len == 0) continue;
            auto run_text = segment.substr(run.byte_start, run.byte_len);
            auto glyphs = shape_single_segment(
                m_impl->faces[run.face_idx].hb_font, run_text);
            for (auto& g : glyphs) g.face_index = run.face_idx;
            segment_glyphs.insert(segment_glyphs.end(),
                std::make_move_iterator(glyphs.begin()),
                std::make_move_iterator(glyphs.end()));
        }

        if (m_letter_spacing != 0.0f) {
            for (auto& g : segment_glyphs) g.advance_x += m_letter_spacing;
        }

        result.push_back(std::move(segment_glyphs));
    }

    return result;
}

auto NanFont::layout_lines(const std::vector<std::vector<GlyphInfo>>& segments,
                           float max_width) const -> TextLayout {
    if (max_width < 0.0f) max_width = 0.0f;

    TextLayout result;
    if (segments.empty()) return result;

    const float lh     = line_height();
    const float asc    = ascent();
    const int   ml     = m_max_lines;
    const bool  single = m_single_line;

    for (const auto& segment : segments) {
        if (segment.empty()) {
            TextLine empty_line;
            empty_line.height   = lh;
            empty_line.baseline = asc;
            empty_line.width    = 0.0f;
            result.lines.push_back(std::move(empty_line));
            if (ml > 0 && static_cast<int>(result.lines.size()) >= ml) goto done;
            continue;
        }

        auto glyphs = segment;

        float segment_total_w = 0.0f;
        for (auto& g : glyphs) segment_total_w += g.advance_x;

        if (single && max_width > 0.0f && segment_total_w > max_width + k_layout_epsilon) {
            TextLine line;
            line.height   = lh;
            line.baseline = asc;
            line = ellipsis_glyphs(glyphs, max_width, lh, asc,
                                   m_impl->faces.empty() ? nullptr : m_impl->faces[0].hb_font);
            result.lines.push_back(std::move(line));
            if (ml > 0 && static_cast<int>(result.lines.size()) >= ml) goto done;
        } else if (max_width > 0.0f) {
            switch (m_overflow) {
            case TextOverflow::wrap: {
                auto wrapped = wrap_lines(glyphs, max_width, lh, asc, ml);
                for (auto& line : wrapped) {
                    result.lines.push_back(std::move(line));
                    if (ml > 0 && static_cast<int>(result.lines.size()) >= ml) goto done;
                }
                break;
            }
            case TextOverflow::clip: {
                auto clipped = clip_glyphs(glyphs, max_width);
                float w = 0.0f;
                for (auto& g : clipped) w += g.advance_x;
                TextLine line;
                line.glyphs   = std::move(clipped);
                line.width    = w;
                line.height   = lh;
                line.baseline = asc;
                result.lines.push_back(std::move(line));
                if (ml > 0 && static_cast<int>(result.lines.size()) >= ml) goto done;
                break;
            }
            case TextOverflow::ellipsis: {
                float total_w = 0.0f;
                for (auto& g : glyphs) total_w += g.advance_x;
                if (total_w <= max_width + k_layout_epsilon) {
                    TextLine line;
                    line.glyphs   = std::move(glyphs);
                    line.width    = total_w;
                    line.height   = lh;
                    line.baseline = asc;
                    result.lines.push_back(std::move(line));
                } else {
                    result.lines.push_back(ellipsis_glyphs(glyphs, max_width, lh, asc,
                        m_impl->faces.empty() ? nullptr : m_impl->faces[0].hb_font));
                }
                if (ml > 0 && static_cast<int>(result.lines.size()) >= ml) goto done;
                break;
            }
            case TextOverflow::scale: {
                auto clipped = clip_glyphs(glyphs, max_width);
                float w = 0.0f;
                for (auto& g : clipped) w += g.advance_x;
                TextLine line;
                line.glyphs   = std::move(clipped);
                line.width    = w;
                line.height   = lh;
                line.baseline = asc;
                result.lines.push_back(std::move(line));
                if (ml > 0 && static_cast<int>(result.lines.size()) >= ml) goto done;
                break;
            }
            }
        } else {
            float total_w = 0.0f;
            for (auto& g : glyphs) total_w += g.advance_x;
            TextLine line;
            line.glyphs   = std::move(glyphs);
            line.width    = total_w;
            line.height   = lh;
            line.baseline = asc;
            result.lines.push_back(std::move(line));
            if (ml > 0 && static_cast<int>(result.lines.size()) >= ml) goto done;
        }
    }

done:
    float line_origin_y = 0.0f;
    bool has_ink = false;
    for (const auto& line : result.lines) {
        result.total_height += line.height;
        result.total_width = std::max(result.total_width, line.width);
    }

    for (auto& line : result.lines) {
        populate_line_ink_bounds(line, m_impl->faces);

        const float line_left = line.ink_left;
        const float line_top = line_origin_y + line.ink_top;
        const float line_right = line.ink_right;
        const float line_bottom = line_origin_y + line.ink_bottom;

        if (!has_ink) {
            result.ink_left = line_left;
            result.ink_top = line_top;
            result.ink_right = line_right;
            result.ink_bottom = line_bottom;
            has_ink = true;
        } else {
            result.ink_left = std::min(result.ink_left, line_left);
            result.ink_top = std::min(result.ink_top, line_top);
            result.ink_right = std::max(result.ink_right, line_right);
            result.ink_bottom = std::max(result.ink_bottom, line_bottom);
        }

        line_origin_y += line.height;
    }

    if (!has_ink) {
        result.ink_left = 0.0f;
        result.ink_top = 0.0f;
        result.ink_right = result.total_width;
        result.ink_bottom = result.total_height;
    }
    return result;
}

auto NanFont::shape(std::string_view text, float max_width) const -> TextLayout {
    auto segments = shape_text(text);
    return layout_lines(segments, max_width);
}

// ═══════════════════════════════════════════════════════════════
// NanFont — 绘制
// ═══════════════════════════════════════════════════════════════

auto NanFont::paint(tvg::SwCanvas& canvas,
                    const TextLayout& layout,
                    float origin_x,
                    float origin_y) const -> void {
    ensure_loaded();
    if (!m_impl || m_impl->faces.empty() || layout.empty()) return;

    const auto rgb = m_color.to<nandina::NanRgb>();
    const auto r = rgb.red();
    const auto g = rgb.green();
    const auto b = rgb.blue();
    const auto a = rgb.alpha();

    auto get_face_for_glyph = [&](const GlyphInfo& glyph) -> FT_Face {
        return glyph.face_index < m_impl->faces.size()
                   ? m_impl->faces[glyph.face_index].ft_face
                   : nullptr;
    };

    float line_y = origin_y;
    for (const auto& line : layout.lines) {
        float cursor_x = origin_x;
        for (const auto& glyph : line.glyphs) {
            const auto face = get_face_for_glyph(glyph);
            if (!face) { cursor_x += glyph.advance_x; continue; }

            if (FT_Load_Glyph(face, glyph.glyph_index, FT_LOAD_DEFAULT) != FT_Err_Ok) {
                cursor_x += glyph.advance_x; continue;
            }
            if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != FT_Err_Ok) {
                cursor_x += glyph.advance_x; continue;
            }

            const auto* bitmap = &face->glyph->bitmap;
            if (bitmap->buffer && bitmap->width > 0 && bitmap->rows > 0) {
                const float pen_x = cursor_x + glyph.offset_x + static_cast<float>(face->glyph->bitmap_left);
                const float pen_y = std::round(line_y + line.baseline - glyph.offset_y - static_cast<float>(face->glyph->bitmap_top));

                std::vector<uint32_t> pixels(bitmap->width * bitmap->rows);
                for (unsigned int row = 0; row < bitmap->rows; ++row) {
                    for (unsigned int col = 0; col < bitmap->width; ++col) {
                        const auto alpha = bitmap->buffer[row * bitmap->pitch + col];
                        if (alpha == 0) {
                            pixels[row * bitmap->width + col] = 0;
                        } else {
                            uint32_t pr = static_cast<uint32_t>(r) * alpha / 255;
                            uint32_t pg = static_cast<uint32_t>(g) * alpha / 255;
                            uint32_t pb = static_cast<uint32_t>(b) * alpha / 255;
                            uint32_t pa = static_cast<uint32_t>(a) * alpha / 255;
                            pixels[row * bitmap->width + col] =
                                (pa << 24) | (pr << 16) | (pg << 8) | pb;
                        }
                    }
                }

                auto picture = tvg::Picture::gen();
                if (picture &&
                    picture->load(pixels.data(),
                                  bitmap->width, bitmap->rows,
                                  tvg::ColorSpace::ARGB8888, true) == tvg::Result::Success) {
                    picture->translate(pen_x, pen_y);
                    canvas.add(std::move(picture));
                }
            }
            cursor_x += glyph.advance_x;
        }
        line_y += line.height;
    }
}

} // namespace nandina::text
