//
// widget/primitives/focus_ring_painter — shared focus-ring drawing.
//
// Plain helper, NOT a node. Normalizes the ring geometry that is currently copied
// into Button / Checkbox / Slider / TextField (each with a slightly different
// expansion gap). `gap` is the inset between the control rect and the ring.
//
// 环必须跟着控件的圆角走：`corner_radius` 是**逻辑单位**的控件圆角（0 = 直角控件）。
// 参数没有默认值，是为了让每个调用点都必须表态 —— 默认值会让圆角控件悄悄退回方框。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_FOCUS_RING_PAINTER_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_FOCUS_RING_PAINTER_HPP

#include "../../render/draw_context.hpp"
#include "../../theme/design_system.hpp"

namespace nandina::widget::primitives
{
    class FocusRingPainter {
    public:
        static auto paint(
            render::DrawContext& ctx,
            foundation::NanRect world,
            const theme::ResolvedFocusRing& ring,
            float parent_opacity,
            float corner_radius,
            float gap = 1.0F
        ) -> void {
            if (ring.width <= 0.0F || ring.color.alpha() <= 0.0F) {
                return;
            }
            // 环画在控件外侧一圈，半径也要跟着外扩同样的量才是**同心**的：
            // 只外扩矩形会让圆角控件（按钮、Chip、开关轨道）外面套上一个方框。
            const auto expansion = ctx.logical_to_screen(ring.width + gap);
            const auto expanded = world.expanded(expansion);
            const auto color = ring.color.with_alpha(ring.color.alpha() * parent_opacity);
            const auto thickness = ctx.logical_to_screen(ring.width);
            if (corner_radius > 0.0F) {
                ctx.device().draw_rounded_rect_outline(
                    expanded,
                    ctx.logical_to_screen(corner_radius) + expansion,
                    thickness,
                    color
                );
                return;
            }
            // 直角控件（如 BreadcrumbLink：它连 pressed 底色都是方的）保持方框环。
            ctx.device().draw_rect_outline(expanded, thickness, color);
        }
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_FOCUS_RING_PAINTER_HPP
