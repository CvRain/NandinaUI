#include <gtest/gtest.h>

#include <memory>

import nandina.foundation.nan_constraints;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.widgets.button;
import nandina.widgets.dialog;

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

auto make_trigger(std::string_view text) -> nandina::widgets::Button::Ptr {
    auto button = nandina::widgets::Button::create();
    button->text(text);
    return button;
}

} // namespace

TEST(WidgetsDialogTest, TriggerClickOpensDialogAndEmitsSignal) {
    auto dialog = nandina::widgets::Dialog::create();
    dialog->trigger(make_trigger("Open"))
        .content(std::make_unique<FixedWidget>(nandina::geometry::NanSize{240.0f, 120.0f}));
    dialog->measure(nandina::geometry::NanConstraints::tight(640.0f, 420.0f));
    dialog->set_bounds(0.0f, 0.0f, 640.0f, 420.0f);
    dialog->layout();

    int changes = 0;
    bool last_open = false;
    dialog->on_open_changed([&](const bool open) {
        ++changes;
        last_open = open;
    });

    EXPECT_FALSE(dialog->open());
    EXPECT_TRUE(dialog->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 10.0,
            .y = 10.0,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(dialog->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 10.0,
            .y = 10.0,
        },
        nandina::runtime::EventType::PointerUp));

    EXPECT_TRUE(dialog->open());
    EXPECT_EQ(changes, 1);
    EXPECT_TRUE(last_open);
}

TEST(WidgetsDialogTest, EscapeAndBackdropCloseDialog) {
    auto dialog = nandina::widgets::Dialog::create();
    dialog->trigger(make_trigger("Open"))
        .content(std::make_unique<FixedWidget>(nandina::geometry::NanSize{240.0f, 120.0f}))
        .open(true);
    dialog->measure(nandina::geometry::NanConstraints::tight(640.0f, 420.0f));
    dialog->set_bounds(0.0f, 0.0f, 640.0f, 420.0f);
    dialog->layout();

    EXPECT_TRUE(dialog->dispatch_event(nandina::runtime::KeyEvent{.key_code = 27}, nandina::runtime::EventType::KeyDown));
    EXPECT_FALSE(dialog->open());

    dialog->open(true);
    dialog->layout();
    EXPECT_TRUE(dialog->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 12.0,
            .y = 12.0,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(dialog->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 12.0,
            .y = 12.0,
        },
        nandina::runtime::EventType::PointerUp));
    EXPECT_FALSE(dialog->open());
}

TEST(WidgetsDialogTest, LayoutCentersPanelAndHidesTriggerWhileOpen) {
    auto dialog = nandina::widgets::Dialog::create();
    dialog->trigger(make_trigger("Open"))
        .content(std::make_unique<FixedWidget>(nandina::geometry::NanSize{260.0f, 140.0f}))
        .open(true);

    const auto preferred = dialog->preferred_size();
    EXPECT_GE(preferred.width(), 260.0f);
    EXPECT_GE(preferred.height(), 140.0f);

    dialog->measure(nandina::geometry::NanConstraints::tight(640.0f, 420.0f));
    dialog->set_bounds(20.0f, 30.0f, 640.0f, 420.0f);
    dialog->layout();

    const auto panel = dialog->panel_bounds();
    EXPECT_GT(panel.width(), 0.0f);
    EXPECT_GT(panel.height(), 0.0f);
    EXPECT_NEAR(panel.center().x(), dialog->bounds().center().x(), 1.0f);
    EXPECT_NEAR(panel.center().y(), dialog->bounds().center().y(), 1.0f);
    EXPECT_FALSE(dialog->children()[1]->visible());
}
