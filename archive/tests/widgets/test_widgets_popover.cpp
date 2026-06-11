#include <gtest/gtest.h>

#include <memory>

import nandina.foundation.nan_constraints;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.widgets.button;
import nandina.widgets.popover;
import nandina.widgets.tooltip;

namespace {

class FixedWidget final : public nandina::runtime::NanWidget {
public:
    explicit FixedWidget(nandina::geometry::NanSize preferred)
        : preferred_(preferred) {}

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return preferred_;
    }

    auto measure(const nandina::geometry::NanConstraints& constraints) -> void override {
        set_measured_layout_state(constraints, constraints.constrain(preferred_));
    }

private:
    nandina::geometry::NanSize preferred_{};
};

} // namespace

TEST(WidgetsPopoverTest, ClickingTriggerTogglesOpenStateAndEmitsSignal) {
    auto popover = nandina::widgets::Popover::create();
    auto trigger = nandina::widgets::Button::create();
    trigger->text("Open");
    popover->trigger(std::move(trigger))
        .content(std::make_unique<FixedWidget>(nandina::geometry::NanSize{140.0f, 60.0f}));
    popover->measure(nandina::geometry::NanConstraints::tight(320.0f, 220.0f));
    popover->set_bounds(0.0f, 0.0f, 320.0f, 220.0f);
    popover->layout();

    int changes = 0;
    bool last_open = false;
    popover->on_open_changed([&](const bool open) {
        ++changes;
        last_open = open;
    });

    EXPECT_FALSE(popover->open());
    EXPECT_TRUE(popover->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 12.0,
            .y = 12.0,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(popover->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 12.0,
            .y = 12.0,
        },
        nandina::runtime::EventType::PointerUp));

    EXPECT_TRUE(popover->open());
    EXPECT_EQ(changes, 1);
    EXPECT_TRUE(last_open);

    EXPECT_TRUE(popover->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 12.0,
            .y = 12.0,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(popover->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 12.0,
            .y = 12.0,
        },
        nandina::runtime::EventType::PointerUp));

    EXPECT_FALSE(popover->open());
    EXPECT_EQ(changes, 2);
    EXPECT_FALSE(last_open);
}

TEST(WidgetsPopoverTest, LayoutPlacesPanelInsideContainerNearTrigger) {
    auto popover = nandina::widgets::Popover::create();
    auto trigger_button = nandina::widgets::Button::create();
    trigger_button->text("Open");
    popover->trigger(std::move(trigger_button))
        .content(std::make_unique<FixedWidget>(nandina::geometry::NanSize{180.0f, 80.0f}))
        .open(true);

    const auto preferred = popover->preferred_size();
    EXPECT_GE(preferred.height(), 80.0f);

    popover->measure(nandina::geometry::NanConstraints::tight(320.0f, 220.0f));
    popover->set_bounds(10.0f, 20.0f, 320.0f, 220.0f);
    popover->layout();

    const auto trigger = popover->trigger_bounds();
    const auto panel = popover->panel_bounds();
    EXPECT_GE(panel.x(), popover->x());
    EXPECT_GE(panel.y(), trigger.bottom());
    EXPECT_LE(panel.right(), popover->x() + popover->width());
    EXPECT_LE(panel.bottom(), popover->y() + popover->height());
}

TEST(WidgetsTooltipTest, HoverOpensAndLeaveClosesTooltip) {
    auto tooltip = nandina::widgets::Tooltip::create();
    auto trigger = nandina::widgets::Button::create();
    trigger->text("Hover me");
    tooltip->trigger(std::move(trigger));
    tooltip->text("Tooltip text");
    tooltip->measure(nandina::geometry::NanConstraints::tight(280.0f, 180.0f));
    tooltip->set_bounds(0.0f, 0.0f, 280.0f, 180.0f);
    tooltip->layout();

    const auto trigger_bounds = tooltip->trigger_bounds();
    EXPECT_FALSE(tooltip->open());
    EXPECT_TRUE(tooltip->dispatch_event(nandina::runtime::PointerMoveEvent{
        .x = trigger_bounds.x() + 4.0,
        .y = trigger_bounds.y() + 4.0,
        .delta_x = 0.0,
        .delta_y = 0.0,
    }));
    EXPECT_TRUE(tooltip->open());

    EXPECT_FALSE(tooltip->dispatch_pointer_leave(nandina::runtime::PointerMoveEvent{}));
    EXPECT_FALSE(tooltip->open());
}
