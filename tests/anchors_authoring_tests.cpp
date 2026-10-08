// Include the author-facing header directly, without the controls umbrella.
#include <nandina/widget/build_context.hpp>

#include <nandina/render/draw_context.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/card.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <array>
#include <vector>

using namespace nandina;
using Catch::Matchers::ContainsSubstring;

namespace
{
    struct Context {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
    };

    template<typename Builder, typename Ref>
    concept CanBind = requires(Builder& builder, const Ref& ref) { builder.bind(ref); };
    using ColumnBuilder = widget::authoring::NodeBuilder<widget::Column>;
    static_assert(CanBind<ColumnBuilder, scene::NodeRef<widget::Column>>);
    static_assert(CanBind<ColumnBuilder, scene::NodeRef<scene::NanControl>>);
    static_assert(!CanBind<ColumnBuilder, scene::NodeRef<widget::Row>>);
    static_assert(!std::is_assignable_v<
                  decltype(std::declval<ColumnBuilder>().anchor.left)&,
                  scene::AnchorTarget>);

    void layout(scene::NanControl& root, float width = 400.0F, float height = 200.0F) {
        const foundation::NanSize size(width, height);
        (void)root.measure_layout(foundation::NanLayoutConstraints::tight(size));
        root.layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, width, height));
    }

    class RecordingDevice final: public render::IRenderDevice {
    public:
        std::vector<float> alphas;
        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor& color) override {
            alphas.push_back(color.alpha());
        }
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void
        draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
    };
} // namespace

TEST_CASE(
    "anchor builders bind anonymous forward references and named relations",
    "[anchors-authoring][forward]"
) {
    Context ctx;
    const auto side = ctx.ui.ref<widget::Column>();
    const auto edit = ctx.ui.ref<widget::Column>();
    auto canvas = ctx.ui.anchor_canvas()
                      .children(
                          ctx.ui.column().bind(edit).anchors(
                              {.left = side.anchor.right + 12.0F,
                               .right = edit.parent.anchor.right,
                               .top = edit.parent.anchor.top,
                               .bottom = edit.parent.anchor.bottom}
                          ),
                          ctx.ui.column()
                              .bind(side)
                              .width(scene::percent(30.0F))
                              .anchors(
                                  {.left = side.parent.anchor.left,
                                   .top = side.parent.anchor.top,
                                   .bottom = side.parent.anchor.bottom}
                              )
                      )
                      .build();
    layout(*canvas);
    REQUIRE(side.lock()->width() == Catch::Approx(120.0F));
    REQUIRE(edit.lock()->position().get_x() == Catch::Approx(132.0F));
    REQUIRE(edit.lock()->width() == Catch::Approx(268.0F));
    REQUIRE(canvas->get_child(0) == edit.lock().get());
    REQUIRE_THROWS_WITH(ctx.ui.column().bind(side), ContainsSubstring("already bound"));

    auto named = ctx.ui.column().width(60.0F).height(25.0F);
    auto copy = named;
    copy.anchors({.right = copy.parent.anchor.right});
    auto follower =
        ctx.ui.column().width(20.0F).height(25.0F).anchors({.right = named.anchor.left - 10.0F});
    auto named_canvas = ctx.ui.anchor_canvas().children(follower, copy).build();
    layout(*named_canvas);
    REQUIRE(named.get().position().get_x() == Catch::Approx(340.0F));
    REQUIRE(follower.get().position().get_x() == Catch::Approx(310.0F));
    // Removing bind(ref), anchors forwarding, or solving in Canvas::add makes this red.
}

TEST_CASE(
    "builder expressions outlive the build stack but never retain nodes",
    "[anchors-authoring][lifetime]"
) {
    Context ctx;
    const auto canvas = ctx.ui.anchor_canvas().build();
    const auto expression = [&] {
        auto node = ctx.ui.column().width(70.0F).height(20.0F);
        node.anchors({.left = node.parent.anchor.left + 15.0F});
        canvas->add(node.build());
        return node.anchor.right;
    }();
    auto follower =
        ctx.ui.column().width(20.0F).height(20.0F).anchors({.left = expression + 5.0F}).build();
    canvas->add(follower);
    layout(*canvas);
    REQUIRE(follower->position().get_x() == Catch::Approx(90.0F));
    auto removed = canvas->remove_child(*canvas->get_child(0));
    std::weak_ptr<scene::NanNode> weak = removed;
    removed.reset();
    REQUIRE(weak.expired());
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("expired"));
    // Capturing a builder/Node by reference, or retaining shared ownership, breaks this contract.
}

