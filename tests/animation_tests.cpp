//
// Animation easing + tween tests.
//

#include "animation/animated_property.hpp"
#include "animation/animation_host.hpp"
#include "animation/behavior.hpp"
#include "animation/easing.hpp"
#include "animation/group.hpp"
#include "animation/keyframes.hpp"
#include "animation/motion.hpp"
#include "animation/spring.hpp"
#include "animation/tween.hpp"
#include "foundation/nandina_color.hpp"
#include "reactive/scope.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/build_context.hpp"
#include "widget/builtin_component_traits.hpp"
#include "widget/button.hpp"
#include "widget/label.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

using namespace nandina;

namespace
{
    class AnimatedProbe final: public scene::NanControl {
    public:
        animation::AnimatedProperty<float> paint_value {0.0F};
        animation::AnimatedProperty<float> layout_value {0.0F};
    };

    class GroupProbe final: public scene::NanControl {
    public:
        animation::AnimatedProperty<float> a {0.0F};
        animation::AnimatedProperty<float> b {0.0F};
        animation::AnimatedProperty<float> c {0.0F};
    };

    void advance(scene::NanSceneTree& tree, const float dt) {
        auto phase = tree.enter_phase(scene::FramePhase::animation);
        tree.advance_animations(dt);
    }

    constexpr auto all_dirty_flags = scene::DirtyFlags::measure | scene::DirtyFlags::layout
        | scene::DirtyFlags::paint | scene::DirtyFlags::semantics;
} // namespace

static_assert(
    static_cast<int>(scene::FramePhase::reactive) < static_cast<int>(scene::FramePhase::animation)
);
static_assert(
    static_cast<int>(scene::FramePhase::animation) < static_cast<int>(scene::FramePhase::layout)
);
static_assert(widget::property::Animatable<widget::Label, decltype(widget::visual::label.color)>);
static_assert(
    widget::property::Animatable<widget::Button, decltype(widget::visual::container.radius)>
);
static_assert(
    !widget::property::Animatable<widget::Label, decltype(widget::visual::container.radius)>
);
static_assert(
    widget::property::Springable<widget::Button, decltype(widget::visual::container.radius)>
);
static_assert(
    !widget::property::Springable<widget::Label, decltype(widget::visual::label.color)>
);

TEST_CASE("easing curves map 0->0 and 1->1", "[animation][easing]") {
    for (const auto easing:
         {animation::Easing::linear,
          animation::Easing::ease_in,
          animation::Easing::ease_out,
          animation::Easing::ease_in_out})
    {
        REQUIRE(animation::ease(easing, 0.0F) == Catch::Approx(0.0F));
        REQUIRE(animation::ease(easing, 1.0F) == Catch::Approx(1.0F));
    }
}

TEST_CASE("easing curves stay within [0,1] and ease-in lags", "[animation][easing]") {
    const float mid = animation::ease(animation::Easing::ease_in, 0.5F);
    REQUIRE(mid > 0.0F);
    REQUIRE(mid < 0.5F); // ease-in 在中点落后于线性。

    const float out = animation::ease(animation::Easing::ease_out, 0.5F);
    REQUIRE(out > 0.5F); // ease-out 在中点超前于线性。

    const float in_out = animation::ease(animation::Easing::ease_in_out, 0.5F);
    REQUIRE(in_out == Catch::Approx(0.5F)); // ease-in-out 中点正好一半。
}

TEST_CASE("tween advances from start to end with easing", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(0.0F, 100.0F, 1.0F, animation::Easing::linear);

    REQUIRE_FALSE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(0.0F));

    REQUIRE(tween.tick(0.25F) == Catch::Approx(25.0F));
    REQUIRE(tween.tick(0.25F) == Catch::Approx(50.0F));
    REQUIRE(tween.tick(0.5F) == Catch::Approx(100.0F));
    REQUIRE(tween.is_finished());
    REQUIRE(tween.progress() == Catch::Approx(1.0F));
}

TEST_CASE("tween zero duration finishes immediately at target", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(3.0F, 9.0F, 0.0F);
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(9.0F));
}

TEST_CASE("tween finish and reset jump without animation", "[animation][tween]") {
    animation::Tween<float> tween(0.0F);
    tween.start(0.0F, 10.0F, 1.0F);
    (void)tween.tick(0.2F);
    REQUIRE_FALSE(tween.is_finished());

    tween.finish();
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(10.0F));

    tween.reset(42.0F);
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(42.0F));
}

