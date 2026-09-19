//
// widget/primitives/arc_painter tests.
//
// ArcPainter's geometry is exercised through a recording device: call counts and
// the inner/outer radius / start / sweep / alpha handed to the backend are
// assertable without a window. These tests do NOT verify pixels — the device only
// records the calls — but they do pin the "one draw_arc call for the whole arc"
// contract that removes the old chord-polyline double-blending.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nandina/render/draw_context.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/widget/primitives/arc_painter.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        struct LineCall {
            foundation::NanPoint a;
            foundation::NanPoint b;
            float thickness = 0.0F;
            foundation::NanColor color;
        };

        struct ArcCall {
            foundation::NanPoint center;
            float inner = 0.0F;
            float outer = 0.0F;
            float start = 0.0F;
            float sweep = 0.0F;
            foundation::NanColor color;
        };

        int rounded_outlines = 0;
        int circles = 0;
        float last_outline_radius = 0.0F;
        float last_outline_thickness = 0.0F;
        foundation::NanRect last_outline_rect;
        foundation::NanColor last_outline_color;
        std::vector<LineCall> lines;
        std::vector<ArcCall> arcs;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_rounded_rect_outline(
            const foundation::NanRect& rect,
            float radius,
            float thickness,
            const foundation::NanColor& color
        ) override {
            ++rounded_outlines;
            last_outline_rect = rect;
            last_outline_radius = radius;
            last_outline_thickness = thickness;
            last_outline_color = color;
        }
        void draw_line(
            const foundation::NanPoint& a,
            const foundation::NanPoint& b,
            float thickness,
            const foundation::NanColor& color
        ) override {
            lines.push_back({a, b, thickness, color});
        }
        void draw_circle(
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++circles;
        }
        void draw_arc(
            const foundation::NanPoint& center,
            float inner,
            float outer,
            float start,
            float sweep,
            const foundation::NanColor& color
        ) override {
            arcs.push_back({center, inner, outer, start, sweep, color});
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
    };

    [[nodiscard]] auto opaque_color() -> foundation::NanColor {
        return foundation::NanColor::from(
            foundation::NanOklch {.light = 0.6F, .chroma = 0.12F, .hue = 250.0F}
        );
    }
} // namespace

TEST_CASE("arc painter ignores non-positive radius and thickness", "[arc-painter]") {
    RecordingDevice dev;
    render::DrawContext context(dev);
    const auto bounds = foundation::NanRect::from_xywh(0.0F, 0.0F, 32.0F, 32.0F);

    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {.radius = 0.0F},
        opaque_color(),
        1.0F
    );
    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {.radius = -4.0F},
        opaque_color(),
        1.0F
    );
    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {.thickness = 0.0F},
        opaque_color(),
        1.0F
    );
    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {.thickness = -2.0F},
        opaque_color(),
        1.0F
    );

    REQUIRE(dev.arcs.empty());
    REQUIRE(dev.rounded_outlines == 0);
    REQUIRE(dev.lines.empty());
    REQUIRE(dev.circles == 0);
}

TEST_CASE("arc painter draws a full ring as one draw_arc call", "[arc-painter]") {
    RecordingDevice dev;
    render::DrawContext context(dev);
    const auto bounds = foundation::NanRect::from_xywh(8.0F, 4.0F, 32.0F, 32.0F);
    const auto color = opaque_color();

    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {
            .radius = 16.0F,
            .thickness = 4.0F,
            .start_radians = 0.0F,
            .sweep_radians = 6.2831853F,
        },
        color,
        1.0F
    );

    REQUIRE(dev.arcs.size() == 1);
    // No chord fallback on the device path: one primitive, no polyline.
    REQUIRE(dev.lines.empty());
    REQUIRE(dev.rounded_outlines == 0);
    const auto& arc = dev.arcs.front();
    // `radius` is the outer radius; `thickness` is the band, so inner = 16 - 4.
    REQUIRE(arc.outer == Catch::Approx(16.0F));
    REQUIRE(arc.inner == Catch::Approx(12.0F));
    REQUIRE(arc.start == Catch::Approx(0.0F));
    REQUIRE(arc.sweep == Catch::Approx(6.2831853F));
    // Centred on the bounds.
    REQUIRE(arc.center.get_x() == Catch::Approx(bounds.get_center().get_x()));
    REQUIRE(arc.center.get_y() == Catch::Approx(bounds.get_center().get_y()));
    REQUIRE(arc.color.alpha() == Catch::Approx(1.0F));
}

