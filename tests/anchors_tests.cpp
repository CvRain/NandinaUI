// Mutation points are recorded per contract; no window or RTTI is needed.
#include <nandina/scene/anchor_canvas.hpp>
#include <nandina/scene/node_ref.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/layout.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <array>
#include <limits>

using namespace nandina;
using Catch::Matchers::ContainsSubstring;

namespace
{
    using Control = scene::NanControl;
    using Canvas = scene::AnchorCanvas;
    using Ref = scene::NodeRef<Control>;

    static_assert(requires(Canvas& canvas) { canvas.set_anchors({}); });

    template<class R, class T>
    concept CanBind = requires(R ref, std::shared_ptr<T> node) { ref.bind(node); };
    static_assert(CanBind<scene::NodeRef<widget::Column>, widget::Column>);
    static_assert(!CanBind<scene::NodeRef<widget::Column>, widget::Row>);
    static_assert(!CanBind<scene::NodeRef<widget::Column>, scene::NanNode>);

    void layout(Canvas& canvas, float width = 400.0F, float height = 200.0F) {
        const foundation::NanSize size(width, height);
        (void)canvas.measure_layout(foundation::NanLayoutConstraints::tight(size));
        canvas.layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, width, height));
    }

    auto item(float width = 40.0F, float height = 20.0F) -> std::shared_ptr<Control> {
        return std::make_shared<Control>(foundation::NanSize(width, height));
    }

    auto fill(const Ref& ref) -> scene::AnchorSpec {
        return {
            .left = ref.parent.anchor.left,
            .right = ref.parent.anchor.right,
            .top = ref.parent.anchor.top,
            .bottom = ref.parent.anchor.bottom
        };
    }
} // namespace

TEST_CASE("node refs share forward identity without owning nodes", "[anchors][ref]") {
    const Ref ref;
    const Ref copy(ref);
    const auto forward = ref.anchor.right;
    REQUIRE_THROWS_WITH(ref.lock(), ContainsSubstring("unbound"));
    REQUIRE_THROWS_AS(ref.bind(nullptr), std::invalid_argument);
    auto node = item();
    copy.bind(node);
    REQUIRE(ref.lock() == node);
    REQUIRE(forward.identity->require() == node);
    REQUIRE_THROWS_WITH(ref.bind(node), ContainsSubstring("already bound"));
    node.reset();
    REQUIRE_THROWS_WITH(copy.lock(), ContainsSubstring("expired"));
    REQUIRE_THROWS_WITH(ref.bind(item()), ContainsSubstring("already bound"));
    // Strong ownership, independent copy slots, or allowing rebind breaks this test.
}

TEST_CASE("anchors solve reverse-inserted siblings and preserve presentation", "[anchors][order]") {
    auto canvas = std::make_shared<Canvas>();
    auto sidebar = item(180.0F);
    auto editor = item();
    Ref side_ref, edit_ref;
    editor->set_anchors(
        {.left = side_ref.anchor.right + 12.0F,
         .right = edit_ref.parent.anchor.right,
         .top = edit_ref.parent.anchor.top,
         .bottom = edit_ref.parent.anchor.bottom}
    );
    sidebar->set_max_width(scene::percent(30.0F));
    sidebar->set_anchors(
        {.left = side_ref.parent.anchor.left,
         .top = side_ref.parent.anchor.top,
         .bottom = side_ref.parent.anchor.bottom}
    );
    canvas->add_child(editor);
    canvas->add_child(sidebar);
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("unbound"));
    REQUIRE(canvas->layout_dirty());
    side_ref.bind(sidebar);
    edit_ref.bind(editor);
    layout(*canvas);
    REQUIRE(sidebar->width() == Catch::Approx(120.0F));
    REQUIRE(editor->position().get_x() == Catch::Approx(132.0F));
    REQUIRE(editor->width() == Catch::Approx(268.0F));
    REQUIRE(editor->height() == Catch::Approx(200.0F));
    editor->visual_part(scene::visual::node)
        .property(scene::visual::translate_t {})
        .set(foundation::NanPoint(8.0F, 0.0F));
    for (int i = 0; i < 3; ++i) {
        layout(*canvas);
        REQUIRE(editor->position().get_x() == Catch::Approx(132.0F));
        REQUIRE(editor->global_bounds().get_x() == Catch::Approx(140.0F));
        REQUIRE(editor->width() == Catch::Approx(268.0F));
    }
    // Insertion-order solving or using global_bounds as input fails on the first/repeated layout.
}

