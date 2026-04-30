//
// Created by cvrain on 2026/5/6.
//
// nan_font.cpp — NanFont 实现（PIMPL）
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
#include <vector>

module nandina.text.nan_font;

import nandina.log;
import nandina.foundation.color;  // for NanColor / NanRgb in paint

namespace nandina::text {

namespace {
    // ═══════════════════════════════════════════════════════════════════════
    // FreeType 全局库句柄（懒加载，线程安全）
    // ═══════════════════════════════════════════════════════════════════════
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
        static GlobalFT s;
        return s;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 辅助：将 size_pt 转换为 FreeType 的 26.6 定点像素高度
    // ═══════════════════════════════════════════════════════════════════════
    [[nodiscard]] constexpr auto pt_to_pixel(float size_pt) noexcept -> float {
        // 1pt = 1/72 inch。DPI 固定为 96（后续可配置）。
        constexpr float dpi = 96.0f;
        return size_pt * dpi / 72.0f;
    }

    [[nodiscard]] constexpr auto pixel_to_ft_26_6(float pixel) noexcept -> int {
        return static_cast<int>(pixel * 64.0f + 0.5f);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 辅助：系统字体查找
    // ═══════════════════════════════════════════════════════════════════════
    [[nodiscard]] auto find_system_font_impl() -> std::string {
        // 候选路径列表（按优先级排序）
        constexpr auto candidates = std::to_array<std::string_view>({
            // Linux — 常见无衬线字体
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
            "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/opentype/noto/NotoSans-Regular.otf",
            // macOS
            "/System/Library/Fonts/Helvetica.ttc",
            "/System/Library/Fonts/SFNSText.ttf",
            "/Library/Fonts/Arial.ttf",
            // Windows（通常通过 %WINDIR% 定位）
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
        });

        for (const auto path : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(path, ec) && !ec) {
                return std::string{path};
            }
        }
        return {};
    }

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// NanFont::Impl — 内部实现
// ═══════════════════════════════════════════════════════════════════════════
struct NanFont::Impl {
    FT_Face      ft_face{nullptr};
    hb_font_t*   hb_font{nullptr};
    float        size_pt{14.0f};
    float        pixel_height{0.0f};
    bool         ft_acquired{false};

    // 缓存：FreeType 度量
    float ascent_cache{0.0f};
    float descent_cache{0.0f};
    float line_gap_cache{0.0f};

    ~Impl() {
        if (hb_font) {
            hb_font_destroy(hb_font);
            hb_font = nullptr;
        }
        if (ft_face) {
            FT_Done_Face(ft_face);
            ft_face = nullptr;
        }
        if (ft_acquired) {
            global_ft().release();
            ft_acquired = false;
        }
    }

