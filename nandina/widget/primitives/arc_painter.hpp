//
// widget/primitives/arc_painter — shared ring / arc drawing.
//
// A plain helper, NOT a node: it does not participate in layout, hit testing, or
// lifecycle. Controls own the geometry and call it from on_draw so ring
// rendering (outer radius / thickness / start angle / sweep) lives in one place.
//
// Geometry notes: the ring is submitted as one `IRenderDevice::draw_arc` call,
// which the raylib backend renders as a native antialiased annular sector (SDF).
// A single primitive avoids the double-blending a polyline of translucent
// capsules produced where neighbouring chords overlapped. `radius` is the outer
// radius and `thickness` the band width; angles follow the device convention
// (+Y is 0, increasing toward +X).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_ARC_PAINTER_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_ARC_PAINTER_HPP

#include "../../render/draw_context.hpp"

namespace nandina::widget::primitives
{
    /** Ring / arc geometry. All lengths are logical px. */
    struct ArcStyle {
        float radius = 8.0F;               // outer radius, logical px
        float thickness = 2.0F;            // ring thickness
        float start_radians = 0.0F;        // arc start
        float sweep_radians = 6.2831853F;  // arc extent (6.2831853F == full ring)
    };

    class ArcPainter {
    public:
        /// Stroke a ring or arc of `style` in `color`, centred on `world_bounds`.
        /// @param world_bounds already in world/screen space (see BoxPainter callers).
        /// @param opacity      extra multiplier applied to the colour alpha.
        static void paint(
            render::DrawContext& context,
            foundation::NanRect world_bounds,
            const ArcStyle& style,
            const foundation::NanColor& color,
            float opacity = 1.0F
        );
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_ARC_PAINTER_HPP
