#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <thorvg-1/thorvg.h>

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

class ThorvgCanvasScope {
public:
    ThorvgCanvasScope(const std::uint32_t width, const std::uint32_t height)
        : pixels_(width * height, 0u), width_(width) {
        if (tvg::Initializer::init(0u) != tvg::Result::Success) {
            throw std::runtime_error("tvg::Initializer::init failed");
        }
        canvas_.reset(tvg::SwCanvas::gen());
        if (!canvas_) {
            tvg::Initializer::term();
            throw std::runtime_error("tvg::SwCanvas::gen() returned nullptr");
        }
        if (canvas_->target(
                pixels_.data(),
                width,
                width,
                height,
                tvg::ColorSpace::ARGB8888) != tvg::Result::Success) {
            canvas_.reset();
            tvg::Initializer::term();
            throw std::runtime_error("tvg::SwCanvas::target failed");
        }
    }

    ~ThorvgCanvasScope() {
        canvas_.reset();
        tvg::Initializer::term();
    }

    auto canvas() const noexcept -> tvg::SwCanvas& {
        return *canvas_;
    }

    auto render() const -> void {
        ASSERT_EQ(canvas_->draw(true), tvg::Result::Success);
        ASSERT_EQ(canvas_->sync(), tvg::Result::Success);
    }

    [[nodiscard]] auto pixel_at(const std::uint32_t x, const std::uint32_t y) const noexcept -> std::uint32_t {
        return pixels_.at(static_cast<std::size_t>(y) * width_ + x);
    }

private:
    std::vector<std::uint32_t> pixels_;
    std::unique_ptr<tvg::SwCanvas> canvas_;
    std::uint32_t width_;
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

TEST(WidgetsLabelTest, BreakWordMeasureHandlesExtremelyNarrowWidth) {
    auto label = nandina::widgets::Label::create();
    label->set_text("abc");
    label->set_font_size(9.0f);
    label->set_wrap_policy(nandina::text::TextWrapPolicy::break_word);

    label->measure(nandina::geometry::NanConstraints{0.0f, 1.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    const auto size = label->measured_size();

    EXPECT_GT(size.width(), 0.0f);
    EXPECT_GT(size.height(), label->font().line_height() * 2.0f);
}

TEST(WidgetsLabelTest, TypographyRoleAppliesResolvedTypeStyle) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.typography.title_large.font_size = 26.0f;
    style.typography.title_large.font_weight = nandina::theme::NanFontWeight::bold;
    style.typography.title_large.line_height = 34.0f;
    style.typography.title_large.letter_spacing = 0.75f;
    nandina::theme::NanStylePrimitives::set_current(style);

    auto label = nandina::widgets::Label::create();
    label->set_typography_role(nandina::theme::NanTypographyRole::title_large);

    ASSERT_TRUE(label->typography_role().has_value());
    EXPECT_EQ(*label->typography_role(), nandina::theme::NanTypographyRole::title_large);
    EXPECT_FLOAT_EQ(label->font_size(), 26.0f);
    EXPECT_EQ(label->font_weight(), nandina::text::NanFontWeight::bold);
    EXPECT_FLOAT_EQ(label->font().line_height(), 34.0f);
    EXPECT_FLOAT_EQ(label->font().letter_spacing(), 0.75f);
}

TEST(WidgetsLabelTest, DrawClipsWrappedTextToAssignedBounds) {
    auto label = nandina::widgets::Label::create();
    label->set_text("Wrapped text should not bleed below the assigned label bounds when the layout produces more lines than the visible region can hold.");
    label->set_font_size(11.0f);

    label->measure(nandina::geometry::NanConstraints{0.0f, 120.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    label->set_bounds(8.0f, 8.0f, 120.0f, 20.0f);
    label->layout();

    ThorvgCanvasScope canvas_scope{160u, 64u};
    label->draw(canvas_scope.canvas());
    canvas_scope.render();

    for (std::uint32_t y = 32; y < 64; ++y) {
        for (std::uint32_t x = 0; x < 160; ++x) {
            EXPECT_EQ(canvas_scope.pixel_at(x, y), 0u);
        }
    }
}
