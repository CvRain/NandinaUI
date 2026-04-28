//
// Created by cvrain on 2026/4/28.
//
module;

#include <array>
#include <cstdint>
#include <span>

export module nandina.theme.nan_color_palette;

export import nandina.foundation.color;
import nandina.theme.nan_theme_types;

/**
 * nandina.theme.nan_color_palette
 *
 * 语义颜色调色板。
 *
 * 管理 light/dark 两套语义颜色配置，语义角色与原始色值的映射。
 *
 * 设计理念：
 *   - 组件按 NanColorRole 查找颜色，从不直接引用 #hex 值
 *   - 默认值基于 Material Design 3 的 reference palette
 *   - 用户可构造完全自定义的调色板
 */

export namespace nandina::theme {

/**
 * @brief 单个语义颜色角色的色值。
 *
 * 存储一个 NanColorRole 在 light / dark 两种方案下的色值。
 */
struct NanRoleColorPair {
    NanColorRole role;   ///< 颜色角色
    NanColor     light;  ///< 浅色模式色值
    NanColor     dark;   ///< 深色模式色值
};

/**
 * @brief 语义颜色调色板。
 *
 * 管理全部 NanColorRole 的色值映射。
 * 默认提供 Material 3 reference palette 的色值。
 *
 * 使用方式：
 *   - 通过 `get(role, scheme)` 检索色值
 *   - 通过 `set(role, light, dark)` 覆盖特定角色的色值
 *   - 通过 `set_all_light(color_map)` / `set_all_dark(color_map)` 批量覆盖
 */
class NanColorPalette {
public:
    /// 默认构造：生成 Material 3 reference palette
    NanColorPalette();

    /// 从预构建的色值数组构造
    explicit NanColorPalette(std::array<NanRoleColorPair, static_cast<std::size_t>(NanColorRole::_count)> pairs);

    /**
     * @brief 获取色值。
     *
     * @param role   语义角色
     * @param scheme 色彩方案
     * @return 对应色值
     */
    [[nodiscard]] auto get(NanColorRole role, NanColorScheme scheme) const -> const NanColor&;

    /**
     * @brief 设置单个角色的 light / dark 色值。
     */
    auto set(NanColorRole role, NanColor light, NanColor dark) -> void;

    /// 获取全部色值对（用于遍历/序列化）
    [[nodiscard]] auto pairs() const -> const std::array<NanRoleColorPair, static_cast<std::size_t>(NanColorRole::_count)>&;

private:
    std::array<NanRoleColorPair, static_cast<std::size_t>(NanColorRole::_count)> pairs_;
};

// ═════════════════════════════════════════════════════════════════════════════
// § 默认 Material 3 Reference Palette（基于 Catppuccin Mocha/Latte 风格微调）
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 生成一组合理的默认色值。
 *
 * 源自 Material Design 3 reference palette 概念。
 * light scheme 接近白色背景；dark scheme 接近深色背景。
 * 用户应基于此构建自己品牌的调色板。
 */
[[nodiscard]] inline auto make_default_light_palette() -> std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)>;

[[nodiscard]] inline auto make_default_dark_palette() -> std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)>;

// ═════════════════════════════════════════════════════════════════════════════
// § 实现
// ═════════════════════════════════════════════════════════════════════════════

namespace detail {

/// 从 RGB 分量构造 NanColor
[[nodiscard]] inline auto rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) -> NanColor {
    return NanColor::from(NanRgb{r, g, b, a});
}

} // namespace detail