TEST_CASE(
    "anchors retain hidden targets and reject dead or foreign targets",
    "[anchors][lifetime]"
) {
    auto canvas = std::make_shared<Canvas>();
    auto other = std::make_shared<Canvas>();
    auto target = item();
    auto follower = item();
    target->set_name("target");
    follower->set_name("follower");
    Ref target_ref(target), follower_ref(follower);
    follower->set_anchors({.left = target_ref.anchor.right + 5.0F});
    canvas->add_child(follower);
    canvas->add_child(target);
    target->set_visible(false);
    layout(*canvas);
    REQUIRE(follower->position().get_x() == Catch::Approx(45.0F));
    REQUIRE(target_ref.lock() == target);
    (void)canvas->remove_child(*target);
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("detached or outside"));
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("follower"));
    other->add_child(target);
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("target"));
    canvas->reparent(target);
    REQUIRE_NOTHROW(layout(*canvas));
    (void)canvas->remove_child(*target);
    target.reset();
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("expired"));
    // Skipping invisible targets or caching resolved raw pointers breaks this test.
}

TEST_CASE(
    "anchors validate syntax and reject cycles without partial geometry",
    "[anchors][validation]"
) {
    auto canvas = std::make_shared<Canvas>();
    auto first = item();
    auto second = item();
    Ref a(first), b(second);
    canvas->add_child(first);
    canvas->add_child(second);
    layout(*canvas);
    REQUIRE_THROWS_AS(first->set_anchors({.left = a.parent.anchor.top}), std::invalid_argument);
    REQUIRE_THROWS_AS(
        first->set_anchors(
            {.left = a.parent.anchor.left, .horizontal_center = a.parent.anchor.horizontal_center}
        ),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        first->set_anchors({.left = a.parent.anchor.left + std::numeric_limits<float>::infinity()}),
        std::invalid_argument
    );
    first->set_anchors({.left = a.anchor.right});
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("self reference"));
    first->set_anchors({.left = b.parent.anchor.left});
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("another node"));
    first->set_anchors({.left = b.anchor.right});
    second->set_anchors({.left = a.anchor.right});
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("cycle"));
    REQUIRE(first->position().get_x() == Catch::Approx(0.0F));
    second->set_anchors({.left = b.parent.anchor.left + 50.0F});
    first->set_anchors({.left = b.anchor.right, .right = a.parent.anchor.left});
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("span"));
    REQUIRE(second->position().get_x() == Catch::Approx(0.0F));
    // Removing cycle detection, or committing each solved sibling immediately, fails here.
}

TEST_CASE(
    "anchors stretch respects canvas-relative limits and rejects flow sizes",
    "[anchors][sizes]"
) {
    auto canvas = std::make_shared<Canvas>();
    auto child = item();
    Ref ref(child);
    canvas->add_child(child);
    child->set_anchors(
        {.left = ref.parent.anchor.left + 50.0F, .right = ref.parent.anchor.right - 50.0F}
    );
    child->set_min_width(scene::percent(50.0F)).set_max_width(scene::percent(80.0F));
    layout(*canvas);
    REQUIRE(child->width() == Catch::Approx(300.0F));
    child->set_max_width(scene::percent(60.0F));
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("min/max"));
    child->set_max_width(scene::percent(80.0F));
    child->set_width(100.0F);
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("conflict"));
    child->set_width(scene::percent(50.0F));
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("conflict"));
    child->set_width(scene::fill);
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("fill"));
    child->set_width(scene::content);
    child->set_anchors(
        {.horizontal_center = ref.parent.anchor.horizontal_center,
         .vertical_center = ref.parent.anchor.vertical_center}
    );
    child->set_width(scene::percent(50.0F));
    layout(*canvas);
    REQUIRE(child->width() == Catch::Approx(200.0F));
    REQUIRE(child->position().get_x() == Catch::Approx(100.0F));
    REQUIRE(child->position().get_y() == Catch::Approx(90.0F));
    child->set_min_width(350.0F);
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("min/max"));
    // Resolve percentage max against the 300px span instead of 400px canvas -> first layout fails.
}

