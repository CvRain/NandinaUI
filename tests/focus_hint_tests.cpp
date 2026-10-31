//
// focus_hint_tests — D1 phase-1 focus hint policy (headless).
//
// Covers the modality gate, editable-target exception, window activation, user
// preference ordering, paint-only invalidation and "no duplicate focus events".
//

#include <catch2/catch_test_macros.hpp>

#include <nandina/scene/control.hpp>
#include <nandina/scene/node2d.hpp>
#include <nandina/scene/scene_tree.hpp>

#include <memory>

using namespace nandina;

namespace
{
    class HintTarget: public scene::NanNode2D {
    public:
        explicit HintTarget(const bool always_hint = false): always_hint_(always_hint) {}

        [[nodiscard]] auto contains_point(const foundation::NanPoint local) const -> bool override {
            return local.get_x() >= -10 && local.get_x() <= 10 && local.get_y() >= -10
                && local.get_y() <= 10;
        }

        [[nodiscard]] auto global_bounds() const -> foundation::NanRect override {
            const auto p = global_position();
            return foundation::NanRect::from_xywh(p.get_x() - 10, p.get_y() - 10, 20, 20);
        }

        [[nodiscard]] auto is_focusable() const -> bool override {
            return true;
        }

        [[nodiscard]] auto focus_hint_requirement() const -> scene::FocusHintRequirement override {
            return always_hint_ ? scene::FocusHintRequirement::always_required
                                : scene::FocusHintRequirement::context_dependent;
        }

        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() == scene::EventType::focus_enter) {
                ++focus_enters;
            }
            if (event.type() == scene::EventType::focus_leave) {
                ++focus_leaves;
            }
            return false;
        }

        int focus_enters = 0;
        int focus_leaves = 0;

    private:
        bool always_hint_;
    };

    struct Fixture {
        scene::NanSceneTree tree;
        std::shared_ptr<scene::NanControl> root = std::make_shared<scene::NanControl>();
        std::shared_ptr<HintTarget> first = std::make_shared<HintTarget>();
        std::shared_ptr<HintTarget> second = std::make_shared<HintTarget>();

        Fixture() {
            first->set_position(foundation::NanPoint(-30.0F, 0.0F));
            root->add_child(first);
            root->add_child(second);
            tree.set_root(root);
        }
    };

    auto click(scene::NanSceneTree& tree, const foundation::NanPoint point) -> void {
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                point
            )
        );
    }
} // namespace

TEST_CASE(
    "no interaction history shows the hint, pointer hides it, keyboard restores",
    "[focus][hint]"
) {
    Fixture fixture;
    auto& tree = fixture.tree;
    tree.set_window_active(true);

    // Programmatic focus before any input: accepted default is visible.
    tree.set_focus(fixture.first.get());
    REQUIRE(tree.focused_node() == fixture.first.get());
    REQUIRE(tree.input_modality() == scene::InputModality::unknown);
    REQUIRE(tree.should_show_focus_hint());

    // Pointer press establishes the pointing context before focus callbacks run.
    click(tree, foundation::NanPoint(-30.0F, 0.0F));
    REQUIRE(tree.focused_node() == fixture.first.get());
    REQUIRE(tree.input_modality() == scene::InputModality::pointer);
    REQUIRE_FALSE(tree.should_show_focus_hint());

    // Passive events do not change the hint strategy.
    tree.dispatch_mouse_move(
        scene::MouseMoveEvent(foundation::NanPoint(0.0F, 0.0F), foundation::NanPoint(1.0F, 0.0F))
    );
    tree.dispatch_mouse_wheel(
        scene::MouseWheelEvent(
            foundation::NanPoint(0.0F, 0.0F),
            foundation::NanPoint(0.0F, 1.0F),
            scene::KeyModifiers {}
        )
    );
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::left_shift, scene::KeyEvent::Action::press)
    );
    REQUIRE(tree.input_modality() == scene::InputModality::pointer);
    REQUIRE_FALSE(tree.should_show_focus_hint());

    // Tab is a meaningful keyboard input even though the scene itself does not navigate.
    tree.dispatch_key(scene::KeyEvent(foundation::LogicalKey::tab, scene::KeyEvent::Action::press));
    REQUIRE(tree.input_modality() == scene::InputModality::keyboard);
    REQUIRE(tree.should_show_focus_hint());

    // Wheel/move do not erase the keyboard hint.
    tree.dispatch_mouse_wheel(
        scene::MouseWheelEvent(
            foundation::NanPoint(0.0F, 0.0F),
            foundation::NanPoint(0.0F, 1.0F),
            scene::KeyModifiers {}
        )
    );
    REQUIRE(tree.should_show_focus_hint());
}

TEST_CASE("editable targets keep a visible hint for pointer focus", "[focus][hint]") {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto editable = std::make_shared<HintTarget>(/*always_hint=*/true);
    root->add_child(editable);
    tree.set_root(root);
    tree.set_window_active(true);

    click(tree, foundation::NanPoint(0.0F, 0.0F));
    REQUIRE(tree.focused_node() == editable.get());
    REQUIRE(tree.input_modality() == scene::InputModality::pointer);
    REQUIRE(tree.should_show_focus_hint());

    tree.set_window_active(false);
    REQUIRE_FALSE(tree.should_show_focus_hint());
    tree.set_window_active(true);
    REQUIRE(tree.should_show_focus_hint());
}

