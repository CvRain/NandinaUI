// Public group authoring: declaration, input, presentation, and weak-owner playback.

#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/controls.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace nandina;
using namespace nandina::widget::authoring;

namespace
{
    struct AuthoringContext {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
        scene::NanSceneTree tree;

        AuthoringContext() {
            tree.set_theme_manager(themes);
        }
    };

    class LayoutCounter final: public scene::NanControl {
    public:
        int measures = 0;
        int layouts = 0;

    protected:
        auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override {
            ++measures;
            return NanControl::on_measure(constraints);
        }

        void on_layout() override {
            ++layouts;
            NanControl::on_layout();
        }
    };

    auto presentation(widget::Button& button) -> scene::NodePresentation& {
        return static_cast<scene::NanNode2D&>(button).visual_part(scene::visual::node);
    }

    void advance(scene::NanSceneTree& tree, float dt) {
        auto phase = tree.enter_phase(scene::FramePhase::animation);
        tree.advance_animations(dt);
    }

    auto linear(float duration) -> motion::TweenSpec {
        return motion::tween(duration).easing(motion::ease_linear);
    }

    template<typename Path, typename Value>
    concept GroupTarget =
        requires(Path path, Value value) { to(path, value, motion::tween(0.2F)); };
} // namespace

static_assert(GroupTarget<decltype(widget::visual::opacity), float>);
static_assert(GroupTarget<decltype(widget::visual::translate), foundation::NanPoint>);
static_assert(GroupTarget<decltype(widget::visual::scale), foundation::NanPoint>);
static_assert(!GroupTarget<decltype(widget::visual::label.font_size), float>);
static_assert(!GroupTarget<decltype(widget::visual::container.radius), float>);

TEST_CASE("group declaration before mount has no presentation effects", "[authoring][group]") {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Declare");
    auto button = builder.build();
    const auto handle = builder.group(parallel(
        to(widget::visual::opacity, 0.0F, linear(1.0F)),
        to(widget::visual::translate, foundation::NanPoint(40.0F, 0.0F), linear(1.0F))
    ));
    static_assert(std::copy_constructible<decltype(handle)>);
    static_assert(std::same_as<decltype(handle.play()), bool>);

    REQUIRE(button->local_opacity() == Catch::Approx(1.0F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(0.0F));
    REQUIRE(
        *presentation(*button).property(scene::visual::opacity_t {}).target() == Catch::Approx(1.0F)
    );
    REQUIRE_FALSE(handle.play());
    REQUIRE(ctx.tree.animation_host().active_count() == 0);

    ctx.tree.set_root(button);
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
    REQUIRE(button->local_opacity() == Catch::Approx(1.0F));
    REQUIRE(handle.play());
    advance(ctx.tree, 0.5F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
}

TEST_CASE(
    "button pointer callback plays L2 group without layout and updates geometry",
    "[authoring][group][input][layout][semantics]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Animate");
    auto button = builder.build();
    button->set_transform_origin(scene::TransformOrigin::top_left);
    const auto handle = builder.group(parallel(
        to(widget::visual::opacity, 0.0F, linear(1.0F)),
        to(widget::visual::translate, foundation::NanPoint(40.0F, 20.0F), linear(1.0F)),
        to(widget::visual::scale, foundation::NanPoint(2.0F, 2.0F), linear(1.0F))
    ));
    int clicks = 0;
    bool accepted = false;
    builder.on_click([handle, &clicks, &accepted] {
        ++clicks;
        accepted = handle.play();
    });
    auto root = std::make_shared<LayoutCounter>();
    root->add_child(button);
    ctx.tree.set_root(root);
    const foundation::NanSize viewport(100.0F, 40.0F);
    REQUIRE(ctx.tree.layout_root(viewport) == 1);
    REQUIRE(ctx.tree.update_semantics());
    REQUIRE(ctx.tree.hit_test(foundation::NanPoint(5.0F, 5.0F)) == button.get());
    ctx.tree.dispatch_mouse_move(
        scene::MouseMoveEvent {foundation::NanPoint(5.0F, 5.0F), foundation::NanPoint::zero()}
    );
    REQUIRE(button->hovered());
    for (const auto action:
         {scene::MouseButtonEvent::Action::press, scene::MouseButtonEvent::Action::release})
    {
        ctx.tree.dispatch_mouse_button(
            scene::MouseButtonEvent {
                scene::MouseButtonEvent::Button::left,
                action,
                foundation::NanPoint(5.0F, 5.0F)
            }
        );
        if (action == scene::MouseButtonEvent::Action::press) {
            REQUIRE(button->pressed());
        }
    }
    REQUIRE_FALSE(button->pressed());
    REQUIRE(clicks == 1);
    REQUIRE(accepted);
    // Settle input-state invalidation before measuring animation-only layout work.
    (void)ctx.tree.layout_root(viewport);
    const int measures = root->measures;
    const int layouts = root->layouts;
    button->clear_dirty(scene::layout_dirty_flags);
    root->clear_dirty(scene::layout_dirty_flags);

    advance(ctx.tree, 0.5F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(20.0F));
    REQUIRE(presentation(*button).translate().get_y() == Catch::Approx(10.0F));
    REQUIRE(presentation(*button).scale().get_x() == Catch::Approx(1.5F));
    REQUIRE_FALSE(button->is_dirty(scene::layout_dirty_flags));
    REQUIRE_FALSE(root->is_dirty(scene::layout_dirty_flags));
    REQUIRE(ctx.tree.layout_root(viewport) == 0);
    REQUIRE(root->measures == measures);
    REQUIRE(root->layouts == layouts);
    const auto bounds = button->global_bounds();
    REQUIRE(bounds.get_x() == Catch::Approx(20.0F));
    REQUIRE(bounds.get_y() == Catch::Approx(10.0F));
    REQUIRE(bounds.get_width() == Catch::Approx(150.0F));
    REQUIRE(bounds.get_height() == Catch::Approx(60.0F));
    REQUIRE(ctx.tree.hit_test(foundation::NanPoint(165.0F, 65.0F)) == button.get());
    REQUIRE(ctx.tree.hit_test(foundation::NanPoint(175.0F, 65.0F)) == nullptr);
    REQUIRE(ctx.tree.update_semantics());
    const auto* semantic = ctx.tree.semantics_tree().find(button->semantics_id());
    REQUIRE(semantic != nullptr);
    REQUIRE(semantic->bounds == bounds);

    // A real later layout must not overwrite the presentation transform.
    button->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 40.0F));
    REQUIRE(button->global_bounds() == bounds);
    REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
}

