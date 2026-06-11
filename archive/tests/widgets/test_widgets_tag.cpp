#include <gtest/gtest.h>

import nandina.foundation.color;
import nandina.theme;
import nandina.widgets.text;
import nandina.widgets.tag;

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

auto find_label(nandina::widgets::Tag& tag) -> nandina::widgets::Text* {
    for (auto& child : tag.children()) {
        if (auto* label = dynamic_cast<nandina::widgets::Text*>(child.get())) {
            return label;
        }
    }
    return nullptr;
}

} // namespace

TEST(WidgetsTagTest, DefaultsResolveFromThemeStyle) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.tag.corner_radius = 18.0f;
    style.tag.border_width = 2.0f;
    style.tag.bg = nandina::NanColor::from(nandina::NanRgb{10, 20, 30});
    style.tag.text = nandina::NanColor::from(nandina::NanRgb{40, 50, 60});
    style.tag.border = nandina::NanColor::from(nandina::NanRgb{70, 80, 90});
    style.tag.md.font_size = 13.0f;
    style.tag.md.padding_h = 14.0f;
    style.tag.md.padding_v = 6.0f;
    nandina::theme::NanStylePrimitives::set_current(style);

    auto tag = nandina::widgets::Tag::create();
    tag->text("Beta");

    auto* label = find_label(*tag);
    ASSERT_NE(label, nullptr);

    EXPECT_FLOAT_EQ(tag->corner_radius(), 18.0f);
    EXPECT_FLOAT_EQ(tag->border_width(), 2.0f);
    EXPECT_FLOAT_EQ(tag->padding().left(), 14.0f);
    EXPECT_FLOAT_EQ(tag->padding().top(), 6.0f);
    EXPECT_FLOAT_EQ(label->font_size(), 13.0f);

    const auto bg = tag->bg_color().to<nandina::NanRgb>();
    const auto border = tag->border_color().to<nandina::NanRgb>();
    const auto text = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 10u);
    EXPECT_EQ(bg.green(), 20u);
    EXPECT_EQ(bg.blue(), 30u);
    EXPECT_EQ(border.red(), 70u);
    EXPECT_EQ(border.green(), 80u);
    EXPECT_EQ(border.blue(), 90u);
    EXPECT_EQ(text.red(), 40u);
    EXPECT_EQ(text.green(), 50u);
    EXPECT_EQ(text.blue(), 60u);
}

TEST(WidgetsTagTest, ColorVariantSelectsSemanticFamily) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.tag.secondary_family.bg = nandina::NanColor::from(nandina::NanRgb{11, 21, 31});
    style.tag.secondary_family.text = nandina::NanColor::from(nandina::NanRgb{12, 22, 32});
    style.tag.secondary_family.border = nandina::NanColor::from(nandina::NanRgb{13, 23, 33});
    style.tag.destructive_family.bg = nandina::NanColor::from(nandina::NanRgb{41, 51, 61});
    style.tag.destructive_family.text = nandina::NanColor::from(nandina::NanRgb{42, 52, 62});
    style.tag.destructive_family.border = nandina::NanColor::from(nandina::NanRgb{43, 53, 63});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto tag = nandina::widgets::Tag::create();
    auto* label = find_label(*tag);
    ASSERT_NE(label, nullptr);

    tag->color_variant(nandina::theme::ColorVariant::secondary);
    auto bg = tag->bg_color().to<nandina::NanRgb>();
    auto border = tag->border_color().to<nandina::NanRgb>();
    auto text = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 11u);
    EXPECT_EQ(bg.green(), 21u);
    EXPECT_EQ(bg.blue(), 31u);
    EXPECT_EQ(border.red(), 13u);
    EXPECT_EQ(border.green(), 23u);
    EXPECT_EQ(border.blue(), 33u);
    EXPECT_EQ(text.red(), 12u);
    EXPECT_EQ(text.green(), 22u);
    EXPECT_EQ(text.blue(), 32u);

    tag->color_variant(nandina::theme::ColorVariant::destructive);
    bg = tag->bg_color().to<nandina::NanRgb>();
    border = tag->border_color().to<nandina::NanRgb>();
    text = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 41u);
    EXPECT_EQ(bg.green(), 51u);
    EXPECT_EQ(bg.blue(), 61u);
    EXPECT_EQ(border.red(), 43u);
    EXPECT_EQ(border.green(), 53u);
    EXPECT_EQ(border.blue(), 63u);
    EXPECT_EQ(text.red(), 42u);
    EXPECT_EQ(text.green(), 52u);
    EXPECT_EQ(text.blue(), 62u);
}

TEST(WidgetsTagTest, DisabledUsesFamilyDisabledColors) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.tag.secondary_family.bg_disabled = nandina::NanColor::from(nandina::NanRgb{91, 92, 93});
    style.tag.secondary_family.text_disabled = nandina::NanColor::from(nandina::NanRgb{94, 95, 96});
    style.tag.secondary_family.border_disabled = nandina::NanColor::from(nandina::NanRgb{97, 98, 99});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto tag = nandina::widgets::Tag::create();
    auto* label = find_label(*tag);
    ASSERT_NE(label, nullptr);

    tag->color_variant(nandina::theme::ColorVariant::secondary)
        .disabled(true);

    const auto bg = tag->bg_color().to<nandina::NanRgb>();
    const auto border = tag->border_color().to<nandina::NanRgb>();
    const auto text = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 91u);
    EXPECT_EQ(bg.green(), 92u);
    EXPECT_EQ(bg.blue(), 93u);
    EXPECT_EQ(border.red(), 97u);
    EXPECT_EQ(border.green(), 98u);
    EXPECT_EQ(border.blue(), 99u);
    EXPECT_EQ(text.red(), 94u);
    EXPECT_EQ(text.green(), 95u);
    EXPECT_EQ(text.blue(), 96u);
}