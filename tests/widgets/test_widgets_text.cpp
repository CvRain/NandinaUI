#include <gtest/gtest.h>

import nandina.foundation.color;
import nandina.foundation.nan_constraints;
import nandina.theme;
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

} // namespace

TEST(WidgetsTextTest, DefaultsResolveFromThemeTextStyle) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.text.font_color = nandina::NanColor::from(nandina::NanRgb{12, 34, 56});
    style.text.max_lines = 2;
    style.text.single_line = false;
    nandina::theme::NanStylePrimitives::set_current(style);

    auto text = nandina::widgets::Text::create();
    text->set_text("Overview");

    const auto resolved = text->color().to<nandina::NanRgb>();
    EXPECT_EQ(resolved.red(), 12u);
    EXPECT_EQ(resolved.green(), 34u);
    EXPECT_EQ(resolved.blue(), 56u);
    EXPECT_EQ(text->font().max_lines(), 2);
    EXPECT_FALSE(text->font().single_line());
}

TEST(WidgetsTextTest, WrappedMeasureUsesConstraintWidth) {
    auto text = nandina::widgets::Text::create();
    text->set_text("Nandina UI text primitive verifies measured height");
    text->set_font_size(9.0f);

    text->measure(nandina::geometry::NanConstraints{0.0f, 240.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    const auto wide_size = text->measured_size();

    text->measure(nandina::geometry::NanConstraints{0.0f, 72.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    const auto narrow_size = text->measured_size();

    EXPECT_GT(wide_size.width(), 0.0f);
    EXPECT_GT(wide_size.height(), 0.0f);
    EXPECT_GT(narrow_size.height(), wide_size.height());
}

TEST(WidgetsTextTest, TypographyRoleAppliesResolvedTypeStyle) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.typography.title_large.font_size = 26.0f;
    style.typography.title_large.font_weight = nandina::theme::NanFontWeight::bold;
    style.typography.title_large.line_height = 34.0f;
    style.typography.title_large.letter_spacing = 0.75f;
    nandina::theme::NanStylePrimitives::set_current(style);

    auto text = nandina::widgets::Text::create();   
    text->set_typography_role(nandina::theme::NanTypographyRole::title_large);

    ASSERT_TRUE(text->typography_role().has_value());
    EXPECT_EQ(*text->typography_role(), nandina::theme::NanTypographyRole::title_large);
    EXPECT_FLOAT_EQ(text->font_size(), 26.0f);
    EXPECT_EQ(text->font_weight(), nandina::text::NanFontWeight::bold);
    EXPECT_FLOAT_EQ(text->font().line_height(), 34.0f);
    EXPECT_FLOAT_EQ(text->font().letter_spacing(), 0.75f);
}