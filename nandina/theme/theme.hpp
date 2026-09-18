//
// theme/theme — reference color scales, semantic color scheme, and token aggregate.
//

#ifndef NANDINA_EXPERIMENT_THEME_THEME_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_HPP

#include "../foundation/nandina_color.hpp"
#include "appearance.hpp"
#include "tokens.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace nandina::theme
{
    using foundation::NanColor;
    using foundation::NanHexRgb;
    using foundation::NanOklch;
    using foundation::NanRgb;

    [[nodiscard]] inline auto nan_color(float light, float chroma, float hue, float alpha = 1.0F)
        -> NanColor {
        return NanColor::from(
            NanOklch {.light = light, .chroma = chroma, .hue = hue, .alpha = alpha}
        );
    }

    enum class ColorShade : std::size_t {
        shade_50,
        shade_100,
        shade_200,
        shade_300,
        shade_400,
        shade_500,
        shade_600,
        shade_700,
        shade_800,
        shade_900,
        shade_950,
    };

    struct NanColorScale {
        static constexpr std::size_t stop_count = 11;

        std::array<NanColor, stop_count> stops;

        [[nodiscard]] auto at(ColorShade shade) const noexcept -> const NanColor& {
            return stops[static_cast<std::size_t>(shade)];
        }

        [[nodiscard]] auto at(ColorShade shade) noexcept -> NanColor& {
            return stops[static_cast<std::size_t>(shade)];
        }
    };

    /**
     * 16 进制色阶的命名结构（浅 → 深，50 → 950）。
     * 供主题作者用命名字段表达色阶，避免 11 个位置化十六进制值难以阅读。
     * 档位语义与 `ColorShade` 一致：50 最浅（背景/表面）、950 最深（文字/base）。
     */
    struct NanHexScale {
        std::uint32_t shade_50;
        std::uint32_t shade_100;
        std::uint32_t shade_200;
        std::uint32_t shade_300;
        std::uint32_t shade_400;
        std::uint32_t shade_500;
        std::uint32_t shade_600;
        std::uint32_t shade_700;
        std::uint32_t shade_800;
        std::uint32_t shade_900;
        std::uint32_t shade_950;
    };

    /** 把命名的 16 进制色阶编译为 NanColorScale（内部转 OKLCH）。 */
    [[nodiscard]] auto nan_color_scale(const NanHexScale& scale) -> NanColorScale;

    // Reference colors are authoring inputs. Components consume the resolved semantic scheme below.
    struct NanReferencePalette {
        NanColorScale primary;
        NanColorScale secondary;
        NanColorScale tertiary;
        NanColorScale neutral;
        NanColorScale success;
        NanColorScale warning;
        NanColorScale error;
    };

    /** 从参考色阶生成语义色时的 tone 选择。 */
    struct PaletteVariantPolicy {
        ColorShade light_brand = ColorShade::shade_500;
        ColorShade dark_brand = ColorShade::shade_500;
        // on_primary（前景字）档位。默认为最深档（butter 的「On Accent = Base」：
        // 浅色强调色配深字）。中/深色强调色的族（fluent/material）翻转用浅档。
        ColorShade light_on_brand = ColorShade::shade_950;
        ColorShade dark_on_brand = ColorShade::shade_950;

        /**
         * `secondary` 是**中性**次操作色（shadcn 约定），因此取中性档而不是品牌辅色档；
         * 其前景用中性文本档。品牌辅色走 `tertiary`。
         */
        ColorShade light_secondary = ColorShade::shade_200;
        ColorShade dark_secondary = ColorShade::shade_800;
        ColorShade light_on_secondary = ColorShade::shade_900;
        ColorShade dark_on_secondary = ColorShade::shade_50;

        /** Material 风格可选项：暗色外观使用更亮的 400 档品牌色，前景字随外观翻转。 */
        [[nodiscard]] static constexpr auto material_dark_tone() -> PaletteVariantPolicy {
            return {
                .light_brand = ColorShade::shade_500,
                .dark_brand = ColorShade::shade_400,
                .light_on_brand = ColorShade::shade_50,
                .dark_on_brand = ColorShade::shade_950,
            };
        }
    };

    /**
     * 语义色板的直接声明形式。
     *
     * 参考色阶（`NanReferencePalette` + `make_color_scheme`）适合「一族色阶派生明暗两套」
     * 的品牌主题；但很多设计系统（如 shadcn/ui）是**逐角色手写**语义值的。本结构就是
     * 那条作者路径：核心角色按 shadcn 的 `x` / `x_foreground` 约定命名，其余为扩展角色。
     *
     * 没有默认构造：所有字段都必须在聚合初始化里给出，避免出现半初始化的色板。
     * 框架的默认亮/暗色板见 `default_light_palette()` / `default_dark_palette()`。
     */
    struct SemanticColorSpec {
        // ─── shadcn 对齐的核心角色（每个底色配一个前景色）─────────────────────
        NanColor background;
        NanColor foreground;
        NanColor card;
        NanColor card_foreground;
        NanColor popover;
        NanColor popover_foreground;
        NanColor primary;
        NanColor primary_foreground;
        NanColor secondary;
        NanColor secondary_foreground;
        NanColor muted;
        NanColor muted_foreground;
        NanColor accent;
        NanColor accent_foreground;
        NanColor destructive;
        NanColor destructive_foreground;
        NanColor border;
        NanColor input;
        NanColor ring;

        // ─── 扩展角色 ──────────────────────────────────────────────────────────
        NanColor surface;
        NanColor surface_foreground;
        NanColor surface_variant;
        NanColor surface_variant_foreground;
        NanColor tertiary;
        NanColor tertiary_foreground;
        NanColor success;
        NanColor success_foreground;
        NanColor warning;
        NanColor warning_foreground;
        NanColor error;
        NanColor error_foreground;
        NanColor info;
        NanColor info_foreground;
        NanColor selection;
    };

    /**
     * 解析后的语义色板：组件唯一允许引用的颜色层。
     *
     * 命名约定沿用 shadcn/ui：底色 `x` 配前景色。历史字段（`on_x`、`outline*`）
     * 保留为兼容别名，新代码优先用 shadcn 名。
     */
    struct NanColorScheme {
        NanColorScheme();

        /** 由直接声明的语义值构造（shadcn 风格作者路径）。 */
        explicit NanColorScheme(const SemanticColorSpec& spec);

        // ─── shadcn 对齐角色（新代码用这些）───────────────────────────────────
        NanColor background;
        NanColor foreground;
        NanColor card;
        NanColor card_foreground;
        NanColor popover;
        NanColor popover_foreground;
        NanColor primary;
        NanColor primary_foreground;
        NanColor secondary;
        NanColor secondary_foreground;
        NanColor muted;
        NanColor muted_foreground;
        /** hover / 选中 / 下拉高亮底色。注意这不是品牌强调色（那是 primary）。 */
        NanColor accent;
        NanColor accent_foreground;
        NanColor destructive;
        NanColor destructive_foreground;
        NanColor border;
        NanColor input;
        NanColor ring;
        NanColor surface;
        NanColor surface_foreground;
        NanColor surface_variant;
        NanColor surface_variant_foreground;
        NanColor tertiary;
        NanColor tertiary_foreground;
        NanColor success;
        NanColor success_foreground;
        NanColor warning;
        NanColor warning_foreground;
        NanColor error;
        NanColor error_foreground;
        NanColor info;
        NanColor info_foreground;
        NanColor focus_ring;
        NanColor selection;

        // ─── 兼容别名（与上面的字段同值；逐步迁移到 shadcn 名）────────────────
        NanColor on_background;
        NanColor on_primary;
        NanColor on_secondary;
        NanColor on_tertiary;
        NanColor on_surface;
        NanColor on_surface_variant;
        NanColor on_muted;
        NanColor outline;
        NanColor outline_variant;
        NanColor on_success;
        NanColor on_warning;
        NanColor on_error;
        NanColor on_info;

    private:
        struct GeneratedTag {};

        NanColorScheme(
            const NanReferencePalette& reference,
            ColorAppearance appearance,
            PaletteVariantPolicy policy,
            GeneratedTag
        );

        friend auto make_color_scheme(
            const NanReferencePalette& reference,
            ColorAppearance appearance,
            PaletteVariantPolicy policy
        ) -> NanColorScheme;
    };

    // Compatibility name retained while callers migrate from palette terminology.
    using NanPalette = NanColorScheme;

    struct NanTheme {
        NanTokens tokens;
        NanColorScheme palette;
    };

    /** @return 框架内置的七组 11 档参考色阶。 */
    [[nodiscard]] auto default_reference_palette() -> NanReferencePalette;

    /** 由参考色阶和外观策略生成组件消费的语义色板。 */
    [[nodiscard]] auto make_color_scheme(
        const NanReferencePalette& reference,
        ColorAppearance appearance,
        PaletteVariantPolicy policy = {}
    ) -> NanColorScheme;

    [[nodiscard]] auto default_light_palette() -> NanColorScheme;
    [[nodiscard]] auto default_dark_palette() -> NanColorScheme;

    [[nodiscard]] auto default_theme() -> NanTheme;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_HPP
