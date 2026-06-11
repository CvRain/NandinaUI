#include <gtest/gtest.h>

import nandina.runtime.nan_event;
import nandina.theme;
import nandina.widgets.focus_ring;
import nandina.widgets.switch_widget;
import nandina.widgets.text;

namespace {

auto find_text_child(nandina::widgets::Switch& sw) -> nandina::widgets::Text* {
    for (auto& child : sw.children()) {
        if (auto* text = dynamic_cast<nandina::widgets::Text*>(child.get())) {
            return text;
        }
    }
    return nullptr;
}

auto find_focus_ring(nandina::widgets::Switch& sw) -> nandina::widgets::FocusRing* {
    for (auto& child : sw.children()) {
        if (auto* focus_ring = dynamic_cast<nandina::widgets::FocusRing*>(child.get())) {
            return focus_ring;
        }
    }
    return nullptr;
}

} // namespace

TEST(WidgetsSwitchTest, DefaultStateUnchecked) {
    auto sw = nandina::widgets::Switch::create();

    EXPECT_FALSE(sw->checked());
    EXPECT_FALSE(sw->disabled());
    EXPECT_EQ(sw->color_variant(), nandina::theme::ColorVariant::inherit);
    EXPECT_EQ(sw->size(), nandina::widgets::SwitchSize::md);
}

TEST(WidgetsSwitchTest, LabelIsStoredAndRenderedByTextChild) {
    auto sw = nandina::widgets::Switch::create();
    sw->label("Enable sync");

    auto* text = find_text_child(*sw);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(sw->label(), "Enable sync");
    EXPECT_EQ(text->text(), "Enable sync");
}

TEST(WidgetsSwitchTest, CheckedTogglesStateAndEmitsSignal) {
    auto sw = nandina::widgets::Switch::create();
    int call_count = 0;
    bool last_value = false;
    sw->on_checked_changed([&](bool value) {
        ++call_count;
        last_value = value;
    });

    sw->checked(true);
    EXPECT_TRUE(sw->checked());
    EXPECT_EQ(call_count, 1);
    EXPECT_TRUE(last_value);

    sw->checked(true);
    EXPECT_EQ(call_count, 1);

    sw->checked(false);
    EXPECT_FALSE(sw->checked());
    EXPECT_EQ(call_count, 2);
    EXPECT_FALSE(last_value);
}

TEST(WidgetsSwitchTest, PointerLifecycleTogglesWhenEnabled) {
    auto sw = nandina::widgets::Switch::create();
    sw->set_bounds(0.0f, 0.0f, 96.0f, 32.0f);

    EXPECT_TRUE(sw->dispatch_event(nandina::runtime::PointerMoveEvent{.x = 8.0, .y = 8.0}));
    EXPECT_TRUE(sw->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 8.0,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(sw->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 8.0,
        },
        nandina::runtime::EventType::PointerUp));

    EXPECT_TRUE(sw->checked());
}

TEST(WidgetsSwitchTest, DisabledBlocksPointerAndFocusState) {
    auto sw = nandina::widgets::Switch::create();
    sw->set_bounds(0.0f, 0.0f, 96.0f, 32.0f);

    auto* focus_ring = find_focus_ring(*sw);
    ASSERT_NE(focus_ring, nullptr);

    EXPECT_TRUE(sw->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(focus_ring->active());

    sw->disabled(true);
    EXPECT_TRUE(sw->disabled());
    EXPECT_FALSE(focus_ring->active());

    EXPECT_FALSE(sw->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_FALSE(sw->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 8.0,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_FALSE(sw->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 8.0,
        },
        nandina::runtime::EventType::PointerUp));
    EXPECT_FALSE(sw->checked());
}

TEST(WidgetsSwitchTest, SizeAffectsPreferredSize) {
    auto sw = nandina::widgets::Switch::create();
    sw->label("Option");
    const auto md_pref = sw->preferred_size();

    sw->size(nandina::widgets::SwitchSize::sm);
    const auto sm_pref = sw->preferred_size();

    EXPECT_LT(sm_pref.width(), md_pref.width());
    EXPECT_LT(sm_pref.height(), md_pref.height());
}
