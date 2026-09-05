//
// Render-layer unit tests (Catch2 v3).
//
// RecordingDevice captures every device call into a list of structs, so the
// draw traversal becomes assertable without a window. This is the direct payoff
// of the IRenderDevice abstraction: draw ordering, inherited opacity, clip
// intersection, and visibility skipping are now testable.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "render/clip_stack.hpp"
#include "render/draw_context.hpp"
#include "render/render_device.hpp"
#include "render/backends/sdf_primitive_geometry.hpp"
#include "scene/control.hpp"
#include "scene/canvas_layer.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"
#include "widget/primitives/box_painter.hpp"
#include "widget/primitives/focus_ring_painter.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{

/// Records draw/clip calls for assertions. No windowing.
class RecordingDevice final : public render::IRenderDevice {
public:
    struct RectCall {
        foundation::NanRect rect;
        float alpha;  // color alpha in [0,1], for opacity checks
    };
    struct ClipCall {
        foundation::NanRect rect;
        bool cleared;  // true if this was a clear_clip()
    };

    std::vector<RectCall> rects;
    std::vector<ClipCall> clips;
    std::vector<std::string> events;
    int begin_count = 0;
    int end_count = 0;
    float last_radius = 0.0F;
    float last_thickness = 0.0F;

    void begin_frame() override { ++begin_count; }
    void end_frame() override { ++end_count; }

    void set_clip(const foundation::NanRect& r) override {
        clips.push_back({r, false});
        events.push_back("clip");
    }
    void clear_clip() override {
        clips.push_back({foundation::NanRect::empty(), true});
        events.push_back("clear");
    }

    void draw_rect(const foundation::NanRect& r, const foundation::NanColor& c) override {
        rects.push_back({r, c.alpha()});
        events.push_back("rect");
    }
    void draw_rect_outline(const foundation::NanRect& r, float thickness,
                           const foundation::NanColor& c) override {
        last_thickness = thickness;
        rects.push_back({r, c.alpha()});
    }
    void draw_rounded_rect(const foundation::NanRect& r, float radius,
                           const foundation::NanColor& c) override {
        last_radius = radius;
        rects.push_back({r, c.alpha()});
    }
    void draw_rounded_rect_outline(
        const foundation::NanRect& r,
        float radius,
        float thickness,
        const foundation::NanColor& c
    ) override {
        last_radius = radius;
        last_thickness = thickness;
        rects.push_back({r, c.alpha()});
    }
    void draw_line(const foundation::NanPoint&, const foundation::NanPoint&, float,
                   const foundation::NanColor&) override {}
    void draw_circle(const foundation::NanPoint&, float,
                     const foundation::NanColor&) override {}
    void draw_text(std::string_view, const foundation::NanPoint&, float,
                   const foundation::NanColor&) override {}
};

/// A node that draws one rect at its world origin, tagged via fill alpha so the
/// recorded call order can be identified. opacity() is folded into the alpha.
class RectNode : public scene::NanNode2D {
public:
    explicit RectNode(float tag_alpha) : tag_(tag_alpha) {}

    [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
        return false;
    }

    void on_draw(render::DrawContext& ctx) override {
        const auto p = ctx.world_transform().position();
        const auto rect = foundation::NanRect::from_xywh(p.get_x(), p.get_y(), 10, 10);
        const auto color = foundation::NanColor::from(
            foundation::NanOklch{.light = 0.5F, .chroma = 0.1F, .hue = 120.0F, .alpha = 1.0F});
        ctx.device().draw_rect(rect, color.with_alpha(tag_ * ctx.opacity()));
    }

private:
    float tag_;
};

} // namespace

TEST_CASE("draw brackets the frame exactly once", "[render][frame]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    tree.set_root(std::make_shared<RectNode>(1.0F));

    tree.draw(dev);

    REQUIRE(dev.begin_count == 1);
    REQUIRE(dev.end_count == 1);
    REQUIRE(dev.rects.size() == 1);
}

TEST_CASE("DrawContext keeps viewport and framebuffer scales separate", "[render][scale]") {
    RecordingDevice dev;
    render::DrawContext context(
        dev,
        foundation::NanTransform2D {
            foundation::NanPoint(12.0F, 8.0F),
            0.0F,
            foundation::NanPoint(1.5F, 1.5F),
        },
        {.logical_to_screen = 1.5F, .screen_to_physical = 2.0F}
    );

    REQUIRE(context.world_transform().position().get_x() == Catch::Approx(12.0F));
    REQUIRE(context.logical_to_screen(4.0F) == Catch::Approx(6.0F));
    REQUIRE(context.logical_to_physical(4.0F) == Catch::Approx(12.0F));
}

