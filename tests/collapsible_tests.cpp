// Disclosure composition: retained content, input, focus and scoped bindings.
#include <nandina/scene/node_ref.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/controls.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace foundation = nandina::foundation;
namespace reactive = nandina::reactive;
namespace scene = nandina::scene;
namespace semantics = nandina::semantics;
namespace theme = nandina::theme;
namespace widget = nandina::widget;

namespace
{
    class Content final: public scene::NanControl {
    public:
        int measures = 0;
        int enters = 0;
        int exits = 0;
        float natural_width = 200;
        float line_height = 20;

    protected:
        auto on_measure(foundation::NanLayoutConstraints c) -> foundation::NanSize override {
            ++measures;
            const float width = std::isfinite(c.max_width) ? c.max_width : natural_width;
            const float lines = width > 0 ? std::max(1.0F, std::ceil(natural_width / width)) : 1;
            return c.constrain(foundation::NanSize(width, lines * line_height));
        }
        void on_enter_tree() override {
            ++enters;
        }
        void on_exit_tree() override {
            ++exits;
        }
    };
    struct Fixture {
        std::shared_ptr<widget::Collapsible> node = widget::Collapsible::create("Details");
        scene::NanSceneTree tree;
        Fixture() {
            tree.set_root(node);
        }
        void layout(float width = 300, float height = 400) {
            (void)tree.layout_root(foundation::NanSize(width, height));
        }
        auto activate() -> bool {
            return tree.perform_semantics_action(
                node->trigger().semantics_id(),
                {.action = semantics::Action::activate}
            );
        }
    };
    auto natural(widget::Collapsible& node, float width) -> foundation::NanSize {
        return node.measure_layout({.max_width = width});
    }
    void
    key(scene::NanSceneTree& tree,
        foundation::LogicalKey code,
        scene::KeyEvent::Action action = scene::KeyEvent::Action::press) {
        tree.dispatch_key(scene::KeyEvent(code, action));
    }
    void click(scene::NanSceneTree& tree, foundation::NanPoint point) {
        tree.dispatch_mouse_move(scene::MouseMoveEvent(point, foundation::NanPoint {}));
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                point
            )
        );
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::release,
                point
            )
        );
    }
} // namespace

TEST_CASE("Collapsible defaults to a closed non-focusable container", "[collapsible]") {
    auto node = widget::Collapsible::create("Details");
    CHECK_FALSE(node->open());
    CHECK_FALSE(node->disabled());
    CHECK_FALSE(node->is_focusable());
    CHECK(node->trigger().is_focusable());
    CHECK(node->child() == nullptr);
    CHECK(node->gap() == 0);
    CHECK(node->overflow() == scene::ControlOverflow::clip);
    CHECK(node->text() == "Details");
    CHECK(node->trigger().treatment() == theme::ButtonTreatment::ghost);
}

TEST_CASE(
    "Collapsible closed content is retained unmeasured and natural height wraps",
    "[collapsible][layout]"
) {
    auto node = widget::Collapsible::create("Details");
    auto content = std::make_shared<Content>();
    node->set_child(content).set_gap(7);
    const auto closed = natural(*node, 100);
    CHECK(content->measures == 0);
    node->set_open(true);
    auto size = natural(*node, 100);
    CHECK(size.get_height() == Catch::Approx(closed.get_height() + 47));
    node->layout_to(foundation::NanRect::from_origin_size(foundation::NanPoint {}, size));
    CHECK(content->width() == Catch::Approx(100));
    CHECK(content->height() == Catch::Approx(40));
    size = natural(*node, 400);
    CHECK(size.get_height() == Catch::Approx(closed.get_height() + 27));
    node->layout_to(foundation::NanRect::from_origin_size(foundation::NanPoint {}, size));
    CHECK(content->width() == Catch::Approx(400));
    CHECK(content->height() == Catch::Approx(20));
    node->set_open(false);
    const int before = content->measures;
    CHECK(natural(*node, 100).get_height() == Catch::Approx(closed.get_height()));
    CHECK(content->measures == before);
    CHECK(node->child() == content.get());
    CHECK(content->visible());
}

