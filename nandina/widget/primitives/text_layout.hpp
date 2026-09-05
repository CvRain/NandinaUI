//
// widget/primitives/text_layout — backend-neutral text layout values.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_HPP

#include "../../foundation/nandina_color.hpp"
#include "../../scene/control.hpp"
#include "../../text/font_family.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::widget::primitives
{

    enum class TextOverflow {
        clip,
        ellipsis,
        wrap,
        scale,
    };

    /// 允许字形墨迹在 measured advance 之外悬垂的余量（逻辑 px）。
    ///
    /// 字形位图在渲染时按其物理像素网格吸附（见 GlyphAtlasTexture::draw），且次像素下
    /// 最后一个字形的右侧墨迹可能比其 x_advance 多出 1~2px（尤其是全宽 CJK 与比例字体
    /// 混排）。若把文字裁剪到 measured advance，会把这部分墨迹裁掉。本函数给出覆盖该
    /// 悬垂幅度的余量，随字号缩放并保底 2px；仅用于"裁剪出文本"的一侧（右侧）。
    [[nodiscard]] inline auto glyph_overhang_allowance(float font_size) -> float {
        return std::max(2.0F, font_size * 0.08F);
    }

    struct TextStyle {
        foundation::NanColor color = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 255}
        );
        float font_size = 16.0F;
        text::FontRequest font;
        TextOverflow overflow = TextOverflow::ellipsis;
        int max_lines = 1;
    };

    struct TextLayoutInput {
        std::string_view text;
        TextStyle style;
        scene::LayoutConstraints constraints = scene::LayoutConstraints::loose();
    };

    enum class TextAffinity : std::uint8_t {
        upstream,
        downstream,
    };

    struct TextCaretStop {
        /// UTF-8 byte boundary in the original TextLayoutInput source.
        std::size_t source_offset = 0;
        /// Line-local visual pen position; it may exceed a clipped result width.
        float x = 0.0F;
        TextAffinity affinity = TextAffinity::downstream;
    };

    struct TextLayoutLine {
        struct Glyph {
            std::uint32_t glyph_index = 0;
            std::size_t font_index = 0;
            std::size_t cluster = 0;
            float x_advance = 0.0F;
            float y_advance = 0.0F;
            float x_offset = 0.0F;
            float y_offset = 0.0F;
        };

        std::size_t text_offset = 0;
        std::size_t text_length = 0;
        std::string visible_text;
        std::vector<Glyph> glyphs;
        std::vector<TextCaretStop> caret_stops;
        foundation::NanSize size {};
        float baseline = 0.0F;
        bool right_to_left = false;
        bool missing_glyphs = false;

        /// Resolve a source byte to the preceding represented cluster boundary.
        [[nodiscard]] auto caret_for_source(
            std::size_t source_offset,
            TextAffinity affinity = TextAffinity::downstream
        ) const -> TextCaretStop;
        /// Return the visually nearest caret stop without clipping to line width.
        [[nodiscard]] auto caret_for_x(float x) const -> TextCaretStop;
    };

    struct TextLayoutResult {
        foundation::NanSize size {};
        std::vector<TextLayoutLine> lines;
        float font_size = 16.0F;
        float baseline = 0.0F;
        bool overflowed = false;
        bool missing_glyphs = false;

        /// Resolve a layout-local point to the nearest line and visual caret stop.
        [[nodiscard]] auto caret_for_point(foundation::NanPoint point) const -> TextCaretStop;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_HPP
