//
// widget/primitives/shadow_painter — shared soft-shadow drawing.
//
// A plain helper, NOT a node: it does not participate in layout, hit testing, or
// lifecycle. Controls own the resolved geometry and call it from on_draw before
// painting the container, so elevation stays in exactly one place.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_SHADOW_PAINTER_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_SHADOW_PAINTER_HPP

#include "../../render/draw_context.hpp"
#include "../../theme/design_system.hpp"

namespace nandina::widget::primitives
{
    class ShadowPainter {
    public:
        /// 在 `world` 矩形下方绘制软阴影（offset 偏移 + spread 软边衰减）。
        /// 阴影颜色 alpha ≤ 0 或 spread ≤ 0 时跳过。
        static auto paint(
            render::DrawContext& ctx,
            foundation::NanRect world,
            const float radius,
            const theme::ResolvedShadowStyle& shadow,
            const float parent_opacity
        ) -> void {
            if (shadow.color.alpha() <= 0.0F || shadow.spread <= 0.0F) {
                return;
            }
            const float offset_x = ctx.logical_to_screen(shadow.offset_x);
            const float offset_y = ctx.logical_to_screen(shadow.offset_y);
            const auto shadow_rect = foundation::NanRect::from_xywh(
                world.get_left() + offset_x,
                world.get_top() + offset_y,
                world.get_width(),
                world.get_height()
            );
            const auto color = shadow.color.with_alpha(shadow.color.alpha() * parent_opacity);
            ctx.device().draw_rounded_rect_shadow(
                shadow_rect,
                ctx.logical_to_screen(radius),
                ctx.logical_to_screen(shadow.spread),
                color
            );
        }
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_SHADOW_PAINTER_HPP
