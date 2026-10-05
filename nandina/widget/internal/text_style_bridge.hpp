//
// widget/internal/text_style_bridge — ResolvedTypeStyle → primitives::TextStyle
//
// 组件在 `on_style_context_changed()` / `on_theme_changed()` 里都要做同一件事：把解析后的
// 排版（主题侧）转成 `Text` 能吃的运行时样式，并遵守"样式上下文优先"的继承规则。
//
// 这段转换原先在 select / dropdown_menu / combobox / command_palette 各有一份，四份只差
// `overflow` 一个字段（下拉菜单的标签要裁掉、组合框的值要省略号）。把它收敛成一处，是为了
// 避免同一类"改了 TextStyle 却漏改某个副本"的静默失效。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_TEXT_STYLE_BRIDGE_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_TEXT_STYLE_BRIDGE_HPP

#include "../../text/font_request.hpp"
#include "../../theme/design_system.hpp"
#include "../../theme/style_context.hpp"
#include "../primitives/text_layout.hpp"

namespace nandina::widget::internal
{
    /**
     * 由解析后的排版 + 继承的样式上下文构造 Text 的运行时样式。
     *
     * 继承规则：样式上下文里**显式给出**的颜色 / 字号 / 字体覆盖配方值，否则用配方解析值；
     * 字体再回退到文本节点当前的字体（`fallback_font`），这样组件不必自己知道默认字体。
     *
     * @param overflow  超出盒子的处理方式。默认 `clip`（下拉菜单这类定宽标签的常规选择）；
     *                  需要省略号的场景显式传 `TextOverflow::ellipsis`。
     * @param max_lines 最多铺几行。默认 1（单行标签）。
     */
    [[nodiscard]] inline auto make_text_style(
        const theme::ResolvedStyleContext& context,
        const theme::ResolvedTypeStyle& type,
        const text::FontRequest& fallback_font,
        const primitives::TextOverflow overflow = primitives::TextOverflow::clip,
        const int max_lines = 1
    ) -> primitives::TextStyle {
        return primitives::TextStyle {
            .color = context.text_color_from_context ? context.text_color : type.color,
            .font_size = context.font_size_from_context ? context.font_size : type.font_size,
            .font = context.font_from_context ? context.font : fallback_font,
            .overflow = overflow,
            .max_lines = max_lines,
            // 对齐来自解析后的排版：组件的标签对齐因此可被主题 / 配方调，而不是写死的。
            .align = type.align,
        };
    }
} // namespace nandina::widget::internal

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_TEXT_STYLE_BRIDGE_HPP
