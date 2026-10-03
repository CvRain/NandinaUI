//
// Animation easing + tween tests.
//

#include <nandina/animation/animated_property.hpp>
#include <nandina/animation/animation_host.hpp>
#include <nandina/animation/behavior.hpp>
#include <nandina/animation/easing.hpp>
#include <nandina/animation/group.hpp>
#include <nandina/animation/keyframes.hpp>
#include <nandina/animation/spring.hpp>
#include <nandina/animation/tween.hpp>
#include <nandina/foundation/motion/spec.hpp>
#include <nandina/foundation/motion/spring.hpp>
#include <nandina/foundation/nandina_color.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/reactive/signal.hpp>
#include <nandina/scene/control.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/builtin_component_traits.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/label.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <limits>
#include <optional>
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

    class TransformCacheProbe final: public scene::NanControl {
    public:
        // effective_transform resolves its origin through this virtual query.
        mutable unsigned origin_queries = 0;
        auto as_control() const -> const scene::NanControl* override {
            ++origin_queries;
            return this;
        }
    };

    void advance(scene::NanSceneTree& tree, const float dt) {
        auto phase = tree.enter_phase(scene::FramePhase::animation);
        tree.advance_animations(dt);
    }

    constexpr auto all_dirty_flags = scene::DirtyFlags::measure | scene::DirtyFlags::layout
        | scene::DirtyFlags::paint | scene::DirtyFlags::semantics | scene::DirtyFlags::transform;
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
static_assert(!widget::property::Springable<widget::Label, decltype(widget::visual::label.color)>);
static_assert(widget::property::Animatable<scene::NanNode2D, decltype(widget::visual::opacity)>);
static_assert(widget::property::Animatable<scene::NanNode2D, decltype(widget::visual::translate)>);
static_assert(widget::property::Animatable<scene::NanNode2D, decltype(widget::visual::scale)>);

// 场景层三条路径必须对**组件**同样成立，而不只是对裸 `NanNode2D`。这里刻意选两个会
// 隐藏基类 `visual_part` 重载的类型：`Button` 自己声明了 label/container 两个重载，
// `Text` 声明了 label 重载 —— 二者都没有 `using NanNode2D::visual_part`，所以
// `property::detail::visual_part` 里那条 `derived_from<NanNode2D>` 的兜底重载是承重的。
// 缺了它，下面这些断言会静默失效（概念不满足 ⇒ 整个 DSL 入口消失）。
static_assert(widget::property::Animatable<widget::Button, decltype(widget::visual::opacity)>);
static_assert(widget::property::Animatable<widget::Button, decltype(widget::visual::translate)>);
static_assert(widget::property::Animatable<widget::Button, decltype(widget::visual::scale)>);
static_assert(
    widget::property::Animatable<widget::primitives::Text, decltype(widget::visual::opacity)>
);
static_assert(
    widget::property::Animatable<widget::primitives::Text, decltype(widget::visual::scale)>
);
// opacity 是浮点路径，弹簧必须能配上去（这条曾经是假的：门面上少了 set_spring）。
static_assert(widget::property::Springable<widget::Button, decltype(widget::visual::opacity)>);
static_assert(!widget::property::Springable<widget::Button, decltype(widget::visual::scale)>);

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

