// accordion_tests — batch 7 Accordion（组合 Collapsible、single/multiple、漫游、事务性）。
//
// 覆盖：section 组合与布局、事务性校验、single/multiple、程序化静默与未知身份、同 id
// 状态保留、disabled、Up/Down/Home/End 漫游（跳过 disabled）、标题更新、gap、语义与
// BuildContext 入口。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/node_ref.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/accordion.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/label.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace nandina;

namespace
{
    struct Fixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
    };

    auto make_label(Fixture& fixture, std::string text) -> std::shared_ptr<widget::Label> {
        return widget::Label::create(fixture.graph, std::move(text), fixture.themes.theme());
    }

    auto make_accordion(Fixture& fixture, const std::size_t count = 3)
        -> std::shared_ptr<widget::Accordion> {
        auto accordion = widget::Accordion::create(fixture.themes.theme());
        std::vector<widget::AccordionSection> sections;
        sections.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            sections.push_back(
                widget::AccordionSection {
                    .id = "s" + std::to_string(index),
                    .title = "Section " + std::to_string(index),
                    .content = make_label(fixture, "body " + std::to_string(index)),
                }
            );
        }
        accordion->set_sections(std::move(sections));
        return accordion;
    }

    auto mount(const std::shared_ptr<widget::Accordion>& accordion)
        -> std::unique_ptr<scene::NanSceneTree> {
        auto tree = std::make_unique<scene::NanSceneTree>();
        tree->set_root(accordion);
        (void)tree->layout_root(foundation::NanSize(400.0F, 400.0F));
        return tree;
    }

    void press_key(scene::NanSceneTree& tree, const foundation::LogicalKey key) {
        tree.dispatch_key(scene::KeyEvent(key, scene::KeyEvent::Action::press, {}));
    }

    void click(scene::NanSceneTree& tree, const foundation::NanPoint point) {
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

TEST_CASE("Accordion composes Collapsible sections", "[accordion][layout]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    CHECK(accordion->section_count() == 3);
    CHECK(accordion->section_node("s0") != nullptr);
    CHECK(accordion->section_node("nope") == nullptr);
    REQUIRE(accordion->trigger_node("s1") != nullptr);
    CHECK(accordion->trigger_node("s1")->text() == "Section 1");
    CHECK(accordion->height() > 0.0F);
    CHECK(accordion->section_node("s0")->global_bounds().get_height() > 0.0F);
}

TEST_CASE("Accordion validates sections transactionally", "[accordion][validation]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 2);
    CHECK(accordion->section_count() == 2);

    auto first = make_label(fixture, "x");
    auto second = make_label(fixture, "y");
    CHECK_THROWS_AS(
        accordion->set_sections({
            {.id = "a", .title = "A", .content = first},
            {.id = "a", .title = "B", .content = second},
        }),
        std::invalid_argument
    );
    CHECK(accordion->section_count() == 2);

    CHECK_THROWS_AS(
        accordion->set_sections({
            {.id = "", .title = "A", .content = first},
        }),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        accordion->set_sections({
            {.id = "a", .title = "", .content = first},
        }),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        accordion->set_sections({
            {.id = "a", .title = "A", .content = nullptr},
        }),
        std::invalid_argument
    );
    // 同一内容不能重复出现在两个 section。
    CHECK_THROWS_AS(
        accordion->set_sections({
            {.id = "a", .title = "A", .content = first},
            {.id = "b", .title = "B", .content = first},
        }),
        std::invalid_argument
    );

    auto mounted = make_label(fixture, "mounted");
    scene::NanSceneTree other;
    other.set_root(mounted);
    CHECK_THROWS_AS(
        accordion->set_sections({
            {.id = "a", .title = "A", .content = mounted},
        }),
        std::logic_error
    );
    CHECK(accordion->section_count() == 2);
}

TEST_CASE(
    "Accordion rejects section content that would create a scene cycle",
    "[accordion][validation]"
) {
    Fixture fixture;

    auto self = widget::Accordion::create(fixture.themes.theme());
    CHECK_THROWS_AS(
        self->set_sections({
            {.id = "self", .title = "Self", .content = self},
        }),
        std::logic_error
    );
    CHECK(self->section_count() == 0);
    CHECK(self->parent() == nullptr);

    auto ancestor = std::make_shared<scene::NanControl>();
    auto nested = widget::Accordion::create(fixture.themes.theme());
    ancestor->add_child(nested);
    CHECK_THROWS_AS(
        nested->set_sections({
            {.id = "ancestor", .title = "Ancestor", .content = ancestor},
        }),
        std::logic_error
    );
    CHECK(nested->section_count() == 0);
    CHECK(nested->parent() == ancestor.get());
}

TEST_CASE(
    "Accordion stages valid sections and rejects build failures before deferral",
    "[accordion][sections][validation]"
) {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 2);
    auto tree = mount(accordion);
    REQUIRE(accordion->toggle("s1"));

    std::vector<widget::AccordionSection> replacement;
    for (std::size_t index = 0; index < 3; ++index) {
        replacement.push_back(
            widget::AccordionSection {
                .id = "s" + std::to_string(index),
                .title = "Staged " + std::to_string(index),
                .content = make_label(fixture, "staged body " + std::to_string(index)),
            }
        );
    }
    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        accordion->set_sections(std::move(replacement));
        CHECK(accordion->section_count() == 2);
        CHECK(accordion->trigger_node("s0")->text() == "Section 0");
    }

    (void)tree->layout_root(foundation::NanSize(400.0F, 400.0F));
    CHECK(accordion->section_count() == 3);
    CHECK(accordion->trigger_node("s0")->text() == "Staged 0");
    CHECK(accordion->is_open("s1"));

    scene::NodeRef<scene::NanControl> anchor_identity;
    auto anchored = make_label(fixture, "anchored");
    anchored->set_anchors({.left = anchor_identity.parent.anchor.left});
    auto valid = make_label(fixture, "valid");
    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        CHECK_THROWS_AS(
            accordion->set_sections({
                {.id = "valid", .title = "Valid", .content = valid},
                {.id = "anchored", .title = "Anchored", .content = anchored},
            }),
            std::logic_error
        );
        CHECK(accordion->section_count() == 3);
        CHECK(valid->parent() == nullptr);
    }
    (void)tree->layout_root(foundation::NanSize(400.0F, 400.0F));
    CHECK(accordion->section_count() == 3);
    CHECK(accordion->is_open("s1"));

    CHECK_THROWS_AS(
        accordion->set_mode(static_cast<widget::AccordionMode>(99)),
        std::invalid_argument
    );
    CHECK(accordion->mode() == widget::AccordionMode::single);
}

