#include <gtest/gtest.h>

import nandina.foundation.color;
import nandina.foundation.nan_constraints;
import nandina.theme;
import nandina.widgets.label;

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

TEST(WidgetsLabelTest, SemanticStatesResolveThemeOverridesAndRequiredWidth) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.label.font_color = nandina::NanColor::from(nandina::NanRgb{12, 34, 56});
    style.label.error_font_color = nandina::NanColor::from(nandina::NanRgb{230, 69, 83});
    style.label.disabled_font_color = nandina::NanColor::from(nandina::NanRgb{154, 157, 180});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto label = nandina::widgets::Label::create();
    label->set_text("Password");

    const auto normal = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(normal.red(), 12u);
    EXPECT_EQ(normal.green(), 34u);
    EXPECT_EQ(normal.blue(), 56u);

    const auto normal_width = label->preferred_size().width();
    label->set_required(true);
    EXPECT_GT(label->preferred_size().width(), normal_width);

    label->set_error(true);
    const auto error = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(error.red(), 230u);
    EXPECT_EQ(error.green(), 69u);
    EXPECT_EQ(error.blue(), 83u);

    label->set_error(false);
    label->set_disabled(true);
    const auto disabled = label->color().to<nandina::NanRgb>();
    EXPECT_EQ(disabled.red(), 154u);
    EXPECT_EQ(disabled.green(), 157u);
    EXPECT_EQ(disabled.blue(), 180u);
}

TEST(WidgetsLabelTest, WrappedMeasureUsesConstraintWidth) {
    auto label = nandina::widgets::Label::create();
    label->set_text("Nandina UI layout wrapping verifies measured height");
    label->set_font_size(9.0f);

    label->measure(nandina::geometry::NanConstraints{0.0f, 240.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    const auto wide_size = label->measured_size();

    label->measure(nandina::geometry::NanConstraints{0.0f, 72.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    const auto narrow_size = label->measured_size();

    EXPECT_GT(wide_size.width(), 0.0f);
    EXPECT_GT(wide_size.height(), 0.0f);
    EXPECT_GT(narrow_size.height(), wide_size.height());
}
