//
// widget/primitives/ripple_painter - shared clipped impact feedback painter.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_RIPPLE_PAINTER_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_RIPPLE_PAINTER_HPP

#include "../../render/draw_context.hpp"
#include "../../theme/design_system.hpp"

#include <algorithm>
#include <cmath>

namespace nandina::widget::primitives
{
    class RipplePainter {
    public:
        static void paint(
            render::DrawContext& ctx,
            const foundation::NanRect& world,
            const foundation::NanPoint& origin,
            const float container_radius,
            const theme::ResolvedRippleStyle& style,
            const float progress,
            const float parent_opacity
        ) {
            if (progress <= 0.0F || progress >= 1.0F || style.color.alpha() <= 0.0F) {
                return;
            }
            const float far_x = std::max(
                std::abs(origin.get_x() - world.get_left()),
                std::abs(world.get_right() - origin.get_x())
            );
            const float far_y = std::max(
                std::abs(origin.get_y() - world.get_top()),
                std::abs(world.get_bottom() - origin.get_y())
            );
            const float eased = 1.0F - std::pow(1.0F - std::clamp(progress, 0.0F, 1.0F), 3.0F);
            const float radius = std::hypot(far_x, far_y) * eased;
            const float alpha = style.color.alpha() * (1.0F - progress) * parent_opacity;
            ctx.device().draw_circle_clipped_rounded_rect(
                origin,
                radius,
                world,
                ctx.logical_to_screen(container_radius),
                style.color.with_alpha(alpha)
            );
        }
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_RIPPLE_PAINTER_HPP