TEST_CASE("point tween interpolates both axes", "[animation][tween][point]") {
    animation::Tween<foundation::NanPoint> tween;
    tween.start(
        foundation::NanPoint(2.0F, -4.0F),
        foundation::NanPoint(10.0F, 8.0F),
        1.0F,
        animation::Easing::linear
    );

    const auto midpoint = tween.tick(0.5F);
    REQUIRE(midpoint.get_x() == Catch::Approx(6.0F));
    REQUIRE(midpoint.get_y() == Catch::Approx(2.0F));
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

TEST_CASE("node opacity transition stays paint-only", "[animation][node-presentation]") {
    scene::NanSceneTree tree;
    auto node = std::make_shared<scene::NanNode2D>();
    tree.set_root(node);

    auto opacity = node->visual_part(scene::visual::node).property(scene::visual::opacity_t {});
    opacity.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    node->clear_dirty(all_dirty_flags);
    opacity.set(0.0F);

    REQUIRE(*opacity.target() == Catch::Approx(0.0F));
    REQUIRE(*opacity.value() == Catch::Approx(1.0F));
    REQUIRE(tree.animation_host().active_count() == 1);
    REQUIRE_FALSE(node->is_dirty(scene::layout_dirty_flags));

    advance(tree, 0.5F);
    REQUIRE(node->local_opacity() == Catch::Approx(0.5F));
    REQUIRE(node->is_dirty(scene::DirtyFlags::paint));
    REQUIRE_FALSE(node->is_dirty(scene::layout_dirty_flags));
    REQUIRE_FALSE(node->is_dirty(scene::DirtyFlags::semantics));
}

TEST_CASE(
    "presentation transform updates bounds hit testing and semantics without layout",
    "[animation][node-presentation][hit-test]"
) {
    scene::NanSceneTree tree;
    auto control = std::make_shared<scene::NanControl>();
    tree.set_root(control);
    control->layout_to(foundation::NanRect::from_xywh(10.0F, 20.0F, 100.0F, 40.0F));
    (void)tree.update_semantics();
    control->clear_dirty(all_dirty_flags);

    control->set_transform_origin(scene::TransformOrigin::center);
    control->set_presentation_translate(foundation::NanPoint(5.0F, 3.0F));
    control->set_presentation_scale(foundation::NanPoint(2.0F, 2.0F));

    const auto bounds = control->global_bounds();
    REQUIRE(bounds.get_x() == Catch::Approx(-35.0F));
    REQUIRE(bounds.get_y() == Catch::Approx(3.0F));
    REQUIRE(bounds.get_width() == Catch::Approx(200.0F));
    REQUIRE(bounds.get_height() == Catch::Approx(80.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(-30.0F, 10.0F)) == control.get());
    REQUIRE(tree.hit_test(foundation::NanPoint(-40.0F, 10.0F)) == nullptr);
    REQUIRE(control->is_dirty(scene::DirtyFlags::paint));
    REQUIRE(control->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE_FALSE(control->is_dirty(scene::layout_dirty_flags));
    REQUIRE(tree.semantics_dirty());
}

TEST_CASE(
    "presentation transform survives layout and origin follows the new size",
    "[animation][node-presentation][layout]"
) {
    scene::NanControl control;
    control.set_transform_origin(scene::TransformOrigin::bottom_right);
    control.set_presentation_translate(foundation::NanPoint(4.0F, 6.0F));
    control.set_presentation_scale(foundation::NanPoint(2.0F, 3.0F));

    control.layout_to(foundation::NanRect::from_xywh(20.0F, 30.0F, 50.0F, 40.0F));
    auto bounds = control.global_bounds();
    REQUIRE(bounds.get_x() == Catch::Approx(-26.0F));
    REQUIRE(bounds.get_y() == Catch::Approx(-44.0F));
    REQUIRE(bounds.get_width() == Catch::Approx(100.0F));
    REQUIRE(bounds.get_height() == Catch::Approx(120.0F));

    control.layout_to(foundation::NanRect::from_xywh(40.0F, 50.0F, 100.0F, 20.0F));
    bounds = control.global_bounds();
    REQUIRE(bounds.get_x() == Catch::Approx(-56.0F));
    REQUIRE(bounds.get_y() == Catch::Approx(16.0F));
    REQUIRE(bounds.get_width() == Catch::Approx(200.0F));
    REQUIRE(bounds.get_height() == Catch::Approx(60.0F));
}

TEST_CASE(
    "a presentation transform animates bounds hit testing and the semantics snapshot in step",
    "[animation][node-presentation][hit-test][mid-flight]"
) {
    scene::NanSceneTree tree;
    auto control = std::make_shared<scene::NanControl>();
    tree.set_root(control);
    // 裸 NanControl 的 role 是 none、不会进语义树，给它一个角色才读得到快照。
    control->set_semantics_override(semantics::Properties {.role = semantics::Role::button});
    control->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 40.0F));

    control->set_transform_origin(scene::TransformOrigin::top_left);
    auto translate =
        control->visual_part(scene::visual::node).property(scene::visual::translate_t {});
    translate.set_behavior(
        animation::Behavior<foundation::NanPoint>(1.0F, animation::Easing::linear)
    );

    const auto snapshot_bounds = [&tree, &control]() -> std::optional<foundation::NanRect> {
        const auto* snapshot = tree.semantics_tree().find(control->semantics_id());
        if (snapshot == nullptr) {
            return std::nullopt;
        }
        return snapshot->bounds;
    };

    // 先把变换缓存与语义快照都**预热**出来。之后每一帧都先清掉脏位再推进，这样每个
    // 断言看到的都必须是这一帧当场算出来的状态，而不是上一帧留下的缓存。
    REQUIRE(control->global_bounds().get_x() == Catch::Approx(0.0F));
    REQUIRE(tree.update_semantics());
    const auto initial_bounds = snapshot_bounds();
    REQUIRE(initial_bounds.has_value());
    REQUIRE(initial_bounds->get_x() == Catch::Approx(0.0F));

    translate.set(foundation::NanPoint(100.0F, 0.0F));
    REQUIRE(translate.value()->get_x() == Catch::Approx(0.0F));

    float previous = 0.0F;
    for (int frame = 1; frame <= 30; ++frame) {
        control->clear_dirty(all_dirty_flags);
        advance(tree, 1.0F / 30.0F);

        const float value = translate.value()->get_x();
        // linear、1 秒、每帧 1/30 秒 ⇒ 第 n 帧正好是 100 * n / 30
        REQUIRE(value == Catch::Approx(100.0F * static_cast<float>(frame) / 30.0F));
        REQUIRE(value > previous);
        previous = value;

        // 缓存、bounds、命中、语义快照四者必须与动画值同步
        REQUIRE(control->global_transform().position().get_x() == Catch::Approx(value));
        REQUIRE(control->global_bounds().get_x() == Catch::Approx(value));
        REQUIRE(tree.hit_test(foundation::NanPoint(value + 5.0F, 20.0F)) == control.get());
        REQUIRE(tree.hit_test(foundation::NanPoint(value - 5.0F, 20.0F)) == nullptr);
        REQUIRE(control->is_dirty(scene::DirtyFlags::transform));
        REQUIRE(control->is_dirty(scene::DirtyFlags::semantics));
        REQUIRE_FALSE(control->is_dirty(scene::layout_dirty_flags));

        REQUIRE(tree.semantics_dirty());
        REQUIRE(tree.update_semantics());
        const auto bounds = snapshot_bounds();
        REQUIRE(bounds.has_value());
        REQUIRE(bounds->get_x() == Catch::Approx(value));
    }

    advance(tree, 1.0F / 30.0F);
    REQUIRE(control->global_bounds().get_x() == Catch::Approx(100.0F));
    REQUIRE(tree.animation_host().active_count() == 0);
}

