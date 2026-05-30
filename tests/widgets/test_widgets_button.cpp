#include <gtest/gtest.h>

#include <memory>

import nandina.foundation.color;
import nandina.runtime.nan_event;
import nandina.theme;
import nandina.widgets.button;
import nandina.widgets.focus_ring;

namespace {

class ScopedStyleReset final {
public:
    ScopedStyleReset()
        : saved_(nandina::theme::NanStylePrimitives::current()) {
    }

    ~ScopedStyleReset() {
        nandina::theme::NanStylePrimitives::set_current(saved_);
    }

private:
    nandina::theme::NanStylePrimitives saved_;
};

auto find_focus_ring(nandina::widgets::Button& button) -> nandina::widgets::FocusRing* {
    for (auto& child : button.children()) {
        auto* focus_ring = dynamic_cast<nandina::widgets::FocusRing*>(child.get());
        if (focus_ring) {
            return focus_ring;
        }
    }
    return nullptr;
}

} // namespace

TEST(WidgetsButtonTest, ClickLifecycleStopsWhenDisabled) {
    auto button = nandina::widgets::Button::create();
    button->set_bounds(0.0f, 0.0f, 120.0f, 36.0f);

    int press_count = 0;
    int release_count = 0;
    int click_count = 0;

    button->on_press([&] { ++press_count; });
    button->on_release([&] { ++release_count; });
    button->on_click([&] { ++click_count; });

    EXPECT_TRUE(button->dispatch_event(nandina::runtime::PointerMoveEvent{
        .x = 8.0,
        .y = 6.0,
        .delta_x = 0.0,
        .delta_y = 0.0,
    }));
    EXPECT_TRUE(button->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 6.0,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_TRUE(button->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 6.0,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerUp));

    EXPECT_EQ(press_count, 1);
    EXPECT_EQ(release_count, 1);
    EXPECT_EQ(click_count, 1);

    auto* focus_ring = find_focus_ring(*button);
    ASSERT_NE(focus_ring, nullptr);
    EXPECT_TRUE(button->dispatch_event(nandina::runtime::FocusEvent{.got_focus = true}));
    EXPECT_TRUE(focus_ring->active());

    button->disabled(true);
    EXPECT_FALSE(focus_ring->active());

    EXPECT_FALSE(button->dispatch_event(nandina::runtime::PointerMoveEvent{
        .x = 8.0,
        .y = 6.0,
        .delta_x = 0.0,
        .delta_y = 0.0,
    }));
    EXPECT_FALSE(button->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 6.0,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerDown));
    EXPECT_FALSE(button->dispatch_event(
        nandina::runtime::PointerButtonEvent{
            .button = nandina::types::PointerButton::Left,
            .x = 8.0,
            .y = 6.0,
            .is_repeat = false,
        },
        nandina::runtime::EventType::PointerUp));

    EXPECT_EQ(press_count, 1);
    EXPECT_EQ(release_count, 1);
    EXPECT_EQ(click_count, 1);
}

TEST(WidgetsButtonTest, PresetAndSizeResolveFromThemeOverrides) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.button.corner_radius = 11.0f;
    style.button.md.font_size = 15.0f;
    style.button.md.padding_h = 18.0f;
    style.button.md.padding_v = 6.0f;

    style.button.filled.bg = nandina::NanColor::from(nandina::NanRgb{10, 20, 30});
    style.button.filled.text = nandina::NanColor::from(nandina::NanRgb{210, 220, 230});
    style.button.outlined.text = nandina::NanColor::from(nandina::NanRgb{1, 2, 3});
    style.button.outlined.border = nandina::NanColor::from(nandina::NanRgb{4, 5, 6});
    style.button.outlined.border_width = 2.0f;
    style.button.outlined.bg_disabled = nandina::NanColor::from(nandina::NanRgb{7, 8, 9});
    style.button.outlined.text_disabled = nandina::NanColor::from(nandina::NanRgb{11, 12, 13});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto button = nandina::widgets::Button::create();

    EXPECT_FLOAT_EQ(button->corner_radius(), 11.0f);
    EXPECT_FLOAT_EQ(button->font_size(), 15.0f);
    EXPECT_FLOAT_EQ(button->padding().left(), 18.0f);
    EXPECT_FLOAT_EQ(button->padding().top(), 6.0f);

    const auto filled_bg = button->bg_color().to<nandina::NanRgb>();
    EXPECT_EQ(filled_bg.red(), 10u);
    EXPECT_EQ(filled_bg.green(), 20u);
    EXPECT_EQ(filled_bg.blue(), 30u);

    const auto filled_text = button->font_color().to<nandina::NanRgb>();
    EXPECT_EQ(filled_text.red(), 210u);
    EXPECT_EQ(filled_text.green(), 220u);
    EXPECT_EQ(filled_text.blue(), 230u);

    button->variant(nandina::widgets::ButtonVariant::outline);
    EXPECT_FLOAT_EQ(button->border_width(), 2.0f);

    const auto outline_border = button->border_color().to<nandina::NanRgb>();
    EXPECT_EQ(outline_border.red(), 4u);
    EXPECT_EQ(outline_border.green(), 5u);
    EXPECT_EQ(outline_border.blue(), 6u);

    const auto outline_text = button->font_color().to<nandina::NanRgb>();
    EXPECT_EQ(outline_text.red(), 1u);
    EXPECT_EQ(outline_text.green(), 2u);
    EXPECT_EQ(outline_text.blue(), 3u);

    button->disabled(true);
    const auto outline_disabled_bg = button->bg_color().to<nandina::NanRgb>();
    EXPECT_EQ(outline_disabled_bg.red(), 7u);
    EXPECT_EQ(outline_disabled_bg.green(), 8u);
    EXPECT_EQ(outline_disabled_bg.blue(), 9u);

    const auto outline_disabled_text = button->font_color().to<nandina::NanRgb>();
    EXPECT_EQ(outline_disabled_text.red(), 11u);
    EXPECT_EQ(outline_disabled_text.green(), 12u);
    EXPECT_EQ(outline_disabled_text.blue(), 13u);
}
