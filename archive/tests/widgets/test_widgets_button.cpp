#include <gtest/gtest.h>

#include <memory>
#include <vector>

import nandina.foundation.color;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.theme;
import nandina.widgets.button;
import nandina.widgets.focus_ring;
import nandina.widgets.icon;
import nandina.widgets.sidebar_menu_button;

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

auto find_icons(nandina::runtime::NanWidget& root) -> std::vector<nandina::widgets::Icon*> {
    std::vector<nandina::widgets::Icon*> icons;
    for (auto& child : root.children()) {
        if (auto* icon = dynamic_cast<nandina::widgets::Icon*>(child.get())) {
            icons.push_back(icon);
        }
        auto nested = find_icons(*child);
        icons.insert(icons.end(), nested.begin(), nested.end());
    }
    return icons;
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

TEST(WidgetsButtonTest, LeftAndRightIconsUseIndependentSlotsAndFollowResolvedColor) {
    auto button = nandina::widgets::Button::create();
    button->text("Save")
        .icon_left(nandina::widgets::IconType::Check)
        .icon_right(nandina::widgets::IconType::ArrowDown);

    ASSERT_TRUE(button->left_icon_type().has_value());
    ASSERT_TRUE(button->right_icon_type().has_value());
    EXPECT_EQ(*button->left_icon_type(), nandina::widgets::IconType::Check);
    EXPECT_EQ(*button->right_icon_type(), nandina::widgets::IconType::ArrowDown);

    const auto icons = find_icons(*button);
    ASSERT_EQ(icons.size(), 2u);
    EXPECT_EQ(icons[0]->type(), nandina::widgets::IconType::Check);
    EXPECT_EQ(icons[1]->type(), nandina::widgets::IconType::ArrowDown);

    const auto text_color = button->font_color().to<nandina::NanRgb>();
    const auto left_color = icons[0]->color().to<nandina::NanRgb>();
    const auto right_color = icons[1]->color().to<nandina::NanRgb>();
    EXPECT_EQ(left_color.red(), text_color.red());
    EXPECT_EQ(left_color.green(), text_color.green());
    EXPECT_EQ(left_color.blue(), text_color.blue());
    EXPECT_EQ(right_color.red(), text_color.red());
    EXPECT_EQ(right_color.green(), text_color.green());
    EXPECT_EQ(right_color.blue(), text_color.blue());

    button->variant(nandina::widgets::ButtonVariant::outline);
    const auto outline_text = button->font_color().to<nandina::NanRgb>();
    const auto updated_left = icons[0]->color().to<nandina::NanRgb>();
    const auto updated_right = icons[1]->color().to<nandina::NanRgb>();
    EXPECT_EQ(updated_left.red(), outline_text.red());
    EXPECT_EQ(updated_left.green(), outline_text.green());
    EXPECT_EQ(updated_left.blue(), outline_text.blue());
    EXPECT_EQ(updated_right.red(), outline_text.red());
    EXPECT_EQ(updated_right.green(), outline_text.green());
    EXPECT_EQ(updated_right.blue(), outline_text.blue());
}

TEST(WidgetsButtonTest, ColorVariantSelectsSemanticButtonFamily) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.button.primary_family.outlined.text = nandina::NanColor::from(nandina::NanRgb{10, 20, 30});
    style.button.primary_family.outlined.border = nandina::NanColor::from(nandina::NanRgb{11, 21, 31});
    style.button.secondary_family.outlined.text = nandina::NanColor::from(nandina::NanRgb{40, 50, 60});
    style.button.secondary_family.outlined.border = nandina::NanColor::from(nandina::NanRgb{41, 51, 61});
    style.button.destructive_family.outlined.text = nandina::NanColor::from(nandina::NanRgb{70, 80, 90});
    style.button.destructive_family.outlined.border = nandina::NanColor::from(nandina::NanRgb{71, 81, 91});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto button = nandina::widgets::Button::create();
    button->variant(nandina::widgets::ButtonVariant::outline);

    button->color_variant(nandina::theme::ColorVariant::primary);
    auto text = button->font_color().to<nandina::NanRgb>();
    auto border = button->border_color().to<nandina::NanRgb>();
    EXPECT_EQ(text.red(), 10u);
    EXPECT_EQ(text.green(), 20u);
    EXPECT_EQ(text.blue(), 30u);
    EXPECT_EQ(border.red(), 11u);
    EXPECT_EQ(border.green(), 21u);
    EXPECT_EQ(border.blue(), 31u);

    button->color_variant(nandina::theme::ColorVariant::secondary);
    text = button->font_color().to<nandina::NanRgb>();
    border = button->border_color().to<nandina::NanRgb>();
    EXPECT_EQ(text.red(), 40u);
    EXPECT_EQ(text.green(), 50u);
    EXPECT_EQ(text.blue(), 60u);
    EXPECT_EQ(border.red(), 41u);
    EXPECT_EQ(border.green(), 51u);
    EXPECT_EQ(border.blue(), 61u);

    button->color_variant(nandina::theme::ColorVariant::destructive);
    text = button->font_color().to<nandina::NanRgb>();
    border = button->border_color().to<nandina::NanRgb>();
    EXPECT_EQ(text.red(), 70u);
    EXPECT_EQ(text.green(), 80u);
    EXPECT_EQ(text.blue(), 90u);
    EXPECT_EQ(border.red(), 71u);
    EXPECT_EQ(border.green(), 81u);
    EXPECT_EQ(border.blue(), 91u);
}

TEST(WidgetsButtonTest, SidebarMenuButtonActiveStateUsesSemanticAccentColor) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.button.secondary_family.ghost.text = nandina::NanColor::from(nandina::NanRgb{31, 41, 51});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto button = nandina::widgets::SidebarMenuButton::create();
    button->color_variant(nandina::theme::ColorVariant::secondary);
    button->active(true);

    const auto accent = button->accent_color().to<nandina::NanRgb>();
    const auto text = button->font_color().to<nandina::NanRgb>();
    EXPECT_EQ(accent.red(), 31u);
    EXPECT_EQ(accent.green(), 41u);
    EXPECT_EQ(accent.blue(), 51u);
    EXPECT_EQ(text.red(), 31u);
    EXPECT_EQ(text.green(), 41u);
    EXPECT_EQ(text.blue(), 51u);
}