TEST_CASE("shared painters scale logical visual metrics", "[render][scale][painter]") {
    RecordingDevice dev;
    render::DrawContext context(
        dev,
        foundation::NanTransform2D::from_scale(2.0F),
        {.logical_to_screen = 2.0F}
    );
    const auto color = foundation::NanColor::from(
        foundation::NanOklch {.light = 0.6F, .chroma = 0.1F, .hue = 220.0F}
    );
    const theme::ResolvedBoxStyle box {
        .fill = color,
        .border = color,
        .border_width = 1.0F,
        .radius = 6.0F,
    };
    const auto world = render::world_bounds_from_local(
        context.world_transform(),
        foundation::NanRect::from_xywh(0.0F, 0.0F, 40.0F, 20.0F)
    );

    widget::primitives::BoxPainter::paint(context, world, box, 1.0F);
    REQUIRE(world.get_width() == Catch::Approx(80.0F));
    REQUIRE(dev.last_radius == Catch::Approx(12.0F));
    REQUIRE(dev.last_thickness == Catch::Approx(2.0F));

    widget::primitives::FocusRingPainter::paint(
        context,
        world,
        {.color = color, .width = 2.0F},
        1.0F
    );
    REQUIRE(dev.last_thickness == Catch::Approx(4.0F));
    REQUIRE(dev.rects.back().rect.get_left() == Catch::Approx(-6.0F));
    REQUIRE(dev.rects.back().rect.get_width() == Catch::Approx(92.0F));
}

TEST_CASE("viewport root transform survives CanvasLayer traversal", "[render][scale][layer]") {
    RecordingDevice dev;
    auto content = std::make_shared<RectNode>(1.0F);
    content->set_position(foundation::NanPoint(4.0F, 6.0F));
    auto layer = scene::CanvasLayer::create();
    layer->set_canvas_transform(
        foundation::NanTransform2D::from_position(foundation::NanPoint(2.0F, 3.0F))
    );
    layer->add_child(content);
    auto stack = scene::LayerStack::create();
    stack->add_layer(layer);

    scene::NanSceneTree tree;
    tree.set_root(stack);
    render::DrawContext context(
        dev,
        foundation::NanTransform2D {
            foundation::NanPoint(10.0F, 20.0F),
            0.0F,
            foundation::NanPoint(2.0F, 2.0F),
        },
        {.logical_to_screen = 2.0F}
    );
    tree.render(context);

    REQUIRE(dev.rects.size() == 1);
    REQUIRE(dev.rects.front().rect.get_left() == Catch::Approx(22.0F));
    REQUIRE(dev.rects.front().rect.get_top() == Catch::Approx(38.0F));
}

TEST_CASE("children draw in z-index order (low to high)", "[render][zorder]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(0.1F);
    auto low = std::make_shared<RectNode>(0.3F);
    auto high = std::make_shared<RectNode>(0.9F);
    low->set_z_index(1);
    high->set_z_index(10);
    // Add high first to prove ordering is by z_index, not insertion.
    root->add_child(high);
    root->add_child(low);
    tree.set_root(root);

    tree.draw(dev);

    // Order: root (0.1), then low z=1 (0.3), then high z=10 (0.9).
    REQUIRE(dev.rects.size() == 3);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(0.1F));
    REQUIRE(dev.rects[1].alpha == Catch::Approx(0.3F));
    REQUIRE(dev.rects[2].alpha == Catch::Approx(0.9F));
}

TEST_CASE("world transform composes down the tree", "[render][transform]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(1.0F);
    auto child = std::make_shared<RectNode>(1.0F);
    root->set_position(foundation::NanPoint(100, 50));
    child->set_position(foundation::NanPoint(10, 5));
    root->add_child(child);
    tree.set_root(root);

    tree.draw(dev);

    REQUIRE(dev.rects.size() == 2);
    // root at (100,50)
    REQUIRE(dev.rects[0].rect.get_left() == Catch::Approx(100.0F));
    REQUIRE(dev.rects[0].rect.get_top() == Catch::Approx(50.0F));
    // child world = root + local = (110, 55)
    REQUIRE(dev.rects[1].rect.get_left() == Catch::Approx(110.0F));
    REQUIRE(dev.rects[1].rect.get_top() == Catch::Approx(55.0F));
}

