//
// Created by cvrain on 2026/4/28.
//
// NandinaUI — Theme 类型系统单元测试
// 覆盖：
//   - NanColorRole 枚举值完整性与转化
//   - NanColorScheme 枚举
//   - NanColorRole 的 _count 哨兵值
//   - NanPrimitiveTokens 结构体默认值
//   - NanFontWeight 枚举值
//   - NanComponentState 枚举值
//

#include <gtest/gtest.h>

import nandina.theme;

using namespace nandina::theme;

// ═══════════════════════════════════════════════════════════════════════════
// NanColorRole 枚举测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanColorRoleTest, Basic) {
    // 确认基本枚举值存在且可比较
    EXPECT_EQ(NanColorRole::primary,      NanColorRole::primary);
    EXPECT_NE(NanColorRole::primary,      NanColorRole::primaryContainer);
    EXPECT_NE(NanColorRole::primary,      NanColorRole::secondary);
    EXPECT_NE(NanColorRole::tertiary,     NanColorRole::tertiaryContainer);
}

TEST(NanColorRoleTest, CountValue) {
    // 验证 _count 哨兵值
    constexpr auto count_val = static_cast<std::size_t>(NanColorRole::_count);
    EXPECT_GT(count_val, 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanColorScheme 枚举测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanColorSchemeTest, Basic) {
    EXPECT_EQ(NanColorScheme::light, NanColorScheme::light);
    EXPECT_EQ(NanColorScheme::dark,  NanColorScheme::dark);
    EXPECT_NE(NanColorScheme::light, NanColorScheme::dark);
}

TEST(NanColorSchemeTest, Conversion) {
    auto s = static_cast<std::uint8_t>(NanColorScheme::light);
    EXPECT_EQ(s, 0);
    s = static_cast<std::uint8_t>(NanColorScheme::dark);
    EXPECT_EQ(s, 1);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanComponentState 枚举测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanComponentStateTest, Basic) {
    EXPECT_NE(NanComponentState::enabled,  NanComponentState::disabled);
    EXPECT_NE(NanComponentState::hovered,  NanComponentState::focused);
    EXPECT_NE(NanComponentState::pressed,  NanComponentState::dragged);
    EXPECT_NE(NanComponentState::selected, NanComponentState::disabled);
}

TEST(NanComponentStateTest, CountValue) {
    constexpr auto count_val = static_cast<std::size_t>(NanComponentState::_count);
    EXPECT_GT(count_val, 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanFontWeight 枚举测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanFontWeightTest, Basic) {
    EXPECT_EQ(NanFontWeight::regular, NanFontWeight::regular);
    EXPECT_EQ(NanFontWeight::bold,   NanFontWeight::bold);
    EXPECT_NE(NanFontWeight::regular, NanFontWeight::bold);
}

TEST(NanFontWeightTest, UnderlyingValues) {
    EXPECT_EQ(static_cast<int>(NanFontWeight::thin),       100);
    EXPECT_EQ(static_cast<int>(NanFontWeight::regular),    400);
    EXPECT_EQ(static_cast<int>(NanFontWeight::medium),     500);
    EXPECT_EQ(static_cast<int>(NanFontWeight::bold),       700);
    EXPECT_EQ(static_cast<int>(NanFontWeight::black),      900);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanSpacingTokens 结构体默认值测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanSpacingTokensTest, Defaults) {
    NanSpacingTokens t;
    EXPECT_FLOAT_EQ(t.none,     0.0f);
    EXPECT_FLOAT_EQ(t.xsmall,   4.0f);
    EXPECT_FLOAT_EQ(t.small,    8.0f);
    EXPECT_FLOAT_EQ(t.medium,   12.0f);
    EXPECT_FLOAT_EQ(t.large,    16.0f);
    EXPECT_FLOAT_EQ(t.xlarge,   24.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanRadiusTokens 结构体默认值测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanRadiusTokensTest, Defaults) {
    NanRadiusTokens t;
    EXPECT_FLOAT_EQ(t.none,   0.0f);
    EXPECT_FLOAT_EQ(t.small,  8.0f);
    EXPECT_FLOAT_EQ(t.medium, 12.0f);
    EXPECT_FLOAT_EQ(t.full,   999.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanElevationTokens 结构体默认值测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanElevationTokensTest, Defaults) {
    NanElevationTokens t;
    EXPECT_FLOAT_EQ(t.level0, 0.0f);
    EXPECT_FLOAT_EQ(t.level1, 1.0f);
    EXPECT_FLOAT_EQ(t.level3, 6.0f);
    EXPECT_FLOAT_EQ(t.level5, 12.0f);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanOpacityTokens 结构体默认值测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanOpacityTokensTest, Defaults) {
    NanOpacityTokens t;
    EXPECT_FLOAT_EQ(t.opaque,   1.0f);
    EXPECT_FLOAT_EQ(t.high,     0.87f);
    EXPECT_FLOAT_EQ(t.medium,   0.60f);
    EXPECT_FLOAT_EQ(t.disabled, 0.12f);
}

// ═══════════════════════════════════════════════════════════════════════════
// NanPrimitiveTokens 聚合默认值测试
// ═══════════════════════════════════════════════════════════════════════════

TEST(NanPrimitiveTokensTest, DefaultValues) {
    NanPrimitiveTokens tokens;

    // Spacing
    EXPECT_FLOAT_EQ(tokens.spacing.xsmall, 4.0f);
    EXPECT_FLOAT_EQ(tokens.spacing.large, 16.0f);

    // Radius
    EXPECT_FLOAT_EQ(tokens.radius.medium, 12.0f);
    EXPECT_FLOAT_EQ(tokens.radius.full, 999.0f);

    // Elevation
    EXPECT_FLOAT_EQ(tokens.elevation.level0, 0.0f);
    EXPECT_FLOAT_EQ(tokens.elevation.level5, 12.0f);

    // Opacity
    EXPECT_FLOAT_EQ(tokens.opacity.high, 0.87f);
    EXPECT_FLOAT_EQ(tokens.opacity.scrim, 0.32f);

    // Typography — 验证默认 body 大小
    EXPECT_FLOAT_EQ(tokens.typography.body_medium.font_size, 14.0f);
    EXPECT_EQ(tokens.typography.body_medium.font_weight, NanFontWeight::regular);
    EXPECT_FLOAT_EQ(tokens.typography.display_large.font_size, 57.0f);
    EXPECT_FLOAT_EQ(tokens.typography.label_small.font_size, 11.0f);
}