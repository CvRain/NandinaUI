//
// widget/primitives/focus_ring_painter — shared focus-ring drawing.
//
// Plain helper, NOT a node. Normalizes the ring geometry that is currently copied
// into Button / Checkbox / Slider / TextField (each with a slightly different
// expansion gap). `gap` is the inset between the control rect and the ring.
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
            float gap = 1.0F
        ) -> void {
            if (ring.width <= 0.0F || ring.color.alpha() <= 0.0F) {
                return;
            }
            const auto expanded = world.expanded(ctx.logical_to_screen(ring.width + gap));
            ctx.device().draw_rect_outline(
                expanded,
                ctx.logical_to_screen(ring.width),
                ring.color.with_alpha(ring.color.alpha() * parent_opacity)
            );
        }
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_FOCUS_RING_PAINTER_HPP