TEST_CASE("invisible subtree produces no draw calls", "[render][visibility]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(1.0F);
    auto hidden = std::make_shared<RectNode>(1.0F);
    auto grandchild = std::make_shared<RectNode>(1.0F);
    hidden->add_child(grandchild);
    hidden->set_visible(false);
    root->add_child(hidden);
    tree.set_root(root);

    tree.draw(dev);

    // Only root draws; hidden and its child are skipped entirely.
    REQUIRE(dev.rects.size() == 1);
}

TEST_CASE("ClipStack intersects with the parent clip", "[render][clip]") {
    RecordingDevice dev;
    render::ClipStack clip(dev);

    {
        auto outer = clip.push(foundation::NanRect::from_xywh(0, 0, 100, 100));
        REQUIRE(dev.clips.size() == 1);
        REQUIRE(dev.clips[0].rect.get_width() == Catch::Approx(100.0F));

        {
            // Child clip exceeds parent on the right/bottom; result is the
            // intersection (50..100 x 50..100 => x=50,y=50,w=50,h=50).
            auto inner = clip.push(foundation::NanRect::from_xywh(50, 50, 100, 100));
            REQUIRE(clip.depth() == 2);
            const auto& top = dev.clips.back();
            REQUIRE(top.rect.get_left() == Catch::Approx(50.0F));
            REQUIRE(top.rect.get_top() == Catch::Approx(50.0F));
            REQUIRE(top.rect.get_width() == Catch::Approx(50.0F));
            REQUIRE(top.rect.get_height() == Catch::Approx(50.0F));
        }
        // inner popped: back to the outer rect.
        REQUIRE(clip.depth() == 1);
        REQUIRE(dev.clips.back().rect.get_width() == Catch::Approx(100.0F));
    }
    // outer popped: stack empty => clear_clip issued.
    REQUIRE(clip.depth() == 0);
    REQUIRE(dev.clips.back().cleared);
}

TEST_CASE("SDF quad bounds preserve the exterior antialiasing band", "[render][sdf]") {
    const auto shape = foundation::NanRect::from_xywh(10.0F, 20.0F, 40.0F, 24.0F);

    const auto fill = render::detail::sdf_quad_bounds(
        shape,
        render::detail::SdfPrimitiveMode::fill
    );
    REQUIRE(fill.get_left() == Catch::Approx(9.0F));
    REQUIRE(fill.get_top() == Catch::Approx(19.0F));
    REQUIRE(fill.get_width() == Catch::Approx(42.0F));
    REQUIRE(fill.get_height() == Catch::Approx(26.0F));

    const auto outline = render::detail::sdf_quad_bounds(
        shape,
        render::detail::SdfPrimitiveMode::outline,
        2.0F
    );
    REQUIRE(outline.get_left() == Catch::Approx(7.0F));
    REQUIRE(outline.get_top() == Catch::Approx(17.0F));
    REQUIRE(outline.get_width() == Catch::Approx(46.0F));
    REQUIRE(outline.get_height() == Catch::Approx(30.0F));
}

TEST_CASE("SDF component outlines keep their full width inside bounds", "[render][sdf]") {
    const auto outer = foundation::NanRect::from_xywh(10.0F, 20.0F, 40.0F, 24.0F);
    const auto outline = render::detail::sdf_inner_outline_geometry(outer, 6.0F, 1.0F);

    // 1px 边框的中心线内移 0.5px：外边界仍是原始 bounds，内边界恰好
    // 落在下一条整数像素边界，避免两列半透明像素造成发虚。
    REQUIRE(outline.centerline_bounds.get_left() == Catch::Approx(10.5F));
    REQUIRE(outline.centerline_bounds.get_top() == Catch::Approx(20.5F));
    REQUIRE(outline.centerline_bounds.get_width() == Catch::Approx(39.0F));
    REQUIRE(outline.centerline_bounds.get_height() == Catch::Approx(23.0F));
    REQUIRE(outline.centerline_radius == Catch::Approx(5.5F));
    REQUIRE(outline.half_width == Catch::Approx(0.5F));

    const auto quad = render::detail::sdf_quad_bounds(
        outline.centerline_bounds,
        render::detail::SdfPrimitiveMode::outline,
        outline.half_width
    );
    REQUIRE(quad.get_left() == Catch::Approx(9.0F));
    REQUIRE(quad.get_top() == Catch::Approx(19.0F));
    REQUIRE(quad.get_right() == Catch::Approx(51.0F));
    REQUIRE(quad.get_bottom() == Catch::Approx(45.0F));
}

