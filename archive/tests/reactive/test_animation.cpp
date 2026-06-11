#include <gtest/gtest.h>
#include <memory>

import nandina.reactive.animation;

namespace {

TEST(AnimationTest, EasingLinear_ReturnsInput) {
    EXPECT_FLOAT_EQ(nandina::reactive::apply_easing(0.0f, nandina::reactive::Easing::Linear), 0.0f);
    EXPECT_FLOAT_EQ(nandina::reactive::apply_easing(0.5f, nandina::reactive::Easing::Linear), 0.5f);
    EXPECT_FLOAT_EQ(nandina::reactive::apply_easing(1.0f, nandina::reactive::Easing::Linear), 1.0f);
}

TEST(AnimationTest, EasingClampsInput) {
    EXPECT_FLOAT_EQ(nandina::reactive::apply_easing(-0.5f, nandina::reactive::Easing::Linear), 0.0f);
    EXPECT_FLOAT_EQ(nandina::reactive::apply_easing(1.5f, nandina::reactive::Easing::Linear), 1.0f);
}

TEST(AnimationTest, EaseIn_StartsSlow) {
    const float t05 = nandina::reactive::apply_easing(0.5f, nandina::reactive::Easing::EaseIn);
    const float t025 = nandina::reactive::apply_easing(0.25f, nandina::reactive::Easing::EaseIn);
    EXPECT_LT(t025, 0.25f);   // early values are compressed
    EXPECT_LT(t05, 0.5f);     // cubic starts slow, below linear
    EXPECT_GT(t05, 0.0f);
}

TEST(AnimationTest, EaseOut_StartsFast) {
    const float t05 = nandina::reactive::apply_easing(0.5f, nandina::reactive::Easing::EaseOut);
    EXPECT_GT(t05, 0.5f);     // cubic decelerates, above linear
    EXPECT_LT(t05, 1.0f);
}

TEST(AnimationTest, SineInOut_Smooth) {
    EXPECT_FLOAT_EQ(nandina::reactive::apply_easing(0.0f, nandina::reactive::Easing::SineInOut), 0.0f);
    EXPECT_FLOAT_EQ(nandina::reactive::apply_easing(1.0f, nandina::reactive::Easing::SineInOut), 1.0f);
    EXPECT_NEAR(nandina::reactive::apply_easing(0.5f, nandina::reactive::Easing::SineInOut), 0.5f, 0.01f);
}

TEST(AnimationTest, NanAnimation_CreateAndTick) {
    auto anim = nandina::reactive::NanAnimation::create(1.0f);
    float progress = -1.0f;
    bool completed = false;

    anim->on_update([&](float t) { progress = t; });
    anim->on_complete([&] { completed = true; });

    EXPECT_FALSE(anim->is_running());
    EXPECT_FLOAT_EQ(anim->progress(), 0.0f);

    anim->start();
    EXPECT_TRUE(anim->is_running());
    EXPECT_FLOAT_EQ(progress, 0.0f);

    anim->tick(0.5f);
    EXPECT_NEAR(progress, 0.5f, 0.01f);
    EXPECT_FALSE(completed);

    anim->tick(0.5f);
    EXPECT_FLOAT_EQ(progress, 1.0f);
    EXPECT_TRUE(completed);
    EXPECT_FALSE(anim->is_running());
}

TEST(AnimationTest, NanAnimation_Loops) {
    auto anim = nandina::reactive::NanAnimation::create(1.0f);
    anim->set_loop(true);

    int update_count = 0;
    anim->on_update([&](float) { ++update_count; });
    anim->start();

    anim->tick(0.3f);
    EXPECT_EQ(update_count, 2); // start at 0 + tick at ~0.3
    EXPECT_TRUE(anim->is_running());

    anim->tick(0.7f);
    // Should have looped back: progress resets to 0 then continues
    EXPECT_TRUE(anim->is_running());
    EXPECT_GT(update_count, 2);
}

TEST(AnimationTest, NanAnimation_PauseResume) {
    auto anim = nandina::reactive::NanAnimation::create(1.0f);
    float progress = 0.0f;
    anim->on_update([&](float t) { progress = t; });
    anim->start();

    anim->tick(0.4f);
    const float after_tick = progress;

    anim->pause();
    anim->tick(0.5f);
    EXPECT_FLOAT_EQ(progress, after_tick);

    anim->resume();
    anim->tick(0.1f);
    EXPECT_GT(progress, after_tick);
}

TEST(AnimationTest, NanAnimation_StopResets) {
    auto anim = nandina::reactive::NanAnimation::create(1.0f);
    anim->start();
    anim->tick(0.5f);
    anim->stop();
    EXPECT_FALSE(anim->is_running());
    EXPECT_FLOAT_EQ(anim->progress(), 0.0f);
}

TEST(AnimationTest, AnimationManager_AddAndUpdate) {
    nandina::reactive::AnimationManager mgr;
    float progress = 0.0f;

    auto anim = nandina::reactive::NanAnimation::create(1.0f);
    anim->on_update([&](float t) { progress = t; });
    anim->start();

    mgr.add(std::move(anim));
    EXPECT_EQ(mgr.count(), 1u);

    mgr.update(0.6f);
    EXPECT_NEAR(progress, 0.6f, 0.01f);
    EXPECT_EQ(mgr.count(), 1u);

    mgr.update(0.4f);
    EXPECT_FLOAT_EQ(progress, 1.0f);

    // Completed animation auto-removed
    EXPECT_EQ(mgr.count(), 0u);
}

TEST(AnimationTest, AnimationManager_Remove) {
    nandina::reactive::AnimationManager mgr;

    auto anim = nandina::reactive::NanAnimation::create(1.0f);
    auto* raw = anim.get();
    anim->start();

    mgr.add(std::move(anim));
    EXPECT_EQ(mgr.count(), 1u);

    mgr.remove(raw);
    EXPECT_EQ(mgr.count(), 0u);
}

TEST(AnimationTest, AnimationManager_Clear) {
    nandina::reactive::AnimationManager mgr;

    for (int i = 0; i < 3; ++i) {
        auto anim = nandina::reactive::NanAnimation::create(1.0f);
        anim->start();
        mgr.add(std::move(anim));
    }

    EXPECT_EQ(mgr.count(), 3u);
    mgr.clear();
    EXPECT_EQ(mgr.count(), 0u);
}

} // namespace
