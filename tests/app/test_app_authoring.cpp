#include <gtest/gtest.h>

import nandina.app.authoring;
import nandina.runtime.nan_widget;
import nandina.layout.core;
import nandina.foundation.color;
import nandina.widgets.button;
import nandina.widgets.card;
import nandina.widgets.label;
import nandina.widgets.panel;

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

TEST(AppAuthoringTest, PaddingNodeSupportsChainedInsetsConfiguration) {
    auto mounted = nandina::app::mount(
        nandina::app::padding(
            nandina::app::adopt(TestWidget::create()))
            .padding(3.0f, 5.0f, 7.0f, 11.0f));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(10.0f, 20.0f, 100.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* padding = mounted->children().front().get();
    ASSERT_NE(padding, nullptr);
    ASSERT_EQ(padding->child_count(), 1u);

    const auto child_bounds = padding->children().front()->bounds();
    EXPECT_FLOAT_EQ(child_bounds.x(), 13.0f);
    EXPECT_FLOAT_EQ(child_bounds.y(), 25.0f);
    EXPECT_FLOAT_EQ(child_bounds.width(), 90.0f);
    EXPECT_FLOAT_EQ(child_bounds.height(), 44.0f);
}

TEST(AppAuthoringTest, SizedBoxNodeSupportsChainedFixedDimensions) {
    auto mounted = nandina::app::mount(
        nandina::app::center(
            nandina::app::sized_box(
                nandina::app::adopt(TestWidget::create()))
                .width(48.0f)
                .height(18.0f)));

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
    EXPECT_FLOAT_EQ(sized_box_bounds.x(), 26.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.y(), 21.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.width(), 48.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.height(), 18.0f);

    const auto child_bounds = sized_box->children().front()->bounds();
    EXPECT_FLOAT_EQ(child_bounds.x(), 26.0f);
    EXPECT_FLOAT_EQ(child_bounds.y(), 21.0f);
    EXPECT_FLOAT_EQ(child_bounds.width(), 48.0f);
    EXPECT_FLOAT_EQ(child_bounds.height(), 18.0f);
}

TEST(AppAuthoringTest, ColumnNodeSupportsChainedLayoutContainerConfiguration) {
    auto mounted = nandina::app::mount(
        nandina::app::column(nandina::app::children(
            nandina::app::adopt(TestWidget::create()),
            nandina::app::adopt(TestWidget::create())))
            .padding(4.0f, 6.0f, 8.0f, 10.0f)
            .gap(5.0f)
            .align_items(nandina::layout::LayoutAlignment::stretch));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(0.0f, 0.0f, 80.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* column = mounted->children().front().get();
    ASSERT_NE(column, nullptr);
    ASSERT_EQ(column->child_count(), 2u);

    const auto first_bounds = column->children()[0]->bounds();
    const auto second_bounds = column->children()[1]->bounds();

    EXPECT_FLOAT_EQ(first_bounds.x(), 4.0f);
    EXPECT_FLOAT_EQ(first_bounds.y(), 6.0f);
    EXPECT_FLOAT_EQ(first_bounds.width(), 68.0f);
    EXPECT_FLOAT_EQ(first_bounds.height(), 12.0f);

    EXPECT_FLOAT_EQ(second_bounds.x(), 4.0f);
    EXPECT_FLOAT_EQ(second_bounds.y(), 23.0f);
    EXPECT_FLOAT_EQ(second_bounds.width(), 68.0f);
    EXPECT_FLOAT_EQ(second_bounds.height(), 12.0f);
}

TEST(AppAuthoringTest, StackNodeSupportsChainedAlignmentConfiguration) {
    auto mounted = nandina::app::mount(
        nandina::app::stack(nandina::app::children(
            nandina::app::adopt(TestWidget::create()),
            nandina::app::adopt(TestWidget::create())))
            .align_items(nandina::layout::LayoutAlignment::center)
            .justify_content(nandina::layout::LayoutAlignment::center));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(0.0f, 0.0f, 100.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* stack = mounted->children().front().get();
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->child_count(), 2u);

    const auto first_bounds = stack->children()[0]->bounds();
    const auto second_bounds = stack->children()[1]->bounds();

    EXPECT_FLOAT_EQ(first_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(first_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(first_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(first_bounds.height(), 12.0f);

    EXPECT_FLOAT_EQ(second_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(second_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(second_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(second_bounds.height(), 12.0f);
}

TEST(AppAuthoringTest, LabelFactorySupportsChainedTextStylingAndRefBinding) {
    nandina::app::Ref<nandina::widgets::Label> label_ref;

    auto mounted = nandina::app::mount(
        nandina::app::label("Overview")
            .font_size(10.0f)
            .align(nandina::widgets::TextAlign::Center)
            .vertical_align(nandina::widgets::TextVerticalAlign::Center)
            .color(nandina::NanColor::from(nandina::NanRgb{1, 2, 3}))
            .bind(label_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(label_ref);
    EXPECT_EQ(label_ref->text(), "Overview");
    EXPECT_FLOAT_EQ(label_ref->font_size(), 10.0f);
    const auto color = label_ref->color().to<nandina::NanRgb>();
    EXPECT_EQ(color.red(), 1u);
    EXPECT_EQ(color.green(), 2u);
    EXPECT_EQ(color.blue(), 3u);
}

TEST(AppAuthoringTest, ButtonFactorySupportsTextColorsAndRefBinding) {
    nandina::app::Ref<nandina::widgets::Button> button_ref;

    nandina::widgets::ButtonColors colors;
    colors.bg = nandina::NanColor::from(nandina::NanRgb{9, 8, 7});
    colors.text = nandina::NanColor::from(nandina::NanRgb{6, 5, 4});

    auto mounted = nandina::app::mount(
        nandina::app::button("Run")
            .colors(colors)
            .bind(button_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(button_ref);
    EXPECT_EQ(button_ref->text(), "Run");
    const auto bg = button_ref->colors().bg.to<nandina::NanRgb>();
    const auto text = button_ref->colors().text.to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 9u);
    EXPECT_EQ(bg.green(), 8u);
    EXPECT_EQ(bg.blue(), 7u);
    EXPECT_EQ(text.red(), 6u);
    EXPECT_EQ(text.green(), 5u);
    EXPECT_EQ(text.blue(), 4u);
}

TEST(AppAuthoringTest, CardFactorySupportsTitleStylingAndChildRefs) {
    nandina::app::Ref<nandina::widgets::Card> card_ref;
    nandina::app::Ref<TestWidget> child_ref;

    auto mounted = nandina::app::mount(
        nandina::app::card(nandina::app::children(
            nandina::app::adopt(TestWidget::create()).bind(child_ref)))
            .title("Stats")
            .bg_color(nandina::NanColor::from(nandina::NanRgb{50, 60, 70}))
            .corner_radius(12.0f)
            .bind(card_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(card_ref);
    ASSERT_TRUE(child_ref);
    EXPECT_EQ(card_ref->title(), "Stats");
    EXPECT_EQ(child_ref.get(), card_ref->children().front().get());
    EXPECT_FLOAT_EQ(card_ref->corner_radius(), 12.0f);
    const auto bg = card_ref->bg_color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 50u);
    EXPECT_EQ(bg.green(), 60u);
    EXPECT_EQ(bg.blue(), 70u);
}

TEST(AppAuthoringTest, PanelFactorySupportsTitleStylingAndChildRefs) {
    nandina::app::Ref<nandina::widgets::Panel> panel_ref;
    nandina::app::Ref<TestWidget> child_ref;

    auto mounted = nandina::app::mount(
        nandina::app::panel(nandina::app::children(
            nandina::app::adopt(TestWidget::create()).bind(child_ref)))
            .title("Settings")
            .bg_color(nandina::NanColor::from(nandina::NanRgb{42, 44, 62}))
            .corner_radius(9.0f)
            .bind(panel_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(panel_ref);
    ASSERT_TRUE(child_ref);
    EXPECT_EQ(panel_ref->title(), "Settings");
    EXPECT_EQ(child_ref.get(), panel_ref->children().front().get());
    EXPECT_FLOAT_EQ(panel_ref->corner_radius(), 9.0f);
    const auto bg = panel_ref->bg_color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 42u);
    EXPECT_EQ(bg.green(), 44u);
    EXPECT_EQ(bg.blue(), 62u);
}