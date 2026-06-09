#include <gtest/gtest.h>

import nandina.foundation.color;
import nandina.runtime.nan_widget;
import nandina.widgets.icon;

TEST(WidgetsIconTest, DefaultsExposeStableGeometryAndColor) {
    auto icon = nandina::widgets::Icon::create();

    EXPECT_EQ(icon->type(), nandina::widgets::IconType::Square);
    EXPECT_FLOAT_EQ(icon->size(), 16.0f);

    const auto preferred = icon->preferred_size();
    EXPECT_FLOAT_EQ(preferred.width(), 16.0f);
    EXPECT_FLOAT_EQ(preferred.height(), 16.0f);

    const auto color = icon->color().to<nandina::NanRgb>();
    EXPECT_EQ(color.red(), 220u);
    EXPECT_EQ(color.green(), 220u);
    EXPECT_EQ(color.blue(), 240u);
}

TEST(WidgetsIconTest, TypeAndColorSettersMarkOnlyPaintDirty) {
    auto icon = nandina::widgets::Icon::create();
    icon->layout();
    icon->clear_dirty_recursive();
    EXPECT_FALSE(icon->dirty());
    EXPECT_FALSE(icon->is_layout_dirty());

    icon->set_type(nandina::widgets::IconType::Check);
    EXPECT_EQ(icon->type(), nandina::widgets::IconType::Check);
    EXPECT_TRUE(icon->dirty());
    EXPECT_FALSE(icon->is_layout_dirty());

    icon->clear_dirty_recursive();
    icon->set_color(nandina::NanColor::from(nandina::NanRgb{1, 2, 3}));
    const auto color = icon->color().to<nandina::NanRgb>();
    EXPECT_EQ(color.red(), 1u);
    EXPECT_EQ(color.green(), 2u);
    EXPECT_EQ(color.blue(), 3u);
    EXPECT_TRUE(icon->dirty());
    EXPECT_FALSE(icon->is_layout_dirty());
}

TEST(WidgetsIconTest, SizeSetterUpdatesPreferredSizeAndMarksLayoutDirty) {
    auto icon = nandina::widgets::Icon::create();
    icon->layout();
    icon->clear_dirty_recursive();

    icon->set_size(24.0f);

    EXPECT_FLOAT_EQ(icon->size(), 24.0f);
    EXPECT_TRUE(icon->dirty());
    EXPECT_TRUE(icon->is_layout_dirty());

    const auto preferred = icon->preferred_size();
    EXPECT_FLOAT_EQ(preferred.width(), 24.0f);
    EXPECT_FLOAT_EQ(preferred.height(), 24.0f);
}

TEST(WidgetsIconTest, AdditionalIconTypesAreStored) {
    auto icon = nandina::widgets::Icon::create();

    icon->set_type(nandina::widgets::IconType::ArrowDown);
    EXPECT_EQ(icon->type(), nandina::widgets::IconType::ArrowDown);

    icon->set_type(nandina::widgets::IconType::Dots);
    EXPECT_EQ(icon->type(), nandina::widgets::IconType::Dots);

    icon->set_type(nandina::widgets::IconType::Dot);
    EXPECT_EQ(icon->type(), nandina::widgets::IconType::Dot);
}