TEST_CASE("SDF component outlines snap every fractional edge to whole pixels", "[render][sdf]") {
    // 模拟 Button：位置和文本测量宽度都可能带小数。只吸附 origin 不足以让
    // right/bottom 变成整数，必须独立吸附四条边。
    const auto outer = foundation::NanRect::from_ltrb(12.2F, 51.1F, 82.7F, 91.2F);
    const auto snapped = render::detail::snap_outline_bounds(outer);
    REQUIRE(snapped.get_left() == Catch::Approx(12.0F));
    REQUIRE(snapped.get_top() == Catch::Approx(51.0F));
    REQUIRE(snapped.get_right() == Catch::Approx(83.0F));
    REQUIRE(snapped.get_bottom() == Catch::Approx(91.0F));

    const auto outline = render::detail::sdf_inner_outline_geometry(outer, 6.0F, 1.0F);
    REQUIRE(outline.centerline_bounds.get_left() == Catch::Approx(12.5F));
    REQUIRE(outline.centerline_bounds.get_top() == Catch::Approx(51.5F));
    REQUIRE(outline.centerline_bounds.get_right() == Catch::Approx(82.5F));
    REQUIRE(outline.centerline_bounds.get_bottom() == Catch::Approx(90.5F));
    REQUIRE(outline.centerline_bounds.get_width() == Catch::Approx(70.0F));
    REQUIRE(outline.centerline_bounds.get_height() == Catch::Approx(39.0F));
}

TEST_CASE("SDF segment bounds support zero-length round dots", "[render][sdf]") {
    const foundation::NanPoint point(12.0F, 18.0F);
    const auto bounds = render::detail::sdf_segment_bounds(point, point, 2.0F);

    REQUIRE(bounds.get_left() == Catch::Approx(9.0F));
    REQUIRE(bounds.get_top() == Catch::Approx(15.0F));
    REQUIRE(bounds.get_width() == Catch::Approx(6.0F));
    REQUIRE(bounds.get_height() == Catch::Approx(6.0F));
}

TEST_CASE("NanControl overflow clip applies to child drawing", "[render][clip][control]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 30.0F));
    root->set_position(foundation::NanPoint(10.0F, 20.0F));
    root->set_overflow(scene::ControlOverflow::clip);

    auto child = std::make_shared<RectNode>(1.0F);
    child->set_position(foundation::NanPoint(30.0F, 20.0F));
    root->add_child(child);
    tree.set_root(root);

    tree.draw(dev);

    REQUIRE(dev.clips.size() == 2);
    REQUIRE_FALSE(dev.clips.front().cleared);
    REQUIRE(dev.clips.front().rect.get_left() == Catch::Approx(10.0F));
    REQUIRE(dev.clips.front().rect.get_top() == Catch::Approx(20.0F));
    REQUIRE(dev.clips.front().rect.get_width() == Catch::Approx(40.0F));
    REQUIRE(dev.clips.front().rect.get_height() == Catch::Approx(30.0F));
    REQUIRE(dev.clips.back().cleared);
    REQUIRE(dev.events.size() == 3);
    REQUIRE(dev.events[0] == "clip");
    REQUIRE(dev.events[1] == "rect");
    REQUIRE(dev.events[2] == "clear");
}

TEST_CASE("nested control overflow clips are intersected", "[render][clip][control]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 100.0F));
    root->set_overflow(scene::ControlOverflow::clip);

    auto inner = std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 100.0F));
    inner->set_position(foundation::NanPoint(50.0F, 50.0F));
    inner->set_overflow(scene::ControlOverflow::clip);

    auto child = std::make_shared<RectNode>(1.0F);
    inner->add_child(child);
    root->add_child(inner);
    tree.set_root(root);

    tree.draw(dev);

    REQUIRE(dev.clips.size() == 4);
    REQUIRE(dev.clips[0].rect.get_width() == Catch::Approx(100.0F));
    REQUIRE(dev.clips[1].rect.get_left() == Catch::Approx(50.0F));
    REQUIRE(dev.clips[1].rect.get_top() == Catch::Approx(50.0F));
    REQUIRE(dev.clips[1].rect.get_width() == Catch::Approx(50.0F));
    REQUIRE(dev.clips[1].rect.get_height() == Catch::Approx(50.0F));
    REQUIRE(dev.clips[2].rect.get_width() == Catch::Approx(100.0F));
    REQUIRE(dev.clips[3].cleared);
}