TEST_CASE(
    "transform and semantics dirty bits are independent",
    "[animation][node-presentation][dirty]"
) {
    scene::NanSceneTree tree;
    auto control = std::make_shared<scene::NanControl>();
    tree.set_root(control);

    // 纯语义变化：不该声称几何变了。
    control->clear_dirty(all_dirty_flags);
    control->mark_dirty(scene::DirtyFlags::semantics);
    REQUIRE(control->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE_FALSE(control->is_dirty(scene::DirtyFlags::transform));
    REQUIRE(tree.semantics_dirty());

    // 几何变化：蕴含语义（bounds 变了），但反过来不成立。
    control->clear_dirty(all_dirty_flags);
    control->mark_dirty(scene::DirtyFlags::transform);
    REQUIRE(control->is_dirty(scene::DirtyFlags::transform));
    REQUIRE(control->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE_FALSE(control->is_dirty(scene::layout_dirty_flags));
}

TEST_CASE("semantic changes preserve a warm transform cache", "[animation][cache][dirty]") {
    TransformCacheProbe control;
    (void)control.global_transform();
    const auto warm_queries = control.origin_queries;
    REQUIRE(warm_queries > 0);
    control.mark_dirty(scene::DirtyFlags::semantics);
    (void)control.global_transform();
    REQUIRE(control.origin_queries == warm_queries);

    control.mark_dirty(scene::DirtyFlags::transform);
    (void)control.global_transform();
    REQUIRE(control.origin_queries > warm_queries);
    const auto refreshed_queries = control.origin_queries;
    // The bit remains set: every new mutation must still invalidate the cache.
    control.mark_dirty(scene::DirtyFlags::transform);
    (void)control.global_transform();
    REQUIRE(control.origin_queries > refreshed_queries);
}

TEST_CASE(
    "an opacity spring installed through the component DSL animates and stays paint-only",
    "[animation][node-presentation][spring][widget]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);

    // 走组件作者真正写的那条路：builder 上装弹簧，之后再改值。
    auto button = ui.make<widget::Button>("fade")
                      .spring(widget::visual::opacity, motion::spring(300.0F, 12.0F))
                      .build();
    tree.set_root(button);

    // 组件自己声明了 visual_part 重载、隐藏了基类那个（`Button` 只有 label/container
    // 两个候选），所以测试里读裸 endpoint 必须显式回到基类 —— 作者侧应当用
    // `widget::property::write` / builder，那条路有 detail::visual_part 兜底。
    const auto opacity_endpoint = [](widget::Button& node) -> decltype(auto) {
        return static_cast<scene::NanNode2D&>(node)
            .visual_part(scene::visual::node)
            .property(scene::visual::opacity_t {});
    };

    // 构建出来的节点带着初始的 layout/paint 脏位（还没跑过一次布局），先把基线清掉，
    // 后面才谈得上"动画有没有让它重新变脏"。
    button->clear_dirty(all_dirty_flags);

    auto opacity = opacity_endpoint(*button);
    REQUIRE(button->local_opacity() == Catch::Approx(1.0F));
    REQUIRE(tree.animation_host().active_count() == 0);

    // 第一次设目标就该动画，而不是跳过去（opacity 的 endpoint 带初值，所以弹簧立即生效）。
    widget::property::write(*button, widget::visual::opacity, 0.0F);
    REQUIRE(*opacity.value() == Catch::Approx(1.0F));
    REQUIRE(*opacity.target() == Catch::Approx(0.0F));
    REQUIRE(tree.animation_host().active_count() == 1);

    // 常规帧率验证组件路径；独立的 spring 测试覆盖卡顿与不同帧分割。
    for (int frame = 0; frame < 6; ++frame) {
        advance(tree, 1.0F / 60.0F);
    }
    const float descending = *opacity.value();
    REQUIRE(descending < 1.0F);
    REQUIRE(descending > 0.0F);
    REQUIRE(button->is_dirty(scene::DirtyFlags::paint));
    REQUIRE_FALSE(button->is_dirty(scene::layout_dirty_flags));

    // 中途改目标：继续沿同一条轨道推进，不重置、也不新增轨道。
    widget::property::write(*button, widget::visual::opacity, 1.0F);
    REQUIRE(*opacity.value() == Catch::Approx(descending));
    REQUIRE(*opacity.target() == Catch::Approx(1.0F));
    REQUIRE(tree.animation_host().active_count() == 1);

    for (int frame = 0; frame < 3; ++frame) {
        advance(tree, 1.0F / 60.0F);
        REQUIRE_FALSE(button->is_dirty(scene::layout_dirty_flags));
    }
    REQUIRE(*opacity.value() != Catch::Approx(descending));

    for (int frame = 0; frame < 240; ++frame) {
        advance(tree, 1.0F / 60.0F);
        REQUIRE_FALSE(button->is_dirty(scene::layout_dirty_flags));
    }
    REQUIRE(*opacity.value() == Catch::Approx(1.0F).margin(0.01F));
    REQUIRE(tree.animation_host().active_count() == 0);
}

TEST_CASE(
    "an underdamped opacity spring overshoots but presentation stays inside [0,1]",
    "[animation][node-presentation][spring][clamp]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);

    // ζ = 12 / (2 * sqrt(300)) ≈ 0.35 ⇒ 过冲约 30%，足够越过 [0,1] 两端。
    auto button = ui.make<widget::Button>("pulse")
                      .spring(widget::visual::opacity, motion::spring(300.0F, 12.0F))
                      .build();
    tree.set_root(button);
    const auto opacity_endpoint = [](widget::Button& node) -> decltype(auto) {
        return static_cast<scene::NanNode2D&>(node)
            .visual_part(scene::visual::node)
            .property(scene::visual::opacity_t {});
    };
    button->clear_dirty(all_dirty_flags);
    auto opacity = opacity_endpoint(*button);

    const auto settle_to = [&](const float target) {
        widget::property::write(*button, widget::visual::opacity, target);
        for (int frame = 0; frame < 240; ++frame) {
            advance(tree, 1.0F / 60.0F);
        }
    };

    settle_to(0.0F);
    REQUIRE(*opacity.value() == Catch::Approx(0.0F).margin(0.01F));

    // 0 → 1：物理值应当冲过 1，呈现值必须停在 1。
    widget::property::write(*button, widget::visual::opacity, 1.0F);
    float max_raw = 0.0F;
    float max_presented = 0.0F;
    for (int frame = 0; frame < 240; ++frame) {
        advance(tree, 1.0F / 60.0F);
        max_raw = std::max(max_raw, *opacity.value());
        max_presented = std::max(max_presented, button->local_opacity());
    }
    REQUIRE(max_raw > 1.0F); // 前提：欠阻尼弹簧确实过冲了，否则这条测试没验证到东西
    REQUIRE(max_presented <= 1.0F); // 呈现仍然守 [0,1] 契约
    REQUIRE(*opacity.value() == Catch::Approx(1.0F).margin(0.01F));

    // 1 → 0：同理，物理值会冲到 0 以下，呈现值不能是负数。
    widget::property::write(*button, widget::visual::opacity, 0.0F);
    float min_raw = 1.0F;
    float min_presented = 1.0F;
    for (int frame = 0; frame < 240; ++frame) {
        advance(tree, 1.0F / 60.0F);
        min_raw = std::min(min_raw, *opacity.value());
        min_presented = std::min(min_presented, button->local_opacity());
    }
    REQUIRE(min_raw < 0.0F);
    REQUIRE(min_presented >= 0.0F);
}

TEST_CASE(
    "moving a parent invalidates descendant geometry caches",
    "[animation][node-presentation][cache][descendant]"
) {
    scene::NanSceneTree tree;
    auto parent = std::make_shared<scene::NanControl>();
    auto child = std::make_shared<scene::NanControl>();
    tree.set_root(parent);
    parent->add_child(child);

    parent->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 40.0F));
    child->layout_to(foundation::NanRect::from_xywh(10.0F, 10.0F, 20.0F, 20.0F));

    // 先把父子的缓存都预热出来。
    REQUIRE(parent->global_bounds().get_x() == Catch::Approx(0.0F));
    REQUIRE(child->global_bounds().get_x() == Catch::Approx(10.0F));

    // 父节点只动表现层 —— 子节点自己没变，但它的世界坐标变了，缓存必须跟着失效。
    parent->set_presentation_translate(foundation::NanPoint(5.0F, 0.0F));
    REQUIRE(parent->global_bounds().get_x() == Catch::Approx(5.0F));
    REQUIRE(child->global_bounds().get_x() == Catch::Approx(15.0F));
    REQUIRE(child->is_dirty(scene::DirtyFlags::transform));
    REQUIRE(tree.hit_test(foundation::NanPoint(20.0F, 20.0F)) == child.get());

    // 改父节点尺寸（位置不动）：缩放中心按父节点尺寸解析，父与子的缓存都必须重算。
    parent->set_presentation_translate(foundation::NanPoint(0.0F, 0.0F));
    parent->set_transform_origin(scene::TransformOrigin::center);
    parent->set_presentation_scale(foundation::NanPoint(2.0F, 2.0F));
    REQUIRE(parent->global_bounds().get_x() == Catch::Approx(-50.0F));
    REQUIRE(child->global_bounds().get_x() == Catch::Approx(-30.0F));

    parent->set_size(foundation::NanSize(50.0F, 40.0F));
    REQUIRE(parent->global_bounds().get_x() == Catch::Approx(-25.0F));
    REQUIRE(parent->global_bounds().get_width() == Catch::Approx(100.0F));
    REQUIRE(child->global_bounds().get_x() == Catch::Approx(-5.0F));
}