TEST_CASE(
    "Collapsible preserves content mounted state while hiding hit and semantics",
    "[collapsible][lifecycle]"
) {
    Fixture f;
    auto content = std::make_shared<Content>();
    f.node->set_child(content);
    f.node->set_open(true);
    f.layout();
    CHECK(content->enters == 1);
    auto point = content->global_bounds().get_center();
    CHECK(f.tree.hit_test(point) == content.get());
    f.node->set_open(false);
    CHECK(content->is_inside_tree());
    CHECK(content->exits == 0);
    CHECK_FALSE(content->is_visible_in_tree());
    CHECK(f.tree.hit_test(point) != content.get());
    f.node->set_open(true);
    f.layout();
    CHECK(content->is_visible_in_tree());
    CHECK(content->enters == 1);
    CHECK(content->exits == 0);
}

TEST_CASE(
    "Collapsible suppresses gaps without visible content and preserves authored visibility",
    "[collapsible][layout]"
) {
    auto node = widget::Collapsible::create("Details");
    const float closed = natural(*node, 300).get_height();
    node->set_open(true);
    node->set_gap(19);
    CHECK(natural(*node, 300).get_height() == closed);
    auto content = std::make_shared<Content>();
    content->set_visible(false);
    node->set_child(content);
    CHECK(natural(*node, 300).get_height() == closed);
    node->set_open(false);
    node->set_open(true);
    CHECK_FALSE(content->visible());
    CHECK(content->measures == 0);
}

TEST_CASE(
    "Collapsible setter is silent and user activation emits exactly once",
    "[collapsible][events]"
) {
    Fixture f;
    std::vector<bool> calls;
    std::vector<bool> events;
    f.node->set_on_change([&](bool open) { calls.push_back(open); });
    auto sub = f.node->open_changed().subscribe([&](bool open) { events.push_back(open); });
    f.node->set_open(true);
    f.node->set_open(true);
    CHECK(calls.empty());
    CHECK(events.empty());
    REQUIRE(f.activate());
    CHECK_FALSE(f.node->open());
    CHECK(calls == std::vector<bool> {false});
    CHECK(events == calls);
    f.node->toggle();
    CHECK(events == std::vector<bool> {false, true});
}

TEST_CASE(
    "Collapsible mouse and keyboard use the same toggle without repeat",
    "[collapsible][input]"
) {
    Fixture f;
    f.layout();
    click(f.tree, f.node->trigger().global_bounds().get_center());
    CHECK(f.node->open());
    CHECK(f.tree.focused_node() == &f.node->trigger());
    key(f.tree, foundation::LogicalKey::space);
    CHECK_FALSE(f.node->open());
    key(f.tree, foundation::LogicalKey::space, scene::KeyEvent::Action::repeat);
    key(f.tree, foundation::LogicalKey::space, scene::KeyEvent::Action::release);
    CHECK_FALSE(f.node->open());
    key(f.tree, foundation::LogicalKey::enter);
    CHECK(f.node->open());
    key(f.tree, foundation::LogicalKey::enter, scene::KeyEvent::Action::repeat);
    CHECK(f.node->open());
}

TEST_CASE(
    "Collapsible restores content focus and releases capture when closing",
    "[collapsible][focus]"
) {
    Fixture f;
    auto button = widget::Button::create("Inside");
    f.node->set_child(button);
    f.node->set_open(true);
    f.layout();
    f.tree.set_focus(button.get());
    f.tree.set_pointer_capture(button.get());
    f.node->set_open(false);
    CHECK(f.tree.focused_node() == &f.node->trigger());
    CHECK(f.tree.pointer_capture() == nullptr);
    f.tree.focus_next();
    CHECK(f.tree.focused_node() != button.get());
    f.node->set_open(true);
    CHECK(f.tree.focused_node() == &f.node->trigger());
}

TEST_CASE(
    "Collapsible programmatic set_open is not restricted by disabled",
    "[collapsible][disabled]"
) {
    auto node = widget::Collapsible::create("Details");
    node->set_disabled(true);
    CHECK_FALSE(node->open());
    node->set_open(true);
    CHECK(node->open());
    node->toggle();
    CHECK(node->open());
    node->set_open(false);
    CHECK_FALSE(node->open());
}