TEST_CASE("Accordion single mode keeps one section open", "[accordion][state]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    CHECK(accordion->mode() == widget::AccordionMode::single);
    CHECK(accordion->toggle("s0"));
    CHECK(accordion->is_open("s0"));
    CHECK(accordion->open_ids() == std::vector<std::string> {"s0"});

    CHECK(accordion->toggle("s1"));
    CHECK_FALSE(accordion->is_open("s0"));
    CHECK(accordion->is_open("s1"));
    CHECK(accordion->open_ids() == std::vector<std::string> {"s1"});

    // 允许全部收起。
    CHECK(accordion->toggle("s1"));
    CHECK(accordion->open_ids().empty());
}

TEST_CASE("Accordion multiple mode allows several open", "[accordion][state]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    accordion->set_mode(widget::AccordionMode::multiple);
    CHECK(accordion->toggle("s0"));
    CHECK(accordion->toggle("s2"));
    CHECK(accordion->is_open("s0"));
    CHECK(accordion->is_open("s2"));
    CHECK(accordion->open_ids() == std::vector<std::string> {"s0", "s2"});

    // 切回 single 保留 section 序首个。
    accordion->set_mode(widget::AccordionMode::single);
    CHECK(accordion->is_open("s0"));
    CHECK_FALSE(accordion->is_open("s2"));
}

TEST_CASE("Accordion programmatic state is silent and validates ids", "[accordion][state]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    int events = 0;
    accordion->set_on_open_changed([&events] { ++events; });

    CHECK(accordion->set_open("s1", true));
    CHECK(accordion->is_open("s1"));
    CHECK(events == 0);

    CHECK(accordion->toggle("s2"));
    CHECK(events == 1);
    CHECK_FALSE(accordion->is_open("s1"));
    CHECK(accordion->is_open("s2"));

    CHECK_FALSE(accordion->set_open("nope", true));
    CHECK_FALSE(accordion->set_open_ids({"s0", "nope"}));
    CHECK_FALSE(accordion->is_open("s0"));

    CHECK(accordion->set_open_ids({"s0", "s1"}));
    CHECK(accordion->is_open("s0"));
    CHECK_FALSE(accordion->is_open("s1"));
    CHECK(events == 1);

    accordion->clear_open();
    CHECK(accordion->open_ids().empty());
    CHECK(events == 1);
}