inline auto make_default_light_palette() -> std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)> {
    using detail::rgb;
    std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)> colors{};

    // Primary: 靛蓝色系
    colors[static_cast<std::size_t>(NanColorRole::primary)]              = rgb(103, 80, 217);    // Indigo 400
    colors[static_cast<std::size_t>(NanColorRole::onPrimary)]            = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::primaryContainer)]     = rgb(234, 221, 255);   // Indigo 100
    colors[static_cast<std::size_t>(NanColorRole::onPrimaryContainer)]   = rgb(33, 0, 93);

    // Secondary
    colors[static_cast<std::size_t>(NanColorRole::secondary)]            = rgb(98, 91, 113);
    colors[static_cast<std::size_t>(NanColorRole::onSecondary)]          = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::secondaryContainer)]   = rgb(232, 222, 248);
    colors[static_cast<std::size_t>(NanColorRole::onSecondaryContainer)] = rgb(30, 25, 43);

    // Tertiary
    colors[static_cast<std::size_t>(NanColorRole::tertiary)]             = rgb(125, 82, 96);
    colors[static_cast<std::size_t>(NanColorRole::onTertiary)]           = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::tertiaryContainer)]    = rgb(255, 216, 228);
    colors[static_cast<std::size_t>(NanColorRole::onTertiaryContainer)]  = rgb(49, 17, 29);

    // Error
    colors[static_cast<std::size_t>(NanColorRole::error)]                = rgb(179, 38, 30);
    colors[static_cast<std::size_t>(NanColorRole::onError)]              = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::errorContainer)]       = rgb(249, 222, 220);
    colors[static_cast<std::size_t>(NanColorRole::onErrorContainer)]     = rgb(65, 14, 11);

    // Surface
    colors[static_cast<std::size_t>(NanColorRole::surface)]              = rgb(255, 251, 254);
    colors[static_cast<std::size_t>(NanColorRole::onSurface)]            = rgb(28, 27, 31);
    colors[static_cast<std::size_t>(NanColorRole::surfaceVariant)]       = rgb(231, 224, 236);
    colors[static_cast<std::size_t>(NanColorRole::onSurfaceVariant)]     = rgb(73, 69, 79);
    colors[static_cast<std::size_t>(NanColorRole::surfaceTint)]          = rgb(103, 80, 217);

    // Background
    colors[static_cast<std::size_t>(NanColorRole::background)]           = rgb(255, 251, 254);
    colors[static_cast<std::size_t>(NanColorRole::onBackground)]         = rgb(28, 27, 31);

    // Outline
    colors[static_cast<std::size_t>(NanColorRole::outline)]              = rgb(121, 116, 126);
    colors[static_cast<std::size_t>(NanColorRole::outlineVariant)]       = rgb(196, 199, 197);

    // Shadow / Scrim
    colors[static_cast<std::size_t>(NanColorRole::shadow)]               = rgb(0, 0, 0);
    colors[static_cast<std::size_t>(NanColorRole::scrim)]                = rgb(0, 0, 0);

    // Inverse
    colors[static_cast<std::size_t>(NanColorRole::inverseSurface)]       = rgb(49, 48, 51);
    colors[static_cast<std::size_t>(NanColorRole::inverseOnSurface)]     = rgb(244, 239, 244);
    colors[static_cast<std::size_t>(NanColorRole::inversePrimary)]       = rgb(208, 188, 255);

    return colors;
}