TEST_CASE(
    "Collapsible disabled trigger does not disable expanded content",
    "[collapsible][disabled]"
) {
    Fixture f;
    auto content = widget::Button::create("Inside");
    f.node->set_child(content);
    f.node->set_open(true);
    f.node->set_disabled(true);
    f.layout();
    CHECK_FALSE(f.node->trigger().is_focusable());
    f.node->toggle();
    CHECK(f.node->open());
    CHECK_FALSE(f.activate());
    CHECK_FALSE(content->disabled());
    f.tree.set_focus(content.get());
    f.node->set_open(false);
    CHECK(f.tree.focused_node() == nullptr);
    f.node->set_open(true);
    CHECK(f.node->open());
}

TEST_CASE(
    "Collapsible replacement and clear restore focus without disturbing outside focus",
    "[collapsible][slots]"
) {
    Fixture f;
    auto first = widget::Button::create("First");
    auto second = widget::Button::create("Second");
    f.node->set_child(first);
    f.node->set_open(true);
    f.layout();
    f.tree.set_focus(first.get());
    f.node->set_child(second);
    CHECK_FALSE(first->is_inside_tree());
    CHECK(f.tree.focused_node() == &f.node->trigger());
    f.layout();
    f.tree.set_focus(second.get());
    f.node->clear_child();
    CHECK(f.node->child() == nullptr);
    CHECK_FALSE(second->is_inside_tree());
    CHECK(f.tree.focused_node() == &f.node->trigger());
    f.node->clear_child();
    CHECK(f.tree.focused_node() == &f.node->trigger());
}

TEST_CASE(
    "Collapsible invalid content replacement preserves current ownership",
    "[collapsible][validation]"
) {
    auto node = widget::Collapsible::create("Details");
    auto current = std::make_shared<Content>();
    node->set_child(current);
    CHECK_THROWS_AS(node->set_child(nullptr), std::invalid_argument);
    CHECK_THROWS_AS(node->set_child(node), std::logic_error);
    auto other = widget::Collapsible::create("Other");
    CHECK_THROWS_AS(other->set_child(current), std::logic_error);
    scene::NodeRef<scene::NanControl> ref;
    auto anchored = std::make_shared<Content>();
    ref.bind(anchored);
    anchored->set_anchors({.left = ref.parent.anchor.left});
    CHECK_THROWS_AS(node->set_child(anchored), std::logic_error);
    CHECK(node->child() == current.get());
    CHECK(anchored->parent() == nullptr);
    node->set_child(current);
    CHECK(node->child() == current.get());
}