TEST_CASE("public parallel preserves independent tween durations", "[authoring][group]") {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Parallel");
    auto button = builder.build();
    ctx.tree.set_root(button);
    const auto handle = builder.group(parallel(
        to(widget::visual::opacity, 0.0F, linear(0.2F)),
        to(widget::visual::translate, foundation::NanPoint(40.0F, 0.0F), linear(0.4F))
    ));
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
    REQUIRE(button->local_opacity() == Catch::Approx(1.0F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(0.0F));
    REQUIRE(handle.play());
    advance(ctx.tree, 0.1F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(10.0F));
    advance(ctx.tree, 0.1F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.0F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(20.0F));
    advance(ctx.tree, 0.2F);
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(40.0F));
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
}

TEST_CASE(
    "public translate group invalidates a warm transform cache",
    "[authoring][group][transform][hit-test][semantics]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Translate");
    auto button = builder.build();
    ctx.tree.set_root(button);
    REQUIRE(ctx.tree.layout_root(foundation::NanSize(100.0F, 40.0F)) == 1);
    const auto handle = builder.group(
        parallel(to(widget::visual::translate, foundation::NanPoint(80.0F, 20.0F), linear(0.4F)))
    );

    // Warm every geometry consumer before playback; no scale change may refresh it.
    REQUIRE(button->global_transform().position().get_x() == Catch::Approx(0.0F));
    REQUIRE(button->global_bounds().get_x() == Catch::Approx(0.0F));
    REQUIRE(ctx.tree.hit_test(foundation::NanPoint(5.0F, 5.0F)) == button.get());
    REQUIRE(ctx.tree.update_semantics());
    const auto* initial = ctx.tree.semantics_tree().find(button->semantics_id());
    REQUIRE(initial != nullptr);
    REQUIRE(initial->bounds.get_x() == Catch::Approx(0.0F));
    button->clear_dirty(
        scene::layout_dirty_flags | scene::DirtyFlags::paint | scene::DirtyFlags::semantics
        | scene::DirtyFlags::transform
    );

    REQUIRE(handle.play());
    advance(ctx.tree, 0.2F);
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(40.0F));
    REQUIRE(presentation(*button).translate().get_y() == Catch::Approx(10.0F));
    REQUIRE(button->global_transform().position().get_x() == Catch::Approx(40.0F));
    REQUIRE(button->global_transform().position().get_y() == Catch::Approx(10.0F));
    const auto bounds = button->global_bounds();
    REQUIRE(bounds.get_x() == Catch::Approx(40.0F));
    REQUIRE(bounds.get_y() == Catch::Approx(10.0F));
    REQUIRE(bounds.get_width() == Catch::Approx(100.0F));
    REQUIRE(bounds.get_height() == Catch::Approx(40.0F));
    REQUIRE(ctx.tree.hit_test(foundation::NanPoint(135.0F, 45.0F)) == button.get());
    REQUIRE(ctx.tree.hit_test(foundation::NanPoint(5.0F, 5.0F)) == nullptr);
    REQUIRE(ctx.tree.hit_test(foundation::NanPoint(145.0F, 45.0F)) == nullptr);
    REQUIRE(button->is_dirty(scene::DirtyFlags::transform));
    REQUIRE(button->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE_FALSE(button->is_dirty(scene::layout_dirty_flags));
    REQUIRE(ctx.tree.layout_root(foundation::NanSize(100.0F, 40.0F)) == 0);
    REQUIRE(ctx.tree.semantics_dirty());
    REQUIRE(ctx.tree.update_semantics());
    const auto* midpoint = ctx.tree.semantics_tree().find(button->semantics_id());
    REQUIRE(midpoint != nullptr);
    REQUIRE(midpoint->bounds == bounds);
}