TEST_CASE("tween clamps dt overshoot and reuses target", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(0.0F, 4.0F, 1.0F, animation::Easing::linear);
    (void)tween.tick(5.0F); // 远超时长
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(4.0F));
}

TEST_CASE("tween ignores negative and NaN dt", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(0.0F, 10.0F, 1.0F, animation::Easing::linear);

    REQUIRE(tween.tick(-0.5F) == Catch::Approx(0.0F));
    REQUIRE(tween.tick(std::numeric_limits<float>::quiet_NaN()) == Catch::Approx(0.0F));
    REQUIRE_FALSE(tween.is_finished());
}

TEST_CASE("color tween interpolates OKLCH hue over the shortest arc", "[animation][color]") {
    const auto from = foundation::NanColor::from_oklch(0.4F, 0.1F, 350.0F, 0.2F);
    const auto to = foundation::NanColor::from_oklch(0.8F, 0.3F, 10.0F, 1.0F);
    animation::Tween<foundation::NanColor> tween;
    tween.start(from, to, 1.0F, animation::Easing::linear);

    const auto mid = tween.tick(0.5F).oklch();
    REQUIRE(mid.light == Catch::Approx(0.6F));
    REQUIRE(mid.chroma == Catch::Approx(0.2F));
    REQUIRE(mid.hue == Catch::Approx(0.0F).margin(0.001F));
    REQUIRE(mid.alpha == Catch::Approx(0.6F));
}

TEST_CASE(
    "animated property jumps until an enabled behavior is installed",
    "[animation][property]"
) {
    animation::AnimatedProperty<float> property(2.0F);
    property.set_target(8.0F);
    REQUIRE(property.target() == Catch::Approx(8.0F));
    REQUIRE(property.value() == Catch::Approx(8.0F));
    REQUIRE_FALSE(property.is_animating());

    property.set_behavior(animation::Behavior<float>(1.0F).set_enabled(false));
    property.set_target(12.0F);
    REQUIRE(property.value() == Catch::Approx(12.0F));

    property.set_behavior(animation::Behavior<float>(0.0F));
    property.set_target(16.0F);
    REQUIRE(property.value() == Catch::Approx(16.0F));
}

TEST_CASE("animated property separates target and presentation value", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);

    REQUIRE(property.target() == Catch::Approx(10.0F));
    REQUIRE(property.value() == Catch::Approx(0.0F));
    REQUIRE(property.is_animating());
    REQUIRE(property.tick(0.25F) == Catch::Approx(2.5F));
    REQUIRE(property.progress() == Catch::Approx(0.25F));
}

TEST_CASE(
    "animated property retargets continuously from its current value",
    "[animation][property]"
) {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    REQUIRE(property.tick(0.5F) == Catch::Approx(5.0F));

    property.set_target(15.0F);
    REQUIRE(property.value() == Catch::Approx(5.0F));
    REQUIRE(property.target() == Catch::Approx(15.0F));
    REQUIRE(property.tick(0.5F) == Catch::Approx(10.0F));
    REQUIRE(property.tick(5.0F) == Catch::Approx(15.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE("writing the same target does not restart an active property", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    REQUIRE(property.tick(0.25F) == Catch::Approx(2.5F));

    property.set_target(10.0F);
    REQUIRE(property.tick(0.25F) == Catch::Approx(5.0F));
}

TEST_CASE("clearing behavior finishes the current property transition", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    (void)property.tick(0.25F);

    property.clear_behavior();
    REQUIRE(property.value() == Catch::Approx(10.0F));
    REQUIRE_FALSE(property.is_animating());
    REQUIRE_FALSE(property.behavior().has_value());
}

TEST_CASE("disabling behavior finishes an active property transition", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    (void)property.tick(0.25F);

    property.set_behavior(animation::Behavior<float>(1.0F).set_enabled(false));
    REQUIRE(property.value() == Catch::Approx(10.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE("behavior rejects invalid durations", "[animation][behavior]") {
    REQUIRE_THROWS_AS(animation::Behavior<float>(-0.1F), std::invalid_argument);
    REQUIRE_THROWS_AS(
        animation::Behavior<float>(std::numeric_limits<float>::infinity()),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        animation::Behavior<float>(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );
}

TEST_CASE("animation host advances only active properties with manual dt", "[animation][host]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    probe->clear_dirty(all_dirty_flags);

    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    REQUIRE(tree.animation_host().active_count() == 1);
    REQUIRE(probe->paint_value.target() == Catch::Approx(10.0F));
    REQUIRE(probe->paint_value.value() == Catch::Approx(0.0F));
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::paint));

    advance(tree, 0.25F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(2.5F));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
    REQUIRE_FALSE(probe->is_dirty(scene::layout_dirty_flags));
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::semantics));

    probe->clear_dirty(all_dirty_flags);
    advance(tree, 5.0F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));

    probe->clear_dirty(all_dirty_flags);
    advance(tree, 0.5F);
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::paint));
}