TEST_CASE("Collapsible validates text gap and complete layout sums", "[collapsible][validation]") {
    CHECK_THROWS_AS(widget::Collapsible::create(""), std::invalid_argument);
    auto node = widget::Collapsible::create("Details");
    CHECK_THROWS_AS(node->set_text(""), std::invalid_argument);
    CHECK(node->text() == "Details");
    for (float invalid:
         {-1.0F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
    {
        CHECK_THROWS_AS(node->set_gap(invalid), std::invalid_argument);
        CHECK(node->gap() == 0);
    }
    auto content = std::make_shared<Content>();
    content->line_height = std::numeric_limits<float>::max() * 0.75F;
    node->set_child(content);
    node->set_gap(content->line_height);
    node->set_open(true);
    CHECK_THROWS_AS(natural(*node, 300), std::invalid_argument);
}

TEST_CASE(
    "Collapsible content and trigger semantics share expanded state",
    "[collapsible][semantics]"
) {
    Fixture f;
    auto content = widget::Button::create("Inside");
    f.node->set_child(content);
    f.layout();
    REQUIRE(f.tree.update_semantics());
    auto trigger_id = f.node->trigger().semantics_id();
    const auto* trigger = f.tree.semantics_tree().find(trigger_id);
    REQUIRE(trigger);
    CHECK(trigger->properties.role == semantics::Role::button);
    CHECK(trigger->properties.label == "Details");
    CHECK(trigger->properties.state.expanded == false);
    CHECK(f.tree.semantics_tree().find(content->semantics_id()) == nullptr);
    REQUIRE(f.activate());
    f.layout();
    REQUIRE(f.tree.update_semantics());
    trigger = f.tree.semantics_tree().find(trigger_id);
    CHECK(trigger->properties.state.expanded == true);
    REQUIRE(f.tree.semantics_tree().find(content->semantics_id()));
    CHECK(f.tree.perform_semantics_action(
        content->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    f.node->set_text("Renamed");
    REQUIRE(f.tree.update_semantics());
    CHECK(f.tree.semantics_tree().find(trigger_id)->properties.label == "Renamed");
}

TEST_CASE(
    "Collapsible reentrant callbacks preserve the initiating action payload",
    "[collapsible][events]"
) {
    auto node = widget::Collapsible::create("Details");
    std::vector<bool> events;
    node->set_on_change([&](bool open) { node->set_open(!open); });
    auto sub = node->open_changed().subscribe([&](bool open) { events.push_back(open); });
    node->toggle();
    CHECK_FALSE(node->open());
    CHECK(events == std::vector<bool> {true});
}

TEST_CASE(
    "Collapsible signal binding applies the initial value immediately",
    "[collapsible][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    reactive::Signal<bool> open(graph, true);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);
    auto node = ui.make<widget::Collapsible>(open, "Details").build();
    CHECK(node->open());
    node->toggle();
    CHECK_FALSE(open.peek());
    open.set(true);
    CHECK(node->open());
    open.set(false);
    CHECK_FALSE(node->open());
}

TEST_CASE(
    "Collapsible node retained after page teardown stops signal and callback writes",
    "[collapsible][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    reactive::Signal<bool> open(graph, false);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);
    int calls = 0;
    auto node = ui.make<widget::Collapsible>(open, "Details")
                    .child(ui.make<widget::Label>("Body"))
                    .on_change([&](bool) { ++calls; })
                    .build();
    CHECK_FALSE(node->open());
    open.set(true);
    CHECK(node->open());
    node->toggle();
    CHECK_FALSE(open.peek());
    CHECK(calls == 1);
    scope.clear();
    node->toggle();
    CHECK(node->open());
    CHECK_FALSE(open.peek());
    CHECK(calls == 1);
    open.set(true);
    open.set(false);
    CHECK(node->open());
}

TEST_CASE(
    "Collapsible retained trigger is inert after owner destruction",
    "[collapsible][lifetime]"
) {
    std::shared_ptr<scene::NanNode> trigger;
    {
        auto node = widget::Collapsible::create("Details");
        trigger = node->get_child(0)->shared_from_this();
        CHECK(trigger->resolved_semantics_properties().state.expanded == false);
    }
    CHECK_FALSE(trigger->resolved_semantics_properties().state.expanded.has_value());
    CHECK(trigger->on_semantics_action({.action = semantics::Action::activate}));
}

TEST_CASE(
    "Collapsible deferred content replacement preserves only the final child",
    "[collapsible][slots]"
) {
    Fixture f;
    auto first = std::make_shared<Content>();
    auto second = std::make_shared<Content>();
    auto third = std::make_shared<Content>();
    f.node->set_child(first);
    f.node->set_open(true);
    f.layout();
    {
        auto phase = f.tree.enter_phase(scene::FramePhase::process);
        f.node->set_child(second);
        f.node->set_child(third);
    }
    f.tree.flush_tree_mutations();
    f.layout();
    CHECK(f.node->child() == third.get());
    CHECK_FALSE(first->is_inside_tree());
    CHECK_FALSE(second->is_inside_tree());
    CHECK(third->is_inside_tree());
}

TEST_CASE(
    "Collapsible repeated layouts converge and hidden state does not exit the subtree",
    "[collapsible][layout]"
) {
    Fixture f;
    auto content = std::make_shared<Content>();
    f.node->set_child(content);
    f.node->set_open(true);
    f.layout();
    CHECK(f.tree.layout_root(foundation::NanSize(300, 400)) == 0);
    f.node->set_open(false);
    CHECK(f.tree.layout_root(foundation::NanSize(300, 400)) == 1);
    CHECK(f.tree.layout_root(foundation::NanSize(300, 400)) == 0);
    CHECK(content->enters == 1);
    CHECK(content->exits == 0);
    f.node->set_open(true);
    f.layout();
    CHECK(content->enters == 1);
}

TEST_CASE(
    "Collapsible hidden Label binding remains active without tree reentry",
    "[collapsible][reactive]"
) {
    reactive::Graph graph;
    reactive::Signal<std::string> source(graph, "initial");
    auto label = widget::Label::create(graph);
    label->bind_text(source);
    Fixture f;
    f.node->set_child(label);
    f.node->set_open(true);
    f.layout();
    f.node->set_open(false);
    source.set("changed while hidden");
    CHECK(label->text() == "changed while hidden");
    CHECK(label->text_is_bound());
    f.node->set_open(true);
    f.layout();
    CHECK(label->text() == "changed while hidden");
    CHECK(label->text_is_bound());
}

TEST_CASE(
    "Collapsible collapse preserves outside focus and invisible trigger clears content focus",
    "[collapsible][focus]"
) {
    auto root = widget::Column::create();
    auto node = widget::Collapsible::create("Details");
    auto inside = widget::Button::create("Inside");
    auto outside = widget::Button::create("Outside");
    node->set_child(inside);
    node->set_open(true);
    root->add(node).add(outside);
    scene::NanSceneTree tree;
    tree.set_root(root);
    (void)tree.layout_root(foundation::NanSize(300, 400));
    tree.set_focus(outside.get());
    node->set_open(false);
    CHECK(tree.focused_node() == outside.get());
    node->set_open(true);
    tree.set_focus(inside.get());
    root->set_visible(false);
    node->set_open(false);
    CHECK(tree.focused_node() == nullptr);
}

TEST_CASE(
    "Collapsible trigger style follows theme and typed override persists",
    "[collapsible][theme]"
) {
    theme::ThemeManager themes;
    auto node = widget::Collapsible::create("Details");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(node);
    (void)tree.layout_root(foundation::NanSize(300, 300));
    const auto original = node->trigger().resolved_style().label.color;
    themes.set_preference(theme::ThemePreference::dark);
    CHECK_FALSE(node->trigger().resolved_style().label.color.approx_equals(original));
    const auto color = theme::nan_color(0.7F, 0.1F, 180);
    node->set_trigger_override(
        {.label_color = theme::ThemeColor::literal(color),
         .metrics_height = theme::ThemeScalar::literal(57)}
    );
    node->set_open(true);
    (void)tree.layout_root(foundation::NanSize(300, 300));
    CHECK(node->trigger().height() == Catch::Approx(57));
    CHECK(node->trigger().resolved_style().label.color.approx_equals(color));
    themes.set_preference(theme::ThemePreference::light);
    CHECK(node->trigger().resolved_style().label.color.approx_equals(color));
    CHECK(natural(*node, 300).get_height() == Catch::Approx(57));
}

TEST_CASE("Collapsible restored trigger uses unified focus hint modality", "[collapsible][focus]") {
    Fixture f;
    auto inside = widget::Button::create("Inside");
    f.node->set_child(inside);
    f.node->set_open(true);
    f.layout();
    f.tree.set_focus(inside.get(), {.reason = scene::FocusRequestReason::navigation});
    f.node->set_open(false);
    CHECK(f.tree.focused_node() == &f.node->trigger());
    CHECK(f.tree.should_show_focus_hint());
    click(f.tree, f.node->trigger().global_bounds().get_center());
    CHECK_FALSE(f.tree.should_show_focus_hint());
}

TEST_CASE(
    "Collapsible trigger callback survives removal of the last owner handle",
    "[collapsible][lifetime]"
) {
    auto node = widget::Collapsible::create("Details");
    auto trigger = node->get_child(0)->shared_from_this();
    std::weak_ptr<widget::Collapsible> weak = node;
    node->set_on_change([&](bool) { node.reset(); });
    CHECK(trigger->on_semantics_action({.action = semantics::Action::activate}));
    CHECK(weak.expired());
    CHECK_FALSE(trigger->resolved_semantics_properties().state.expanded.has_value());
}

TEST_CASE(
    "Collapsible deferred clear keeps content detached after replacement",
    "[collapsible][slots]"
) {
    Fixture f;
    auto first = std::make_shared<Content>();
    auto second = std::make_shared<Content>();
    f.node->set_child(first);
    {
        auto phase = f.tree.enter_phase(scene::FramePhase::process);
        f.node->set_child(second);
        f.node->clear_child();
    }
    f.tree.flush_tree_mutations();
    CHECK(f.node->child() == nullptr);
    CHECK_FALSE(first->is_inside_tree());
    CHECK_FALSE(second->is_inside_tree());
    CHECK(f.node->get_child(1)->child_count() == 0);
}

TEST_CASE(
    "Collapsible accepts typed open builder and wraps Label in a scroll viewport",
    "[collapsible][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);
    auto node =
        ui.make<widget::Collapsible>("Details")
            .open(true)
            .gap(3)
            .child(ui.make<widget::Label>("one two three four five six seven eight nine ten")
                       .text_overflow(nandina::text::TextOverflow::wrap)
                       .max_lines(50))
            .build();
    auto scroll = widget::ScrollView::create();
    scroll->set_child(node);
    scene::NanSceneTree tree;
    tree.set_root(scroll);
    (void)tree.layout_root(foundation::NanSize(100, 70));
    CHECK(node->open());
    CHECK(node->height() > node->trigger().height());
    CHECK(scroll->maximum_scroll_offset().get_y() > 0);
    node->set_open(false);
    (void)tree.layout_root(foundation::NanSize(100, 70));
    CHECK(node->height() == Catch::Approx(node->trigger().height()));
    CHECK(scroll->maximum_scroll_offset().get_y() == 0);
}

TEST_CASE("Collapsible expanded state survives merged semantics", "[collapsible][semantics]") {
    Fixture f;
    f.node->set_semantics_composition(semantics::Composition::merge_descendants);
    f.layout();
    REQUIRE(f.tree.update_semantics());
    CHECK(f.tree.semantics_tree().find(f.node->semantics_id())->properties.state.expanded == false);
    f.node->set_open(true);
    REQUIRE(f.tree.update_semantics());
    CHECK(f.tree.semantics_tree().find(f.node->semantics_id())->properties.state.expanded == true);
    f.node->set_semantics_override(
        {.role = semantics::Role::generic, .state = {.expanded = false}}
    );
    REQUIRE(f.tree.update_semantics());
    CHECK(f.tree.semantics_tree().find(f.node->semantics_id())->properties.state.expanded == false);
}

TEST_CASE(
    "Collapsible focus callbacks can replace content without leaving stale siblings",
    "[collapsible][slots]"
) {
    Fixture f;
    auto first = widget::Button::create("First");
    auto outer = std::make_shared<Content>();
    auto inner = std::make_shared<Content>();
    f.node->set_child(first);
    f.node->set_open(true);
    f.layout();
    f.tree.set_focus(first.get());
    first->set_on_focus_changed([&](bool focused) {
        if (!focused) {
            f.node->set_child(inner);
        }
    });
    f.node->set_child(outer);
    CHECK(f.node->child() == inner.get());
    CHECK(inner->is_inside_tree());
    CHECK_FALSE(outer->is_inside_tree());
    CHECK(f.node->get_child(1)->child_count() == 1);
}

TEST_CASE(
    "Collapsible clear yields to a newer focus-callback replacement",
    "[collapsible][slots]"
) {
    Fixture f;
    auto first = widget::Button::create("First");
    auto replacement = std::make_shared<Content>();
    f.node->set_child(first);
    f.node->set_open(true);
    f.layout();
    f.tree.set_focus(first.get());
    first->set_on_focus_changed([&](bool focused) {
        if (!focused) {
            f.node->set_child(replacement);
        }
    });
    f.node->clear_child();
    CHECK(f.node->child() == replacement.get());
    CHECK(replacement->is_inside_tree());
    CHECK(f.node->get_child(1)->child_count() == 1);
}

TEST_CASE(
    "Collapsible instance visual overrides remain above typed trigger rules",
    "[collapsible][theme]"
) {
    auto node = widget::Collapsible::create("Details");
    const auto fill = theme::nan_color(0.6F, 0.09F, 120);
    const auto color = theme::nan_color(0.8F, 0.07F, 220);
    node->set_trigger_override(
        {.container_fill = theme::ThemeColor::token(theme::ColorToken::surface)}
    );
    node->visual_part(widget::visual::container).property(widget::visual::fill_t {}).set(fill);
    node->visual_part(widget::visual::label).property(widget::visual::color_t {}).set(color);
    CHECK(node->trigger().resolved_style().container.fill.approx_equals(fill));
    CHECK(node->trigger().resolved_style().label.color.approx_equals(color));
    node->set_open(true);
    node->toggle();
    CHECK(node->trigger().resolved_style().container.fill.approx_equals(fill));
}

namespace
{
    template<typename Builder>
    concept CanOpen = requires(Builder builder) { builder.open(true); };
    static_assert(CanOpen<widget::authoring::NodeBuilder<widget::Collapsible>>);
    static_assert(!CanOpen<widget::authoring::NodeBuilder<widget::Label>>);
} // namespace