inline auto make_default_dark_palette() -> std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)> {
    using detail::rgb;
    std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)> colors{};

    // Primary
    colors[static_cast<std::size_t>(NanColorRole::primary)]              = rgb(208, 188, 255);   // Indigo 100
    colors[static_cast<std::size_t>(NanColorRole::onPrimary)]            = rgb(55, 30, 115);
    colors[static_cast<std::size_t>(NanColorRole::primaryContainer)]     = rgb(79, 55, 139);
    colors[static_cast<std::size_t>(NanColorRole::onPrimaryContainer)]   = rgb(234, 221, 255);

    // Secondary
    colors[static_cast<std::size_t>(NanColorRole::secondary)]            = rgb(204, 194, 220);
    colors[static_cast<std::size_t>(NanColorRole::onSecondary)]          = rgb(51, 45, 65);
    colors[static_cast<std::size_t>(NanColorRole::secondaryContainer)]   = rgb(74, 68, 88);
    colors[static_cast<std::size_t>(NanColorRole::onSecondaryContainer)] = rgb(232, 222, 248);

    // Tertiary
    colors[static_cast<std::size_t>(NanColorRole::tertiary)]             = rgb(239, 184, 200);
    colors[static_cast<std::size_t>(NanColorRole::onTertiary)]           = rgb(73, 37, 50);
    colors[static_cast<std::size_t>(NanColorRole::tertiaryContainer)]    = rgb(99, 59, 72);
    colors[static_cast<std::size_t>(NanColorRole::onTertiaryContainer)]  = rgb(255, 216, 228);

    // Error
    colors[static_cast<std::size_t>(NanColorRole::error)]                = rgb(242, 184, 181);
    colors[static_cast<std::size_t>(NanColorRole::onError)]              = rgb(96, 20, 12);
    colors[static_cast<std::size_t>(NanColorRole::errorContainer)]       = rgb(140, 29, 24);
    colors[static_cast<std::size_t>(NanColorRole::onErrorContainer)]     = rgb(249, 222, 220);

    // Surface
    colors[static_cast<std::size_t>(NanColorRole::surface)]              = rgb(28, 27, 31);
    colors[static_cast<std::size_t>(NanColorRole::onSurface)]            = rgb(230, 225, 229);
    colors[static_cast<std::size_t>(NanColorRole::surfaceVariant)]       = rgb(73, 69, 79);
    colors[static_cast<std::size_t>(NanColorRole::onSurfaceVariant)]     = rgb(196, 199, 197);
    colors[static_cast<std::size_t>(NanColorRole::surfaceTint)]          = rgb(208, 188, 255);

    // Background
    colors[static_cast<std::size_t>(NanColorRole::background)]           = rgb(28, 27, 31);
    colors[static_cast<std::size_t>(NanColorRole::onBackground)]         = rgb(230, 225, 229);

    // Outline
    colors[static_cast<std::size_t>(NanColorRole::outline)]              = rgb(147, 143, 153);
    colors[static_cast<std::size_t>(NanColorRole::outlineVariant)]       = rgb(68, 71, 70);

    // Shadow / Scrim
    colors[static_cast<std::size_t>(NanColorRole::shadow)]               = rgb(0, 0, 0);
    colors[static_cast<std::size_t>(NanColorRole::scrim)]                = rgb(0, 0, 0);

    // Inverse
    colors[static_cast<std::size_t>(NanColorRole::inverseSurface)]       = rgb(230, 225, 229);
    colors[static_cast<std::size_t>(NanColorRole::inverseOnSurface)]     = rgb(49, 48, 51);
    colors[static_cast<std::size_t>(NanColorRole::inversePrimary)]       = rgb(103, 80, 217);

    return colors;
}

inline NanColorPalette::NanColorPalette() {
    const auto light_colors = make_default_light_palette();
    const auto dark_colors  = make_default_dark_palette();
    for (std::size_t i = 0; i < static_cast<std::size_t>(NanColorRole::_count); ++i) {
        pairs_[i] = NanRoleColorPair{
            .role  = static_cast<NanColorRole>(i),
            .light = light_colors[i],
            .dark  = dark_colors[i]
        };
    }
}

inline NanColorPalette::NanColorPalette(std::array<NanRoleColorPair, static_cast<std::size_t>(NanColorRole::_count)> pairs)
    : pairs_(pairs) {}

inline auto NanColorPalette::get(NanColorRole role, NanColorScheme scheme) const -> const NanColor& {
    const auto idx = static_cast<std::size_t>(role);
    switch (scheme) {
        case NanColorScheme::light: return pairs_[idx].light;
        case NanColorScheme::dark:  return pairs_[idx].dark;
    }
    return pairs_[idx].light; // fallback
}

inline auto NanColorPalette::set(NanColorRole role, NanColor light, NanColor dark) -> void {
    const auto idx = static_cast<std::size_t>(role);
    pairs_[idx].light = std::move(light);
    pairs_[idx].dark  = std::move(dark);
}

inline auto NanColorPalette::pairs() const -> const std::array<NanRoleColorPair, static_cast<std::size_t>(NanColorRole::_count)>& {
    return pairs_;
}

} // namespace nandina::theme