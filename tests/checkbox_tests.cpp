//
// Checkbox interaction, semantics, theme, and authoring tests.
//

#include "reactive/scope.hpp"
#include "scene/input_event.hpp"
#include "scene/scene_tree.hpp"
#include "theme/design_system.hpp"
#include "theme/theme_manager.hpp"
#include "widget/build_context.hpp"
#include "widget/checkbox.hpp"
#include "widget/controls.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace nandina;

TEST_CASE("checkbox style resolves semantic theme tokens", "[checkbox][theme]") {
    auto design = theme::default_design_system();
    design.light.primary = theme::nan_color(0.62F, 0.16F, 250.0F);
    design.tokens.spacing.sm = 11.0F;

    const auto checked = theme::resolve_checkbox(
        design,
        theme::ColorAppearance::light,
        /*checked=*/true,
        theme::CheckboxVisualState::normal
    );
    const auto unchecked = theme::resolve_checkbox(
        design,
        theme::ColorAppearance::light,
        /*checked=*/false,
        theme::CheckboxVisualState::normal
    );

    REQUIRE(checked.indicator.fill.oklch().light == Catch::Approx(0.62F));
    REQUIRE(checked.metrics.gap == Catch::Approx(11.0F));
    REQUIRE(unchecked.indicator.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(unchecked.indicator.border.alpha() == Catch::Approx(1.0F));
}

TEST_CASE("checkbox activation toggles value and semantic state", "[checkbox][semantics]") {
    auto checkbox = std::make_shared<widget::Checkbox>("Enable notifications");
    bool observed = false;
    auto subscription =
        checkbox->checked_changed().subscribe([&](const bool value) { observed = value; });
    scene::NanSceneTree tree;
    tree.set_root(checkbox);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* before = tree.semantics_tree().find(checkbox->semantics_id());
    REQUIRE(before != nullptr);
    REQUIRE(before->properties.role == semantics::Role::checkbox);
    REQUIRE(before->properties.state.checked == false);
    REQUIRE(tree.perform_semantics_action(
        checkbox->semantics_id(),
        {.action = semantics::Action::activate}
    ));

    REQUIRE(checkbox->checked());
    REQUIRE(observed);
    REQUIRE(tree.update_semantics());
    const auto* after = tree.semantics_tree().find(checkbox->semantics_id());
    REQUIRE(after != nullptr);
    REQUIRE(after->properties.state.checked == true);
}

TEST_CASE("checkbox supports keyboard activation and disabled state", "[checkbox][input]") {
    auto checkbox = std::make_shared<widget::Checkbox>("Keyboard option");
    scene::NanSceneTree tree;
    tree.set_root(checkbox);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    tree.set_focus(checkbox.get());
    tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press));
    REQUIRE(checkbox->checked());

    checkbox->set_disabled(true);
    tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press));
    REQUIRE(checkbox->checked());
    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(checkbox->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.state.disabled);
    REQUIRE(node->properties.actions == semantics::Action::none);
}

TEST_CASE("BuildContext checkbox synchronizes a boolean signal", "[checkbox][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto& enabled = ui.signal<bool>(false);
    auto checkbox = ui.make<widget::Checkbox>(enabled, "Enable sync").build();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(checkbox);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.perform_semantics_action(
        checkbox->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(enabled.peek());

    enabled.set(false);
    REQUIRE_FALSE(checkbox->checked());
}

TEST_CASE("checkbox builder forwards checked and change modifiers", "[checkbox][authoring]") {
    int changes = 0;
    bool observed = true;
    auto checkbox = widget::authoring::make<widget::Checkbox>("Builder option")
                        .checked(true)
                        .on_change([&](const bool value) {
                            ++changes;
                            observed = value;
                        })
                        .build();

    REQUIRE(checkbox->checked());
    checkbox->toggle();
    REQUIRE_FALSE(checkbox->checked());
    REQUIRE(changes == 1);
    REQUIRE_FALSE(observed);
}
