//
// text/text_layout — backend-neutral text layout values.
//

#ifndef NANDINA_EXPERIMENT_TEXT_TEXT_LAYOUT_HPP
#define NANDINA_EXPERIMENT_TEXT_TEXT_LAYOUT_HPP

#include "../foundation/layout_constraints.hpp"
#include "../foundation/nandina_color.hpp"
#include "../theme/visual_state.hpp"
#include "font_family.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::text
{

    /// `TextAlign` 保留现有 theme 定义；共享文本布局值重导出同一个类型，
    /// 不为迁移协议另造一个对齐枚举。
    using theme::TextAlign;

    /// 一行在盒子内的水平偏移量。
    ///
    /// @param align      对齐方式。
    /// @param box_width  可用宽度（被指派的盒子宽度）。
    /// @param line_width 该行自身的测量宽度。
    /// @return 相对盒子左边缘的偏移；盒子比行还窄时返回 0（宁可贴左，也不要把文字推出盒子）。
    [[nodiscard]] inline auto
    text_align_offset(const TextAlign align, const float box_width, const float line_width) noexcept
        -> float {
        const float slack = box_width - line_width;
        if (slack <= 0.0F) {
            return 0.0F;
        }
        switch (align) {
            case TextAlign::center:
                return slack * 0.5F;
            case TextAlign::end:
                return slack;
            case TextAlign::start:
                break;
        }
        return 0.0F;
    }

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
        /// 在给定宽度内的水平对齐；见 text_align_offset()。默认 `start`，与加入之前的行为一致。
        TextAlign align = TextAlign::start;

        /// 逐字段比较（浮点按 `nan_epsilon` 容差，颜色按 `NanColor::approx_equals`）。
        ///
        /// 存在的理由：组件在 `set_style()` 之前都要判断"样式是否真的变了" —— 否则每次
        /// 重解析主题都会触发无谓的重排与重绘。这个判断原先在 20 个组件里**各抄了一份**，
        /// 于是给 TextStyle 加字段会让 20 处同时静默失效（样式变化不再被检测到）。收敛到
        /// 这里之后，加字段只需要改这一处。
        ///
        /// 新增字段时请一并更新本函数 —— 否则又是同一种静默失效，只是换了个地方。
        [[nodiscard]] auto approx_equals(const TextStyle& other) const noexcept -> bool;
    };

    struct TextLayoutInput {
        std::string_view text;
        TextStyle style;
        foundation::NanLayoutConstraints constraints = foundation::NanLayoutConstraints::loose();
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
        /// 绘制期的行起点水平偏移，由 `Text::draw_in()` 按目标矩形写入，整形后端不设置它。
        ///
        /// 它**只影响绘制**：`size` / `baseline` / `caret_stops` 仍是未偏移的布局局部坐标，
        /// 所以任何命中测试（例如 `caret_for_point`）都工作在未对齐的空间里。要画对齐文本
        /// 又要命中的消费者，必须自己把指针 x 减去同一个偏移。
        float origin_x = 0.0F;
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

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_TEXT_LAYOUT_HPP
