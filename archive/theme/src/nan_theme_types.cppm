//
// Created by cvrain on 2026/4/28.
//
module;

#include <cstdint>

export module nandina.theme.nan_theme_types;

/**
 * nandina.theme.nan_theme_types
 *
 * 主题系统的基础枚举与类型定义。
 *
 * 包含：
 *   - NanColorScheme    — 色彩方案（light / dark）
 *   - NanColorRole      — 语义颜色角色（遵循 Material Design 3 色彩系统）
 *   - NanComponentState — 组件交互状态
 *
 * 本模块无上层主题模块依赖，仅依赖 foundation。
 */

export namespace nandina::theme {

/**
 * @brief 色彩方案。
 *
 * 表明当前主题的亮/暗偏好。
 */
enum class NanColorScheme : std::uint8_t {
    light = 0, ///< 浅色模式
    dark  = 1  ///< 深色模式
};

/**
 * @brief 语义颜色角色。
 *
 * 定义 Material Design 3 语义颜色系统中的全部角色。
 * 组件按角色名从主题中查找颜色，而非直接持有具体色值。
 *
 * 分组：
 *   Primary / Secondary / Tertiary — 主要/次要/第三主色
 *   Error  — 错误与警告色
 *   Surface — 表⾯与容器色
 *   Background — 背景色
 *   Outline — 轮廓与分割线色
 *   Inverse — 反色（用于在 Surface 上显示的反色文本/图标）
 */
enum class NanColorRole : std::uint8_t {
    // ── Primary ──
    primary,              ///< 主色调，品牌色/动作色
    onPrimary,            ///< 主色背景上的内容色（文本/图标）
    primaryContainer,     ///< 主色容器背景
    onPrimaryContainer,   ///< 主色容器上的内容色

    // ── Secondary ──
    secondary,            ///< 次要色
    onSecondary,          ///< 次要色背景上的内容色
    secondaryContainer,   ///< 次要色容器背景
    onSecondaryContainer, ///< 次要色容器上的内容色

    // ── Tertiary ──
    tertiary,             ///< 第三色
    onTertiary,           ///< 第三色背景上的内容色
    tertiaryContainer,    ///< 第三色容器背景
    onTertiaryContainer,  ///< 第三色容器上的内容色

    // ── Error ──
    error,                ///< 错误色
    onError,              ///< 错误色背景上的内容色
    errorContainer,       ///< 错误色容器背景
    onErrorContainer,     ///< 错误色容器上的内容色

    // ── Surface ──
    surface,              ///< 表面底色
    onSurface,            ///< 表面上的内容色
    surfaceVariant,       ///< 表面变体色
    onSurfaceVariant,     ///< 表面变体上的内容色
    surfaceTint,          ///< 表面色调色（Elevation 光影色）

    // ── Background ──
    background,           ///< 背景色
    onBackground,         ///< 背景上的内容色

    // ── Outline ──
    outline,              ///< 轮廓/边框色
    outlineVariant,       ///< 轮廓变体色（更淡）

    // ── Shadow / Scrim ──
    shadow,               ///< 投影色
    scrim,                ///< 遮罩层色

    // ── Inverse ──
    inverseSurface,       ///< 反色表面（用于在 Surface 之上翻转显示）
    inverseOnSurface,     ///< 反色表面上的内容色
    inversePrimary,       ///< 反色主色

    _count ///< 枚举值总数（内部使用）
};

/**
 * @brief 组件交互状态。
 *
 * 用于按状态解析对应的颜色/样式 Token。
 * 状态可以组合（例如：hovered + focused 同时激活），
 * 此时按优先级（_count 顺序从高到低）查找匹配值。
 */
enum class NanComponentState : std::uint8_t {
    enabled  = 0,  ///< 启用态（默认）
    hovered  = 1,  ///< 悬停态
    focused  = 2,  ///< 焦点态
    pressed  = 3,  ///< 按捺态
    dragged  = 4,  ///< 拖动态
    selected = 5,  ///< 选中态
    disabled = 6,  ///< 禁用态

    _count   ///< 枚举值总数（内部使用）
};

} // namespace nandina::theme