TEST_CASE("Accordion set_sections retains open ids", "[accordion][sections]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    CHECK(accordion->toggle("s1"));
    int events = 0;
    accordion->set_on_open_changed([&events] { ++events; });

    std::vector<widget::AccordionSection> next;
    for (std::size_t index = 0; index < 3; ++index) {
        next.push_back(
            widget::AccordionSection {
                .id = "s" + std::to_string(index),
                .title = "New " + std::to_string(index),
                .content = make_label(fixture, "new body " + std::to_string(index)),
            }
        );
    }
    accordion->set_sections(std::move(next));
    CHECK(accordion->is_open("s1"));
    CHECK(accordion->trigger_node("s1")->text() == "New 1");
    CHECK(events == 0);

    // 移除 s1 后其展开身份被丢弃。
    accordion->set_sections({
        {.id = "s0", .title = "A", .content = make_label(fixture, "a")},
        {.id = "s2", .title = "B", .content = make_label(fixture, "b")},
    });
    CHECK_FALSE(accordion->is_open("s1"));
    CHECK(accordion->section_count() == 2);
}

TEST_CASE("Accordion disabled sections ignore user toggles", "[accordion][disabled]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    CHECK(accordion->set_section_disabled("s1", true));
    CHECK(accordion->section_node("s1")->disabled());
    CHECK_FALSE(accordion->focus_section("s1"));
    CHECK(accordion->toggle("s1"));
    CHECK_FALSE(accordion->is_open("s1"));

    // 程序化 set_open 不受 disabled 限制（Collapsible 契约）。
    CHECK(accordion->set_open("s1", true));
    CHECK(accordion->is_open("s1"));
}

TEST_CASE("Accordion arrow keys roam between triggers", "[accordion][keyboard]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    CHECK(accordion->focus_section("s0"));
    press_key(*tree, foundation::LogicalKey::down);
    CHECK(tree->focused_node() == accordion->trigger_node("s1"));

    press_key(*tree, foundation::LogicalKey::down);
    CHECK(tree->focused_node() == accordion->trigger_node("s2"));

    press_key(*tree, foundation::LogicalKey::down);
    CHECK(tree->focused_node() == accordion->trigger_node("s2"));

    press_key(*tree, foundation::LogicalKey::home);
    CHECK(tree->focused_node() == accordion->trigger_node("s0"));

    press_key(*tree, foundation::LogicalKey::end);
    CHECK(tree->focused_node() == accordion->trigger_node("s2"));

    // disabled 的 section 被跳过。
    CHECK(accordion->set_section_disabled("s1", true));
    CHECK(accordion->focus_section("s0"));
    press_key(*tree, foundation::LogicalKey::down);
    CHECK(tree->focused_node() == accordion->trigger_node("s2"));
}

TEST_CASE(
    "Accordion trigger Enter and Space toggle without arrow-key side effects",
    "[accordion][keyboard][input]"
) {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 2);
    auto tree = mount(accordion);
    int events = 0;
    accordion->set_on_open_changed([&events] { ++events; });

    CHECK(accordion->focus_section("s0"));
    press_key(*tree, foundation::LogicalKey::space);
    CHECK(accordion->is_open("s0"));
    CHECK(events == 1);

    press_key(*tree, foundation::LogicalKey::space);
    CHECK_FALSE(accordion->is_open("s0"));
    CHECK(events == 2);

    press_key(*tree, foundation::LogicalKey::enter);
    CHECK(accordion->is_open("s0"));
    CHECK(events == 3);

    press_key(*tree, foundation::LogicalKey::down);
    CHECK(tree->focused_node() == accordion->trigger_node("s1"));
    CHECK(events == 3);
}