TEST_CASE("window inactivity gates hints and preserves logical focus", "[focus][hint]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    tree.set_focus(fixture.first.get());
    const auto enters = fixture.first->focus_enters;
    REQUIRE(tree.should_show_focus_hint());

    tree.set_focus_hint_always_visible(true);
    REQUIRE(tree.should_show_focus_hint());

    tree.set_window_active(false);
    REQUIRE_FALSE(tree.should_show_focus_hint());
    REQUIRE(tree.focused_node() == fixture.first.get());

    tree.set_window_active(true);
    REQUIRE(tree.should_show_focus_hint());
    REQUIRE(fixture.first->focus_enters == enters); // no synthesized focus-enter
}

TEST_CASE("always-visible preference covers pointer hiding", "[focus][hint]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    tree.set_window_active(true);
    tree.set_focus_hint_always_visible(true);
    click(tree, foundation::NanPoint(0.0F, 0.0F));
    REQUIRE(tree.input_modality() == scene::InputModality::pointer);
    REQUIRE(tree.should_show_focus_hint());

    tree.set_focus_hint_always_visible(false);
    REQUIRE_FALSE(tree.should_show_focus_hint());
}

TEST_CASE("explicit request context overrides the recent modality", "[focus][hint]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    tree.set_window_active(true);

    tree.record_input_modality(scene::InputModality::keyboard);
    tree.set_focus(
        fixture.first.get(),
        scene::FocusRequestContext {
            .reason = scene::FocusRequestReason::programmatic,
            .modality = scene::InputModality::pointer
        }
    );
    REQUIRE_FALSE(tree.should_show_focus_hint());

    tree.set_focus(
        fixture.second.get(),
        scene::FocusRequestContext {
            .reason = scene::FocusRequestReason::programmatic,
            .modality = scene::InputModality::keyboard
        }
    );
    REQUIRE(tree.should_show_focus_hint());

    // Navigation is keyboard interaction even without an explicit modality.
    tree.record_input_modality(scene::InputModality::pointer);
    tree.focus_next();
    REQUIRE(tree.input_modality() == scene::InputModality::keyboard);
    REQUIRE(tree.should_show_focus_hint());
}

TEST_CASE("hint-only changes add paint and nothing else", "[focus][hint]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    auto* target = fixture.first.get();
    tree.set_focus(target);
    REQUIRE(tree.should_show_focus_hint());
    (void)tree.update_semantics();
    const auto revision = tree.semantics_tree().revision;
    REQUIRE_FALSE(tree.semantics_dirty());

    const auto all = scene::DirtyFlags::style | scene::DirtyFlags::measure
        | scene::DirtyFlags::layout | scene::DirtyFlags::paint | scene::DirtyFlags::semantics
        | scene::DirtyFlags::transform;
    target->clear_dirty(all);
    // Pre-existing invalidation must not be cleared by the hint update.
    target->mark_dirty(scene::DirtyFlags::layout);

    tree.record_input_modality(scene::InputModality::pointer);
    REQUIRE(target->is_dirty(scene::DirtyFlags::paint));
    REQUIRE(target->is_dirty(scene::DirtyFlags::layout));
    REQUIRE_FALSE(target->is_dirty(scene::DirtyFlags::measure));
    REQUIRE_FALSE(target->is_dirty(scene::DirtyFlags::transform));
    REQUIRE_FALSE(target->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE_FALSE(tree.semantics_dirty());
    REQUIRE_FALSE(tree.update_semantics());
    REQUIRE(tree.semantics_tree().revision == revision);
    REQUIRE(tree.focused_node() == target);
}

TEST_CASE("context changes with an unchanged hint result do not repaint", "[focus][hint]") {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto editable = std::make_shared<HintTarget>(/*always_hint=*/true);
    root->add_child(editable);
    tree.set_root(root);
    tree.set_window_active(true);

    tree.record_input_modality(scene::InputModality::pointer);
    tree.set_focus(editable.get());
    REQUIRE(tree.should_show_focus_hint());

    editable->clear_dirty(scene::DirtyFlags::paint);
    tree.record_input_modality(scene::InputModality::keyboard);
    REQUIRE(tree.should_show_focus_hint());
    REQUIRE_FALSE(editable->is_dirty(scene::DirtyFlags::paint));
}

TEST_CASE("hint changes never re-send focus events", "[focus][hint]") {
    Fixture fixture;
    auto& tree = fixture.tree;
    tree.set_focus(fixture.first.get());
    const auto enters = fixture.first->focus_enters;
    const auto leaves = fixture.first->focus_leaves;

    for (int i = 0; i < 3; ++i) {
        tree.record_input_modality(scene::InputModality::pointer);
        tree.record_input_modality(scene::InputModality::keyboard);
        tree.set_window_active(false);
        tree.set_window_active(true);
    }
    REQUIRE(fixture.first->focus_enters == enters);
    REQUIRE(fixture.first->focus_leaves == leaves);
}
