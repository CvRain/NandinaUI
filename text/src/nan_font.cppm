//
// Created by cvrain on 2026/5/6.
//
// nan_font.cppm — NanFont 字体加载与度量接口
//
// 职责：
//   - 封装 FreeType 字体加载与渲染
//   - 封装 HarfBuzz 文本 shaping
//   - 对外仅暴露 C++ 标准类型 + foundation 类型
//
// PIMPL：所有 FreeType / HarfBuzz 类型均隐藏在 nan_font.cpp 中。
// 消费者只需 import nandina.text.nan_font 即可使用字体能力。
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

export import nandina.foundation.color;  // 需要 NanColor 在 paint 签名中

// ============================================================
// 导出接口
// ============================================================
export namespace nandina::text {

    // ── 字体粗细 ──────────────────────────────────────────────
    // 与 nandina::theme::NanFontWeight 语义对齐但独立定义，
    // 避免 text 模块对 theme 产生编译期依赖。
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

    // ── 字体描述符 ────────────────────────────────────────────
    /// 描述如何加载一个字体（族名、字号、粗细、倾斜）。
    /// 支持两种来源：
    ///   1. 系统字体（family 非空，由平台查找）
    ///   2. 文件路径（family 为空，file_path 非空）
    struct NanFontDescriptor {
        std::string   family;      // 字体族名（如 "DejaVu Sans"）
        float         size_pt{14.0f};
        NanFontWeight weight{NanFontWeight::regular};
        bool          italic{false};
    };

    // ── Glyph 度量信息 ────────────────────────────────────────
    /// 单个字形（glyph）的度量数据，由 shaper 产出。
    struct GlyphInfo {
        std::uint32_t glyph_index{0};   // FreeType glyph 索引
        std::uint32_t codepoint{0};     // 原始 Unicode 码点
        float         advance_x{0.0f};  // 水平步进（像素）
        float         advance_y{0.0f};  // 垂直步进（像素）
        float         offset_x{0.0f};   // 渲染 X 偏移（像素）
        float         offset_y{0.0f};   // 渲染 Y 偏移（像素）
    };

    // ── 文本布局结果 ──────────────────────────────────────────
    /// 单行文本的 shaping + 布局结果。
    struct TextLine {
        std::vector<GlyphInfo> glyphs;
        float width{0.0f};        // 行宽（像素）
        float height{0.0f};       // 行高（像素）
        float baseline{0.0f};     // 基线 Y（相对于行顶）

        [[nodiscard]] auto empty() const noexcept -> bool {
            return glyphs.empty();
        }
    };

    /// 多行文本布局结果。
    struct TextLayout {
        std::vector<TextLine> lines;
        float total_width{0.0f};
        float total_height{0.0f};

        [[nodiscard]] auto empty() const noexcept -> bool {
            return lines.empty();
        }

        /// 布局结果的首选尺寸（不约束宽度时）。
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize {
            return {total_width, total_height};
        }
    };

    // ────────────────────────────────────────────────────────────
    // NanFont — 字体句柄
    //
    // 线程安全：读取（度量查询）可在任意线程并发执行。
    //          创建与销毁应在同一线程顺序执行。
    //
    // 使用示例：
    //   auto font = NanFont::load_from_path("/usr/share/fonts/.../DejaVuSans.ttf", 14.0f);
    //   // 或
    //   auto font = NanFont::load_system_default(14.0f);
    //
    //   // 查询度量
    //   float ascent  = font->ascent();
    //   float line_h  = font->line_height();
    //
    //   // Shaping（在一个字体上执行文本成形）
    //   auto layout = font->shape("Hello, 世界！");
    //   for (auto& line : layout.lines) {
    //       for (auto& g : line.glyphs) {
    //           // 消费 glyph 信息...
    //       }
    //   }
    // ────────────────────────────────────────────────────────────
    class NanFont {
    public:
        using Ptr = std::shared_ptr<NanFont>;

        // ── 工厂方法 ───────────────────────────────────────

        /// 从 TrueType / OpenType 文件路径加载字体。
        /// @param file_path 字体文件绝对路径
        /// @param size_pt 字号（pt，point，1pt = 1/72 inch）
        /// @throws std::runtime_error 如果加载失败
        [[nodiscard]] static auto load_from_path(std::string_view file_path,
                                                 float size_pt) -> Ptr;

