//
// widget/primitives/box_painter — shared fill / border / radius drawing.
//
// A plain helper, NOT a node: it does not participate in layout, hit testing, or
// lifecycle. Controls own the resolved geometry and call it from on_draw, so the
// rounded-rect fallback and opacity handling live in exactly one place.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PAINTER_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PAINTER_HPP

#include "../../render/draw_context.hpp"
#include "../../theme/design_system.hpp"

namespace nandina::widget::primitives
{
    class BoxPainter {
    public:
        /// Fill + border in one call.
        static auto paint(
            render::DrawContext& ctx,
            foundation::NanRect world,
            const theme::ResolvedBoxStyle& box,
            float parent_opacity
        ) -> void {
            paint_fill(ctx, world, box, parent_opacity);
            paint_outline(ctx, world, box, parent_opacity);
        }

        /// Filled rectangle, falling back to a plain rect when the device cannot
        /// draw rounded rectangles.
        static auto paint_fill(
            render::DrawContext& ctx,
            foundation::NanRect world,
            const theme::ResolvedBoxStyle& box,
            float parent_opacity
        ) -> void {
            if (box.fill.alpha() <= 0.0F) {
                return;
            }
            const auto color = box.fill.with_alpha(box.fill.alpha() * parent_opacity);
            if (box.radius > 0.0F && ctx.device().supports_rounded_rect()) {
                ctx.device().draw_rounded_rect(
                    world,
                    ctx.logical_to_screen(box.radius),
                    color
                );
            }
            else {
                ctx.device().draw_rect(world, color);
            }
        }

        /// Rectangle outline (border). 圆角 > 0 时用圆角描边，避免方框边角与
        /// 圆角填充之间留白（如勾选框的四个角落）。
        static auto paint_outline(
            render::DrawContext& ctx,
            foundation::NanRect world,
            const theme::ResolvedBoxStyle& box,
            float parent_opacity
        ) -> void {
            if (box.border.alpha() <= 0.0F || box.border_width <= 0.0F) {
                return;
            }
            const auto color = box.border.with_alpha(box.border.alpha() * parent_opacity);
            if (box.radius > 0.0F) {
                ctx.device().draw_rounded_rect_outline(
                    world,
                    ctx.logical_to_screen(box.radius),
                    ctx.logical_to_screen(box.border_width),
                    color
                );
            }
            else {
                ctx.device().draw_rect_outline(
                    world,
                    ctx.logical_to_screen(box.border_width),
                    color
                );
            }
        }
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PAINTER_HPP