TEST_CASE(
    "public sequential keeps the next path delayed until the next frame",
    "[authoring][group]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Sequential");
    auto button = builder.build();
    ctx.tree.set_root(button);
    const auto handle = builder.group(sequential(
        to(widget::visual::opacity, 0.0F, linear(0.2F)),
        to(widget::visual::translate, foundation::NanPoint(20.0F, 0.0F), linear(0.2F))
    ));
    REQUIRE(handle.play());
    advance(ctx.tree, 0.2F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.0F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(0.0F));
    advance(ctx.tree, 0.1F);
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(10.0F));
    advance(ctx.tree, 0.1F);
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(20.0F));
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
}

TEST_CASE("public stagger uses its declared interval", "[authoring][group]") {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Stagger");
    auto button = builder.build();
    ctx.tree.set_root(button);
    const auto handle = builder.group(stagger(
        0.2F,
        to(widget::visual::opacity, 0.0F, linear(0.4F)),
        to(widget::visual::translate, foundation::NanPoint(40.0F, 0.0F), linear(0.4F)),
        to(widget::visual::scale, foundation::NanPoint(2.0F, 2.0F), linear(0.4F))
    ));
    REQUIRE(handle.play());
    advance(ctx.tree, 0.1F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.75F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(0.0F));
    advance(ctx.tree, 0.1F);
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(10.0F));
    REQUIRE(presentation(*button).scale().get_x() == Catch::Approx(1.0F));
    advance(ctx.tree, 0.2F);
    REQUIRE(presentation(*button).scale().get_x() == Catch::Approx(1.5F));
}

TEST_CASE(
    "a node callback capturing its group handle does not retain the node",
    "[authoring][group][lifetime]"
) {
    AuthoringContext ctx;
    std::weak_ptr<widget::Button> weak;
    const auto handle = [&] {
        auto builder = ctx.ui.make<widget::Button>("Weak");
        auto button = builder.build();
        weak = button;
        auto animation = builder.group(parallel(to(widget::visual::opacity, 0.0F, linear(1.0F))));
        builder.on_click([animation] { (void)animation.play(); });
        ctx.tree.set_root(button);
        return animation;
    }();
    REQUIRE_FALSE(weak.expired());
    REQUIRE(handle.play());
    ctx.tree.set_root({});
    REQUIRE(weak.expired());
    REQUIRE_FALSE(handle.play());
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
}

