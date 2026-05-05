#include <gtest/gtest.h>

import nandina.runtime.nan_widget;
import nandina.showcase;
import nandina.showcase.project_progress_card;
import nandina.showcase.stack_demo_card;
import nandina.showcase.chart_card;
import nandina.showcase.footer_section;
import nandina.showcase.header_bar;
import nandina.showcase.dock_bar;
import nandina.showcase.middle_content_section;
import nandina.showcase.recent_activity_card;
import nandina.showcase.sidebar_section;
import nandina.showcase.bottom_content_section;
import nandina.showcase.stats_section;

namespace {

auto child_at(nandina::runtime::NanWidget& widget, const std::size_t index) -> nandina::runtime::NanWidget& {
    auto& children = widget.children();
    EXPECT_LT(index, children.size());
    return *children.at(index);
}

} // namespace

TEST(ShowcaseLayoutTest, HeaderBar_UsesSingleRowAssembly) {
    auto header = nandina::showcase::HeaderBar::create();
    header->set_bounds(240.0f, 0.0f, 760.0f, 44.0f);

    ASSERT_EQ(header->child_count(), 1u);

    auto& row = child_at(*header, 0);
    EXPECT_EQ(row.child_count(), 5u);

    const auto title_bounds = child_at(row, 0).bounds();
    EXPECT_FLOAT_EQ(title_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(title_bounds.y(), 15.0f);

    const auto trailing_button_bounds = child_at(row, 4).bounds();
    EXPECT_FLOAT_EQ(trailing_button_bounds.width(), 32.0f);
    EXPECT_FLOAT_EQ(trailing_button_bounds.height(), 32.0f);
    EXPECT_GT(trailing_button_bounds.x(), 900.0f);
    EXPECT_FLOAT_EQ(trailing_button_bounds.y(), 6.0f);
}

TEST(ShowcaseLayoutTest, DockBar_ComposesRowsInsteadOfManualFrames) {
    auto dock = nandina::showcase::DockBar::create();
    dock->set_bounds(240.0f, 664.0f, 760.0f, 56.0f);

    ASSERT_EQ(dock->child_count(), 2u);

    auto& icon_row = child_at(*dock, 0);
    auto& overlay_row = child_at(*dock, 1);

    EXPECT_EQ(icon_row.child_count(), 7u);
    EXPECT_EQ(overlay_row.child_count(), 3u);

    const auto first_icon_bounds = child_at(icon_row, 0).bounds();
    EXPECT_FLOAT_EQ(first_icon_bounds.width(), 32.0f);
    EXPECT_FLOAT_EQ(first_icon_bounds.height(), 32.0f);
    EXPECT_GT(first_icon_bounds.x(), 400.0f);
    EXPECT_LT(first_icon_bounds.x(), 500.0f);

    const auto thumbnail_bounds = child_at(overlay_row, 0).bounds();
    EXPECT_FLOAT_EQ(thumbnail_bounds.x(), 252.0f);
    EXPECT_FLOAT_EQ(thumbnail_bounds.y(), 670.0f);
    EXPECT_FLOAT_EQ(thumbnail_bounds.width(), 120.0f);
    EXPECT_FLOAT_EQ(thumbnail_bounds.height(), 44.0f);

    const auto clock_bounds = child_at(overlay_row, 2).bounds();
    EXPECT_GT(clock_bounds.x(), thumbnail_bounds.right());
    EXPECT_LT(clock_bounds.right(), dock->bounds().right());
    EXPECT_GT(clock_bounds.y(), dock->bounds().y());
    EXPECT_LT(clock_bounds.bottom(), dock->bounds().bottom());
}

TEST(ShowcaseLayoutTest, StatsSection_NestsLayoutTreeInsteadOfDirectFrameDispatch) {
    auto stats = nandina::showcase::StatsSection::create();
    stats->set_bounds(260.0f, 64.0f, 700.0f, 100.0f);

    ASSERT_EQ(stats->child_count(), 1u);

    auto& row = child_at(*stats, 0);
    EXPECT_EQ(row.child_count(), 4u);

    auto& first_expanded = child_at(row, 0);
    EXPECT_EQ(first_expanded.child_count(), 1u);

    auto& first_stack = child_at(first_expanded, 0);
    EXPECT_EQ(first_stack.child_count(), 2u);

    const auto first_card_bounds = child_at(first_stack, 0).bounds();
    const auto first_pressable_bounds = child_at(first_stack, 1).bounds();

    EXPECT_FLOAT_EQ(first_card_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(first_card_bounds.y(), 64.0f);
    EXPECT_NEAR(first_card_bounds.width(), 163.0f, 1e-4f);
    EXPECT_FLOAT_EQ(first_card_bounds.height(), 100.0f);

    EXPECT_FLOAT_EQ(first_pressable_bounds.x(), first_card_bounds.x());
    EXPECT_FLOAT_EQ(first_pressable_bounds.y(), first_card_bounds.y());
    EXPECT_FLOAT_EQ(first_pressable_bounds.width(), first_card_bounds.width());
    EXPECT_FLOAT_EQ(first_pressable_bounds.height(), first_card_bounds.height());
}

TEST(ShowcaseLayoutTest, ChartCard_KeepsTitleAndDayLabelsInExpectedSlots) {
    auto chart = nandina::showcase::ChartCard::create();
    chart->set_bounds(260.0f, 180.0f, 342.0f, 200.0f);

    ASSERT_EQ(chart->child_count(), 2u);

    const auto title_bounds = child_at(*chart, 0).bounds();
    EXPECT_FLOAT_EQ(title_bounds.x(), 276.0f);
    EXPECT_FLOAT_EQ(title_bounds.y(), 188.0f);
    EXPECT_FLOAT_EQ(title_bounds.width(), 160.0f);
    EXPECT_FLOAT_EQ(title_bounds.height(), 18.0f);

    auto& day_strip = child_at(*chart, 1);
    ASSERT_EQ(day_strip.child_count(), 7u);

    const auto first_day_bounds = child_at(day_strip, 0).bounds();
    const auto last_day_bounds = child_at(day_strip, 6).bounds();

    EXPECT_FLOAT_EQ(first_day_bounds.x(), 270.0f);
    EXPECT_FLOAT_EQ(first_day_bounds.y(), 364.0f);
    EXPECT_FLOAT_EQ(first_day_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(first_day_bounds.height(), 14.0f);

    EXPECT_FLOAT_EQ(last_day_bounds.x(), 572.0f);
    EXPECT_FLOAT_EQ(last_day_bounds.y(), 364.0f);
    EXPECT_FLOAT_EQ(last_day_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(last_day_bounds.height(), 14.0f);
}

TEST(ShowcaseLayoutTest, RecentActivityCard_KeepsTitleAndRowsAligned) {
    auto activity = nandina::showcase::RecentActivityCard::create();
    activity->set_bounds(618.0f, 180.0f, 342.0f, 200.0f);

    ASSERT_EQ(activity->child_count(), 2u);

    const auto title_bounds = child_at(*activity, 0).bounds();
    EXPECT_FLOAT_EQ(title_bounds.x(), 634.0f);
    EXPECT_FLOAT_EQ(title_bounds.y(), 188.0f);
    EXPECT_FLOAT_EQ(title_bounds.width(), 160.0f);
    EXPECT_FLOAT_EQ(title_bounds.height(), 18.0f);

    auto& list = child_at(*activity, 1);
    ASSERT_EQ(list.child_count(), 1u);

    auto& column = child_at(list, 0);
    ASSERT_EQ(column.child_count(), 5u);

    auto& first_row = child_at(column, 0);
    auto& last_row = child_at(column, 4);
    ASSERT_EQ(first_row.child_count(), 2u);
    ASSERT_EQ(last_row.child_count(), 2u);

    const auto first_text_bounds = child_at(first_row, 0).bounds();
    const auto first_time_bounds = child_at(first_row, 1).bounds();
    const auto last_text_bounds = child_at(last_row, 0).bounds();
    const auto last_time_bounds = child_at(last_row, 1).bounds();

    EXPECT_FLOAT_EQ(first_text_bounds.x(), 654.0f);
    EXPECT_FLOAT_EQ(first_text_bounds.y(), 218.0f);
    EXPECT_FLOAT_EQ(first_text_bounds.width(), 200.0f);
    EXPECT_FLOAT_EQ(first_text_bounds.height(), 16.0f);

    EXPECT_FLOAT_EQ(first_time_bounds.x(), 654.0f);
    EXPECT_FLOAT_EQ(first_time_bounds.y(), 232.0f);
    EXPECT_FLOAT_EQ(first_time_bounds.width(), 120.0f);
    EXPECT_FLOAT_EQ(first_time_bounds.height(), 12.0f);

    EXPECT_FLOAT_EQ(last_text_bounds.x(), 654.0f);
    EXPECT_FLOAT_EQ(last_text_bounds.y(), 338.0f);
    EXPECT_FLOAT_EQ(last_text_bounds.width(), 200.0f);
    EXPECT_FLOAT_EQ(last_text_bounds.height(), 16.0f);

    EXPECT_FLOAT_EQ(last_time_bounds.x(), 654.0f);
    EXPECT_FLOAT_EQ(last_time_bounds.y(), 352.0f);
    EXPECT_FLOAT_EQ(last_time_bounds.width(), 120.0f);
    EXPECT_FLOAT_EQ(last_time_bounds.height(), 12.0f);
}

TEST(ShowcaseLayoutTest, StackDemoCard_UsesInternalLayerNode) {
    auto stack_demo = nandina::showcase::StackDemoCard::create();
    stack_demo->set_bounds(260.0f, 400.0f, 342.0f, 110.0f);

    ASSERT_EQ(stack_demo->child_count(), 4u);

    const auto title_bounds = child_at(*stack_demo, 1).bounds();
    const auto layers_bounds = child_at(*stack_demo, 2).bounds();
    const auto footer_bounds = child_at(*stack_demo, 3).bounds();

    EXPECT_FLOAT_EQ(title_bounds.x(), 274.0f);
    EXPECT_FLOAT_EQ(title_bounds.y(), 414.0f);
    EXPECT_FLOAT_EQ(layers_bounds.x(), 280.0f);
    EXPECT_FLOAT_EQ(layers_bounds.y(), 430.0f);
    EXPECT_FLOAT_EQ(layers_bounds.width(), 302.0f);
    EXPECT_FLOAT_EQ(layers_bounds.height(), 64.0f);
    EXPECT_FLOAT_EQ(footer_bounds.x(), 274.0f);
    EXPECT_FLOAT_EQ(footer_bounds.y(), 496.0f);
}

TEST(ShowcaseLayoutTest, ProjectProgressCard_UsesInternalRowsNode) {
    auto progress = nandina::showcase::ProjectProgressCard::create();
    progress->set_bounds(618.0f, 400.0f, 342.0f, 110.0f);

    ASSERT_EQ(progress->child_count(), 3u);

    const auto title_bounds = child_at(*progress, 1).bounds();
    auto& rows = child_at(*progress, 2);
    ASSERT_EQ(rows.child_count(), 1u);

    auto& column = child_at(rows, 0);
    ASSERT_EQ(column.child_count(), 4u);

    EXPECT_FLOAT_EQ(title_bounds.x(), 632.0f);
    EXPECT_FLOAT_EQ(title_bounds.y(), 408.0f);

    auto& first_row = child_at(column, 0);
    auto& last_row = child_at(column, 3);
    ASSERT_EQ(first_row.child_count(), 3u);
    ASSERT_EQ(last_row.child_count(), 3u);

    const auto first_label_bounds = child_at(first_row, 0).bounds();
    const auto first_bar_bounds = child_at(first_row, 1).bounds();
    const auto first_pct_bounds = child_at(first_row, 2).bounds();
    const auto last_label_bounds = child_at(last_row, 0).bounds();

    EXPECT_FLOAT_EQ(first_label_bounds.x(), 634.0f);
    EXPECT_FLOAT_EQ(first_label_bounds.y(), 434.0f);
    EXPECT_FLOAT_EQ(first_bar_bounds.x(), 789.0f);
    EXPECT_FLOAT_EQ(first_bar_bounds.y(), 435.0f);
    EXPECT_NEAR(first_bar_bounds.width(), 143.64f, 1e-3f);
    EXPECT_FLOAT_EQ(first_pct_bounds.y(), 434.0f);
    EXPECT_FLOAT_EQ(last_label_bounds.y(), 488.0f);
}

TEST(ShowcaseLayoutTest, FooterSection_UsesPaddingNodeForLabelPlacement) {
    auto footer = nandina::showcase::FooterSection::create();
    footer->set_bounds(260.0f, 530.0f, 700.0f, 26.0f);

    ASSERT_EQ(footer->child_count(), 1u);

    auto& padding = child_at(*footer, 0);
    ASSERT_EQ(padding.child_count(), 1u);

    const auto padding_bounds = padding.bounds();
    const auto label_bounds = child_at(padding, 0).bounds();

    EXPECT_FLOAT_EQ(padding_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(padding_bounds.y(), 530.0f);
    EXPECT_FLOAT_EQ(padding_bounds.width(), 700.0f);
    EXPECT_FLOAT_EQ(padding_bounds.height(), 26.0f);

    EXPECT_FLOAT_EQ(label_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(label_bounds.y(), 542.0f);
    EXPECT_FLOAT_EQ(label_bounds.width(), 700.0f);
    EXPECT_FLOAT_EQ(label_bounds.height(), 14.0f);
}

TEST(ShowcaseLayoutTest, SidebarSection_IsConcreteSidebarWithoutWrapperNode) {
    auto sidebar = nandina::showcase::SidebarSection::create();

    EXPECT_FLOAT_EQ(sidebar->preferred_size().width(), 240.0f);
    ASSERT_EQ(sidebar->child_count(), 1u);

    auto& root_column = child_at(*sidebar, 0);
    ASSERT_EQ(root_column.child_count(), 3u);

    auto& content_expanded = child_at(root_column, 1);
    ASSERT_EQ(content_expanded.child_count(), 1u);

    auto& content_padding = child_at(content_expanded, 0);
    ASSERT_EQ(content_padding.child_count(), 1u);

    auto& content_column = child_at(content_padding, 0);
    ASSERT_EQ(content_column.child_count(), 2u);
}

TEST(ShowcaseLayoutTest, MainComponent_NestsContentSectionsInsideColumnNode) {
    MainComponent component;

    ASSERT_EQ(component.child_count(), 1u);

    auto& shell = child_at(component, 0);
    ASSERT_EQ(shell.child_count(), 1u);

    auto& shell_row = child_at(shell, 0);
    ASSERT_EQ(shell_row.child_count(), 2u);

    auto& sidebar_slot = child_at(shell_row, 0);
    auto& main_expanded = child_at(shell_row, 1);
    ASSERT_EQ(sidebar_slot.child_count(), 1u);
    ASSERT_EQ(main_expanded.child_count(), 1u);

    auto& sidebar = child_at(sidebar_slot, 0);
    ASSERT_EQ(sidebar.child_count(), 1u);

    auto& sidebar_column = child_at(sidebar, 0);
    ASSERT_EQ(sidebar_column.child_count(), 3u);

    auto& main_column = child_at(main_expanded, 0);
    ASSERT_EQ(main_column.child_count(), 3u);

    auto& content_expanded = child_at(main_column, 1);
    ASSERT_EQ(content_expanded.child_count(), 1u);

    auto& content_padding = child_at(content_expanded, 0);
    ASSERT_EQ(content_padding.child_count(), 1u);

    auto& content = child_at(content_padding, 0);
    ASSERT_EQ(content.child_count(), 1u);

    auto& column = child_at(content, 0);
    ASSERT_EQ(column.child_count(), 4u);

    EXPECT_FLOAT_EQ(child_at(column, 0).preferred_size().height(), 100.0f);
    EXPECT_FLOAT_EQ(child_at(column, 1).preferred_size().height(), 200.0f);
    EXPECT_FLOAT_EQ(child_at(column, 2).preferred_size().height(), 110.0f);
    EXPECT_FLOAT_EQ(child_at(column, 3).preferred_size().height(), 26.0f);
}

TEST(ShowcaseLayoutTest, MiddleContentSection_UsesSplitRowContract) {
    auto middle = nandina::showcase::MiddleContentSection::create();

    const auto preferred = middle->preferred_size();
    EXPECT_FLOAT_EQ(preferred.height(), 200.0f);

    middle->set_bounds(260.0f, 180.0f, 700.0f, 200.0f);

    ASSERT_EQ(middle->child_count(), 2u);

    const auto leading_bounds = child_at(*middle, 0).bounds();
    const auto trailing_bounds = child_at(*middle, 1).bounds();

    EXPECT_FLOAT_EQ(leading_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(leading_bounds.y(), 180.0f);
    EXPECT_NEAR(leading_bounds.width(), 410.4f, 1e-4f);
    EXPECT_FLOAT_EQ(leading_bounds.height(), 200.0f);

    EXPECT_NEAR(trailing_bounds.x(), 686.4f, 1e-4f);
    EXPECT_FLOAT_EQ(trailing_bounds.y(), 180.0f);
    EXPECT_NEAR(trailing_bounds.width(), 273.6f, 1e-4f);
    EXPECT_FLOAT_EQ(trailing_bounds.height(), 200.0f);
}

TEST(ShowcaseLayoutTest, BottomContentSection_UsesSplitRowContract) {
    auto bottom = nandina::showcase::BottomContentSection::create();

    const auto preferred = bottom->preferred_size();
    EXPECT_FLOAT_EQ(preferred.height(), 110.0f);

    bottom->set_bounds(260.0f, 400.0f, 700.0f, 110.0f);

    ASSERT_EQ(bottom->child_count(), 2u);

    const auto leading_bounds = child_at(*bottom, 0).bounds();
    const auto trailing_bounds = child_at(*bottom, 1).bounds();

    EXPECT_FLOAT_EQ(leading_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(leading_bounds.y(), 400.0f);
    EXPECT_FLOAT_EQ(leading_bounds.width(), 342.0f);
    EXPECT_FLOAT_EQ(leading_bounds.height(), 110.0f);

    EXPECT_FLOAT_EQ(trailing_bounds.x(), 618.0f);
    EXPECT_FLOAT_EQ(trailing_bounds.y(), 400.0f);
    EXPECT_FLOAT_EQ(trailing_bounds.width(), 342.0f);
    EXPECT_FLOAT_EQ(trailing_bounds.height(), 110.0f);
}