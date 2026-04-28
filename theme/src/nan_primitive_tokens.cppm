//
// Created by cvrain on 2026/4/28.
//
module;

#include <cstdint>
#include <array>

export module nandina.theme.nan_primitive_tokens;

import nandina.theme.nan_theme_types;

/**
 * nandina.theme.nan_primitive_tokens
 *
 * 原始设计 Token（Primitive Tokens）。
 *
 * 这些是设计系统的基础原子值，组件和语义 Token 都需要引用它们。
 *
 * 设计理念（类似 shadcn/ui、Radix 的 primitives）：
 *   - 系统提供一组合理的默认值（类似 CSS 的 `initial`）
 *   - 用户可覆盖任意 Token 以定制品牌风格
 *   - Token 以命名域分组：spacing / radius / typography / elevation / opacity
 */

export namespace nandina::theme {

// ═════════════════════════════════════════════════════════════════════════════
// § Spacing Token — 间距系统 (4px 基准网格)
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 间距 Token。
 *
 * 基于 4px 基准网格的设计系统。
 * 值以像素为单位，用于 padding / margin / gap 等。
 */
struct NanSpacingTokens {
    float none   = 0.0f;   ///< 无间距
    float xxsmall = 2.0f;  ///< 特小: 2px
    float xsmall = 4.0f;   ///< 超小: 4px
    float small  = 8.0f;   ///< 小: 8px
    float medium = 12.0f;  ///< 中: 12px
    float large  = 16.0f;  ///< 大: 16px
    float xlarge = 24.0f;  ///< 超大: 24px
    float xxlarge = 32.0f; ///< 特大: 32px
    float xxxlarge = 48.0f;///< 极大: 48px
};

// ═════════════════════════════════════════════════════════════════════════════
// § Border Radius Token — 圆角系统
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 圆角 Token。
 *
 * 值以像素为单位。
 * 命名语义参考 Material Design 3 的 shape system。
 */
struct NanRadiusTokens {
    float none    = 0.0f;   ///< 无圆角
    float xsmall  = 4.0f;   ///< 极小圆角
    float small   = 8.0f;   ///< 小圆角
    float medium  = 12.0f;  ///< 中圆角
    float large   = 16.0f;  ///< 大圆角
    float xlarge  = 24.0f;  ///< 超大圆角
    float full    = 999.0f; ///< 完全圆角（pill shape）
};

// ═════════════════════════════════════════════════════════════════════════════
// § Elevation Token — 高度阴影系统
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 高度（阴影深度）Token。
 *
 * 值可以映射到阴影偏移量/模糊半径。
 * Level 0 = 无阴影，Level 5 = 最高阴影（如 Dialog/Floating Panel）。
 */
struct NanElevationTokens {
    float level0 = 0.0f;  ///< 无阴影
    float level1 = 1.0f;  ///< 轻微阴影
    float level2 = 3.0f;  ///< 浅阴影
    float level3 = 6.0f;  ///< 中等阴影
    float level4 = 9.0f;  ///< 深阴影
    float level5 = 12.0f; ///< 最深阴影
};

// ═════════════════════════════════════════════════════════════════════════════
// § Opacity Token — 透明度系统
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 透明度 Token。
 *
 * 值范围 0.0–1.0。
 * 用于 disabled 状态模糊、遮罩层、按压反馈等。
 */
struct NanOpacityTokens {
    float opaque    = 1.0f;   ///< 完全不透明
    float high      = 0.87f;  ///< 高不透明度（如 primary text）
    float medium    = 0.60f;  ///< 中不透明度（如 secondary text）
    float low       = 0.38f;  ///< 低不透明度（如 disabled text）
    float disabled  = 0.12f;  ///< 禁用态背景色叠加
    float scrim     = 0.32f;  ///< 遮罩层（modal 背景）
};

// ═════════════════════════════════════════════════════════════════════════════
// § Typography Token — 字体排印系统
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 字体粗细预定义值。
 *
 * 字体粗细值范围为 100–900，因此使用 int 作为基础类型。
 */
enum class NanFontWeight : int {
    thin       = 100,
    extraLight = 200,
    light      = 300,
    regular    = 400,
    medium     = 500,
    semiBold   = 600,
    bold       = 700,
    extraBold  = 800,
    black      = 900
};

/**
 * @brief 单个排版风格。
 *
 * 遵循 Material Design 3 的 type scale 概念。
 * 每个 style 包含字体大小、字重、行高和字间距。
 */
struct NanTypeStyle {
    float           font_size    = 14.0f;  ///< 字体大小 (px)
    NanFontWeight   font_weight  = NanFontWeight::regular; ///< 字体粗细
    float           line_height  = 20.0f;  ///< 行高 (px)
    float           letter_spacing = 0.0f; ///< 字间距 (px)
};

/**
 * @brief 字体排印 Token 集合。
 *
 * 包含从 `display` 到 `label` 的全部文本风格角色。
 * 命名参考 Material Design 3 的 type scale。
 */
struct NanTypographyTokens {
    NanTypeStyle display_large  { .font_size = 57.0f, .font_weight = NanFontWeight::regular, .line_height = 64.0f, .letter_spacing = -0.25f };
    NanTypeStyle display_medium { .font_size = 45.0f, .font_weight = NanFontWeight::regular, .line_height = 52.0f };
    NanTypeStyle display_small  { .font_size = 36.0f, .font_weight = NanFontWeight::regular, .line_height = 44.0f };