TEST_CASE("animation host retargets one property without duplicate tracks", "[animation][host]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));

    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    advance(tree, 0.5F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(5.0F));

    tree.animation_host().set_target(*probe, probe->paint_value, 20.0F, scene::DirtyFlags::paint);
    REQUIRE(tree.animation_host().active_count() == 1);
    REQUIRE(probe->paint_value.value() == Catch::Approx(5.0F));
    advance(tree, 0.5F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(12.5F));
}

TEST_CASE("animation host applies immediate targets and exact dirty flags", "[animation][host]") {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto probe = std::make_shared<AnimatedProbe>();
    root->add_child(probe);
    tree.set_root(root);
    (void)tree.update_semantics();
    REQUIRE_FALSE(tree.semantics_dirty());
    root->clear_dirty(all_dirty_flags);
    probe->clear_dirty(all_dirty_flags);

    tree.animation_host().set_target(
        *probe,
        probe->paint_value,
        4.0F,
        scene::DirtyFlags::paint | scene::DirtyFlags::semantics
    );
    REQUIRE(probe->paint_value.value() == Catch::Approx(4.0F));
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE_FALSE(probe->is_dirty(scene::layout_dirty_flags));
    REQUIRE_FALSE(root->is_dirty(scene::layout_dirty_flags));
    REQUIRE(tree.semantics_dirty());

    probe->layout_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    root->clear_dirty(all_dirty_flags);
    probe->clear_dirty(all_dirty_flags);
    tree.animation_host().set_target(
        *probe,
        probe->layout_value,
        8.0F,
        scene::layout_dirty_flags | scene::DirtyFlags::paint
    );
    advance(tree, 0.25F);
    REQUIRE(probe->is_dirty(scene::DirtyFlags::measure));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::layout));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE(root->is_dirty(scene::DirtyFlags::measure));
    REQUIRE(root->is_dirty(scene::DirtyFlags::layout));
}

TEST_CASE("animation host cancels tracks when an owner exits the tree", "[animation][host]") {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto probe = std::make_shared<AnimatedProbe>();
    root->add_child(probe);
    tree.set_root(root);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    advance(tree, 0.25F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(2.5F));
    REQUIRE(tree.animation_host().active_count() == 1);

    auto detached = root->remove_child(*probe);
    REQUIRE(detached == probe);
    REQUIRE_FALSE(probe->is_inside_tree());
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE_FALSE(probe->paint_value.is_animating());
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));

    advance(tree, 1.0F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
}

TEST_CASE("animation host rejects owners from another scene tree", "[animation][host]") {
    scene::NanSceneTree first;
    scene::NanSceneTree second;
    auto probe = std::make_shared<AnimatedProbe>();
    first.set_root(probe);

    REQUIRE_THROWS_AS(
        second.animation_host()
            .set_target(*probe, probe->paint_value, 1.0F, scene::DirtyFlags::paint),
        std::invalid_argument
    );
}

TEST_CASE(
    "animation phase defers tree mutation and host clear finishes tracks",
    "[animation][host]"
) {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    advance(tree, 0.25F);
    probe->clear_dirty(all_dirty_flags);

    {
        auto phase = tree.enter_phase(scene::FramePhase::animation);
        REQUIRE(tree.defers_tree_mutation());
        tree.animation_host().clear();
    }

    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE_FALSE(probe->paint_value.is_animating());
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
}

