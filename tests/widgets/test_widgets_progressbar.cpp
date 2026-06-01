#include <gtest/gtest.h>

import nandina.foundation.color;
import nandina.theme;
import nandina.widgets.progressbar;

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

TEST(WidgetsProgressBarTest, ColorVariantSelectsSemanticFamily) {
    ScopedStyleReset style_reset;

    auto style = nandina::theme::NanStylePrimitives::default_style();
    style.progress.secondary_family.track_bg = nandina::NanColor::from(nandina::NanRgb{10, 20, 30});
    style.progress.secondary_family.fill = nandina::NanColor::from(nandina::NanRgb{40, 50, 60});
    style.progress.destructive_family.track_bg = nandina::NanColor::from(nandina::NanRgb{70, 80, 90});
    style.progress.destructive_family.fill = nandina::NanColor::from(nandina::NanRgb{100, 110, 120});
    nandina::theme::NanStylePrimitives::set_current(style);

    auto progress = nandina::widgets::ProgressBar::create();
    progress->color_variant(nandina::theme::ColorVariant::secondary);

    auto track = progress->track_color().to<nandina::NanRgb>();
    auto fill = progress->bar_color().to<nandina::NanRgb>();
    EXPECT_EQ(track.red(), 10u);
    EXPECT_EQ(track.green(), 20u);
    EXPECT_EQ(track.blue(), 30u);
    EXPECT_EQ(fill.red(), 40u);
    EXPECT_EQ(fill.green(), 50u);
    EXPECT_EQ(fill.blue(), 60u);

    progress->color_variant(nandina::theme::ColorVariant::destructive);
    track = progress->track_color().to<nandina::NanRgb>();
    fill = progress->bar_color().to<nandina::NanRgb>();
    EXPECT_EQ(track.red(), 70u);
    EXPECT_EQ(track.green(), 80u);
    EXPECT_EQ(track.blue(), 90u);
    EXPECT_EQ(fill.red(), 100u);
    EXPECT_EQ(fill.green(), 110u);
    EXPECT_EQ(fill.blue(), 120u);
}

TEST(WidgetsProgressBarTest, ExplicitColorsOverrideSemanticVariantChanges) {
    auto progress = nandina::widgets::ProgressBar::create();
    progress->set_bar_color(nandina::NanColor::from(nandina::NanRgb{1, 2, 3}));
    progress->set_track_color(nandina::NanColor::from(nandina::NanRgb{4, 5, 6}));
    progress->color_variant(nandina::theme::ColorVariant::destructive);

    const auto fill = progress->bar_color().to<nandina::NanRgb>();
    const auto track = progress->track_color().to<nandina::NanRgb>();
    EXPECT_EQ(fill.red(), 1u);
    EXPECT_EQ(fill.green(), 2u);
    EXPECT_EQ(fill.blue(), 3u);
    EXPECT_EQ(track.red(), 4u);
    EXPECT_EQ(track.green(), 5u);
    EXPECT_EQ(track.blue(), 6u);
}