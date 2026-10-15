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
            const auto color = style.color.with_alpha(alpha);
            if (ctx.device().supports_rounded_clip()) {
                ctx.device().draw_circle_clipped_rounded_rect(
                    origin,
                    radius,
                    world,
                    ctx.logical_to_screen(container_radius),
                    color
                );
                return;
            }

            // 降级：设备不能按圆角轮廓裁剪。接口默认实现只画**未裁剪**的圆，所以这里必须
            // 自己用矩形裁剪保证水波纹不溢出容器（代价是圆角处按方角裁，见
            // platform_backend_contract.md §3 的能力差异表）。
            ctx.device().set_clip(world);
            ctx.device().draw_circle(origin, radius, color);
            ctx.device().clear_clip();
        }
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_RIPPLE_PAINTER_HPP