TEST_CASE("reduced motion forces new targets to jump without a track", "[animation][host][reduced-motion]") {
    scene::NanSceneTree tree;
    theme::ThemeManager themes;
    themes.set_motion_preference(theme::MotionPreference::reduced);
    tree.set_theme_manager(themes);

    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));

    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);

    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
    REQUIRE_FALSE(probe->paint_value.is_animating());
}

TEST_CASE(
    "reduced motion toggled mid-flight finishes active tracks",
    "[animation][host][reduced-motion]"
) {
    scene::NanSceneTree tree;
    theme::ThemeManager themes;
    tree.set_theme_manager(themes);

    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    advance(tree, 0.25F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(2.5F));
    REQUIRE(tree.animation_host().active_count() == 1);

    themes.set_system_reduced_motion(true);
    advance(tree, 0.25F);

    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
    REQUIRE_FALSE(probe->paint_value.is_animating());
}

TEST_CASE(
    "reduced motion off resumes animation for new targets",
    "[animation][host][reduced-motion]"
) {
    scene::NanSceneTree tree;
    theme::ThemeManager themes;
    tree.set_theme_manager(themes);
    themes.set_system_reduced_motion(true);

    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F)); // jumped

    themes.set_system_reduced_motion(false);
    tree.animation_host().set_target(*probe, probe->paint_value, 20.0F, scene::DirtyFlags::paint);
    REQUIRE(tree.animation_host().active_count() == 1);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F)); // starts from current value

    advance(tree, 0.5F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(15.0F));
}

TEST_CASE(
    "builder behavior respects global reduced motion",
    "[animation][authoring][reduced-motion]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    themes.set_motion_preference(theme::MotionPreference::reduced);
    widget::BuildContext ui {graph, scope, themes};

    reactive::Signal<float> radius {graph, 4.0F};
    auto button = ui.make<widget::Button>("Button")
                      .behavior(
                          widget::visual::container.radius,
                          animation::Behavior<float>(1.0F, animation::Easing::linear)
                      )
                      .bind(widget::visual::container.radius, radius)
                      .build();

    auto root = std::make_shared<scene::NanControl>();
    root->add_child(button);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);

    radius.set(20.0F);
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(20.0F));
}

TEST_CASE(
    "builder bindings and behaviors share scene-owned property endpoints",
    "[animation][authoring][endpoint]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    const auto initial_color = foundation::NanColor::from_oklch(0.2F, 0.1F, 40.0F);
    const auto target_color = foundation::NanColor::from_oklch(0.8F, 0.1F, 40.0F);
    reactive::Signal<foundation::NanColor> color {graph, initial_color};
    reactive::Signal<float> radius {graph, 4.0F};

    auto label = ui.make<widget::Label>("Animated label")
                     .bind(widget::visual::label.color, color)
                     .behavior(
                         widget::visual::label.color,
                         animation::Behavior<foundation::NanColor>(1.0F, animation::Easing::linear)
                     )
                     .build();
    auto button = ui.make<widget::Button>("Animated button")
                      .behavior(
                          widget::visual::container.radius,
                          animation::Behavior<float>(1.0F, animation::Easing::linear)
                      )
                      .bind(widget::visual::container.radius, radius)
                      .build();

    auto root = std::make_shared<scene::NanControl>();
    root->add_child(label);
    root->add_child(button);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);

    const auto initial_label_endpoint = label->property(widget::visual::color_t {}).value();
    REQUIRE(initial_label_endpoint != nullptr);
    REQUIRE(initial_label_endpoint->approx_equals(initial_color));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(4.0F));
    REQUIRE(tree.animation_host().active_count() == 0);

    color.set(target_color);
    radius.set(20.0F);
    REQUIRE(tree.animation_host().active_count() == 2);
    REQUIRE(label->color().approx_equals(target_color));
    REQUIRE(
        button->visual_part(widget::visual::container_t {})
            .property(widget::visual::radius_t {})
            .target()
        != nullptr
    );
    REQUIRE(
        *button->visual_part(widget::visual::container_t {})
             .property(widget::visual::radius_t {})
             .target()
        == Catch::Approx(20.0F)
    );

    advance(tree, 0.5F);
    const auto midpoint = label->property(widget::visual::color_t {}).value()->oklch();
    REQUIRE(midpoint.light == Catch::Approx(0.5F));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(12.0F));

    label->set_color(initial_color);
    widget::property::write(*button, widget::visual::container.radius, 8.0F);
    REQUIRE(tree.animation_host().active_count() == 2);
    REQUIRE(label->color().approx_equals(initial_color));
    REQUIRE(
        *button->visual_part(widget::visual::container_t {})
             .property(widget::visual::radius_t {})
             .target()
        == Catch::Approx(8.0F)
    );

    advance(tree, 1.0F);
    REQUIRE(label->property(widget::visual::color_t {}).value()->approx_equals(initial_color));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(8.0F));
    REQUIRE(tree.animation_host().active_count() == 0);

    const auto ignored_color = foundation::NanColor::from_oklch(0.6F, 0.2F, 180.0F);
    scope.clear();
    color.set(ignored_color);
    radius.set(30.0F);
    REQUIRE(label->color().approx_equals(initial_color));
    REQUIRE(
        *button->visual_part(widget::visual::container_t {})
             .property(widget::visual::radius_t {})
             .target()
        == Catch::Approx(8.0F)
    );
}