TEST_CASE(
    "reactive anchors replace specs and release with their build scope",
    "[anchors-authoring][reactive]"
) {
    Context ctx;
    reactive::Signal<scene::AnchorSpec> source(ctx.graph, {});
    auto child = ctx.ui.column().width(40.0F).height(20.0F).anchors(source).build();
    scene::NodeRef<widget::Column> ref(child);
    auto canvas = ctx.ui.anchor_canvas().children(child).build();
    source.set({.left = ref.parent.anchor.left + 10.0F});
    layout(*canvas);
    REQUIRE(child->position().get_x() == Catch::Approx(10.0F));
    source.set({.right = ref.parent.anchor.right - 5.0F});
    layout(*canvas);
    REQUIRE(child->position().get_x() == Catch::Approx(355.0F));
    REQUIRE_FALSE(child->anchors().left.has_value());
    ctx.scope.clear();
    source.set({.left = ref.parent.anchor.left});
    layout(*canvas);
    REQUIRE(child->position().get_x() == Catch::Approx(355.0F));
    REQUIRE_THROWS_WITH(
        widget::authoring::column().anchors(source),
        ContainsSubstring("BuildContext")
    );
    // Replacing effect with a one-time source.get(), or merging specs, makes this red.
}

TEST_CASE("reactive anchors do not own a destroyed target", "[anchors-authoring][weak-binding]") {
    Context ctx;
    auto& source = ctx.ui.signal_value(scene::AnchorSpec {});
    auto child = ctx.ui.column().anchors(source).build();
    std::weak_ptr<widget::Column> weak = child;
    child.reset();
    REQUIRE(weak.expired());
    REQUIRE_NOTHROW(source.set({}));
    // Capturing node_ rather than weak_ptr inside the effect keeps this target alive.
}

TEST_CASE(
    "nested authored canvases solve locally and reject cross canvas targets",
    "[anchors-authoring][nested]"
) {
    Context ctx;
    auto inner = ctx.ui.anchor_canvas();
    inner.anchors(
        {.left = inner.parent.anchor.left + 20.0F,
         .right = inner.parent.anchor.right - 20.0F,
         .top = inner.parent.anchor.top + 10.0F,
         .bottom = inner.parent.anchor.bottom - 10.0F}
    );
    auto child = ctx.ui.column().width(scene::percent(50.0F)).height(20.0F);
    child.anchors({.right = child.parent.anchor.right});
    inner.children(child);
    auto outer = ctx.ui.anchor_canvas().children(inner).build();
    layout(*outer);
    REQUIRE(inner.get().width() == Catch::Approx(360.0F));
    REQUIRE(child.get().width() == Catch::Approx(180.0F));
    REQUIRE(child.get().position().get_x() == Catch::Approx(180.0F));
    REQUIRE(child.get().global_bounds().get_x() == Catch::Approx(200.0F));
    scene::NodeRef<scene::AnchorCanvas> outer_ref(outer);
    child.anchors({.left = outer_ref.anchor.left});
    REQUIRE_THROWS_WITH(layout(*outer), ContainsSubstring("outside this canvas"));
    // Using a global/root percentage basis or ancestor search changes the values/diagnostic.
}

TEST_CASE(
    "authored canvases inside scrolling preserve local anchor geometry",
    "[anchors-authoring][scroll]"
) {
    Context ctx;
    auto canvas = ctx.ui.anchor_canvas().height(500.0F);
    auto child = ctx.ui.column().width(scene::percent(50.0F)).height(30.0F);
    child.anchors({.right = child.parent.anchor.right, .top = child.parent.anchor.top + 120.0F});
    canvas.children(child);
    auto scroll = ctx.ui.scroll_view().child(canvas).build();
    scene::NanSceneTree tree;
    tree.set_root(scroll);
    tree.layout_root(foundation::NanSize(400.0F, 200.0F));
    REQUIRE(child.get().width() == Catch::Approx(200.0F));
    REQUIRE(child.get().position().get_y() == Catch::Approx(120.0F));
    scroll->set_scroll_offset(foundation::NanPoint(0.0F, 100.0F));
    REQUIRE(child.get().position().get_y() == Catch::Approx(120.0F));
    REQUIRE(child.get().global_bounds().get_y() == Catch::Approx(20.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(220.0F, 25.0F)) == child.build().get());
    scroll->set_scroll_offset(foundation::NanPoint(0.0F, 160.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(220.0F, -30.0F)) == nullptr);
    layout(*scroll);
    REQUIRE(canvas.get().height() == Catch::Approx(500.0F));
    auto unbounded = ctx.ui.scroll_view().child(ctx.ui.anchor_canvas()).build();
    REQUIRE_THROWS_WITH(layout(*unbounded), ContainsSubstring("finite"));
    // Using scrolled/global bounds in the solver or removing the finite-canvas check fails here.
}