TEST_CASE(
    "a detached handle replays through the node current tree after reparent",
    "[authoring][group][lifetime]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Move");
    auto button = builder.build();
    auto first_root = std::make_shared<scene::NanControl>();
    auto second_root = std::make_shared<scene::NanControl>();
    first_root->add_child(button);
    ctx.tree.set_root(first_root);
    scene::NanSceneTree second;
    second.set_root(second_root);
    const auto handle = builder.group(parallel(to(widget::visual::opacity, 0.0F, linear(1.0F))));
    REQUIRE(handle.play());
    advance(ctx.tree, 0.25F);
    (void)first_root->remove_child(*button);
    REQUIRE_FALSE(handle.play());
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
    button->set_local_opacity(1.0F);
    first_root->add_child(button);
    second_root->reparent(button);
    REQUIRE(button->get_tree() == &second);
    REQUIRE(handle.play());
    REQUIRE(second.animation_host().active_count() == 1);
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
    advance(ctx.tree, 0.5F);
    REQUIRE(button->local_opacity() == Catch::Approx(1.0F));
    advance(second, 0.5F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
}

TEST_CASE(
    "invalid public declarations preserve an existing ordinary track",
    "[authoring][group][validation]"
) {
    AuthoringContext ctx;
    auto builder =
        ctx.ui.make<widget::Button>("Validate").behavior(widget::visual::opacity, linear(1.0F));
    auto button = builder.build();
    ctx.tree.set_root(button);
    button->set_local_opacity(0.0F);
    advance(ctx.tree, 0.25F);
    const auto opacity = to(widget::visual::opacity, 0.5F, linear(0.2F));
    REQUIRE_THROWS_AS(builder.group(parallel(opacity, opacity)), std::invalid_argument);
    REQUIRE_THROWS_AS(builder.group(sequential(opacity, opacity)), std::invalid_argument);
    REQUIRE_THROWS_AS(builder.group(stagger(0.1F, opacity, opacity)), std::invalid_argument);
    for (const float interval:
         {-0.1F, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
    {
        REQUIRE_THROWS_AS(builder.group(stagger(interval, opacity)), std::invalid_argument);
    }
    REQUIRE_THROWS_AS(
        builder.group(parallel(
            to(widget::visual::opacity, std::numeric_limits<float>::quiet_NaN(), linear(1.0F))
        )),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        builder.group(parallel(
            to(widget::visual::scale,
               foundation::NanPoint(std::numeric_limits<float>::infinity(), 1.0F),
               linear(1.0F))
        )),
        std::invalid_argument
    );
    REQUIRE(ctx.tree.animation_host().active_count() == 1);
    REQUIRE(button->local_opacity() == Catch::Approx(0.75F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(0.0F));
    advance(ctx.tree, 0.25F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
}

TEST_CASE(
    "replaying a public delayed handle creates fresh clip state",
    "[authoring][group][replay]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Replay");
    auto button = builder.build();
    bool staggered = false;
    SECTION("sequential") {}
    SECTION("stagger") {
        staggered = true;
    }
    const auto opacity = to(widget::visual::opacity, 0.0F, linear(0.2F));
    const auto translate =
        to(widget::visual::translate, foundation::NanPoint(20.0F, 0.0F), linear(0.2F));
    const auto handle = builder.group(
        staggered ? stagger(0.2F, opacity, translate) : sequential(opacity, translate)
    );
    for (int play = 0; play < 2; ++play) {
        ctx.tree.set_root({});
        button->set_local_opacity(1.0F);
        button->set_presentation_translate(foundation::NanPoint::zero());
        ctx.tree.set_root(button);
        REQUIRE(handle.play());
        advance(ctx.tree, 0.1F);
        REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
        REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(0.0F));
        advance(ctx.tree, 0.1F);
        advance(ctx.tree, 0.2F);
        REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(20.0F));
        REQUIRE(ctx.tree.animation_host().active_count() == 0);
    }
}

TEST_CASE(
    "disjoint public groups on one node remain concurrent",
    "[authoring][group][concurrent]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Concurrent");
    auto button = builder.build();
    ctx.tree.set_root(button);
    const auto fade = builder.group(parallel(to(widget::visual::opacity, 0.0F, linear(0.4F))));
    const auto move = builder.group(
        parallel(to(widget::visual::translate, foundation::NanPoint(40.0F, 0.0F), linear(0.8F)))
    );
    REQUIRE(fade.play());
    REQUIRE(move.play());
    REQUIRE(ctx.tree.animation_host().active_count() == 2);
    advance(ctx.tree, 0.2F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.5F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(10.0F));
    REQUIRE(ctx.tree.animation_host().active_count() == 2);
    advance(ctx.tree, 0.2F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.0F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(20.0F));
    REQUIRE(ctx.tree.animation_host().active_count() == 1);
    advance(ctx.tree, 0.4F);
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(40.0F));
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
}