TEST_CASE(
    "property endpoints reconcile behavior changes and cleared overrides",
    "[animation][endpoint][lifecycle]"
) {
    reactive::Graph graph;
    auto label = widget::Label::create(graph, "Label");
    auto button = widget::Button::create("Button");
    button->set_font_size(12.0F);
    widget::property::set_behavior(
        *label,
        widget::visual::label.color,
        animation::Behavior<foundation::NanColor>(1.0F, animation::Easing::linear)
    );
    widget::property::set_behavior(
        *button,
        widget::visual::label.font_size,
        animation::Behavior<float>(1.0F, animation::Easing::linear)
    );

    auto root = std::make_shared<scene::NanControl>();
    root->add_child(label);
    root->add_child(button);
    scene::NanSceneTree tree;
    tree.set_root(root);

    const auto target_color = foundation::NanColor::from_oklch(0.7F, 0.2F, 250.0F);
    label->set_color(target_color);
    button->set_font_size(24.0F);
    REQUIRE(tree.animation_host().active_count() == 2);
    advance(tree, 0.25F);

    widget::property::set_behavior(
        *label,
        widget::visual::label.color,
        animation::Behavior<foundation::NanColor>(1.0F).set_enabled(false)
    );
    REQUIRE(label->property(widget::visual::color_t {}).value()->approx_equals(target_color));
    REQUIRE(tree.animation_host().active_count() == 1);

    button->clear_font_size();
    REQUIRE(tree.animation_host().active_count() == 0);
    advance(tree, 1.0F);
    REQUIRE(tree.animation_host().active_count() == 0);
}

TEST_CASE("parallel group fires all clips immediately", "[animation][group]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<GroupProbe>();
    tree.set_root(probe);

    auto group = animation::Group::parallel(
        {animation::Group::clip(
             *probe, probe->a, 10.0F, animation::Behavior<float>(1.0F, animation::Easing::linear), scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe, probe->b, 20.0F, animation::Behavior<float>(1.0F, animation::Easing::linear), scene::DirtyFlags::paint
         )}
    );
    tree.animation_host().run(*probe, std::move(group));
    REQUIRE(tree.animation_host().active_count() == 1); // 单条 group 轨道

    advance(tree, 0.5F);
    REQUIRE(probe->a.value() == Catch::Approx(5.0F));
    REQUIRE(probe->b.value() == Catch::Approx(10.0F));
}

TEST_CASE("sequential group fires a clip only after the previous finishes", "[animation][group]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<GroupProbe>();
    tree.set_root(probe);

    auto group = animation::Group::sequential(
        {animation::Group::clip(
             *probe, probe->a, 10.0F, animation::Behavior<float>(0.2F, animation::Easing::linear), scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe, probe->b, 20.0F, animation::Behavior<float>(0.2F, animation::Easing::linear), scene::DirtyFlags::paint
         )}
    );
    tree.animation_host().run(*probe, std::move(group));

    advance(tree, 0.1F);
    REQUIRE(probe->a.value() == Catch::Approx(5.0F));
    REQUIRE(probe->b.value() == Catch::Approx(0.0F)); // b 尚未触发

    advance(tree, 0.1F);
    REQUIRE(probe->a.value() == Catch::Approx(10.0F)); // a 完成
    REQUIRE(probe->b.value() == Catch::Approx(0.0F));  // b 下一帧才触发

    advance(tree, 0.1F);
    REQUIRE(probe->a.value() == Catch::Approx(10.0F));
    REQUIRE(probe->b.value() == Catch::Approx(10.0F)); // b 触发并推进
}

