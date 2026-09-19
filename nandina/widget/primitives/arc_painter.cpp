//
// widget/primitives/arc_painter — shared ring / arc drawing.
//

#include "arc_painter.hpp"

#include <algorithm>
#include <cmath>

namespace nandina::widget::primitives
{
    void ArcPainter::paint(
        render::DrawContext& context,
        const foundation::NanRect world_bounds,
        const ArcStyle& style,
        const foundation::NanColor& color,
        const float opacity
    ) {
        const float outer = context.logical_to_screen(style.radius);
        const float thickness = context.logical_to_screen(style.thickness);
        const float sweep = style.sweep_radians;
        if (!std::isfinite(outer) || !std::isfinite(thickness) || !std::isfinite(sweep)
            || !std::isfinite(style.start_radians) || outer <= 0.0F || thickness <= 0.0F)
        {
            return;
        }
        // A zero/rounded-to-zero sweep has no extent: nothing to draw.
        if (std::abs(sweep) < foundation::nan_epsilon) {
            return;
        }
        const float alpha = color.alpha() * opacity;
        if (!std::isfinite(alpha) || alpha <= 0.0F) {
            return;
        }
        // `radius` is the outer radius and `thickness` the band width, so the
        // inner radius is outer - thickness. A band wider than the radius
        // collapses the hole; clamping to 0 keeps the previous filled-disc
        // behaviour. The whole annular sector is one antialiased device call,
        // so translucent spans no longer double-blend where chords overlapped.
        const float inner = std::max(0.0F, outer - thickness);
        if (inner >= outer) {
            return;
        }
        context.device().draw_arc(
            world_bounds.get_center(),
            inner,
            outer,
            style.start_radians,
            sweep,
            color.with_alpha(alpha)
        );
    }
} // namespace nandina::widget::primitives