TEST_CASE(
    "anchor canvas requires a finite viewport and composes with columns",
    "[anchors][nested]"
) {
    auto canvas = std::make_shared<Canvas>();
    REQUIRE_THROWS_WITH(
        canvas->measure_layout(foundation::NanLayoutConstraints::loose()),
        ContainsSubstring("finite")
    );
    canvas->set_width(400.0F).set_height(200.0F);
    REQUIRE(
        canvas->measure_layout(foundation::NanLayoutConstraints::loose()).get_width()
        == Catch::Approx(400.0F)
    );
    auto column = widget::Column::create();
    Ref column_ref(column);
    column->set_anchors(fill(column_ref));
    column->set_gap(5.0F);
    auto first = item(40.0F, 20.0F), second = item(40.0F, 30.0F);
    column->add(first).add(second);
    canvas->add_child(column);
    layout(*canvas);
    REQUIRE(column->size() == foundation::NanSize(400.0F, 200.0F));
    REQUIRE(second->position().get_y() == Catch::Approx(25.0F));
    // An implicit content-sized canvas or writing presentation instead of layout_to fails here.
}

TEST_CASE("anchor and flow mixing is rejected before reparent detaches", "[anchors][mixing]") {
    auto canvas = std::make_shared<Canvas>();
    auto row = widget::Row::create();
    auto child = item();
    Ref ref(child);
    child->set_anchors(fill(ref));
    // Detached builders can measure their contents before they are attached.
    REQUIRE_NOTHROW(child->measure_layout(foundation::NanLayoutConstraints::loose()));
    REQUIRE_THROWS_WITH(row->add(child), ContainsSubstring("AnchorCanvas"));
    REQUIRE(child->parent() == nullptr);
    canvas->add_child(child);
    REQUIRE_THROWS_WITH(row->reparent(child), ContainsSubstring("AnchorCanvas"));
    REQUIRE(child->parent() == canvas.get());
    REQUIRE(canvas->child_count() == 1);
    child->set_anchors({});
    row->reparent(child);
    REQUIRE_THROWS_WITH(child->set_anchors(fill(ref)), ContainsSubstring("AnchorCanvas"));
    REQUIRE(child->anchors().empty());
    REQUIRE_THROWS(canvas->add_child(std::make_shared<scene::NanNode2D>()));
    auto expanded = widget::Expanded::create();
    expanded->set_child(item());
    canvas->add_child(expanded);
    REQUIRE_THROWS_WITH(layout(*canvas), ContainsSubstring("Expanded"));
    scene::NanSceneTree tree;
    auto root = item();
    Ref root_ref(root);
    root->set_anchors(fill(root_ref));
    tree.set_root(root);
    REQUIRE_THROWS_WITH(
        tree.layout_root(foundation::NanSize(100.0F, 100.0F)),
        ContainsSubstring("detached")
    );
    // Remove the reparent preflight -> the child is detached on the failing move.
}