TEST_CASE(
    "anchor dependency order never changes drawing or hit z order",
    "[anchors-authoring][z-order]"
) {
    Context ctx;
    auto target = ctx.ui.column().width(80.0F).height(40.0F);
    target.configure([](widget::Column& node) {
        node.set_background(foundation::NanColor::from_rgb(1.0F, 1.0F, 1.0F).with_alpha(0.25F));
    });
    auto follower =
        ctx.ui.column().width(80.0F).height(40.0F).anchors({.left = target.anchor.left});
    follower.configure([](widget::Column& node) {
        node.set_background(foundation::NanColor::from_rgb(1.0F, 1.0F, 1.0F).with_alpha(0.75F));
    });
    auto canvas = ctx.ui.anchor_canvas().children(follower, target).build();
    scene::NanSceneTree tree;
    tree.set_root(canvas);
    tree.layout_root(foundation::NanSize(200.0F, 100.0F));
    RecordingDevice device;
    tree.draw(device);
    REQUIRE(device.alphas == std::vector<float> {0.75F, 0.25F});
    REQUIRE(tree.hit_test(foundation::NanPoint(10.0F, 10.0F)) == target.build().get());
    follower.get().set_z_index(2);
    device.alphas.clear();
    tree.draw(device);
    REQUIRE(device.alphas == std::vector<float> {0.25F, 0.75F});
    REQUIRE(tree.hit_test(foundation::NanPoint(10.0F, 10.0F)) == follower.build().get());
    REQUIRE(canvas->get_child(0) == follower.build().get());
    // Reordering tree children into solver order, or bypassing scene z sorting, makes this red.
}

TEST_CASE(
    "authored multi node mode switches commit before publishing state",
    "[anchors-authoring][batch]"
) {
    Context ctx;
    auto& right = ctx.ui.signal_value(false);
    auto side = ctx.ui.column().width(100.0F).height(40.0F);
    auto edit = ctx.ui.column().height(40.0F);
    edit.anchors({.left = side.anchor.right + 12.0F, .right = edit.parent.anchor.right});
    auto canvas = ctx.ui.anchor_canvas().children(edit, side).build();
    layout(*canvas);
    const auto switch_mode = [side_ref = scene::NodeRef<widget::Column>(side.build()),
                              edit_ref = scene::NodeRef<widget::Column>(edit.build()),
                              weak = std::weak_ptr<scene::AnchorCanvas>(canvas),
                              &right] {
        const auto current = weak.lock();
        if (!current) {
            return;
        }
        const std::array updates {
            scene::AnchorCanvas::Update {side_ref.lock(), {.right = side_ref.parent.anchor.right}},
            scene::AnchorCanvas::Update {
                edit_ref.lock(),
                {.left = edit_ref.parent.anchor.left, .right = side_ref.anchor.left - 12.0F}
            }
        };
        current->set_child_anchors(updates);
        right.set(true);
    };
    switch_mode();
    REQUIRE(right.get());
    layout(*canvas);
    REQUIRE(side.get().position().get_x() == Catch::Approx(300.0F));
    REQUIRE(edit.get().width() == Catch::Approx(288.0F));
    // Skipping batch commit (or writing presentation translation) leaves old layout geometry.
}

