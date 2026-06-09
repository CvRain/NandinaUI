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

TEST(NanBorderTokensTest, Defaults) {
    NanBorderTokens t;
    EXPECT_FLOAT_EQ(t.none, 0.0f);
    EXPECT_FLOAT_EQ(t.hairline, 1.0f);
    EXPECT_FLOAT_EQ(t.thin, 1.0f);
    EXPECT_FLOAT_EQ(t.medium, 2.0f);
    EXPECT_FLOAT_EQ(t.thick, 3.0f);
    EXPECT_FLOAT_EQ(t.divider, 1.0f);
    EXPECT_FLOAT_EQ(t.focus_ring, 2.0f);
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

    // Border
    EXPECT_FLOAT_EQ(tokens.border.thin, 1.0f);
    EXPECT_FLOAT_EQ(tokens.border.divider, 1.0f);
    EXPECT_FLOAT_EQ(tokens.border.focus_ring, 2.0f);

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

TEST(NanTypographyRoleTest, ResolveTypeStyle) {
    NanTypographyTokens tokens;

    const auto& headline = resolve_typography_style(tokens, NanTypographyRole::headline_large);
    EXPECT_FLOAT_EQ(headline.font_size, 32.0f);
    EXPECT_EQ(headline.font_weight, NanFontWeight::regular);

    const auto& label = resolve_typography_style(tokens, NanTypographyRole::label_small);
    EXPECT_FLOAT_EQ(label.font_size, 11.0f);
    EXPECT_EQ(label.font_weight, NanFontWeight::medium);
}

TEST(NanTextStyleTest, Defaults) {
    NanTextStyle style;

    EXPECT_FLOAT_EQ(style.font_size, 14.0f);
    EXPECT_EQ(style.font_weight, nandina::text::NanFontWeight::regular);
    EXPECT_EQ(style.overflow, nandina::text::TextOverflow::wrap);
    EXPECT_EQ(style.wrap_policy, nandina::text::TextWrapPolicy::word);
    EXPECT_FALSE(style.single_line);
    EXPECT_EQ(style.max_lines, 0);

    const auto text = style.font_color.to<nandina::NanRgb>();
    EXPECT_EQ(text.red(), 30u);
    EXPECT_EQ(text.green(), 30u);
    EXPECT_EQ(text.blue(), 46u);
}

TEST(NanLabelStyleTest, Defaults) {
    NanLabelStyle style;

    EXPECT_FLOAT_EQ(style.font_size, 14.0f);
    EXPECT_EQ(style.font_weight, nandina::text::NanFontWeight::regular);

    const auto normal = style.font_color.to<nandina::NanRgb>();
    EXPECT_EQ(normal.red(), 76u);
    EXPECT_EQ(normal.green(), 79u);
    EXPECT_EQ(normal.blue(), 105u);

    const auto disabled = style.disabled_font_color.to<nandina::NanRgb>();
    EXPECT_EQ(disabled.red(), 154u);
    EXPECT_EQ(disabled.green(), 157u);
    EXPECT_EQ(disabled.blue(), 180u);

    const auto error = style.error_font_color.to<nandina::NanRgb>();
    EXPECT_EQ(error.red(), 230u);
    EXPECT_EQ(error.green(), 69u);
    EXPECT_EQ(error.blue(), 83u);

    const auto required = style.required_indicator_color.to<nandina::NanRgb>();
    EXPECT_EQ(required.red(), 230u);
    EXPECT_EQ(required.green(), 69u);
    EXPECT_EQ(required.blue(), 83u);

    EXPECT_FLOAT_EQ(style.required_indicator_gap, 4.0f);
    EXPECT_EQ(style.overflow, nandina::text::TextOverflow::wrap);
    EXPECT_EQ(style.wrap_policy, nandina::text::TextWrapPolicy::word);
    EXPECT_FALSE(style.single_line);
    EXPECT_EQ(style.max_lines, 0);
}

TEST(NanButtonStyleTest, Defaults) {
    NanButtonStyle style;

    EXPECT_FLOAT_EQ(style.corner_radius, 6.0f);
    EXPECT_EQ(style.color_variant, ColorVariant::inherit);
    EXPECT_EQ(style.variant, ButtonVariant::default_variant);
    EXPECT_EQ(style.size, ButtonSize::md);
    EXPECT_EQ(style.font_weight, nandina::text::NanFontWeight::medium);
    EXPECT_EQ(style.overflow, nandina::text::TextOverflow::ellipsis);
    EXPECT_TRUE(style.single_line);

    const auto filled_bg = style.filled.bg.to<nandina::NanRgb>();
    EXPECT_EQ(filled_bg.red(), 99u);
    EXPECT_EQ(filled_bg.green(), 102u);
    EXPECT_EQ(filled_bg.blue(), 241u);

    const auto tonal_text = style.tonal.text.to<nandina::NanRgb>();
    EXPECT_EQ(tonal_text.red(), 69u);
    EXPECT_EQ(tonal_text.green(), 72u);
    EXPECT_EQ(tonal_text.blue(), 200u);

    const auto outlined_border = style.outlined.border.to<nandina::NanRgb>();
    EXPECT_EQ(outlined_border.red(), 180u);
    EXPECT_EQ(outlined_border.green(), 183u);
    EXPECT_EQ(outlined_border.blue(), 220u);
    EXPECT_FLOAT_EQ(style.outlined.border_width, 1.0f);

    EXPECT_FLOAT_EQ(style.md.font_size, 14.0f);
    EXPECT_FLOAT_EQ(style.md.padding_h, 16.0f);
    EXPECT_FLOAT_EQ(style.md.padding_v, 8.0f);
    EXPECT_FLOAT_EQ(style.md.icon_size, 18.0f);
    EXPECT_FALSE(style.md.square);

    const auto secondary_outline = style.secondary_family.outlined.border.to<nandina::NanRgb>();
    EXPECT_GT(secondary_outline.red(), 0u);

    const auto destructive_link = style.destructive_family.link.text.to<nandina::NanRgb>();
    EXPECT_EQ(destructive_link.red(), 230u);

    EXPECT_FLOAT_EQ(style.icon.height, 40.0f);
    EXPECT_TRUE(style.icon.square);
}


TEST(NanInputStyleTest, DefaultsExposeColorFamilies) {
    NanInputStyle style;

    EXPECT_EQ(style.color_variant, ColorVariant::inherit);

    const auto secondary_border = style.secondary_family.border.to<nandina::NanRgb>();
    EXPECT_EQ(secondary_border.red(), 186u);
    EXPECT_EQ(secondary_border.green(), 198u);
    EXPECT_EQ(secondary_border.blue(), 255u);

    const auto destructive_focus = style.destructive_family.border_focus.to<nandina::NanRgb>();
    EXPECT_EQ(destructive_focus.red(), 230u);
    EXPECT_EQ(destructive_focus.green(), 69u);
    EXPECT_EQ(destructive_focus.blue(), 83u);
}

TEST(NanTagStyleTest, DefaultsExposeSemanticFamiliesAndSizes) {
    NanTagStyle style;

    EXPECT_EQ(style.color_variant, ColorVariant::inherit);
    EXPECT_EQ(style.size, TagSize::md);
    EXPECT_TRUE(style.single_line);
    EXPECT_EQ(style.overflow, nandina::text::TextOverflow::ellipsis);

    const auto secondary_text = style.secondary_family.text.to<nandina::NanRgb>();
    EXPECT_EQ(secondary_text.red(), 66u);
    EXPECT_EQ(secondary_text.green(), 84u);
    EXPECT_EQ(secondary_text.blue(), 186u);

    const auto destructive_border = style.destructive_family.border.to<nandina::NanRgb>();
    EXPECT_EQ(destructive_border.red(), 242u);
    EXPECT_EQ(destructive_border.green(), 170u);
    EXPECT_EQ(destructive_border.blue(), 178u);

    EXPECT_FLOAT_EQ(style.sm.font_size, 12.0f);
    EXPECT_FLOAT_EQ(style.lg.padding_h, 12.0f);
}

TEST(NanProgressStyleTest, DefaultsExposeColorFamilies) {
    NanProgressStyle style;

    EXPECT_EQ(style.color_variant, ColorVariant::inherit);

    const auto secondary_fill = style.secondary_family.fill.to<nandina::NanRgb>();
    EXPECT_EQ(secondary_fill.red(), 114u);
    EXPECT_EQ(secondary_fill.green(), 135u);
    EXPECT_EQ(secondary_fill.blue(), 253u);

    const auto destructive_track = style.destructive_family.track_bg.to<nandina::NanRgb>();
    EXPECT_EQ(destructive_track.red(), 251u);
    EXPECT_EQ(destructive_track.green(), 225u);
    EXPECT_EQ(destructive_track.blue(), 228u);
}
