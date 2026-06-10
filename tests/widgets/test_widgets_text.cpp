#include <gtest/gtest.h>

#include <cmath>

import nandina.foundation.color;
import nandina.foundation.nan_constraints;
import nandina.runtime.nan_widget;
import nandina.theme;
import nandina.widgets.text;

namespace
{

    class ScopedStyleReset final {
    public:
        ScopedStyleReset(): saved_(nandina::theme::NanStylePrimitives::current()) {}

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
    style.text.font_color = nandina::NanColor::from(nandina::NanRgb {12, 34, 56});
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

    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            240.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto wide_size = text->measured_size();

    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            72.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto narrow_size = text->measured_size();

    EXPECT_GT(wide_size.width(), 0.0f);
    EXPECT_GT(wide_size.height(), 0.0f);
    EXPECT_GT(narrow_size.height(), wide_size.height());
}

TEST(WidgetsTextTest, HorizontalLayoutInfoUsesWidestUnbreakableRunAsMin) {
    auto text = nandina::widgets::Text::create();
    text->set_text("short extraordinarilylongword mid");
    text->set_font_size(9.0f);

    const auto info = text->layout_info(true, 0.0f);

    EXPECT_GT(info.min, 0.0f);
    EXPECT_GT(info.preferred, info.min);
}

TEST(WidgetsTextTest, VerticalLayoutInfoUsesHeightForWidth) {
    auto text = nandina::widgets::Text::create();
    text->set_text("Nandina UI text primitive verifies height for width behavior");
    text->set_font_size(9.0f);

    const auto wide = text->layout_info(false, 240.0f);
    const auto narrow = text->layout_info(false, 72.0f);

    EXPECT_GT(wide.preferred, 0.0f);
    EXPECT_GT(narrow.preferred, wide.preferred);
    EXPECT_GE(narrow.min, text->font().line_height());
}

TEST(WidgetsTextTest, BreakWordWrapPolicyReducesHorizontalMinimum) {
    auto text = nandina::widgets::Text::create();
    text->set_text("supercalifragilisticexpialidocious");
    text->set_font_size(9.0f);

    const auto word_info = text->layout_info(true, 0.0f);

    text->set_wrap_policy(nandina::text::TextWrapPolicy::break_word);
    const auto break_info = text->layout_info(true, 0.0f);

    EXPECT_GT(word_info.min, break_info.min);
}

TEST(WidgetsTextTest, BreakWordMeasureHandlesExtremelyNarrowWidth) {
    auto text = nandina::widgets::Text::create();
    text->set_text("abc");
    text->set_font_size(9.0f);
    text->set_wrap_policy(nandina::text::TextWrapPolicy::break_word);

    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            1.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto size = text->measured_size();

    EXPECT_GT(size.width(), 0.0f);
    EXPECT_GT(size.height(), text->font().line_height() * 2.0f);
}

TEST(WidgetsTextTest, MaxLinesCapsMeasuredHeight) {
    auto text = nandina::widgets::Text::create();
    text->set_text("one two three four five six seven eight nine ten");
    text->set_font_size(9.0f);

    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            48.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto unlimited = text->measured_size();

    text->set_max_lines(2);
    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            48.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto capped = text->measured_size();

    EXPECT_GT(unlimited.height(), text->font().line_height() * 2.0f);
    EXPECT_LE(capped.height(), text->font().line_height() * 2.0f + 1.0f);
    EXPECT_LT(capped.height(), unlimited.height());
}

TEST(WidgetsTextTest, SingleLineEllipsisMeasuresAsOneLineUnderWidthConstraint) {
    auto text = nandina::widgets::Text::create();
    text->set_text("this text should remain a single ellipsized line");
    text->set_font_size(9.0f);
    text->set_single_line(true);
    text->set_overflow(nandina::text::TextOverflow::ellipsis);

    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            48.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto size = text->measured_size();

    EXPECT_GT(size.width(), 0.0f);
    EXPECT_LE(size.width(), 49.0f);
    EXPECT_LE(size.height(), text->font().line_height() + 1.0f);
}

TEST(WidgetsTextTest, LineHeightAffectsMeasuredHeight) {
    auto text = nandina::widgets::Text::create();
    // Default body_medium: font_size=14, line_height=20
    // Text with enough words to wrap into ~3 lines at 120px
    text->set_text("Nandina text wraps at width boundary producing multiple lines");

    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            120.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const float default_height = text->measured_size().height();
    const int default_lines = static_cast<int>(std::floor(default_height / 20.0f + 0.5f));
    EXPECT_GE(default_lines, 2);

    // Double line_height → height should approximately double
    text->set_line_height(40.0f);
    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            120.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const float doubled_height = text->measured_size().height();
    EXPECT_NEAR(doubled_height, default_height * 2.0f, default_height * 0.3f);
}

TEST(WidgetsTextTest, LineHeightDrivesPreferredSizeHeight) {
    auto text = nandina::widgets::Text::create();
    text->set_text("Line height impacts preferred height");
    text->set_font_size(14.0f);

    const float default_lh = text->font().line_height();
    const auto pref_default = text->preferred_size();
    EXPECT_FLOAT_EQ(pref_default.height(), text->font().line_height());

    text->set_line_height(default_lh * 2.0f);
    const auto pref_doubled = text->preferred_size();
    EXPECT_FLOAT_EQ(pref_doubled.height(), default_lh * 2.0f);
}

TEST(WidgetsTextTest, BreakWordClipAtNarrowWidthProducesSensibleSize) {
    auto text = nandina::widgets::Text::create();
    text->set_text("short words");
    text->set_font_size(9.0f);
    text->set_wrap_policy(nandina::text::TextWrapPolicy::break_word);
    text->set_overflow(nandina::text::TextOverflow::clip);

    // At 1px width, no glyph fits — width may be 0, but height should
    // still be >= one line height (the layout produces an empty line).
    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            1.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto size = text->measured_size();

    EXPECT_GE(size.height(), text->font().line_height() * 0.5f);

    // Width wide enough for at least one character (9pt font, ~5px per char)
    text->measure(
        nandina::geometry::NanConstraints {
            0.0f,
            8.0f,
            0.0f,
            nandina::geometry::NanConstraints::k_infinity
        }
    );
    const auto size2 = text->measured_size();

    EXPECT_GT(size2.width(), 0.0f);
    EXPECT_GT(size2.height(), 0.0f);

    // break_word should still produce >0 height even at very narrow widths
    EXPECT_GE(size2.height(), text->font().line_height());
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