TEST_CASE(
    "active public replay completes the delayed sibling without orphan tracks",
    "[authoring][group][replay]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Active replay");
    auto button = builder.build();
    ctx.tree.set_root(button);
    bool staggered = false;
    SECTION("sequential") {}
    SECTION("stagger") {
        staggered = true;
    }
    const auto opacity = to(widget::visual::opacity, 0.0F, linear(0.4F));
    const auto translate =
        to(widget::visual::translate, foundation::NanPoint(40.0F, 0.0F), linear(0.4F));
    const auto handle = builder.group(
        staggered ? stagger(0.2F, opacity, translate) : sequential(opacity, translate)
    );
    REQUIRE(handle.play());
    advance(ctx.tree, 0.1F);
    REQUIRE(button->local_opacity() == Catch::Approx(0.75F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(0.0F));
    REQUIRE(ctx.tree.animation_host().active_count() == 1);
    REQUIRE(handle.play());
    REQUIRE(button->local_opacity() == Catch::Approx(0.0F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(40.0F));
    // A fresh stagger still schedules its second, already-reached path at 0.2s.
    REQUIRE(ctx.tree.animation_host().active_count() == (staggered ? 1 : 0));
    advance(ctx.tree, 0.1F);
    REQUIRE(ctx.tree.animation_host().active_count() == (staggered ? 1 : 0));
    advance(ctx.tree, 0.1F);
    REQUIRE(ctx.tree.animation_host().active_count() == 0);
    REQUIRE(button->local_opacity() == Catch::Approx(0.0F));
    REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(40.0F));
}

TEST_CASE(
    "public playback accepts reduced motion zero duration and no-change targets",
    "[authoring][group][reduced-motion]"
) {
    AuthoringContext ctx;
    auto builder = ctx.ui.make<widget::Button>("Immediate");
    auto button = builder.build();
    ctx.tree.set_root(button);
    SECTION("reduced motion completes even delayed targets during play") {
        ctx.themes.set_motion_preference(theme::MotionPreference::reduced);
        const auto handle = builder.group(sequential(
            to(widget::visual::opacity, 0.25F, linear(1.0F)),
            to(widget::visual::translate, foundation::NanPoint(40.0F, 0.0F), linear(1.0F))
        ));
        REQUIRE(handle.play());
        REQUIRE(button->local_opacity() == Catch::Approx(0.25F));
        REQUIRE(presentation(*button).translate().get_x() == Catch::Approx(40.0F));
        REQUIRE(ctx.tree.animation_host().active_count() == 0);
    }
    SECTION("zero duration finishes during play and remains accepted at the target") {
        const auto handle =
            builder.group(parallel(to(widget::visual::opacity, 0.25F, linear(0.0F))));
        REQUIRE(handle.play());
        REQUIRE(button->local_opacity() == Catch::Approx(0.25F));
        REQUIRE(ctx.tree.animation_host().active_count() == 0);
        REQUIRE(handle.play());
    }
    SECTION("an already reached nonzero-duration target is accepted") {
        const auto handle =
            builder.group(parallel(to(widget::visual::opacity, 1.0F, linear(1.0F))));
        REQUIRE(handle.play());
        advance(ctx.tree, 0.1F);
        REQUIRE(button->local_opacity() == Catch::Approx(1.0F));
        REQUIRE(ctx.tree.animation_host().active_count() == 0);
    }
}
