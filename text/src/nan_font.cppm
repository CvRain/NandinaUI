//
// nan_font.cppm — NanFont 字体加载、配置、shaping 与绘制
//
// 职责：
//   - 封装 FreeType 字体加载与渲染
//   - 封装 HarfBuzz 文本 shaping
//   - 聚合字体样式（字号/粗细/颜色/族名/行高等）
//   - 聚合文本布局行为（溢出策略/最大行数/单行模式）
//   - 统一 shape() 入口处理 \n 分割 + 溢出
//   - 对外仅暴露 C++ 标准类型 + foundation 类型
//
// 设计约定（P0 统一）：
//   - getter/setter 均支持时使用裸名重载：size() / size(14)
//   - NanFont 可按值拷贝（内部 Impl 通过 shared_ptr 共享 FT_Face）
//   - 首次 shape()/paint() 时自动懒加载系统默认字体
//

module;

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <thorvg-1/thorvg.h>

export module nandina.text.nan_font;

export import nandina.foundation.nan_point;
export import nandina.foundation.nan_rect;
export import nandina.foundation.nan_size;
export import nandina.foundation.color;

// ============================================================
// 导出接口
// ============================================================
export namespace nandina::text {

    // ── 字体粗细 ──────────────────────────────────────────────
    enum class NanFontWeight : int {
        thin       = 100,
        extraLight = 200,
        light      = 300,
        regular    = 400,
        medium     = 500,
        semiBold   = 600,
        bold       = 700,
        extraBold  = 800,
        black      = 900,
    };

    // ── 文本溢出策略 ──────────────────────────────────────────
    enum class TextOverflow : std::uint8_t {
        wrap,      // 换行（默认）
        clip,      // 硬裁剪，超出不显示
        ellipsis,  // 省略号 "..."
        scale,     // 等比缩小字号直到放下（expensive）
    };

    // ── Glyph 度量信息 ────────────────────────────────────────
    /// 单个字形（glyph）的度量数据，由 shaper 产出。
    struct GlyphInfo {
        std::uint32_t glyph_index{0};   // FreeType glyph 索引
        std::uint32_t original_cp{0};   // 原始 Unicode 码点（用于断词等）
        float         advance_x{0.0f};
        float         advance_y{0.0f};
        float         offset_x{0.0f};
        float         offset_y{0.0f};
    };

    // ── 文本布局结果 ──────────────────────────────────────────
    struct TextLine {
        std::vector<GlyphInfo> glyphs;
        float width{0.0f};
        float height{0.0f};
        float baseline{0.0f};

        [[nodiscard]] auto empty() const noexcept -> bool { return glyphs.empty(); }
    };

    struct TextLayout {
        std::vector<TextLine> lines;
        float total_width{0.0f};
        float total_height{0.0f};

        [[nodiscard]] auto empty() const noexcept -> bool { return lines.empty(); }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize {
            return {total_width, total_height};
        }
    };

    // ═══════════════════════════════════════════════════════════
    // NanFont — 字体句柄（聚合资源 + 样式 + 布局行为）
    //
    // 使用示例：
    //   auto font = NanFont{}.size(24).weight(NanFontWeight::bold).color(NanColor::white());
    //   auto layout = font.shape("Hello, World");
    //   font.paint(canvas, layout, x, y);
    //
    // 设计要点：
    //   - 默认构造不加载字体文件，首次 shape()/paint() 自动懒加载系统默认字体
    //   - 可按值拷贝（内部 Impl 通过 shared_ptr 共享 FT_Face）
    //   - getter/setter 均支持的属性使用裸名重载
    // ═══════════════════════════════════════════════════════════
    class NanFont {
    public:
        using Ptr = std::shared_ptr<NanFont>;

        // ── 构造 / 析构 ───────────────────────────────────

        /// 默认构造：不加载任何字体文件
        NanFont();

        ~NanFont();

        // 拷贝：共享内部 Impl，拷贝配置
        NanFont(const NanFont&);
        auto operator=(const NanFont&) -> NanFont&;

        // 移动
        NanFont(NanFont&&) noexcept;
        auto operator=(NanFont&&) noexcept -> NanFont&;

        // ── 工厂 ─────────────────────────────────────────

        /// 从 TrueType / OpenType 文件路径加载字体
        [[nodiscard]] static auto load_from_path(std::string_view file_path,
                                                  float size_pt) -> NanFont;

        /// 从内存中的字体数据加载
        [[nodiscard]] static auto load_from_memory(std::span<const std::byte> data,
                                                    float size_pt) -> NanFont;

        // ── 链式配置（纯配置，不触发 IO）─────────────────

        auto family(std::string name) -> NanFont&;
        auto size(float pt) -> NanFont&;
        auto weight(NanFontWeight w) -> NanFont&;
        auto color(NanColor c) -> NanFont&;
        auto line_height(float lh) -> NanFont&;
        auto letter_spacing(float ls) -> NanFont&;
        auto overflow(TextOverflow o) -> NanFont&;
        auto max_lines(int n) -> NanFont&;
        auto single_line(bool s) -> NanFont&;

        // ── 访问器（const，与链式 setter 重载）───────────

        [[nodiscard]] auto family()      const noexcept -> const std::string&;
        [[nodiscard]] auto size()        const noexcept -> float;
        [[nodiscard]] auto weight()      const noexcept -> NanFontWeight;
        [[nodiscard]] auto color()       const noexcept -> const NanColor&;
        [[nodiscard]] auto line_height() const noexcept -> float;
        [[nodiscard]] auto letter_spacing() const noexcept -> float;
        [[nodiscard]] auto overflow()    const noexcept -> TextOverflow;
        [[nodiscard]] auto max_lines()   const noexcept -> int;
        [[nodiscard]] auto single_line() const noexcept -> bool;

        // ── Shaping（首次调用自动懒加载字体）──────────────

        /// @param text      输入文本（UTF-8）
        /// @param max_width 最大宽度（像素），0 = 不限制
        auto shape(std::string_view text, float max_width = 0.0f) const -> TextLayout;

        // ── 绘制（颜色由 this->color() 提供）──────────────

        auto paint(tvg::SwCanvas& canvas,
                   const TextLayout& layout,
                   float origin_x,
                   float origin_y) const -> void;

        // ── 度量查询（只读，来自已加载字体）───────────────

        [[nodiscard]] auto ascent()     const noexcept -> float;
        [[nodiscard]] auto descent()    const noexcept -> float;
        [[nodiscard]] auto em_size()    const noexcept -> float;

        // ── Glyph 度量 ───────────────────────────────────

        [[nodiscard]] auto glyph_advance(std::uint32_t codepoint) const noexcept -> float;
        [[nodiscard]] auto estimate_text_width(std::string_view text) const noexcept -> float;

        // ── 检查是否已加载 ───────────────────────────────

        [[nodiscard]] auto is_loaded() const noexcept -> bool;

    private:
        struct Impl;
        std::shared_ptr<Impl> m_impl;

        // ── 配置字段（每个 NanFont 实例独立）────────────
        std::string      m_family;
        float            m_size_pt{14.0f};
        NanFontWeight    m_weight{NanFontWeight::regular};
        NanColor         m_color{};
        float            m_line_height_override{0.0f};
        float            m_letter_spacing{0.0f};
        TextOverflow     m_overflow{TextOverflow::wrap};
        int              m_max_lines{0};
        bool             m_single_line{false};

        // 懒加载：若未加载则调用 load_system_default(size())
        auto ensure_loaded() const -> void;

        // 内部：系统默认加载
        [[nodiscard]] static auto load_system_default(float size_pt) -> Ptr;
    };

} // namespace nandina::text