        /// 从内存中的字体数据加载。
        /// @param data 字体文件原始字节（TTF / OTF）
        /// @param size_pt 字号（pt）
        /// @throws std::runtime_error 如果加载失败
        [[nodiscard]] static auto load_from_memory(std::span<const std::byte> data,
                                                   float size_pt) -> Ptr;

        /// 查找并使用系统默认等宽或无衬线字体。
        /// 查找顺序：
        ///   1. Linux: /usr/share/fonts 下的常见路径
        ///   2. macOS: /System/Library/Fonts
        ///   3. Windows: C:\Windows\Fonts
        /// @param size_pt 字号（pt）
        /// @throws std::runtime_error 如果找不到可用字体
        [[nodiscard]] static auto load_system_default(float size_pt) -> Ptr;

        /// 查找系统默认字体路径（不加载）。
        /// 返回第一个找到的 .ttf/.otf 文件路径，找不到则返回空字符串。
        [[nodiscard]] static auto find_system_font_path() -> std::string;

        ~NanFont();

        // 禁止拷贝与移动（由 shared_ptr 管理生命周期）
        NanFont(const NanFont&)            = delete;
        auto operator=(const NanFont&)     = delete;
        NanFont(NanFont&&)                 = delete;
        auto operator=(NanFont&&)          = delete;

        // ── 字体度量 ───────────────────────────────────────

        /// Ascender：基线到顶部的高度（像素）
        [[nodiscard]] auto ascent() const noexcept -> float;

        /// Descender：基线到底部的高度（像素，正值）
        [[nodiscard]] auto descent() const noexcept -> float;

        /// 行高（ascent + descent + line_gap，像素）
        [[nodiscard]] auto line_height() const noexcept -> float;

        /// 推荐的行距（像素），即行与行之间的间距
        [[nodiscard]] auto line_gap() const noexcept -> float;

        /// 单位 EM 的像素大小
        [[nodiscard]] auto em_size() const noexcept -> float;

        /// 当前字号（pt）
        [[nodiscard]] auto size_pt() const noexcept -> float;

        // ── Glyph 度量查询 ─────────────────────────────────

        /// 查询单个 Unicode 码点的水平步进宽度（像素）。
        /// 若码点不存在于当前字体，返回 0。
        [[nodiscard]] auto glyph_advance(std::uint32_t codepoint) const noexcept -> float;

        /// 估算字符串的像素宽度（不含 shaping，适用于快速测量）。
        /// 对于复杂文本（阿拉伯语、印地语等）请使用 shape()。
        [[nodiscard]] auto estimate_text_width(std::string_view text) const noexcept -> float;

        // ── HarfBuzz Shaping ───────────────────────────────

        /// 对 UTF-8 文本执行 HarfBuzz shaping。
        /// 返回按行分组的 glyph 信息与布局数据。
        ///
        /// @param text 输入文本（UTF-8 编码）
        /// @param max_width 最大行宽（像素），0 表示不换行
        /// @param max_lines 最大行数，0 表示不限
        [[nodiscard]] auto shape(std::string_view text,
                                 float max_width = 0.0f,
                                 int max_lines = 0) const -> TextLayout;

        // ── ThorVG 绘制 ──────────────────────────────────

        /// 在 ThorVG canvas 上绘制一个已完成 shaping 和行布局的 TextLayout。
        ///
        /// @param canvas  目标 ThorVG 画布
        /// @param layout  由 shape() 生成的布局数据
        /// @param origin_x 文本块左上角 X 坐标
        /// @param origin_y 文本块左上角 Y 坐标
        /// @param color   文本颜色（NanColor）
        void paint(tvg::SwCanvas& canvas,
                   const TextLayout& layout,
                   float origin_x,
                   float origin_y,
                   const nandina::NanColor& color) const;

        /// 便捷方法：直接绘制一段 UTF-8 文本。
        ///
        /// 在 canvas 上 layout 并绘制文本。
        /// 等效于 shape(text) 然后 paint(canvas, layout, ...)。
        ///
        /// @param canvas  目标 ThorVG 画布
        /// @param text    UTF-8 文本
        /// @param origin_x 文本块左上角 X 坐标
        /// @param origin_y 文本块左上角 Y 坐标
        /// @param color   文本颜色（NanColor）
        void paint_text(tvg::SwCanvas& canvas,
                        std::string_view text,
                        float origin_x,
                        float origin_y,
                        const nandina::NanColor& color) const;

    private:
        // 仅由静态工厂方法构造
        NanFont();

        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace nandina::text