TEST_CASE(
    "a pure size change re-resolves the presentation origin",
    "[animation][node-presentation][layout][cache]"
) {
    scene::NanControl control;
    control.set_transform_origin(scene::TransformOrigin::center);
    control.set_presentation_scale(foundation::NanPoint(2.0F, 2.0F));

    control.layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 40.0F));
    // 先让 global transform 的缓存真正算出来：若换尺寸时不失效缓存，下面会停在旧值上。
    REQUIRE(control.global_bounds().get_x() == Catch::Approx(-50.0F));

    control.set_size(foundation::NanSize(200.0F, 40.0F));
    REQUIRE(control.global_bounds().get_x() == Catch::Approx(-100.0F));
    REQUIRE(control.global_bounds().get_width() == Catch::Approx(400.0F));
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

TEST_CASE(
    "a transition installed on a button radius animates a bound signal change",
    "[animation][host][widget]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);

    // 与 showcase 侧边栏同一条路径：先登记过渡策略，再把信号绑到同一个属性上。策略只是
    // **插值方式**而不是值，所以配件（主题 / tone / treatment）仍然是值的来源。
    auto& hovered = ui.signal_value(false);
    auto button = ui.make<widget::Button>("item")
                      .behavior(widget::visual::container.radius, motion::tween(0.12F))
                      .build();

    const float base = button->resolved_style().container.radius;
    REQUIRE(base > 0.0F);

    auto& radius = ui.computed([&hovered, base] { return hovered.get() ? base + 2.0F : base; });
    ui.bind(button, widget::visual::container.radius, radius);
    tree.set_root(button);

    const auto radius_now = [&button]() -> std::optional<float> {
        const auto* value = button->visual_part(widget::visual::container_t {})
                                .property(widget::visual::radius_t {})
                                .value();
        if (value == nullptr) {
            return std::nullopt;
        }
        return *value;
    };

    // effect 构造时立即执行一次，所以绑定的初值必须当场就生效（否则侧边栏第一帧
    // 会先闪一下配方的默认圆角）。
    REQUIRE(radius_now().has_value());
    REQUIRE(*radius_now() == Catch::Approx(base));
    REQUIRE(tree.animation_host().active_count() == 0);

    // 悬浮：目标已经变了，但当前值还停在基值上 —— 这就是"有动画"和"直接跳"的分界。
    hovered.set(true);
    REQUIRE(*radius_now() == Catch::Approx(base));
    REQUIRE(tree.animation_host().active_count() == 1);

    advance(tree, 0.02F);
    REQUIRE(*radius_now() > base);
    REQUIRE(*radius_now() < base + 2.0F);

    for (int frame = 0; frame < 240; ++frame) {
        advance(tree, 1.0F / 60.0F);
    }
    REQUIRE(*radius_now() == Catch::Approx(base + 2.0F).margin(0.01F));
    REQUIRE(tree.animation_host().active_count() == 0);

    // 离开：同样有一段过渡，而不是瞬时回位。
    hovered.set(false);
    REQUIRE(*radius_now() == Catch::Approx(base + 2.0F));
    for (int frame = 0; frame < 240; ++frame) {
        advance(tree, 1.0F / 60.0F);
    }
    REQUIRE(*radius_now() == Catch::Approx(base).margin(0.01F));
    REQUIRE(tree.animation_host().active_count() == 0);
}