TEST_CASE("Accordion click opens one section and emits once", "[accordion][input]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 3);
    auto tree = mount(accordion);

    int events = 0;
    accordion->set_on_open_changed([&events] { ++events; });

    click(*tree, accordion->trigger_node("s0")->global_bounds().get_center());
    CHECK(accordion->is_open("s0"));
    CHECK(events == 1);

    click(*tree, accordion->trigger_node("s1")->global_bounds().get_center());
    CHECK(accordion->is_open("s1"));
    CHECK_FALSE(accordion->is_open("s0"));
    CHECK(events == 2);
}

TEST_CASE("Accordion updates section title and rejects empty", "[accordion][sections]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 2);
    auto tree = mount(accordion);

    CHECK(accordion->set_section_title("s0", "Renamed"));
    CHECK(accordion->trigger_node("s0")->text() == "Renamed");
    CHECK_THROWS_AS(accordion->set_section_title("s0", ""), std::invalid_argument);
    CHECK(accordion->trigger_node("s0")->text() == "Renamed");
    CHECK_FALSE(accordion->set_section_title("nope", "x"));
}

TEST_CASE("Accordion gap separates sections", "[accordion][layout]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 2);
    accordion->set_gap(10.0F);
    auto tree = mount(accordion);

    const auto top0 = accordion->section_node("s0")->global_bounds();
    const auto top1 = accordion->section_node("s1")->global_bounds();
    CHECK(top1.get_top() - top0.get_bottom() == Catch::Approx(10.0F));

    CHECK_THROWS_AS(accordion->set_gap(-1.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        accordion->set_gap(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );
}

TEST_CASE("Accordion exposes a generic labelled container", "[accordion][semantics]") {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 2);
    accordion->set_label("FAQ");
    auto tree = mount(accordion);

    REQUIRE(tree->update_semantics());
    const auto* node = tree->semantics_tree().find(accordion->semantics_id());
    REQUIRE(node != nullptr);
    CHECK(node->properties.role == semantics::Role::generic);
    CHECK(node->properties.label == "FAQ");
}

TEST_CASE(
    "Accordion stays alive through an aggregate callback that releases its owner",
    "[accordion][lifecycle]"
) {
    Fixture fixture;
    auto accordion = make_accordion(fixture, 2);
    std::weak_ptr<widget::Accordion> lifetime = accordion;
    accordion->set_on_open_changed([&accordion] { accordion.reset(); });
    int later_subscribers = 0;
    auto observer =
        accordion->open_changed().subscribe([&later_subscribers] { ++later_subscribers; });

    CHECK(accordion->toggle("s0"));
    CHECK(later_subscribers == 1);
    CHECK(accordion == nullptr);
    CHECK(lifetime.expired());
}

TEST_CASE(
    "Accordion single-mode convergence is safe across focus-callback replacement",
    "[accordion][state][lifecycle]"
) {
    Fixture fixture;
    auto accordion = widget::Accordion::create(fixture.themes.theme());
    auto body0 = widget::Button::create("body 0", fixture.themes.theme());
    auto body1 = widget::Button::create("body 1", fixture.themes.theme());
    accordion->set_sections({
        {.id = "s0", .title = "S0", .content = body0},
        {.id = "s1", .title = "S1", .content = body1},
    });
    accordion->set_mode(widget::AccordionMode::multiple);
    auto tree = mount(accordion);
    REQUIRE(accordion->toggle("s0"));
    REQUIRE(accordion->toggle("s1"));

    int replacements = 0;
    body1->set_on_focus_changed([&](const bool focused) {
        if (!focused && replacements++ == 0) {
            accordion->set_sections({
                {
                    .id = "new",
                    .title = "New",
                    .content = make_label(fixture, "new body"),
                },
            });
        }
    });
    tree->set_focus(body1.get());
    REQUIRE(tree->focused_node() == body1.get());

    accordion->set_mode(widget::AccordionMode::single);
    CHECK(replacements == 1);
    CHECK(accordion->section_count() == 1);
    CHECK(accordion->section_node("new") != nullptr);
}

TEST_CASE("Accordion is buildable through BuildContext", "[accordion][authoring]") {
    Fixture fixture;
    auto accordion = fixture.ui.make<widget::Accordion>().build();
    REQUIRE(accordion != nullptr);
    CHECK(accordion->section_count() == 0);
    CHECK(accordion->mode() == widget::AccordionMode::single);
}