TEST_CASE("arc painter strokes a partial arc as one draw_arc call", "[arc-painter]") {
    RecordingDevice dev;
    render::DrawContext context(dev);
    const auto bounds = foundation::NanRect::from_xywh(0.0F, 0.0F, 20.0F, 20.0F);
    const auto color = opaque_color();

    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {
            .radius = 10.0F,
            .thickness = 2.0F,
            .start_radians = 0.0F,
            .sweep_radians = 0.5F,
        },
        color,
        1.0F
    );

    REQUIRE(dev.arcs.size() == 1);
    REQUIRE(dev.lines.empty());
    REQUIRE(dev.rounded_outlines == 0);
    const auto& arc = dev.arcs.front();
    REQUIRE(arc.outer == Catch::Approx(10.0F));
    REQUIRE(arc.inner == Catch::Approx(8.0F));
    // Start / sweep are forwarded unchanged to the native primitive.
    REQUIRE(arc.start == Catch::Approx(0.0F));
    REQUIRE(arc.sweep == Catch::Approx(0.5F));
    REQUIRE(arc.color.alpha() == Catch::Approx(1.0F));
}

TEST_CASE("arc painter treats a degenerate sweep as a no-op", "[arc-painter]") {
    RecordingDevice dev;
    render::DrawContext context(dev);
    const auto bounds = foundation::NanRect::from_xywh(0.0F, 0.0F, 20.0F, 20.0F);

    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {.radius = 10.0F, .thickness = 2.0F, .sweep_radians = 0.0F},
        opaque_color(),
        1.0F
    );
    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {
            .radius = 10.0F,
            .thickness = 2.0F,
            .sweep_radians = 1.0e-9F,
        },
        opaque_color(),
        1.0F
    );

    REQUIRE(dev.arcs.empty());
    REQUIRE(dev.rounded_outlines == 0);
    REQUIRE(dev.lines.empty());
    REQUIRE(dev.circles == 0);
}

TEST_CASE("arc painter clamps a band wider than the radius to a disc", "[arc-painter]") {
    RecordingDevice dev;
    render::DrawContext context(dev);
    const auto bounds = foundation::NanRect::from_xywh(0.0F, 0.0F, 20.0F, 20.0F);

    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {
            .radius = 4.0F,
            .thickness = 8.0F,
            .sweep_radians = 6.2831853F,
        },
        opaque_color(),
        1.0F
    );

    REQUIRE(dev.arcs.size() == 1);
    const auto& arc = dev.arcs.front();
    REQUIRE(arc.outer == Catch::Approx(4.0F));
    // inner = max(0, outer - thickness) -> filled disc, not a negative radius.
    REQUIRE(arc.inner == Catch::Approx(0.0F));
}

TEST_CASE("arc painter scales colour alpha and logical metrics", "[arc-painter]") {
    RecordingDevice dev;
    render::DrawContext context(
        dev,
        foundation::NanTransform2D::from_scale(2.0F),
        {.logical_to_screen = 2.0F}
    );
    const auto bounds = foundation::NanRect::from_xywh(0.0F, 0.0F, 64.0F, 64.0F);

    widget::primitives::ArcPainter::paint(
        context,
        bounds,
        widget::primitives::ArcStyle {
            .radius = 16.0F,
            .thickness = 4.0F,
            .sweep_radians = 6.2831853F,
        },
        opaque_color(),
        0.5F
    );

    REQUIRE(dev.arcs.size() == 1);
    const auto& arc = dev.arcs.front();
    // Logical px are scaled to screen px before handing geometry to the device.
    REQUIRE(arc.outer == Catch::Approx(32.0F)); // 16 * 2
    REQUIRE(arc.inner == Catch::Approx(24.0F)); // (16 - 4) * 2
    REQUIRE(arc.sweep == Catch::Approx(6.2831853F));
    REQUIRE(arc.color.alpha() == Catch::Approx(0.5F));
}