TEST_CASE("stagger group fires clips at fixed intervals", "[animation][group]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<GroupProbe>();
    tree.set_root(probe);

    auto group = animation::Group::stagger(
        {animation::Group::clip(
             *probe,
             probe->a,
             10.0F,
             animation::Behavior<float>(0.3F, animation::Easing::linear),
             scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe,
             probe->b,
             20.0F,
             animation::Behavior<float>(0.3F, animation::Easing::linear),
             scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe,
             probe->c,
             30.0F,
             animation::Behavior<float>(0.3F, animation::Easing::linear),
             scene::DirtyFlags::paint
         )},
        0.2F
    );
    tree.animation_host().run(*probe, std::move(group));

    advance(tree, 0.1F); // elapsed 0.1：a 触发
    REQUIRE(probe->a.value() > 0.0F);
    REQUIRE(probe->b.value() == Catch::Approx(0.0F));
    REQUIRE(probe->c.value() == Catch::Approx(0.0F));

    advance(tree, 0.1F); // elapsed 0.2：b 触发
    REQUIRE(probe->b.value() > 0.0F);
    REQUIRE(probe->c.value() == Catch::Approx(0.0F));

    advance(tree, 0.1F); // elapsed 0.3：c 仍未触发
    REQUIRE(probe->c.value() == Catch::Approx(0.0F));

    advance(tree, 0.1F); // elapsed 0.4：c 触发
    REQUIRE(probe->c.value() > 0.0F);
}

TEST_CASE("group finish jumps all clips to target", "[animation][group]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<GroupProbe>();
    tree.set_root(probe);

    auto group = animation::Group::stagger(
        {animation::Group::clip(
             *probe, probe->a, 10.0F, animation::Behavior<float>(1.0F, animation::Easing::linear), scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe, probe->b, 20.0F, animation::Behavior<float>(1.0F, animation::Easing::linear), scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe, probe->c, 30.0F, animation::Behavior<float>(1.0F, animation::Easing::linear), scene::DirtyFlags::paint
         )},
        0.5F
    );
    tree.animation_host().run(*probe, std::move(group));
    advance(tree, 0.1F);
    REQUIRE(probe->a.value() == Catch::Approx(1.0F)); // 仅 a 已触发

    tree.animation_host().clear(); // 触发 group.finish()：所有 clip 跳转到目标
    REQUIRE(probe->a.value() == Catch::Approx(10.0F));
    REQUIRE(probe->b.value() == Catch::Approx(20.0F));
    REQUIRE(probe->c.value() == Catch::Approx(30.0F));
    REQUIRE(tree.animation_host().active_count() == 0);
}

TEST_CASE("group is cancelled when its owner exits the tree", "[animation][group]") {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto probe = std::make_shared<GroupProbe>();
    root->add_child(probe);
    tree.set_root(root);

    auto group = animation::Group::stagger(
        {animation::Group::clip(
             *probe, probe->a, 10.0F, animation::Behavior<float>(1.0F, animation::Easing::linear), scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe, probe->c, 30.0F, animation::Behavior<float>(1.0F, animation::Easing::linear), scene::DirtyFlags::paint
         )},
        1.0F
    );
    tree.animation_host().run(*probe, std::move(group));
    advance(tree, 0.1F);
    REQUIRE(tree.animation_host().active_count() == 1);

    (void)root->remove_child(*probe); // 退出树 → cancel_owner → group.finish()
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(probe->a.value() == Catch::Approx(10.0F));
    REQUIRE(probe->c.value() == Catch::Approx(30.0F));
}

TEST_CASE("spring overshoots and settles at target", "[animation][spring]") {
    animation::Spring<float> spring(0.0F);
    // 欠阻尼：ζ = c / (2√(km)) ≈ 0.35 < 1，会产生 overshoot。
    spring.start(0.0F, 100.0F, animation::SpringSpec(200.0F, 10.0F));
    REQUIRE_FALSE(spring.is_finished());

    bool overshot = false;
    for (int i = 0; i < 600 && !spring.is_finished(); ++i) {
        const float v = spring.tick(1.0F / 60.0F);
        if (v > 100.0F) {
            overshot = true;
        }
    }
    REQUIRE(spring.is_finished());
    REQUIRE(overshot);
    REQUIRE(spring.value() == Catch::Approx(100.0F));
}

