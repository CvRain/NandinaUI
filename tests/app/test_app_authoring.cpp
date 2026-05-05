#include <gtest/gtest.h>

import nandina.app.authoring;
import nandina.runtime.nan_widget;

class TestWidget final : public nandina::runtime::NanWidget {
public:
    static auto create() -> std::unique_ptr<TestWidget> {
        return std::make_unique<TestWidget>();
    }

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return {24.0f, 12.0f};
    }
};

TEST(AppAuthoringTest, MountBindsAndResetsRootRef) {
    nandina::app::Ref<TestWidget> widget_ref;

    auto root = nandina::app::adopt(TestWidget::create()).bind(widget_ref);
    auto mounted = nandina::app::mount(std::move(root));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(widget_ref);
    EXPECT_EQ(widget_ref.get(), mounted->children().front().get());

    mounted.reset();
    EXPECT_FALSE(widget_ref);
}

TEST(AppAuthoringTest, MountPreservesNestedRefsAcrossRowComposition) {
    nandina::app::Ref<TestWidget> left_ref;
    nandina::app::Ref<TestWidget> right_ref;

    auto root = nandina::app::row(nandina::app::children(
        nandina::app::adopt(TestWidget::create()).bind(left_ref),
        nandina::app::adopt(TestWidget::create()).bind(right_ref)));

    auto mounted = nandina::app::mount(std::move(root));

    ASSERT_NE(mounted, nullptr);
    ASSERT_EQ(mounted->child_count(), 1u);

    auto* row = mounted->children().front().get();
    ASSERT_NE(row, nullptr);
    ASSERT_EQ(row->child_count(), 2u);

    EXPECT_EQ(left_ref.get(), row->children()[0].get());
    EXPECT_EQ(right_ref.get(), row->children()[1].get());

    mounted.reset();
    EXPECT_FALSE(left_ref);
    EXPECT_FALSE(right_ref);
}

TEST(AppAuthoringTest, MountedNodeComponentPropagatesBoundsToRootWidget) {
    auto mounted = nandina::app::mount(nandina::app::adopt(TestWidget::create()));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(10.0f, 20.0f, 200.0f, 80.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    const auto bounds = mounted->children().front()->bounds();

    EXPECT_FLOAT_EQ(bounds.x(), 10.0f);
    EXPECT_FLOAT_EQ(bounds.y(), 20.0f);
    EXPECT_FLOAT_EQ(bounds.width(), 200.0f);
    EXPECT_FLOAT_EQ(bounds.height(), 80.0f);
}

TEST(AppAuthoringTest, PaddingWrapsSingleChildAndPreservesRefBinding) {
    nandina::app::Ref<TestWidget> widget_ref;

    auto mounted = nandina::app::mount(
        nandina::app::padding(
            nandina::app::adopt(TestWidget::create()).bind(widget_ref)));

    ASSERT_NE(mounted, nullptr);
    ASSERT_EQ(mounted->child_count(), 1u);

    auto* padding = mounted->children().front().get();
    ASSERT_NE(padding, nullptr);
    ASSERT_EQ(padding->child_count(), 1u);
    EXPECT_EQ(widget_ref.get(), padding->children().front().get());

    mounted.reset();
    EXPECT_FALSE(widget_ref);
}

TEST(AppAuthoringTest, CenterAndSizedBoxPropagateNestedBounds) {
    auto mounted = nandina::app::mount(
        nandina::app::center(
            nandina::app::sized_box(
                nandina::app::adopt(TestWidget::create()))));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(0.0f, 0.0f, 100.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* center = mounted->children().front().get();
    ASSERT_NE(center, nullptr);
    ASSERT_EQ(center->child_count(), 1u);

    auto* sized_box = center->children().front().get();
    ASSERT_NE(sized_box, nullptr);
    ASSERT_EQ(sized_box->child_count(), 1u);

    const auto sized_box_bounds = sized_box->bounds();
    EXPECT_FLOAT_EQ(sized_box_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.height(), 12.0f);

    const auto child_bounds = sized_box->children().front()->bounds();
    EXPECT_FLOAT_EQ(child_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(child_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(child_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(child_bounds.height(), 12.0f);
}

TEST(AppAuthoringTest, StackMountsMultipleChildrenAndPropagatesRefs) {
    nandina::app::Ref<TestWidget> front_ref;
    nandina::app::Ref<TestWidget> back_ref;

    auto mounted = nandina::app::mount(nandina::app::stack(nandina::app::children(
        nandina::app::adopt(TestWidget::create()).bind(back_ref),
        nandina::app::adopt(TestWidget::create()).bind(front_ref))));

    ASSERT_NE(mounted, nullptr);
    ASSERT_EQ(mounted->child_count(), 1u);

    auto* stack = mounted->children().front().get();
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->child_count(), 2u);
    EXPECT_EQ(back_ref.get(), stack->children()[0].get());
    EXPECT_EQ(front_ref.get(), stack->children()[1].get());

    mounted->set_bounds(5.0f, 8.0f, 90.0f, 40.0f);
    const auto first_bounds = stack->children()[0]->bounds();
    const auto second_bounds = stack->children()[1]->bounds();

    EXPECT_FLOAT_EQ(first_bounds.x(), 5.0f);
    EXPECT_FLOAT_EQ(first_bounds.y(), 8.0f);
    EXPECT_FLOAT_EQ(first_bounds.width(), 90.0f);
    EXPECT_FLOAT_EQ(first_bounds.height(), 40.0f);
    EXPECT_FLOAT_EQ(second_bounds.x(), 5.0f);
    EXPECT_FLOAT_EQ(second_bounds.y(), 8.0f);
    EXPECT_FLOAT_EQ(second_bounds.width(), 90.0f);
    EXPECT_FLOAT_EQ(second_bounds.height(), 40.0f);

    mounted.reset();
    EXPECT_FALSE(front_ref);
    EXPECT_FALSE(back_ref);
}