#include "theme_test_support.hpp"
// Include the author-facing header directly, without the controls umbrella.
#include <nandina/widget/build_context.hpp>

#include <nandina/render/draw_context.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/card.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/pointer_area.hpp>
#include <nandina/widget/resizable.hpp>

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
        theme_test::Manager themes;
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

    // Content-height canvas inside a vertical ScrollView sizes from non-cyclic child bounds.
    const auto first = ctx.ui.ref<widget::Column>();
    const auto second = ctx.ui.ref<widget::Column>();
    auto auto_canvas = ctx.ui.anchor_canvas(
        ctx.ui.column()
            .bind(first)
            .width(scene::percent(50.0F))
            .height(180.0F)
            .anchors({.left = first.parent.anchor.left, .top = first.parent.anchor.top}),
        ctx.ui.column().bind(second).height(220.0F).anchors(
            {.left = first.anchor.right + 10.0F,
             .right = second.parent.anchor.right,
             .top = first.anchor.bottom + 20.0F}
        )
    );
    auto auto_scroll = ctx.ui.scroll_view(auto_canvas).build();
    layout(*auto_scroll);
    REQUIRE(auto_canvas.get().width() == Catch::Approx(400.0F));
    REQUIRE(auto_canvas.get().height() == Catch::Approx(420.0F));
    REQUIRE(second.lock()->width() == Catch::Approx(190.0F));
    REQUIRE(auto_scroll->maximum_scroll_offset().get_y() == Catch::Approx(220.0F));

    // Unbounded scrolling axis still rejects fill/percentage canvas height and parent.bottom anchors.
    auto fill_scroll =
        ctx.ui.scroll_view(ctx.ui.anchor_canvas().height(widget::authoring::fill)).build();
    REQUIRE_THROWS_WITH(layout(*fill_scroll), ContainsSubstring("definite"));
    auto pct_scroll =
        ctx.ui.scroll_view(ctx.ui.anchor_canvas().height(scene::percent(50.0F))).build();
    REQUIRE_THROWS_WITH(layout(*pct_scroll), ContainsSubstring("definite"));
    const auto bad_child = ctx.ui.ref<widget::Column>();
    auto cyclic_scroll =
        ctx.ui
            .scroll_view(ctx.ui.anchor_canvas(ctx.ui.column().bind(bad_child).height(30.0F).anchors(
                {.bottom = bad_child.parent.anchor.bottom}
            )))
            .build();
    REQUIRE_THROWS_WITH(layout(*cyclic_scroll), ContainsSubstring("definite"));
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
    "arranged parents host explicit, percentage, filling, expanded and content-sized canvases",
    "[anchors-authoring][arranged-parent]"
) {
    Context ctx;
    // 1. Percentage-sized canvas inside a Column.
    const auto pct_side = ctx.ui.ref<widget::Column>();
    auto pct_canvas = ctx.ui.anchor_canvas()
                          .width(scene::percent(50.0F))
                          .height(scene::percent(40.0F))
                          .children(ctx.ui.column()
                                        .bind(pct_side)
                                        .width(scene::percent(30.0F))
                                        .anchors(
                                            {.left = pct_side.parent.anchor.left,
                                             .top = pct_side.parent.anchor.top,
                                             .bottom = pct_side.parent.anchor.bottom}
                                        ));
    auto pct_page = ctx.ui.column(pct_canvas)
                        .width(widget::authoring::fill)
                        .height(widget::authoring::fill)
                        .build();
    layout(*pct_page, 400.0F, 300.0F);
    REQUIRE(pct_canvas.get().size() == foundation::NanSize(200.0F, 120.0F));
    REQUIRE(pct_side.lock()->size() == foundation::NanSize(60.0F, 120.0F));

    // 2. Column + header + Expanded(AnchorCanvas) occupying remaining vertical space, plus forward ref bound after mount.
    const auto exp_side = ctx.ui.ref<widget::Column>();
    const auto exp_edit = ctx.ui.ref<widget::Column>();
    auto exp_canvas = ctx.ui.anchor_canvas(ctx.ui.column().bind(exp_edit).anchors(
        {.left = exp_side.anchor.right + 12.0F,
         .right = exp_edit.parent.anchor.right,
         .top = exp_edit.parent.anchor.top,
         .bottom = exp_edit.parent.anchor.bottom}
    ));
    auto exp_page = ctx.ui
                        .column(
                            ctx.ui.row().width(widget::authoring::fill).height(56.0F),
                            ctx.ui.expanded(exp_canvas)
                        )
                        .width(widget::authoring::fill)
                        .height(widget::authoring::fill)
                        .build();
    // Forward reference bound AFTER exp_canvas was already mounted inside Expanded & Column!
    exp_canvas.children(ctx.ui.column()
                            .bind(exp_side)
                            .width(scene::percent(25.0F))
                            .anchors(
                                {.left = exp_side.parent.anchor.left,
                                 .top = exp_side.parent.anchor.top,
                                 .bottom = exp_side.parent.anchor.bottom}
                            ));
    scene::NanSceneTree tree;
    tree.set_root(exp_page);
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 300.0F)) >= 1);
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 300.0F)) == 0);
    REQUIRE(
        exp_canvas.get().global_bounds()
        == foundation::NanRect::from_xywh(0.0F, 56.0F, 400.0F, 244.0F)
    );
    REQUIRE(exp_side.lock()->size() == foundation::NanSize(100.0F, 244.0F));
    REQUIRE(exp_edit.lock()->position().get_x() == Catch::Approx(112.0F));
    REQUIRE(exp_edit.lock()->size() == foundation::NanSize(288.0F, 244.0F));

    // 3. Expanded -> Card -> AnchorCanvas pass-through composition.
    const auto card_child = ctx.ui.ref<widget::Column>();
    auto in_card_canvas = ctx.ui.anchor_canvas(ctx.ui.column()
                                                   .bind(card_child)
                                                   .anchors(
                                                       {.left = card_child.parent.anchor.left,
                                                        .right = card_child.parent.anchor.right,
                                                        .top = card_child.parent.anchor.top,
                                                        .bottom = card_child.parent.anchor.bottom}
                                                   ));
    auto card_node = widget::authoring::from(widget::Card::create(ctx.themes.theme()))
                         .content_insets(foundation::NanInsets::all(10.0F))
                         .child(in_card_canvas);
    auto card_page = ctx.ui.column(ctx.ui.row().height(40.0F), ctx.ui.expanded(card_node))
                         .width(widget::authoring::fill)
                         .height(widget::authoring::fill)
                         .build();
    layout(*card_page, 400.0F, 240.0F);
    REQUIRE(in_card_canvas.get().size() == foundation::NanSize(380.0F, 180.0F));
    REQUIRE(card_child.lock()->size() == foundation::NanSize(380.0F, 180.0F));

    // 4. Content-height AnchorCanvas with wrapping Label inside a Column, plus Stack and Resizable.
    constexpr std::string_view kWrapSample =
        "Wrapped description inside an AnchorCanvas whose width fills the Column while height "
        "adapts to the wrapped text and avatar.";
    const auto avatar = ctx.ui.ref<widget::Column>();
    const auto desc = ctx.ui.ref<widget::Label>();
    auto item_canvas =
        ctx.ui
            .anchor_canvas(
                ctx.ui.column().bind(avatar).width(48.0F).height(48.0F).anchors(
                    {.left = avatar.parent.anchor.left, .top = avatar.parent.anchor.top}
                ),
                widget::authoring::from(widget::Label::create(ctx.graph, std::string(kWrapSample)))
                    .bind(desc)
                    .text_overflow(text::TextOverflow::wrap)
                    .max_lines(8)
                    .anchors(
                        {.left = avatar.anchor.right + 12.0F,
                         .right = desc.parent.anchor.right,
                         .top = avatar.anchor.top}
                    )
            )
            .width(widget::authoring::fill);
    auto footer = ctx.ui.row().width(widget::authoring::fill).height(24.0F);
    auto flow_col =
        ctx.ui.column(item_canvas, footer).width(widget::authoring::fill).gap(10.0F).build();
    scene::NanSceneTree flow_tree;
    flow_tree.set_root(flow_col);
    REQUIRE(flow_tree.layout_root(foundation::NanSize(220.0F, 300.0F)) >= 1);
    REQUIRE(flow_tree.layout_root(foundation::NanSize(220.0F, 300.0F)) == 0);
    const float expected_h = std::max(48.0F, desc.lock()->height());
    REQUIRE(desc.lock()->width() == Catch::Approx(160.0F));
    REQUIRE(desc.lock()->line_count() >= 2);
    REQUIRE(item_canvas.get().height() == Catch::Approx(expected_h));
    REQUIRE(footer.get().position().get_y() == Catch::Approx(expected_h + 10.0F));

    // Row + Expanded(AnchorCanvas) width-for-height wrapping on a content-height Row.
    const auto row_lbl = ctx.ui.ref<widget::Label>();
    auto row_canvas = ctx.ui.anchor_canvas(
        widget::authoring::from(widget::Label::create(ctx.graph, std::string(kWrapSample)))
            .bind(row_lbl)
            .text_overflow(text::TextOverflow::wrap)
            .max_lines(8)
            .anchors(
                {.left = row_lbl.parent.anchor.left,
                 .right = row_lbl.parent.anchor.right,
                 .top = row_lbl.parent.anchor.top}
            )
    );
    auto row_wrap =
        ctx.ui.row(ctx.ui.column().width(80.0F).height(20.0F), ctx.ui.expanded(row_canvas))
            .gap(10.0F)
            .width(widget::authoring::fill);
    auto outer_col = ctx.ui.column(row_wrap).width(widget::authoring::fill).build();
    layout(*outer_col, 250.0F, 300.0F);
    REQUIRE(row_lbl.lock()->width() == Catch::Approx(160.0F));
    REQUIRE(row_wrap.get().height() == Catch::Approx(row_lbl.lock()->height()));

    // Stack & Resizable host AnchorCanvas without explicit pixel sizes.
    const auto hud = ctx.ui.ref<widget::Column>();
    auto stack_box =
        ctx.ui
            .stack(
                ctx.ui.column().width(300.0F).height(180.0F),
                ctx.ui.anchor_canvas(ctx.ui.column().bind(hud).width(40.0F).height(20.0F).anchors(
                    {.right = hud.parent.anchor.right - 10.0F,
                     .bottom = hud.parent.anchor.bottom - 8.0F}
                ))
            )
            .build();
    layout(*stack_box, 300.0F, 180.0F);
    REQUIRE(hud.lock()->position() == foundation::NanPoint(250.0F, 152.0F));

    const auto pane_child = ctx.ui.ref<widget::Column>();
    auto split = ctx.ui
                     .resizable(
                         ctx.ui.column(),
                         ctx.ui.anchor_canvas(ctx.ui.column()
                                                  .bind(pane_child)
                                                  .anchors(
                                                      {.left = pane_child.parent.anchor.left,
                                                       .right = pane_child.parent.anchor.right,
                                                       .top = pane_child.parent.anchor.top,
                                                       .bottom = pane_child.parent.anchor.bottom}
                                                  ))
                     )
                     .build();
    layout(*split, 408.0F, 200.0F);
    REQUIRE(pane_child.lock()->size() == split->second_pane_rect().get_size());

    // 5. Missing Expanded / definite height in Column or unstretched Row rejects parent.bottom instead of collapsing to 0.
    const auto bad_col_child = ctx.ui.ref<widget::Column>();
    auto missing_expanded =
        ctx.ui
            .column(
                ctx.ui.row().height(40.0F),
                ctx.ui
                    .anchor_canvas(ctx.ui.column()
                                       .bind(bad_col_child)
                                       .height(20.0F)
                                       .anchors({.bottom = bad_col_child.parent.anchor.bottom}))
                    .width(widget::authoring::fill)
            )
            .build();
    REQUIRE_THROWS_WITH(layout(*missing_expanded, 400.0F, 300.0F), ContainsSubstring("definite"));

    const auto bad_row_child = ctx.ui.ref<widget::Column>();
    auto unstretched_row = ctx.ui
                               .row(
                                   ctx.ui.column().width(80.0F).height(40.0F),
                                   ctx.ui.expanded(ctx.ui.anchor_canvas(
                                       ctx.ui.column()
                                           .bind(bad_row_child)
                                           .width(20.0F)
                                           .anchors({.bottom = bad_row_child.parent.anchor.bottom})
                                   ))
                               )
                               .build();
    REQUIRE_THROWS_WITH(layout(*unstretched_row, 400.0F, 200.0F), ContainsSubstring("definite"));
    unstretched_row->set_cross_alignment(widget::LayoutAlignment::stretch);
    REQUIRE_NOTHROW(layout(*unstretched_row, 400.0F, 200.0F));
    REQUIRE(bad_row_child.lock()->position().get_y() == Catch::Approx(200.0F));

    // 6. Dynamic subtree mount into an already-laid-out NanSceneTree (e.g. Router page switch):
    //    on_ready() runs outside layout_pass_active() while the window root already holds finite
    //    last_layout_constraints; intermediate relayout(loose()) must still defer cleanly.
    auto persistent_shell =
        ctx.ui.column().width(widget::authoring::fill).height(widget::authoring::fill).build();
    scene::NanSceneTree live_tree;
    live_tree.set_root(persistent_shell);
    (void)live_tree.layout_root(foundation::NanSize(600.0F, 400.0F));

    const auto dyn_hud = ctx.ui.ref<widget::Column>();
    auto mounted_page =
        ctx.ui
            .column(ctx.ui.expanded(ctx.ui.stack(
                ctx.ui.column(),
                ctx.ui.anchor_canvas(
                    ctx.ui.column().bind(dyn_hud).width(30.0F).height(16.0F).anchors(
                        {.right = dyn_hud.parent.anchor.right - 6.0F,
                         .bottom = dyn_hud.parent.anchor.bottom - 4.0F}
                    )
                )
            )))
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .build();
    REQUIRE_NOTHROW(persistent_shell->add(mounted_page));
    (void)live_tree.layout_root(foundation::NanSize(600.0F, 400.0F));
    REQUIRE(dyn_hud.lock()->position() == foundation::NanPoint(564.0F, 380.0F));

    // 7. Reviewer edge cases:
    // 7a. Column(stretch) with default-width AnchorCanvas (no .width(fill)) resolves content Y.
    const auto stretch_avatar = ctx.ui.ref<widget::Column>();
    const auto stretch_desc = ctx.ui.ref<widget::Label>();
    auto stretched_canvas = ctx.ui.anchor_canvas(
        ctx.ui.column()
            .bind(stretch_avatar)
            .width(48.0F)
            .height(48.0F)
            .anchors(
                {.left = stretch_avatar.parent.anchor.left, .top = stretch_avatar.parent.anchor.top}
            ),
        widget::authoring::from(widget::Label::create(ctx.graph, std::string(kWrapSample)))
            .bind(stretch_desc)
            .text_overflow(text::TextOverflow::wrap)
            .max_lines(8)
            .anchors(
                {.left = stretch_avatar.anchor.right + 12.0F,
                 .right = stretch_desc.parent.anchor.right,
                 .top = stretch_avatar.anchor.top}
            )
    );
    auto stretch_footer = ctx.ui.row().width(widget::authoring::fill).height(20.0F);
    auto stretch_col = ctx.ui.column(stretched_canvas, stretch_footer)
                           .cross_alignment(widget::LayoutAlignment::stretch)
                           .gap(10.0F)
                           .build();
    layout(*stretch_col, 220.0F, 300.0F);
    REQUIRE(stretched_canvas.get().height() >= 48.0F);
    REQUIRE(
        stretch_footer.get().position().get_y()
        == Catch::Approx(stretched_canvas.get().height() + 10.0F)
    );

    // 7b. shrink-only FlexItem rejects cyclic parent.right; basis-only FlexItem & tight-gap Column succeed.
    const auto shrink_child = ctx.ui.ref<widget::Column>();
    auto shrink_only_item =
        widget::FlexItem::create(scene::LayoutFlexPolicy {.grow = 0.0F, .shrink = 1.0F});
    shrink_only_item->set_child(
        ctx.ui
            .anchor_canvas(ctx.ui.column()
                               .bind(shrink_child)
                               .width(30.0F)
                               .height(18.0F)
                               .anchors({.right = shrink_child.parent.anchor.right}))
            .build()
    );
    auto shrink_row = widget::Row::create();
    shrink_row->add(shrink_only_item);
    REQUIRE_THROWS_WITH(layout(*shrink_row, 300.0F, 80.0F), ContainsSubstring("definite"));

    const auto basis_lbl = ctx.ui.ref<widget::Label>();
    auto basis_item = widget::FlexItem::create(scene::LayoutFlexPolicy {.basis = 160.0F});
    basis_item->set_child(
        ctx.ui
            .anchor_canvas(
                widget::authoring::from(widget::Label::create(ctx.graph, std::string(kWrapSample)))
                    .bind(basis_lbl)
                    .text_overflow(text::TextOverflow::wrap)
                    .max_lines(8)
                    .anchors(
                        {.left = basis_lbl.parent.anchor.left,
                         .right = basis_lbl.parent.anchor.right,
                         .top = basis_lbl.parent.anchor.top}
                    )
            )
            .build()
    );
    auto basis_row = widget::Row::create();
    basis_row->add(basis_item);
    auto basis_outer = widget::Column::create();
    basis_outer->add(basis_row);
    layout(*basis_outer, 250.0F, 300.0F);
    REQUIRE(basis_lbl.lock()->width() == Catch::Approx(160.0F));
    REQUIRE(basis_row->height() == Catch::Approx(basis_lbl.lock()->height()));

    const auto cramped_child = ctx.ui.ref<widget::Column>();
    auto cramped_col =
        ctx.ui
            .column(
                ctx.ui.row().height(4.0F),
                ctx.ui.expanded(
                    ctx.ui.anchor_canvas(ctx.ui.column()
                                             .bind(cramped_child)
                                             .min_width(100.0F)
                                             .anchors(
                                                 {.left = cramped_child.parent.anchor.left,
                                                  .right = cramped_child.parent.anchor.right}
                                             ))
                )
            )
            .gap(20.0F)
            .build();
    REQUIRE_NOTHROW(layout(*cramped_col, 400.0F, 10.0F));
    REQUIRE(cramped_child.lock()->width() == Catch::Approx(400.0F));

    // 7c. Content-height Stack & horizontal ScrollView reject cyclic parent.bottom; ScrollAxis::both solves content.
    const auto bad_stack_child = ctx.ui.ref<widget::Column>();
    auto unexpanded_stack_in_col =
        ctx.ui
            .column(ctx.ui.stack(
                ctx.ui.column(),
                ctx.ui.anchor_canvas(ctx.ui.column()
                                         .bind(bad_stack_child)
                                         .height(20.0F)
                                         .anchors({.bottom = bad_stack_child.parent.anchor.bottom}))
            ))
            .build();
    REQUIRE_THROWS_WITH(
        layout(*unexpanded_stack_in_col, 300.0F, 200.0F),
        ContainsSubstring("definite")
    );

    const auto bad_hscroll_child = ctx.ui.ref<widget::Column>();
    auto hscroll_col =
        ctx.ui
            .column(ctx.ui.scroll_view(widget::ScrollAxis::horizontal)
                        .child(ctx.ui.anchor_canvas(
                            ctx.ui.column()
                                .bind(bad_hscroll_child)
                                .width(80.0F)
                                .height(20.0F)
                                .anchors({.bottom = bad_hscroll_child.parent.anchor.bottom})
                        )))
            .build();
    REQUIRE_THROWS_WITH(layout(*hscroll_col, 300.0F, 200.0F), ContainsSubstring("definite"));

    const auto both_child = ctx.ui.ref<widget::Column>();
    auto both_scroll =
        ctx.ui.scroll_view(widget::ScrollAxis::both)
            .child(ctx.ui.column(
                ctx.ui.anchor_canvas(ctx.ui.column()
                                         .bind(both_child)
                                         .width(90.0F)
                                         .height(50.0F)
                                         .anchors(
                                             {.left = both_child.parent.anchor.left + 10.0F,
                                              .top = both_child.parent.anchor.top + 8.0F}
                                         ))
            ))
            .build();
    layout(*both_scroll, 300.0F, 200.0F);
    REQUIRE(both_scroll->content_size() == foundation::NanSize(100.0F, 58.0F));

    // 7d. Zero-height Column & min_height clamp do NOT mask missing Expanded; theme relayout preserves committed canvas size.
    REQUIRE_THROWS_WITH(layout(*missing_expanded, 400.0F, 0.0F), ContainsSubstring("definite"));

    scene::NanSceneTree card_tree;
    card_tree.set_theme_manager(ctx.themes);
    card_tree.set_root(card_page);
    (void)card_tree.layout_root(foundation::NanSize(400.0F, 240.0F));
    card_child.lock()->set_min_width(100.0F);
    ctx.themes.set_preference(theme::ThemePreference::dark);
    const std::array card_updates {scene::AnchorCanvas::Update {
        card_child.lock(),
        {.left = card_child.parent.anchor.left,
         .right = card_child.parent.anchor.right,
         .top = card_child.parent.anchor.top,
         .bottom = card_child.parent.anchor.bottom}
    }};
    REQUIRE_NOTHROW(in_card_canvas.get().set_child_anchors(card_updates));

    // 7e. PointerArea / IfRegion pass through axis assignment; Resizable 0x0 & forward-ref set_second_child; failed on_measure keeps tree dirty.
    reactive::Signal<bool> branch_on(ctx.graph, true);
    const auto wrapped_hud = ctx.ui.ref<widget::Column>();
    auto pointer_wrap = widget::authoring::from(widget::PointerArea::create())
                            .child(ctx.ui.when(branch_on, [&](widget::BuildContext inner) {
                                return inner.anchor_canvas(
                                    inner.column()
                                        .bind(wrapped_hud)
                                        .width(32.0F)
                                        .height(16.0F)
                                        .anchors(
                                            {.right = wrapped_hud.parent.anchor.right - 4.0F,
                                             .bottom = wrapped_hud.parent.anchor.bottom - 6.0F}
                                        )
                                );
                            }));
    auto wrapped_box = ctx.ui.column(ctx.ui.expanded(pointer_wrap)).build();
    scene::NanSceneTree wrapped_tree;
    wrapped_tree.set_root(wrapped_box);
    (void)wrapped_tree.layout_root(foundation::NanSize(200.0F, 120.0F));
    REQUIRE(wrapped_hud.lock()->position() == foundation::NanPoint(164.0F, 98.0F));

    layout(*split, 0.0F, 0.0F);
    REQUIRE(pane_child.lock()->size() == foundation::NanSize(0.0F, 0.0F));
    layout(*split, 408.0F, 200.0F);
    const auto inset_pane_child = ctx.ui.ref<widget::Column>();
    auto prelayout_split =
        ctx.ui
            .resizable(
                ctx.ui.column(),
                ctx.ui.anchor_canvas(ctx.ui.column()
                                         .bind(inset_pane_child)
                                         .height(20.0F)
                                         .anchors(
                                             {.left = inset_pane_child.parent.anchor.left + 12.0F,
                                              .right = inset_pane_child.parent.anchor.right - 12.0F,
                                              .bottom =
                                                  inset_pane_child.parent.anchor.bottom - 10.0F}
                                         ))
            )
            .ratio(0.56F)
            .min_panes_extent(120.0F, 120.0F)
            .build();
    layout(*prelayout_split, 408.0F, 200.0F);
    REQUIRE(inset_pane_child.lock()->width() == Catch::Approx(176.0F - 24.0F));

    // Unbounded cross-axis measure on a fixed-height Column inside a Row must never pass +inf min_width.
    auto fixed_h_col = ctx.ui.column(ctx.ui.expanded(ctx.ui.column())).height(200.0F).build();
    REQUIRE(
        std::isfinite(
            fixed_h_col->measure_layout(foundation::NanLayoutConstraints::loose()).get_width()
        )
    );
    const auto late_pane_target = ctx.ui.ref<widget::Column>();
    const auto late_pane_dep = ctx.ui.ref<widget::Column>();
    auto late_canvas =
        ctx.ui
            .anchor_canvas(ctx.ui.column()
                               .bind(late_pane_dep)
                               .width(20.0F)
                               .height(20.0F)
                               .anchors({.left = late_pane_target.anchor.right + 4.0F}))
            .build();
    REQUIRE_NOTHROW(split->set_second_child(late_canvas));
    auto bound_target = ctx.ui.column().bind(late_pane_target).width(30.0F).height(20.0F).build();
    late_canvas->add_child(bound_target);
    REQUIRE_NOTHROW(layout(*split, 408.0F, 200.0F));
    REQUIRE(late_pane_dep.lock()->position().get_x() == Catch::Approx(34.0F));

    scene::NanSceneTree fail_tree;
    const auto unbound_ref = ctx.ui.ref<widget::Column>();
    const auto waiting_ref = ctx.ui.ref<widget::Column>();
    auto resolved_peer = ctx.ui.column().width(40.0F).height(16.0F).build();
    auto retry_canvas = ctx.ui
                            .anchor_canvas(ctx.ui.column()
                                               .bind(waiting_ref)
                                               .width(24.0F)
                                               .height(16.0F)
                                               .anchors({.left = unbound_ref.anchor.right + 5.0F}))
                            .width(widget::authoring::fill)
                            .build();
    retry_canvas->add_child(resolved_peer);
    auto retry_scroll =
        ctx.ui.scroll_view(widget::ScrollAxis::vertical).child(retry_canvas).build();
    fail_tree.set_root(retry_scroll);
    REQUIRE_THROWS_WITH(
        fail_tree.layout_root(foundation::NanSize(300.0F, 180.0F)),
        ContainsSubstring("unbound")
    );
    unbound_ref.bind(resolved_peer);
    REQUIRE(fail_tree.layout_root(foundation::NanSize(300.0F, 180.0F)) >= 1);
    REQUIRE(waiting_ref.lock()->position().get_x() == Catch::Approx(45.0F));
}

TEST_CASE(
    "a failed initial anchors binding leaves no subscription behind",
    "[anchors-authoring][failed-binding]"
) {
    Context ctx;
    auto child = ctx.ui.column().width(40.0F).height(20.0F);
    reactive::Signal<scene::AnchorSpec> source(ctx.graph, {.left = child.parent.anchor.top});
    REQUIRE_THROWS_WITH(child.anchors(source), ContainsSubstring("cannot be mixed"));
    REQUIRE(child.get().anchors().empty());
    ctx.scope.clear();
    source.set({.right = child.parent.anchor.right});
    REQUIRE(child.get().anchors().empty());
    auto canvas = ctx.ui.anchor_canvas().children(child).build();
    layout(*canvas);
    REQUIRE(child.get().position().get_x() == Catch::Approx(0.0F));
    // Without make_effect's exception rollback, the rejected effect survives scope.clear()
    // and installs the later right anchor (x = 360) despite construction having failed.
}