TEST_CASE("spring retargets without resetting velocity", "[animation][spring]") {
    animation::Spring<float> spring(0.0F);
    spring.start(0.0F, 100.0F, animation::SpringSpec(200.0F, 10.0F));
    (void)spring.tick(1.0F / 60.0F); // 获得初速度
    const float before = spring.value();

    spring.set_target(150.0F);
    REQUIRE(spring.value() == Catch::Approx(before)); // 位置连续，不回跳
    REQUIRE_FALSE(spring.is_finished());
}

TEST_CASE("spring finish jumps to target", "[animation][spring]") {
    animation::Spring<float> spring(0.0F);
    spring.start(0.0F, 100.0F, animation::SpringSpec(200.0F, 10.0F));
    (void)spring.tick(1.0F / 60.0F);
    spring.finish();
    REQUIRE(spring.is_finished());
    REQUIRE(spring.value() == Catch::Approx(100.0F));
}

TEST_CASE("spring spec rejects invalid parameters", "[animation][spring]") {
    REQUIRE_THROWS_AS(animation::SpringSpec(-1.0F, 10.0F), std::invalid_argument);
    REQUIRE_THROWS_AS(animation::SpringSpec(200.0F, -1.0F), std::invalid_argument);
    REQUIRE_THROWS_AS(animation::SpringSpec(200.0F, 10.0F, 0.0F), std::invalid_argument);
    REQUIRE_THROWS_AS(
        animation::SpringSpec(std::numeric_limits<float>::infinity(), 10.0F),
        std::invalid_argument
    );
}

TEST_CASE("animated property supports spring mode with overshoot", "[animation][property][spring]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_spring(animation::SpringSpec(200.0F, 10.0F));
    property.set_target(100.0F);
    REQUIRE(property.target() == Catch::Approx(100.0F));
    REQUIRE(property.value() == Catch::Approx(0.0F)); // 从当前值起跳
    REQUIRE(property.is_animating());

    bool overshot = false;
    for (int i = 0; i < 600 && property.is_animating(); ++i) {
        const float v = property.tick(1.0F / 60.0F);
        if (v > 100.0F) {
            overshot = true;
        }
    }
    REQUIRE(overshot);
    REQUIRE(property.value() == Catch::Approx(100.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE(
    "animated property spring and behavior are mutually exclusive",
    "[animation][property][spring]"
) {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_spring(animation::SpringSpec(200.0F, 10.0F));
    REQUIRE_FALSE(property.behavior().has_value()); // behavior 被清除
    REQUIRE(property.spring().has_value());

    property.set_target(100.0F);
    REQUIRE(property.is_animating());

    property.clear_spring();
    REQUIRE_FALSE(property.spring().has_value());
    REQUIRE(property.value() == Catch::Approx(100.0F)); // 直跳回目标
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE("keyframes interpolate across time and finish at the last frame", "[animation][keyframes]") {
    animation::Keyframes<float> keyframes;
    keyframes.start(
        {{.time = 0.0F, .value = 0.0F},
         {.time = 0.5F, .value = 10.0F},
         {.time = 1.0F, .value = 0.0F}}
    );
    REQUIRE_FALSE(keyframes.is_finished());
    REQUIRE(keyframes.value() == Catch::Approx(0.0F));

    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(5.0F));  // 0 → 10 中点
    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(10.0F)); // 到 0.5s
    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(5.0F));  // 10 → 0 中点
    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(0.0F));  // 到 1.0s，结束
    REQUIRE(keyframes.is_finished());
    REQUIRE(keyframes.target() == Catch::Approx(0.0F));
}

TEST_CASE("keyframes reject empty, non-increasing, and non-zero start", "[animation][keyframes]") {
    animation::Keyframes<float> keyframes;
    REQUIRE_THROWS_AS(keyframes.start({}), std::invalid_argument);
    REQUIRE_THROWS_AS(
        keyframes.start({{.time = 0.1F, .value = 0.0F}, {.time = 1.0F, .value = 1.0F}}),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        keyframes.start({{.time = 0.0F, .value = 0.0F}, {.time = 0.0F, .value = 1.0F}}),
        std::invalid_argument
    );
}

