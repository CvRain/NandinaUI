#include <gtest/gtest.h>

import nandina.runtime.nan_widget;
import nandina.showcase.chart_card;
import nandina.showcase.header_bar;
import nandina.showcase.dock_bar;
import nandina.showcase.recent_activity_card;
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

    ASSERT_EQ(chart->child_count(), 8u);

    const auto title_bounds = child_at(*chart, 0).bounds();
    EXPECT_FLOAT_EQ(title_bounds.x(), 276.0f);
    EXPECT_FLOAT_EQ(title_bounds.y(), 188.0f);
    EXPECT_FLOAT_EQ(title_bounds.width(), 160.0f);
    EXPECT_FLOAT_EQ(title_bounds.height(), 18.0f);

    const auto first_day_bounds = child_at(*chart, 1).bounds();
    const auto last_day_bounds = child_at(*chart, 7).bounds();

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

    ASSERT_EQ(activity->child_count(), 11u);

    const auto title_bounds = child_at(*activity, 0).bounds();
    EXPECT_FLOAT_EQ(title_bounds.x(), 634.0f);
    EXPECT_FLOAT_EQ(title_bounds.y(), 188.0f);
    EXPECT_FLOAT_EQ(title_bounds.width(), 160.0f);
    EXPECT_FLOAT_EQ(title_bounds.height(), 18.0f);

    const auto first_text_bounds = child_at(*activity, 1).bounds();
    const auto first_time_bounds = child_at(*activity, 2).bounds();
    const auto last_text_bounds = child_at(*activity, 9).bounds();
    const auto last_time_bounds = child_at(*activity, 10).bounds();

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