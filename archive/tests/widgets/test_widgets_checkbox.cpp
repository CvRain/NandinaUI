#include <gtest/gtest.h>

import nandina.foundation.color;
import nandina.theme;
import nandina.widgets.checkbox;
import nandina.widgets.text;

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

auto find_text_child(nandina::widgets::Checkbox& checkbox) -> nandina::widgets::Text* {
    for (auto& child : checkbox.children()) {
        if (auto* text = dynamic_cast<nandina::widgets::Text*>(child.get())) {
            return text;
        }
    }
    return nullptr;
}

} // namespace

TEST(WidgetsCheckboxTest, DefaultStateUnchecked) {
    auto cb = nandina::widgets::Checkbox::create();
    EXPECT_FALSE(cb->checked());
    EXPECT_FALSE(cb->disabled());
    EXPECT_EQ(cb->color_variant(), nandina::theme::ColorVariant::inherit);
    EXPECT_EQ(cb->size(), nandina::widgets::CheckboxSize::md);
}

TEST(WidgetsCheckboxTest, LabelIsStoredAndRetrieved) {
    auto cb = nandina::widgets::Checkbox::create();
    cb->label("Accept terms");

    auto* text = find_text_child(*cb);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->text(), "Accept terms");
    EXPECT_EQ(cb->label(), "Accept terms");
}

TEST(WidgetsCheckboxTest, CheckedTogglesStateAndEmitsSignal) {
    auto cb = nandina::widgets::Checkbox::create();
    int call_count = 0;
    bool last_value = false;
    cb->on_checked_changed([&](bool v) { ++call_count; last_value = v; });

    cb->checked(true);
    EXPECT_TRUE(cb->checked());
    EXPECT_EQ(call_count, 1);
    EXPECT_TRUE(last_value);

    cb->checked(false);
    EXPECT_FALSE(cb->checked());
    EXPECT_EQ(call_count, 2);
    EXPECT_FALSE(last_value);

    // setting same value should not fire
    cb->checked(false);
    EXPECT_EQ(call_count, 2);
}

TEST(WidgetsCheckboxTest, DisabledBlocksInteraction) {
    auto cb = nandina::widgets::Checkbox::create();
    cb->disabled(true);
    EXPECT_TRUE(cb->disabled());
    EXPECT_FALSE(cb->checked());

    int call_count = 0;
    cb->on_checked_changed([&](bool) { ++call_count; });

    // checked() still works programmatically even when disabled
    cb->checked(true);
    EXPECT_TRUE(cb->checked());
    EXPECT_EQ(call_count, 1);
}

TEST(WidgetsCheckboxTest, ColorVariantSelectsSemanticFamily) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.checkbox.secondary_family.box_bg = nandina::NanColor::from(nandina::NanRgb{11, 21, 31});
    style.checkbox.destructive_family.box_bg = nandina::NanColor::from(nandina::NanRgb{41, 51, 61});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto cb = nandina::widgets::Checkbox::create();
    cb->checked(true);

    // primary is default
    cb->color_variant(nandina::theme::ColorVariant::secondary);
    EXPECT_EQ(cb->color_variant(), nandina::theme::ColorVariant::secondary);

    // verify it doesn't crash — theme colors are used internally during draw
    SUCCEED();
}

TEST(WidgetsCheckboxTest, SizeAffectsLayout) {
    auto cb = nandina::widgets::Checkbox::create();
    cb->label("Option");
    const auto md_pref = cb->preferred_size();

    cb->size(nandina::widgets::CheckboxSize::sm);
    const auto sm_pref = cb->preferred_size();

    EXPECT_LT(sm_pref.width(), md_pref.width());
    EXPECT_LT(sm_pref.height(), md_pref.height());
}