TEST_CASE("anchor batches atomically switch sidebar sides", "[anchors][batch]") {
    auto canvas = std::make_shared<Canvas>();
    auto sidebar = item(100.0F), editor = item();
    Ref a(sidebar), b(editor);
    canvas->add_child(sidebar);
    canvas->add_child(editor);
    sidebar->set_anchors({.left = a.parent.anchor.left});
    editor->set_anchors({.left = a.anchor.right + 12.0F, .right = b.parent.anchor.right});
    layout(*canvas);
    const std::array changes {
        Canvas::Update {sidebar, {.right = a.parent.anchor.right}},
        Canvas::Update {editor, {.left = b.parent.anchor.left, .right = a.anchor.left - 12.0F}}
    };
    canvas->clear_layout_dirty();
    canvas->set_child_anchors(changes);
    REQUIRE(canvas->layout_dirty());
    REQUIRE(sidebar->position().get_x() == Catch::Approx(0.0F));
    layout(*canvas);
    REQUIRE(sidebar->position().get_x() == Catch::Approx(300.0F));
    REQUIRE(editor->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(editor->width() == Catch::Approx(288.0F));
    const std::array invalid {
        Canvas::Update {sidebar, {.left = b.anchor.right}},
        Canvas::Update {editor, {.left = a.anchor.right}}
    };
    REQUIRE_THROWS_WITH(canvas->set_child_anchors(invalid), ContainsSubstring("cycle"));
    REQUIRE(sidebar->anchors().right.has_value());
    REQUIRE_FALSE(sidebar->anchors().left.has_value());
    REQUIRE_NOTHROW(layout(*canvas));
    REQUIRE(sidebar->position().get_x() == Catch::Approx(300.0F));
    // Committing descriptions before candidate validation or merging specs breaks this test.
}

TEST_CASE(
    "parent anchors follow reparent and batches reject invalid membership",
    "[anchors][parent]"
) {
    auto first = std::make_shared<Canvas>();
    auto second = std::make_shared<Canvas>();
    auto child = item();
    Ref ref(child);
    child->set_anchors({.right = ref.parent.anchor.right});
    first->add_child(child);
    layout(*first, 200.0F);
    REQUIRE(child->position().get_x() == Catch::Approx(160.0F));
    second->reparent(child);
    layout(*second, 500.0F);
    REQUIRE(child->position().get_x() == Catch::Approx(460.0F));
    const std::array duplicate {Canvas::Update {child, {}}, Canvas::Update {child, {}}};
    REQUIRE_THROWS_WITH(second->set_child_anchors(duplicate), ContainsSubstring("duplicate"));
    const std::array foreign {Canvas::Update {child, {}}};
    REQUIRE_THROWS_WITH(first->set_child_anchors(foreign), ContainsSubstring("direct child"));
    const std::array null {Canvas::Update {nullptr, {}}};
    REQUIRE_THROWS_WITH(second->set_child_anchors(null), ContainsSubstring("direct child"));
    REQUIRE(child->anchors().right.has_value());
    // Caching the old parent, or ignoring membership/duplicates, fails here.
}

TEST_CASE(
    "anchor layout updates bounds hit testing and semantics through layout_to",
    "[anchors][geometry]"
) {
    scene::NanSceneTree tree;
    auto canvas = std::make_shared<Canvas>();
    auto child = item();
    Ref ref(child);
    child->set_semantics_override(semantics::Properties {.role = semantics::Role::button});
    child->set_anchors(
        {.right = ref.parent.anchor.right - 10.0F, .bottom = ref.parent.anchor.bottom - 10.0F}
    );
    canvas->add_child(child);
    tree.set_root(canvas);
    tree.layout_root(foundation::NanSize(400.0F, 200.0F));
    REQUIRE(child->global_bounds().get_x() == Catch::Approx(350.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(355.0F, 175.0F)) == child.get());
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.semantics_tree().find(child->semantics_id()) != nullptr);
    REQUIRE(
        tree.semantics_tree().find(child->semantics_id())->bounds.get_x() == Catch::Approx(350.0F)
    );
    tree.layout_root(foundation::NanSize(500.0F, 300.0F));
    REQUIRE(child->global_bounds().get_x() == Catch::Approx(450.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(455.0F, 275.0F)) == child.get());
    REQUIRE(tree.hit_test(foundation::NanPoint(355.0F, 175.0F)) != child.get());
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.semantics_tree().find(child->semantics_id()) != nullptr);
    REQUIRE(
        tree.semantics_tree().find(child->semantics_id())->bounds.get_x() == Catch::Approx(450.0F)
    );
    // Skip layout_to, or bypass its transform invalidation -> warmed geometry remains stale.
}
