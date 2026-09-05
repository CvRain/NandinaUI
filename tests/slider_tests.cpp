//
// Slider range, input, semantics, theme, and authoring tests.
//

#include "reactive/scope.hpp"
#include "scene/input_event.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "theme/design_system.hpp"
#include "theme/theme_manager.hpp"
#include "widget/build_context.hpp"
#include "widget/controls.hpp"
#include "widget/slider.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace nandina;

TEST_CASE("slider normalizes values to its range and step", "[slider][value]") {
    widget::Slider slider("Zoom", 1.03F, 0.5F, 2.0F, 0.1F);
    REQUIRE(slider.value() == Catch::Approx(1.0F));

    slider.set_value(4.0F);
    REQUIRE(slider.value() == Catch::Approx(2.0F));
    slider.set_value(0.76F);
    REQUIRE(slider.value() == Catch::Approx(0.8F));
    REQUIRE_THROWS_AS(slider.set_range(2.0F, 1.0F), std::invalid_argument);
    REQUIRE_THROWS_AS(slider.set_step(0.0F), std::invalid_argument);
}

TEST_CASE("slider keyboard and semantic actions emit user changes", "[slider][semantics]") {
    auto slider = std::make_shared<widget::Slider>("Interface scale", 1.0F, 0.5F, 1.5F, 0.1F);
    int changes = 0;
    auto subscription = slider->value_changed().subscribe([&](const float) { ++changes; });
    scene::NanSceneTree tree;
    tree.set_root(slider);
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 40.0F)) >= 1);
    tree.set_focus(slider.get());
    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press));
    REQUIRE(slider->value() == Catch::Approx(1.1F));

    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(slider->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::slider);
    REQUIRE(node->properties.value == "1.1");
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::set_value));
    REQUIRE(tree.perform_semantics_action(
        slider->semantics_id(),
        {.action = semantics::Action::set_value, .value = "1.34"}
    ));
    REQUIRE(slider->value() == Catch::Approx(1.3F));
    REQUIRE(tree.perform_semantics_action(
        slider->semantics_id(),
        {.action = semantics::Action::decrement}
    ));
    REQUIRE(slider->value() == Catch::Approx(1.2F));
    REQUIRE(changes == 3);
}

TEST_CASE("slider pointer dragging uses capture outside its bounds", "[slider][input]") {
    auto slider = std::make_shared<widget::Slider>("Volume", 0.0F, 0.0F, 100.0F, 1.0F);
    scene::NanSceneTree tree;
    tree.set_root(slider);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 32.0F)) >= 1);

    tree.dispatch_mouse_move(
        scene::MouseMoveEvent(foundation::NanPoint(10.0F, 16.0F), foundation::NanPoint::zero())
    );
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(10.0F, 16.0F)
        )
    );
    REQUIRE(tree.pointer_capture() == slider.get());
    tree.dispatch_mouse_move(
        scene::MouseMoveEvent(
            foundation::NanPoint(260.0F, 16.0F),
            foundation::NanPoint(250.0F, 0.0F)
        )
    );
    REQUIRE(slider->value() == Catch::Approx(100.0F));
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::release,
            foundation::NanPoint(260.0F, 16.0F)
        )
    );
    REQUIRE(tree.pointer_capture() == nullptr);
}

TEST_CASE("BuildContext slider synchronizes a float signal", "[slider][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto& scale = ui.signal<float>(1.0F);
    auto slider = ui.make<widget::Slider>(scale, "Scale", 0.5F, 2.0F, 0.1F).build();
    scene::NanSceneTree tree;
    tree.set_root(slider);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 32.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.perform_semantics_action(
        slider->semantics_id(),
        {.action = semantics::Action::increment}
    ));
    REQUIRE(scale.peek() == Catch::Approx(1.1F));
    scale.set(1.5F);
    REQUIRE(slider->value() == Catch::Approx(1.5F));
}

TEST_CASE("slider style resolves semantic theme colors", "[slider][theme]") {
    auto design = theme::default_design_system();
    design.light.primary = theme::nan_color(0.72F, 0.12F, 190.0F);
    const auto style = theme::resolve_slider(
        design,
        theme::ColorAppearance::light,
        theme::SliderVisualState::dragging
    );
    REQUIRE(style.active_track.box.fill.oklch().light == Catch::Approx(0.72F));
    REQUIRE(style.thumb.box.radius == Catch::Approx(11.0F));
}

TEST_CASE("slider value label opt-in increases measured height and tracks value", "[slider][label]") {
    auto slider = std::make_shared<widget::Slider>("Zoom", 0.5F, 0.0F, 1.0F, 0.1F);
    REQUIRE_FALSE(slider->show_value_label());
    REQUIRE(slider->value_label_text() == "0.5");

    const auto base = slider->measure_layout(scene::LayoutConstraints::loose());
    slider->set_show_value_label(true);
    REQUIRE(slider->show_value_label());
    const auto with_label = slider->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(with_label.get_height() > base.get_height());

    // 开启后不额外占高（重复调用幂等），值标签随 value 更新。
    const auto again = slider->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(again.get_height() == Catch::Approx(with_label.get_height()));
    slider->set_value(0.9F);
    REQUIRE(slider->value_label_text() == "0.9");
}
