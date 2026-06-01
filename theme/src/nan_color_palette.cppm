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

    // shadcn default light — neutral grayscale
    colors[static_cast<std::size_t>(NanColorRole::primary)]              = rgb(23, 23, 23);
    colors[static_cast<std::size_t>(NanColorRole::onPrimary)]            = rgb(250, 250, 250);
    colors[static_cast<std::size_t>(NanColorRole::primaryContainer)]     = rgb(245, 245, 245);
    colors[static_cast<std::size_t>(NanColorRole::onPrimaryContainer)]   = rgb(23, 23, 23);

    colors[static_cast<std::size_t>(NanColorRole::secondary)]            = rgb(245, 245, 245);
    colors[static_cast<std::size_t>(NanColorRole::onSecondary)]          = rgb(23, 23, 23);
    colors[static_cast<std::size_t>(NanColorRole::secondaryContainer)]   = rgb(245, 245, 245);
    colors[static_cast<std::size_t>(NanColorRole::onSecondaryContainer)] = rgb(23, 23, 23);

    colors[static_cast<std::size_t>(NanColorRole::tertiary)]             = rgb(115, 115, 115);
    colors[static_cast<std::size_t>(NanColorRole::onTertiary)]           = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::tertiaryContainer)]    = rgb(229, 229, 229);
    colors[static_cast<std::size_t>(NanColorRole::onTertiaryContainer)]  = rgb(64, 64, 64);

    colors[static_cast<std::size_t>(NanColorRole::error)]                = rgb(239, 68, 68);
    colors[static_cast<std::size_t>(NanColorRole::onError)]              = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::errorContainer)]       = rgb(254, 226, 226);
    colors[static_cast<std::size_t>(NanColorRole::onErrorContainer)]     = rgb(153, 27, 27);

    colors[static_cast<std::size_t>(NanColorRole::surface)]              = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::onSurface)]            = rgb(10, 10, 10);
    colors[static_cast<std::size_t>(NanColorRole::surfaceVariant)]       = rgb(245, 245, 245);
    colors[static_cast<std::size_t>(NanColorRole::onSurfaceVariant)]     = rgb(115, 115, 115);
    colors[static_cast<std::size_t>(NanColorRole::surfaceTint)]          = rgb(23, 23, 23);

    colors[static_cast<std::size_t>(NanColorRole::background)]           = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::onBackground)]         = rgb(10, 10, 10);

    colors[static_cast<std::size_t>(NanColorRole::outline)]              = rgb(229, 229, 229);
    colors[static_cast<std::size_t>(NanColorRole::outlineVariant)]       = rgb(229, 229, 229);

    colors[static_cast<std::size_t>(NanColorRole::shadow)]               = rgb(0, 0, 0);
    colors[static_cast<std::size_t>(NanColorRole::scrim)]                = rgb(0, 0, 0);

    colors[static_cast<std::size_t>(NanColorRole::inverseSurface)]       = rgb(49, 48, 51);
    colors[static_cast<std::size_t>(NanColorRole::inverseOnSurface)]     = rgb(244, 239, 244);
    colors[static_cast<std::size_t>(NanColorRole::inversePrimary)]       = rgb(208, 188, 255);

    return colors;
}

inline auto make_default_dark_palette() -> std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)> {
    using detail::rgb;
    std::array<NanColor, static_cast<std::size_t>(NanColorRole::_count)> colors{};

    // shadcn default dark — neutral grayscale
    colors[static_cast<std::size_t>(NanColorRole::primary)]              = rgb(250, 250, 250);
    colors[static_cast<std::size_t>(NanColorRole::onPrimary)]            = rgb(23, 23, 23);
    colors[static_cast<std::size_t>(NanColorRole::primaryContainer)]     = rgb(38, 38, 38);
    colors[static_cast<std::size_t>(NanColorRole::onPrimaryContainer)]   = rgb(250, 250, 250);

    colors[static_cast<std::size_t>(NanColorRole::secondary)]            = rgb(38, 38, 38);
    colors[static_cast<std::size_t>(NanColorRole::onSecondary)]          = rgb(250, 250, 250);
    colors[static_cast<std::size_t>(NanColorRole::secondaryContainer)]   = rgb(38, 38, 38);
    colors[static_cast<std::size_t>(NanColorRole::onSecondaryContainer)] = rgb(250, 250, 250);

    colors[static_cast<std::size_t>(NanColorRole::tertiary)]             = rgb(163, 163, 163);
    colors[static_cast<std::size_t>(NanColorRole::onTertiary)]           = rgb(23, 23, 23);
    colors[static_cast<std::size_t>(NanColorRole::tertiaryContainer)]    = rgb(64, 64, 64);
    colors[static_cast<std::size_t>(NanColorRole::onTertiaryContainer)]  = rgb(229, 229, 229);

    colors[static_cast<std::size_t>(NanColorRole::error)]                = rgb(239, 68, 68);
    colors[static_cast<std::size_t>(NanColorRole::onError)]              = rgb(255, 255, 255);
    colors[static_cast<std::size_t>(NanColorRole::errorContainer)]       = rgb(127, 29, 29);
    colors[static_cast<std::size_t>(NanColorRole::onErrorContainer)]     = rgb(254, 226, 226);

    colors[static_cast<std::size_t>(NanColorRole::surface)]              = rgb(23, 23, 23);
    colors[static_cast<std::size_t>(NanColorRole::onSurface)]            = rgb(250, 250, 250);
    colors[static_cast<std::size_t>(NanColorRole::surfaceVariant)]       = rgb(38, 38, 38);
    colors[static_cast<std::size_t>(NanColorRole::onSurfaceVariant)]     = rgb(163, 163, 163);
    colors[static_cast<std::size_t>(NanColorRole::surfaceTint)]          = rgb(250, 250, 250);

    colors[static_cast<std::size_t>(NanColorRole::background)]           = rgb(10, 10, 10);
    colors[static_cast<std::size_t>(NanColorRole::onBackground)]         = rgb(250, 250, 250);

    colors[static_cast<std::size_t>(NanColorRole::outline)]              = rgb(229, 229, 229, 25);
    colors[static_cast<std::size_t>(NanColorRole::outlineVariant)]       = rgb(229, 229, 229, 38);

    colors[static_cast<std::size_t>(NanColorRole::shadow)]               = rgb(0, 0, 0);
    colors[static_cast<std::size_t>(NanColorRole::scrim)]                = rgb(0, 0, 0);

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