TEST_CASE("node local opacity multiplies once per node (no exponential decay)", "[render][opacity]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(1.0F);
    auto child = std::make_shared<RectNode>(1.0F);

    root->set_local_opacity(0.5F);
    root->add_child(child);
    tree.set_root(root);
    tree.draw(dev);

    REQUIRE(dev.rects.size() == 2);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(0.5F)); // root: 0.5
    REQUIRE(dev.rects[1].alpha == Catch::Approx(0.5F)); // child: 0.5 × 1.0 (not 0.25)
}

TEST_CASE("deep nesting multiplies each local opacity exactly once", "[render][opacity]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto a = std::make_shared<RectNode>(1.0F);
    auto b = std::make_shared<RectNode>(1.0F);
    auto c = std::make_shared<RectNode>(1.0F);

    a->set_local_opacity(0.5F);
    b->set_local_opacity(0.5F);
    c->set_local_opacity(0.5F);
    b->add_child(c);
    a->add_child(b);
    tree.set_root(a);
    tree.draw(dev);

    REQUIRE(dev.rects.size() == 3);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(0.5F));
    REQUIRE(dev.rects[1].alpha == Catch::Approx(0.25F));
    REQUIRE(dev.rects[2].alpha == Catch::Approx(0.125F));
}

TEST_CASE("sibling opacity is isolated and the context restores after a subtree", "[render][opacity]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto root = std::make_shared<RectNode>(1.0F);
    auto faded = std::make_shared<RectNode>(1.0F);
    auto faded_child = std::make_shared<RectNode>(1.0F);
    auto sibling = std::make_shared<RectNode>(1.0F);

    faded->set_local_opacity(0.5F);
    faded->add_child(faded_child);
    root->add_child(faded);
    root->add_child(sibling);
    tree.set_root(root);
    tree.draw(dev);

    REQUIRE(dev.rects.size() == 4);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(1.0F)); // root
    REQUIRE(dev.rects[1].alpha == Catch::Approx(0.5F)); // faded subtree root
    REQUIRE(dev.rects[2].alpha == Catch::Approx(0.5F)); // faded child (0.5 × 1.0)
    REQUIRE(dev.rects[3].alpha == Catch::Approx(1.0F)); // sibling restored to opaque
}

TEST_CASE("canvas layers draw by layer order and reset their transform", "[render][canvas-layer]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto stack = scene::LayerStack::create();
    auto foreground = scene::CanvasLayer::create(scene::CanvasSpace::screen, 10);
    auto background = scene::CanvasLayer::create(scene::CanvasSpace::world, -5);
    auto foreground_node = std::make_shared<RectNode>(0.9F);
    auto background_node = std::make_shared<RectNode>(0.2F);
    foreground_node->set_position(foundation::NanPoint(5.0F, 6.0F));
    background_node->set_position(foundation::NanPoint(2.0F, 3.0F));
    foundation::NanTransform2D camera;
    camera.set_position(foundation::NanPoint(100.0F, 50.0F));
    background->set_canvas_transform(camera);
    foreground->add_child(foreground_node);
    background->add_child(background_node);
    stack->add_layer(foreground);
    stack->add_layer(background);
    tree.set_root(stack);

    tree.draw(dev);

    REQUIRE(dev.rects.size() == 2);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(0.2F));
    REQUIRE(dev.rects[0].rect.get_left() == Catch::Approx(102.0F));
    REQUIRE(dev.rects[0].rect.get_top() == Catch::Approx(53.0F));
    REQUIRE(dev.rects[1].alpha == Catch::Approx(0.9F));
    REQUIRE(dev.rects[1].rect.get_left() == Catch::Approx(5.0F));
    REQUIRE(dev.rects[1].rect.get_top() == Catch::Approx(6.0F));
}

TEST_CASE("CanvasLayer inherited transform aliases its canvas transform", "[render][canvas-layer]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto stack = scene::LayerStack::create();
    auto layer = scene::CanvasLayer::create(scene::CanvasSpace::world, 2);
    auto node = std::make_shared<RectNode>(1.0F);
    node->set_position(foundation::NanPoint(3.0F, 4.0F));
    layer->set_position(foundation::NanPoint(20.0F, 10.0F));
    layer->add_child(node);
    stack->add_layer(layer);
    tree.set_root(stack);

    REQUIRE(layer->canvas_transform() == layer->transform());
    REQUIRE(layer->order() == 2);
    REQUIRE(layer->z_index_hint() == 2);
    tree.draw(dev);
    REQUIRE(dev.rects[0].rect.get_left() == Catch::Approx(23.0F));
    REQUIRE(dev.rects[0].rect.get_top() == Catch::Approx(14.0F));

    layer->set_order(7);
    REQUIRE(layer->z_index_hint() == 7);
}