TEST_CASE("animated property plays keyframes and clears back to target", "[animation][property][keyframes]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_keyframes(
        {{.time = 0.0F, .value = 0.0F},
         {.time = 0.4F, .value = 8.0F},
         {.time = 0.8F, .value = 4.0F}}
    );
    REQUIRE(property.target() == Catch::Approx(4.0F)); // 末帧值
    REQUIRE(property.is_animating());
    REQUIRE(property.value() == Catch::Approx(0.0F));

    REQUIRE(property.tick(0.2F) == Catch::Approx(4.0F)); // 0 → 8 中点
    REQUIRE(property.tick(0.2F) == Catch::Approx(8.0F)); // 到 0.4s
    REQUIRE(property.tick(0.2F) == Catch::Approx(6.0F)); // 8 → 4 中点
    REQUIRE(property.tick(0.2F) == Catch::Approx(4.0F)); // 到 0.8s，结束
    REQUIRE_FALSE(property.is_animating());

    property.clear_keyframes();
    REQUIRE_FALSE(property.keyframes().has_value());
    REQUIRE(property.value() == Catch::Approx(4.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE(
    "animated property keyframes and behavior are mutually exclusive",
    "[animation][property][keyframes]"
) {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_keyframes(
        {{.time = 0.0F, .value = 0.0F}, {.time = 1.0F, .value = 10.0F}}
    );
    REQUIRE_FALSE(property.behavior().has_value()); // behavior 被清除
    REQUIRE(property.keyframes().has_value());

    property.set_target(20.0F); // set_target 清除 keyframes，无 behavior 直跳
    REQUIRE_FALSE(property.keyframes().has_value());
    REQUIRE(property.value() == Catch::Approx(20.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE("motion::tween builds a behavior spec", "[animation][motion]") {
    const auto spec = animation::motion::tween(0.24F).easing(animation::motion::ease_out);
    const auto behavior = spec.behavior<float>();
    REQUIRE(behavior.duration() == Catch::Approx(0.24F));
    REQUIRE(behavior.easing() == animation::Easing::ease_out);
    REQUIRE(behavior.enabled());

    REQUIRE_THROWS_AS(animation::motion::tween(-0.1F), std::invalid_argument);
}

TEST_CASE("motion::spring builds a spring spec fluently", "[animation][motion]") {
    const auto spec =
        animation::motion::spring().stiffness(200.0F).damping(12.0F).mass(2.0F);
    REQUIRE(spec.stiffness() == Catch::Approx(200.0F));
    REQUIRE(spec.damping() == Catch::Approx(12.0F));
    REQUIRE(spec.mass() == Catch::Approx(2.0F));
}

TEST_CASE(
    "builder accepts motion::tween and binds a visual property",
    "[animation][motion][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    reactive::Signal<float> radius {graph, 4.0F};
    auto button = ui.make<widget::Button>("Button")
                      .behavior(
                          widget::visual::container.radius,
                          animation::motion::tween(0.4F).easing(animation::motion::ease_standard)
                      )
                      .bind(widget::visual::container.radius, radius)
                      .build();

    auto root = std::make_shared<scene::NanControl>();
    root->add_child(button);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);

    radius.set(24.0F);
    REQUIRE(tree.animation_host().active_count() == 1);
    advance(tree, 0.2F);
    // 0.4s ease_in_out 中点：半径到 24 与 4 的中点 14。
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(14.0F));
}

TEST_CASE(
    "builder accepts motion::spring for a float visual property",
    "[animation][motion][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    reactive::Signal<float> radius {graph, 4.0F};
    auto button = ui.make<widget::Button>("Button")
                      .spring(
                          widget::visual::container.radius,
                          animation::motion::spring().stiffness(200.0F).damping(10.0F)
                      )
                      .bind(widget::visual::container.radius, radius)
                      .build();

    auto root = std::make_shared<scene::NanControl>();
    root->add_child(button);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);

    radius.set(24.0F);
    REQUIRE(tree.animation_host().active_count() == 1);

    bool overshot = false;
    for (int i = 0; i < 240 && tree.animation_host().active_count() > 0; ++i) {
        advance(tree, 1.0F / 60.0F);
        if (button->resolved_style().container.radius > 24.0F) {
            overshot = true;
        }
    }
    REQUIRE(overshot);
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(24.0F).margin(0.05F));
}
