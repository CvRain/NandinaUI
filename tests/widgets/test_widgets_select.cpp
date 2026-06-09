#include <gtest/gtest.h>

import nandina.foundation.nan_constraints;
import nandina.runtime.nan_event;
import nandina.widgets.select;

TEST(WidgetsSelectTest, AddOptionSelectsFirstOptionByDefault) {
    auto select = nandina::widgets::Select::create();
    select->add_option("Small", "sm")
        .add_option("Medium", "md")
        .add_option("Large", "lg");

    EXPECT_EQ(select->option_count(), 3u);
    EXPECT_TRUE(select->has_selection());
    EXPECT_EQ(select->selected_index(), 0u);
    EXPECT_EQ(select->selected_label(), "Small");
    EXPECT_EQ(select->selected_value(), "sm");
}

TEST(WidgetsSelectTest, SelectedIndexUpdatesValueAndEmitsSignal) {
    auto select = nandina::widgets::Select::create();
    select->add_option("Small", "sm")
        .add_option("Medium", "md")
        .add_option("Large", "lg");

    int changes = 0;
    std::size_t last_index = 0;
    select->on_changed([&](const std::size_t index) {
        ++changes;
        last_index = index;
    });

    select->selected_index(2);

    EXPECT_EQ(changes, 1);
    EXPECT_EQ(last_index, 2u);
    EXPECT_EQ(select->selected_label(), "Large");
    EXPECT_EQ(select->selected_value(), "lg");
}

TEST(WidgetsSelectTest, TriggerClickTogglesOpenState) {
    auto select = nandina::widgets::Select::create();
    select->add_option("Small", "sm").add_option("Medium", "md");
    select->measure(nandina::geometry::NanConstraints::tight(320.0f, 180.0f));
    select->set_bounds(0.0f, 0.0f, 320.0f, 180.0f);
    select->layout();

    const auto trigger = select->trigger_bounds();
    EXPECT_FALSE(select->open());
    EXPECT_TRUE(select->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = trigger.x() + 8.0,
            .y = trigger.y() + 8.0,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(select->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = trigger.x() + 8.0,
            .y = trigger.y() + 8.0,
        },
        nandina::runtime::EventType::PointerUp));
    EXPECT_TRUE(select->open());

    EXPECT_TRUE(select->dispatch_event(nandina::runtime::KeyEvent{.key_code = 27}, nandina::runtime::EventType::KeyDown));
    EXPECT_FALSE(select->open());
}

TEST(WidgetsSelectTest, PreferredSizeAndLayoutReserveOptionsPanel) {
    auto select = nandina::widgets::Select::create();
    select->add_option("Small", "sm").add_option("Medium", "md").add_option("Large", "lg");

    const auto preferred = select->preferred_size();
    EXPECT_GE(preferred.width(), 220.0f);
    EXPECT_GT(preferred.height(), 80.0f);

    select->open(true);
    select->measure(nandina::geometry::NanConstraints::tight(320.0f, 220.0f));
    select->set_bounds(10.0f, 20.0f, 320.0f, 220.0f);
    select->layout();

    const auto trigger = select->trigger_bounds();
    const auto panel = select->panel_bounds();
    EXPECT_GT(panel.y(), trigger.y());
    EXPECT_GE(panel.width(), trigger.width());
}