TEST_CASE("a zero-duration transition jumps without a track", "[animation][host][widget]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);

    // 主题把 `motion.short_duration` 调成 0 时，微交互应该就地震到目标、不占动画轨道。
    // 侧边栏的时长直接取自该令牌，所以这条性质是它"可被主题降速/关闭"的前提。
    auto& hovered = ui.signal_value(false);
    auto button = ui.make<widget::Button>("item")
                      .behavior(widget::visual::container.radius, motion::tween(0.0F))
                      .build();

    const float base = button->resolved_style().container.radius;
    auto& radius = ui.computed([&hovered, base] { return hovered.get() ? base + 2.0F : base; });
    ui.bind(button, widget::visual::container.radius, radius);
    tree.set_root(button);

    hovered.set(true);

    const auto* value = button->visual_part(widget::visual::container_t {})
                            .property(widget::visual::radius_t {})
                            .value();
    REQUIRE(value != nullptr);
    REQUIRE(*value == Catch::Approx(base + 2.0F));
    REQUIRE(tree.animation_host().active_count() == 0);
}

TEST_CASE(
    "reduced motion forces new targets to jump without a track",
    "[animation][host][reduced-motion]"
) {
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
             *probe,
             probe->a,
             10.0F,
             animation::Behavior<float>(1.0F, animation::Easing::linear),
             scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe,
             probe->b,
             20.0F,
             animation::Behavior<float>(1.0F, animation::Easing::linear),
             scene::DirtyFlags::paint
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
             *probe,
             probe->a,
             10.0F,
             animation::Behavior<float>(0.2F, animation::Easing::linear),
             scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe,
             probe->b,
             20.0F,
             animation::Behavior<float>(0.2F, animation::Easing::linear),
             scene::DirtyFlags::paint
         )}
    );
    tree.animation_host().run(*probe, std::move(group));

    advance(tree, 0.1F);
    REQUIRE(probe->a.value() == Catch::Approx(5.0F));
    REQUIRE(probe->b.value() == Catch::Approx(0.0F)); // b 尚未触发

    advance(tree, 0.1F);
    REQUIRE(probe->a.value() == Catch::Approx(10.0F)); // a 完成
    REQUIRE(probe->b.value() == Catch::Approx(0.0F)); // b 下一帧才触发

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
             *probe,
             probe->a,
             10.0F,
             animation::Behavior<float>(1.0F, animation::Easing::linear),
             scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe,
             probe->b,
             20.0F,
             animation::Behavior<float>(1.0F, animation::Easing::linear),
             scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe,
             probe->c,
             30.0F,
             animation::Behavior<float>(1.0F, animation::Easing::linear),
             scene::DirtyFlags::paint
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
             *probe,
             probe->a,
             10.0F,
             animation::Behavior<float>(1.0F, animation::Easing::linear),
             scene::DirtyFlags::paint
         ),
         animation::Group::clip(
             *probe,
             probe->c,
             30.0F,
             animation::Behavior<float>(1.0F, animation::Easing::linear),
             scene::DirtyFlags::paint
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

TEST_CASE(
    "spring consumes a hitch consistently across damping regimes",
    "[animation][spring][hitch]"
) {
    for (const float damping: {0.0F, 12.0F, 39.999F, 40.0F, 40.001F, 100.0F}) {
        CAPTURE(damping);
        motion::Spring<double> whole;
        motion::Spring<double> split;
        const motion::SpringSpec spec(400.0F, damping);
        whole.start(1.0, 0.0, spec);
        split.start(1.0, 0.0, spec);
        whole.tick(0.125F);
        for (int frame = 0; frame < 16; ++frame) {
            split.tick(0.0078125F);
        }
        REQUIRE(std::isfinite(whole.value()));
        REQUIRE(whole.value() == Catch::Approx(split.value()).margin(1e-10));
        REQUIRE(std::abs(whole.value()) <= 1.0);
        // Compare again after retargeting: this also checks the retained velocity.
        whole.set_target(0.5);
        split.set_target(0.5);
        whole.tick(0.0625F);
        split.tick(0.0625F);
        REQUIRE(whole.value() == Catch::Approx(split.value()).margin(1e-10));
        if (damping > 0.0F) {
            for (int frame = 0; frame < 600; ++frame) {
                whole.tick(1.0F / 60.0F);
            }
            REQUIRE(whole.is_finished());
            REQUIRE(whole.value() == 0.5);
        }
    }
}

TEST_CASE("spring matches critical and undamped analytical motion", "[animation][spring][hitch]") {
    motion::Spring<double> critical;
    critical.start(1.0, 0.0, motion::SpringSpec(400.0F, 40.0F));
    REQUIRE(critical.tick(0.125F) == Catch::Approx(3.5 * std::exp(-2.5)).margin(1e-12));
    motion::Spring<double> undamped;
    undamped.start(1.0, 0.0, motion::SpringSpec(400.0F, 0.0F));
    REQUIRE(undamped.tick(0.125F) == Catch::Approx(std::cos(2.5)).margin(1e-12));
}

TEST_CASE(
    "spring tolerates stiff parameters and rejects nonfinite time",
    "[animation][spring][hitch]"
) {
    for (const auto spec:
         {motion::SpringSpec(1e8F, 200.0F, 0.001F),
          motion::SpringSpec(1.0F, 1e20F, 0.001F),
          motion::SpringSpec(300.0F, 12.0F)})
    {
        motion::Spring<double> spring;
        spring.start(1.0, 0.0, spec);
        for (const float dt:
             {-1.0F,
              0.0F,
              std::numeric_limits<float>::quiet_NaN(),
              std::numeric_limits<float>::infinity()})
        {
            REQUIRE(spring.tick(dt) == 1.0);
        }
        for (const float dt: {0.1F, 0.5F, 2.0F, std::numeric_limits<float>::max()}) {
            REQUIRE(std::isfinite(spring.tick(dt)));
            REQUIRE(std::abs(spring.value()) <= 1.0);
        }
        REQUIRE(spring.is_finished());
        REQUIRE(spring.value() == 0.0);
    }
}

TEST_CASE("a stiff float spring retains finite internal velocity", "[animation][spring][hitch]") {
    motion::Spring<float> spring;
    spring.start(
        1.0F,
        0.0F,
        motion::SpringSpec(
            std::numeric_limits<float>::max(),
            0.0F,
            std::numeric_limits<float>::denorm_min()
        )
    );
    for (int step = 0; step < 8; ++step) {
        const auto value = spring.tick(1e-40F);
        REQUIRE(std::isfinite(value));
        REQUIRE(std::abs(value) <= 1.00001F);
    }
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

TEST_CASE(
    "animated property supports spring mode with overshoot",
    "[animation][property][spring]"
) {
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

TEST_CASE(
    "keyframes interpolate across time and finish at the last frame",
    "[animation][keyframes]"
) {
    animation::Keyframes<float> keyframes;
    keyframes.start(
        {{.time = 0.0F, .value = 0.0F},
         {.time = 0.5F, .value = 10.0F},
         {.time = 1.0F, .value = 0.0F}}
    );
    REQUIRE_FALSE(keyframes.is_finished());
    REQUIRE(keyframes.value() == Catch::Approx(0.0F));

    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(5.0F)); // 0 → 10 中点
    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(10.0F)); // 到 0.5s
    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(5.0F)); // 10 → 0 中点
    REQUIRE(keyframes.tick(0.25F) == Catch::Approx(0.0F)); // 到 1.0s，结束
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

TEST_CASE(
    "animated property plays keyframes and clears back to target",
    "[animation][property][keyframes]"
) {
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
    property.set_keyframes({{.time = 0.0F, .value = 0.0F}, {.time = 1.0F, .value = 10.0F}});
    REQUIRE_FALSE(property.behavior().has_value()); // behavior 被清除
    REQUIRE(property.keyframes().has_value());

    property.set_target(20.0F); // set_target 清除 keyframes，无 behavior 直跳
    REQUIRE_FALSE(property.keyframes().has_value());
    REQUIRE(property.value() == Catch::Approx(20.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE("motion::tween builds a behavior spec", "[animation][motion]") {
    const auto spec = motion::tween(0.24F).easing(motion::ease_out);
    const auto behavior = spec.behavior<float>();
    REQUIRE(behavior.duration() == Catch::Approx(0.24F));
    REQUIRE(behavior.easing() == animation::Easing::ease_out);
    REQUIRE(behavior.enabled());

    REQUIRE_THROWS_AS(motion::tween(-0.1F), std::invalid_argument);
}

TEST_CASE("motion::spring builds a spring spec fluently", "[animation][motion]") {
    const auto spec = motion::spring().stiffness(200.0F).damping(12.0F).mass(2.0F);
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
                          motion::tween(0.4F).easing(motion::ease_standard)
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
                          motion::spring().stiffness(200.0F).damping(10.0F)
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

TEST_CASE(
    "a real pointer hover drives a bound visual property through the interaction callback",
    "[animation][host][widget][input]"
) {
    // 上一条测试直接 `set()` 信号，绕过了"输入事件 → 交互回调 → 信号"这一段。而 showcase
    // 侧边栏出过的正是这一段：交互回调写的是它在**自己内部**新建的信号，动画读的是外层
    // 另一个 —— 于是动画永远不触发，而原语层的测试全绿（它们只测插值，不测接线）。
    //
    // 这条把整条链一起走：真实悬浮事件 → Button::on_hover_changed → 信号 → computed →
    // 绑定的视觉属性 → 解析出的样式值。中间的 `REQUIRE(hovered.get())` 是关键的锚点：
    // 回调只要写到了别的信号上，它立刻失败。
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);

    auto& hovered = ui.signal_value(false);
    auto button = ui.make<widget::Button>("item")
                      .behavior(widget::visual::container.radius, motion::tween(0.12F))
                      .on_hover_changed([&hovered](const bool value) { hovered.set(value); })
                      .build();

    const float base = button->resolved_style().container.radius;
    REQUIRE(base > 0.0F);

    auto& radius = ui.computed([&hovered, base] { return hovered.get() ? base + 2.0F : base; });
    ui.bind(button, widget::visual::container.radius, radius);
    tree.set_root(button);
    (void)tree.layout_root(foundation::NanSize(160.0F, 40.0F));

    const auto radius_now = [&button]() -> std::optional<float> {
        const auto* value = button->visual_part(widget::visual::container_t {})
                                .property(widget::visual::radius_t {})
                                .value();
        if (value == nullptr) {
            return std::nullopt;
        }
        return *value;
    };

    REQUIRE(radius_now().has_value());
    REQUIRE(*radius_now() == Catch::Approx(base));
    REQUIRE_FALSE(hovered.get());

    // 指针移到按钮上：整条链应当自己跑起来，不需要测试替它 set 任何信号。
    tree.dispatch_mouse_move(
        scene::MouseMoveEvent {button->global_bounds().get_center(), foundation::NanPoint::zero()}
    );
    REQUIRE(hovered.get());
    // 目标变了、当前值还在基值 —— 说明过渡真的在走，而不是直接跳。
    REQUIRE(*radius_now() == Catch::Approx(base));

    tree.advance_animations(1.0F);
    REQUIRE(*radius_now() == Catch::Approx(base + 2.0F).margin(0.01F));

    // 移出同样要能回落，否则会留下一个"卡在悬浮态"的条目。
    tree.dispatch_mouse_move(
        scene::MouseMoveEvent {foundation::NanPoint(-10.0F, -10.0F), foundation::NanPoint::zero()}
    );
    REQUIRE_FALSE(hovered.get());
    tree.advance_animations(1.0F);
    REQUIRE(*radius_now() == Catch::Approx(base).margin(0.01F));
}