    // ── 加载字体（从 FT_Face）──────────────────────────────────
    auto init_from_face(FT_Face face, float size_pt_val) -> void {
        ft_face      = face;
        size_pt      = size_pt_val;
        pixel_height = pt_to_pixel(size_pt_val);

        // 设置字符大小
        FT_Set_Char_Size(ft_face,
                         0,
                         pixel_to_ft_26_6(pixel_height),
                         0,  // 水平 DPI（0=默认 72）
                         96  // 垂直 DPI = 96
        );

        // 创建 HarfBuzz font（基于 FreeType face）
        hb_font = hb_ft_font_create_referenced(ft_face);
        if (!hb_font) {
            throw std::runtime_error("hb_ft_font_create_referenced returned nullptr");
        }
        // 设置缩放以确保 hb 与 FT 的字号一致
        hb_font_set_scale(hb_font,
                          static_cast<int>(pixel_height * 64.0f),
                          static_cast<int>(pixel_height * 64.0f));

        // 缓存度量
        const auto& metrics = ft_face->size->metrics;
        ascent_cache   = static_cast<float>(metrics.ascender)  / 64.0f;
        descent_cache  = static_cast<float>(-metrics.descender) / 64.0f;  // 转为正值
        // line_gap = height - ascender + descender（FT 中 descender 为负）
        // 需要保证非负
        const float raw_gap = static_cast<float>(metrics.height - metrics.ascender + metrics.descender) / 64.0f;
        line_gap_cache = (raw_gap > 0.0f) ? raw_gap : 0.0f;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// NanFont 静态工厂方法
// ═══════════════════════════════════════════════════════════════════════════
NanFont::NanFont() : m_impl(std::make_unique<Impl>()) {}

NanFont::~NanFont() = default;

auto NanFont::load_from_path(std::string_view file_path, float size_pt) -> Ptr {
    auto log = nandina::log::get("text.nan_font");
    log.debug("Loading font from \"{}\" size={:.1f}pt", file_path, size_pt);

    if (size_pt <= 0.0f) {
        throw std::runtime_error(std::format("Invalid font size: {}", size_pt));
    }

    std::string path_str{file_path};
    std::error_code ec;
    if (!std::filesystem::exists(path_str, ec) || ec) {
        throw std::runtime_error(std::format("Font file not found: {}", path_str));
    }

    // 获取全局 FreeType 句柄
    global_ft().acquire();

    FT_Face face = nullptr;
    const FT_Error error = FT_New_Face(global_ft().library, path_str.c_str(), 0, &face);
    if (error != FT_Err_Ok || !face) {
        global_ft().release();
        throw std::runtime_error(std::format("FT_New_Face failed for \"{}\": error={}", path_str, error));
    }

    auto font  = Ptr{new NanFont()};
    font->m_impl->ft_acquired = true;

    try {
        font->m_impl->init_from_face(face, size_pt);
    } catch (...) {
        FT_Done_Face(face);
        throw;
    }

    log.info("Font loaded: \"{}\" {:.1f}pt (pixel={:.2f})", path_str, size_pt, font->m_impl->pixel_height);
    return font;
}

auto NanFont::load_from_memory(std::span<const std::byte> data, float size_pt) -> Ptr {
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
        0,
        &face);
    if (error != FT_Err_Ok || !face) {
        global_ft().release();
        throw std::runtime_error(std::format("FT_New_Memory_Face failed: error={}", error));
    }

    auto font  = Ptr{new NanFont()};
    font->m_impl->ft_acquired = true;

    try {
        font->m_impl->init_from_face(face, size_pt);
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
    return load_from_path(path, size_pt);
}

auto NanFont::find_system_font_path() -> std::string {
    return find_system_font_impl();
}

// ═══════════════════════════════════════════════════════════════════════════
// 字体度量
// ═══════════════════════════════════════════════════════════════════════════

auto NanFont::ascent() const noexcept -> float {
    return m_impl ? m_impl->ascent_cache : 0.0f;
}

auto NanFont::descent() const noexcept -> float {
    return m_impl ? m_impl->descent_cache : 0.0f;
}

auto NanFont::line_height() const noexcept -> float {
    return m_impl
        ? m_impl->ascent_cache + m_impl->descent_cache + m_impl->line_gap_cache
        : 0.0f;
}

auto NanFont::line_gap() const noexcept -> float {
    return m_impl ? m_impl->line_gap_cache : 0.0f;
}

auto NanFont::em_size() const noexcept -> float {
    return m_impl ? m_impl->pixel_height : 0.0f;
}

auto NanFont::size_pt() const noexcept -> float {
    return m_impl ? m_impl->size_pt : 0.0f;
}

// ═══════════════════════════════════════════════════════════════════════════
// Glyph 度量查询
// ═══════════════════════════════════════════════════════════════════════════

auto NanFont::glyph_advance(std::uint32_t codepoint) const noexcept -> float {
    if (!m_impl || !m_impl->ft_face) {
        return 0.0f;
    }

    const auto glyph_index = FT_Get_Char_Index(m_impl->ft_face, codepoint);
    if (glyph_index == 0) {
        return 0.0f;
    }

    if (FT_Load_Glyph(m_impl->ft_face, glyph_index, FT_LOAD_DEFAULT) != FT_Err_Ok) {
        return 0.0f;
    }

    return static_cast<float>(m_impl->ft_face->glyph->advance.x) / 64.0f;
}

auto NanFont::estimate_text_width(std::string_view text) const noexcept -> float {
    if (!m_impl || !m_impl->ft_face || text.empty()) {
        return 0.0f;
    }

    float total = 0.0f;
    // 简单逐字符累加 advance（不含 kerning 和 shaping）
    for (std::size_t i = 0; i < text.size(); ) {
        // 解码 UTF-8 码点
        std::uint32_t cp = 0;
        unsigned char lead = static_cast<unsigned char>(text[i]);
        int bytes = 0;

        if (lead < 0x80) {
            cp = lead;
            bytes = 1;
        } else if ((lead & 0xE0) == 0xC0) {
            if (i + 1 >= text.size()) break;
            cp = ((lead & 0x1Fu) << 6u) |
                 (static_cast<unsigned char>(text[i + 1]) & 0x3Fu);
            bytes = 2;
        } else if ((lead & 0xF0) == 0xE0) {
            if (i + 2 >= text.size()) break;
            cp = ((lead & 0x0Fu) << 12u) |
                 ((static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 6u) |
                 (static_cast<unsigned char>(text[i + 2]) & 0x3Fu);
            bytes = 3;
        } else if ((lead & 0xF8) == 0xF0) {
            if (i + 3 >= text.size()) break;
            cp = ((lead & 0x07u) << 18u) |
                 ((static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 12u) |
                 ((static_cast<unsigned char>(text[i + 2]) & 0x3Fu) << 6u) |
                 (static_cast<unsigned char>(text[i + 3]) & 0x3Fu);
            bytes = 4;
        } else {
            // 无效 UTF-8 字节，跳过
            ++i;
            continue;
        }

        total += glyph_advance(cp);
        i += static_cast<std::size_t>(bytes);
    }
    return total;
}

// ═══════════════════════════════════════════════════════════════════════════
// HarfBuzz Shaping
// ═══════════════════════════════════════════════════════════════════════════

auto NanFont::shape(std::string_view text,
                     float max_width,
                     int max_lines) const -> TextLayout {
    TextLayout result;

    if (text.empty() || !m_impl || !m_impl->hb_font) {
        return result;
    }

    if (max_lines < 0) {
        max_lines = 0;
    }
    if (max_width < 0.0f) {
        max_width = 0.0f;
    }

    // ── 1. 创建 HarfBuzz buffer 并 shaping ──
    auto* buffer = hb_buffer_create();
    if (!buffer) {
        return result;
    }

    hb_buffer_add_utf8(buffer,
                       text.data(),
                       static_cast<int>(text.size()),
                       0,  // 起始偏移
                       static_cast<int>(text.size()));  // 长度
    hb_buffer_guess_segment_properties(buffer);
    hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer, HB_SCRIPT_COMMON);
    hb_buffer_set_language(buffer, hb_language_from_string("en", -1));

    hb_shape(m_impl->hb_font, buffer, nullptr, 0);

    // ── 2. 提取 glyph 信息 ──
    unsigned int glyph_count = 0;
    auto* glyph_info  = hb_buffer_get_glyph_infos(buffer, &glyph_count);
    auto* glyph_pos   = hb_buffer_get_glyph_positions(buffer, &glyph_count);

    std::vector<GlyphInfo> all_glyphs;
    all_glyphs.reserve(glyph_count);

    for (unsigned int i = 0; i < glyph_count; ++i) {
        all_glyphs.push_back(GlyphInfo{
            .glyph_index = glyph_info[i].codepoint,  // hb 用 codepoint 字段存 glyph index
            .codepoint   = glyph_info[i].codepoint,  // 此即为 FT glyph index
            .advance_x   = static_cast<float>(glyph_pos[i].x_advance) / 64.0f,
            .advance_y   = static_cast<float>(glyph_pos[i].y_advance) / 64.0f,
            .offset_x    = static_cast<float>(glyph_pos[i].x_offset)  / 64.0f,
            .offset_y    = static_cast<float>(glyph_pos[i].y_offset)  / 64.0f,
        });
    }

    // ── 3. 换行处理 ──
    const float line_h = line_height();

    if (max_width > 0.0f) {
        // 按最大宽度折行
        TextLine current_line;
        current_line.height   = line_h;
        current_line.baseline = ascent();
        float cursor_x = 0.0f;

        for (auto& g : all_glyphs) {
            // 检查是否需要换行
            if (!current_line.glyphs.empty() && cursor_x + g.advance_x > max_width) {
                // 当前行结束，提交
                current_line.width = cursor_x;
                result.lines.push_back(std::move(current_line));

                current_line = TextLine{};
                current_line.height   = line_h;
                current_line.baseline = ascent();
                cursor_x = 0.0f;

                // 检查行数限制
                if (max_lines > 0 && result.lines.size() >= static_cast<std::size_t>(max_lines)) {
                    break;
                }
            }

            current_line.glyphs.push_back(std::move(g));
            cursor_x += g.advance_x;
        }

        // 最后一行
        if (!current_line.glyphs.empty()) {
            current_line.width = cursor_x;
            result.lines.push_back(std::move(current_line));
        }
    } else {
        // 不换行 — 所有 glyph 放在一行
        TextLine line;
        line.height   = line_h;
        line.baseline = ascent();
        line.glyphs   = std::move(all_glyphs);

        float total_w = 0.0f;
        for (const auto& g : line.glyphs) {
            total_w += g.advance_x;
        }
        line.width = total_w;
        result.lines.push_back(std::move(line));
    }

    // ── 4. 计算总尺寸 ──
    result.total_height = 0.0f;
    result.total_width  = 0.0f;
    for (const auto& line : result.lines) {
        result.total_height += line.height;
        result.total_width   = std::max(result.total_width, line.width);
    }

    hb_buffer_destroy(buffer);
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// ThorVG 绘制
// ═══════════════════════════════════════════════════════════════════════════

void NanFont::paint(tvg::SwCanvas& canvas,
                    const TextLayout& layout,
                    float origin_x,
                    float origin_y,
                    const nandina::NanColor& color) const {
    if (!m_impl || !m_impl->ft_face || layout.empty()) {
        return;
    }

    const auto rgb = color.to<nandina::NanRgb>();
    const auto paint_r = rgb.red();
    const auto paint_g = rgb.green();
    const auto paint_b = rgb.blue();
    const auto paint_a = rgb.alpha();

    float cursor_y = origin_y;

    for (const auto& line : layout.lines) {
        // 光标 X 从行左边开始
        float cursor_x = origin_x;

        for (const auto& glyph : line.glyphs) {
            // 通过 FreeType 加载 glyph 轮廓
            if (FT_Load_Glyph(m_impl->ft_face,
                              glyph.glyph_index,
                              FT_LOAD_NO_BITMAP) != FT_Err_Ok) {
                // 跳过无法加载的 glyph
                cursor_x += glyph.advance_x;
                continue;
            }

            // 获取 glyph 轮廓
            FT_Glyph ft_glyph = nullptr;
            if (FT_Get_Glyph(m_impl->ft_face->glyph, &ft_glyph) != FT_Err_Ok) {
                cursor_x += glyph.advance_x;
                continue;
            }

            // 检查是否有轮廓（某些空格字符可能没有轮廓）
            if (ft_glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
                auto* outline_glyph = reinterpret_cast<FT_OutlineGlyph>(ft_glyph);
                const FT_Outline& outline = outline_glyph->outline;

                // 计算渲染位置（基线对齐 + HarfBuzz offset）
                const float render_x = cursor_x + glyph.offset_x;
                // baseline_y = cursor_y + line.baseline 将基线对齐到正确行位置
                const float render_y = (cursor_y + line.baseline) + glyph.offset_y;

                // 将 FreeType 26.6 定点坐标转换为浮点像素，并加入 raw_points
                std::vector<geometry::NanPoint> raw_points;
                raw_points.reserve(outline.n_points);

                for (short pi = 0; pi < outline.n_points; ++pi) {
                    raw_points.emplace_back(
                        render_x + static_cast<float>(outline.points[pi].x) / 64.0f,
                        render_y - static_cast<float>(outline.points[pi].y) / 64.0f
                    );
                }

                // 遍历轮廓的每个 contour，用 ThorVG Shape 路径构建 glyph 形状
                auto* shape = tvg::Shape::gen();

                for (short ci = 0; ci < outline.n_contours; ++ci) {
                    const short contour_start = (ci == 0) ? 0 : (outline.contours[ci - 1] + 1);
                    const short contour_end   = outline.contours[ci];
                    const short point_count   = contour_end - contour_start + 1;

                    if (point_count < 1) {
                        continue;
                    }

                    // ── 第一阶段：找到第一个 on-curve 点 ──
                    // 如果 contour 以 off-curve 开头，则需要找到第一个 on-curve 点。
                    // 如果整个 contour 都没有 on-curve 点，将第一个点视为隐含 on-curve。
                    int first_on_idx = -1;
                    for (short pi = contour_start; pi <= contour_end; ++pi) {
                        if (FT_CURVE_TAG(outline.tags[pi]) == FT_CURVE_TAG_ON) {
                            first_on_idx = pi;
                            break;
                        }
                    }

                    // 如果没有 on-curve 点，将轮廓起点视为隐含 on-curve 点
                    if (first_on_idx == -1) {
                        first_on_idx = contour_start;
                    }

                    // 从第一个 on-curve 点开始路径
                    const auto& start_pt = raw_points[static_cast<std::size_t>(first_on_idx)];
                    shape->moveTo(start_pt.x(), start_pt.y());

                    // ── 第二阶段：遍历所有点并构建路径 ──
                    // 使用环形遍历：从 first_on_idx 之后开始，绕回 contour_start，直到回到 first_on_idx
                    // prev_on_x, prev_on_y 追踪上一个 on-curve（或等效）点的位置
                    float prev_on_x = start_pt.x();
                    float prev_on_y = start_pt.y();

                    // 顺序遍历：从 first_on_idx + 1 开始，到 contour_end，再从 contour_start 到 first_on_idx
                    const int total = contour_end - contour_start + 1;
                    for (int step = 1; step <= total; ) {
                        const int abs_idx = (first_on_idx - contour_start + step) % total + contour_start;
                        const std::uint8_t tag = FT_CURVE_TAG(outline.tags[abs_idx]);
                        const auto& pt = raw_points[static_cast<std::size_t>(abs_idx)];

                        if (tag == FT_CURVE_TAG_ON) {
                            // on-curve 点：直接 lineTo
                            shape->lineTo(pt.x(), pt.y());
                            prev_on_x = pt.x();
                            prev_on_y = pt.y();
                            ++step;
                        } else if (tag == FT_CURVE_TAG_CONIC) {
                            // 二次贝塞尔（TrueType conic）
                            // 当前是 conic 控制点，需要看下一个点
                            const int next_idx = (abs_idx == contour_end) ? contour_start : (abs_idx + 1);
                            const std::uint8_t next_tag = FT_CURVE_TAG(outline.tags[next_idx]);
                            const auto& next_pt = raw_points[static_cast<std::size_t>(next_idx)];

                            if (next_tag == FT_CURVE_TAG_ON) {
                                // conic → on：标准二次贝塞尔
                                // 转换为三次贝塞尔：C₀ = P₀ + 2/3*(P₁ - P₀), C₁ = P₂ + 2/3*(P₁ - P₂)
                                const float cx1 = prev_on_x + 2.0f/3.0f * (pt.x() - prev_on_x);
                                const float cy1 = prev_on_y + 2.0f/3.0f * (pt.y() - prev_on_y);
                                const float cx2 = next_pt.x() + 2.0f/3.0f * (pt.x() - next_pt.x());
                                const float cy2 = next_pt.y() + 2.0f/3.0f * (pt.y() - next_pt.y());
                                shape->cubicTo(cx1, cy1, cx2, cy2, next_pt.x(), next_pt.y());
                                prev_on_x = next_pt.x();
                                prev_on_y = next_pt.y();
                                step += 2;  // 跳过 conic 控制点和 on 终点
                            } else {
                                // conic → conic：两个连续 conic 点，隐含中点
                                const float mx = (pt.x() + next_pt.x()) * 0.5f;
                                const float my = (pt.y() + next_pt.y()) * 0.5f;
                                // 从 prev_on 经 pt 到隐含中点的三次贝塞尔等效
                                const float cx1 = prev_on_x + 2.0f/3.0f * (pt.x() - prev_on_x);
                                const float cy1 = prev_on_y + 2.0f/3.0f * (pt.y() - prev_on_y);
                                const float cx2 = mx + 2.0f/3.0f * (pt.x() - mx);
                                const float cy2 = my + 2.0f/3.0f * (pt.y() - my);
                                shape->cubicTo(cx1, cy1, cx2, cy2, mx, my);
                                prev_on_x = mx;
                                prev_on_y = my;
                                step += 2;  // 跳过两个 conic 控制点
                            }
                        } else if (tag == FT_CURVE_TAG_CUBIC) {
                            // 三次贝塞尔（PostScript cubic）
                            // 需要连续两个控制点，然后一个 on-curve 终点
                            if (step + 2 > total) {
                                ++step;
                                break;  // 数据不足
                            }
                            const int next1_idx = (abs_idx == contour_end) ? contour_start : (abs_idx + 1);
                            const int next2_idx = (next1_idx == contour_end) ? contour_start : (next1_idx + 1);
                            const auto& c1 = pt;
                            const auto& c2 = raw_points[static_cast<std::size_t>(next1_idx)];
                            const auto& end = raw_points[static_cast<std::size_t>(next2_idx)];
                            shape->cubicTo(c1.x(), c1.y(),
                                           c2.x(), c2.y(),
                                           end.x(), end.y());
                            prev_on_x = end.x();
                            prev_on_y = end.y();
                            step += 3;  // 跳过两个控制点 + 一个终点
                        } else {
                            // 未知标签，跳过
                            ++step;
                        }
                    }

                    // 闭合 contour
                    shape->close();
                }

                // 设置填充颜色
                shape->fill(paint_r, paint_g, paint_b, paint_a);
                canvas.add(shape);
            }

            // 释放 FT glyph
            FT_Done_Glyph(ft_glyph);

            // 前进光标
            cursor_x += glyph.advance_x;
        }

        // 移动到下一行
        cursor_y += line.height;
    }
}

void NanFont::paint_text(tvg::SwCanvas& canvas,
                         std::string_view text,
                         float origin_x,
                         float origin_y,
                         const nandina::NanColor& color) const {
    // 先 shape，再 paint
    auto layout = shape(text, 0.0f, 0);
    paint(canvas, layout, origin_x, origin_y, color);
}

} // namespace nandina::text