    NanTypeStyle headline_large  { .font_size = 32.0f, .font_weight = NanFontWeight::regular, .line_height = 40.0f };
    NanTypeStyle headline_medium { .font_size = 28.0f, .font_weight = NanFontWeight::regular, .line_height = 36.0f };
    NanTypeStyle headline_small  { .font_size = 24.0f, .font_weight = NanFontWeight::regular, .line_height = 32.0f };

    NanTypeStyle title_large  { .font_size = 22.0f, .font_weight = NanFontWeight::regular, .line_height = 28.0f };
    NanTypeStyle title_medium { .font_size = 16.0f, .font_weight = NanFontWeight::medium,  .line_height = 24.0f, .letter_spacing = 0.15f };
    NanTypeStyle title_small  { .font_size = 14.0f, .font_weight = NanFontWeight::medium,  .line_height = 20.0f, .letter_spacing = 0.1f  };

    NanTypeStyle body_large  { .font_size = 16.0f, .font_weight = NanFontWeight::regular, .line_height = 24.0f, .letter_spacing = 0.5f  };
    NanTypeStyle body_medium { .font_size = 14.0f, .font_weight = NanFontWeight::regular, .line_height = 20.0f, .letter_spacing = 0.25f };
    NanTypeStyle body_small  { .font_size = 12.0f, .font_weight = NanFontWeight::regular, .line_height = 16.0f, .letter_spacing = 0.4f  };

    NanTypeStyle label_large  { .font_size = 14.0f, .font_weight = NanFontWeight::medium,  .line_height = 20.0f, .letter_spacing = 0.1f  };
    NanTypeStyle label_medium { .font_size = 12.0f, .font_weight = NanFontWeight::medium,  .line_height = 16.0f, .letter_spacing = 0.5f  };
    NanTypeStyle label_small  { .font_size = 11.0f, .font_weight = NanFontWeight::medium,  .line_height = 16.0f, .letter_spacing = 0.5f  };
};

// ═════════════════════════════════════════════════════════════════════════════
// § NanPrimitiveTokens — 聚合所有原始 Token
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 聚合所有 Primitive Token 的容器。
 *
 * 每个 Theme 实例包含一份 NanPrimitiveTokens，用户可覆盖部分或全部 Token。
 * 所有值都有合理默认值。
 */
struct NanPrimitiveTokens {
    NanSpacingTokens     spacing;
    NanRadiusTokens      radius;
    NanElevationTokens   elevation;
    NanOpacityTokens     opacity;
    NanTypographyTokens  typography;
};

} // namespace nandina::theme