TEST_CASE(
    "a canvas page root chains a header, sidebar and editor through sibling anchors",
    "[anchors-authoring][page-root]"
) {
    Context ctx;
    constexpr float kHeaderHeight = 56.0F;
    constexpr float kSidebarWidth = 220.0F;
    constexpr float kGap = 12.0F;
    const auto header = ctx.ui.ref<widget::Row>();
    const auto side = ctx.ui.ref<widget::Column>();
    const auto edit = ctx.ui.ref<widget::Column>();

    auto canvas = ctx.ui.anchor_canvas()
                      .children(
                          ctx.ui.row()
                              .bind(header)
                              .height(kHeaderHeight)
                              .anchors(
                                  {.left = header.parent.anchor.left,
                                   .right = header.parent.anchor.right,
                                   .top = header.parent.anchor.top}
                              ),
                          ctx.ui.column()
                              .bind(side)
                              .width(kSidebarWidth)
                              .anchors(
                                  {.left = side.parent.anchor.left,
                                   .top = header.anchor.bottom + kGap,
                                   .bottom = side.parent.anchor.bottom}
                              ),
                          ctx.ui.column().bind(edit).anchors(
                              {.left = side.anchor.right + kGap,
                               .right = edit.parent.anchor.right,
                               .top = header.anchor.bottom + kGap,
                               .bottom = edit.parent.anchor.bottom}
                          )
                      )
                      .build();
    layout(*canvas, 400.0F, 300.0F);
    // The editor depends on the sidebar, which depends on the header: the solve order
    // must come from the relations, not from the mount order.
    REQUIRE(header.lock()->height() == Catch::Approx(kHeaderHeight));
    REQUIRE(side.lock()->width() == Catch::Approx(kSidebarWidth));
    REQUIRE(side.lock()->position().get_y() == Catch::Approx(kHeaderHeight + kGap));
    REQUIRE(side.lock()->height() == Catch::Approx(300.0F - kHeaderHeight - kGap));
    REQUIRE(edit.lock()->position().get_x() == Catch::Approx(kSidebarWidth + kGap));
    REQUIRE(edit.lock()->width() == Catch::Approx(400.0F - kSidebarWidth - kGap));

    const std::array updates {
        scene::AnchorCanvas::Update {
            side.lock(),
            {.right = side.parent.anchor.right,
             .top = header.anchor.bottom + kGap,
             .bottom = side.parent.anchor.bottom}
        },
        scene::AnchorCanvas::Update {
            edit.lock(),
            {.left = edit.parent.anchor.left,
             .right = side.anchor.left - kGap,
             .top = header.anchor.bottom + kGap,
             .bottom = edit.parent.anchor.bottom}
        }
    };
    canvas->set_child_anchors(updates);
    layout(*canvas, 400.0F, 300.0F);
    REQUIRE(side.lock()->position().get_x() == Catch::Approx(180.0F));
    REQUIRE(edit.lock()->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(edit.lock()->width() == Catch::Approx(168.0F));
    // Only relations changed: the header keeps its rect, the child order and node
    // identities are untouched, and no node was re-mounted.
    REQUIRE(header.lock()->height() == Catch::Approx(kHeaderHeight));
    REQUIRE(canvas->get_child(0) == header.lock().get());
    REQUIRE(canvas->get_child(1) == side.lock().get());
    REQUIRE(canvas->get_child(2) == edit.lock().get());
    // Solving from mount order, or rebuilding the tree on switch, changes these values.
}

TEST_CASE(
    "an arranged parent hosts a sized canvas but rejects a filling one",
    "[anchors-authoring][arranged-parent]"
) {
    Context ctx;
    // A definite size gives the canvas a finite bound even under the arranged parent's
    // loose() relayout, so an explicitly sized canvas composes with Column/Row/Card.
    const auto side = ctx.ui.ref<widget::Column>();
    auto sized = ctx.ui.anchor_canvas().width(200.0F).height(120.0F).children(
        ctx.ui.column().bind(side).width(60.0F).anchors(
            {.left = side.parent.anchor.left, .top = side.parent.anchor.top}
        )
    );
    auto page = ctx.ui.column()
                    .width(widget::authoring::fill)
                    .height(widget::authoring::fill)
                    .children(sized)
                    .build();
    layout(*page, 400.0F, 300.0F);
    REQUIRE(side.lock()->width() == Catch::Approx(60.0F));
    REQUIRE(side.lock()->global_bounds().get_x() == Catch::Approx(0.0F));

    // `fill` has no definite size while the arranged parent measures loosely, so the same
    // composition is rejected: a canvas cannot take an arranged parent's remainder.
    REQUIRE_THROWS_WITH(
        ctx.ui.column().children(ctx.ui.anchor_canvas().height(widget::authoring::fill)),
        ContainsSubstring("finite")
    );
    REQUIRE_THROWS_WITH(
        ctx.ui.column().children(ctx.ui.anchor_canvas().width(widget::authoring::fill)),
        ContainsSubstring("finite")
    );
    // Making on_measure fall back to the declared size instead of throwing turns this green
    // and requires the same change in anchors.md §5.1 and the scroll boundary case.
}
