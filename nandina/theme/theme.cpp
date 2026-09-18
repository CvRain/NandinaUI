//
// theme/theme — built-in reference scales and semantic palette generation.
//

#include "theme.hpp"

namespace nandina::theme
{
    using foundation::nan_srgb_to_linear_channel;

    namespace
    {
        [[nodiscard]] auto scale(std::array<NanOklch, NanColorScale::stop_count> values)
            -> NanColorScale {
            NanColorScale result;
            for (std::size_t index = 0; index < values.size(); ++index) {
                result.stops[index] = NanColor::from(values[index]);
            }
            return result;
        }

        [[nodiscard]] auto brand_shade(
            const ColorAppearance appearance,
            const PaletteVariantPolicy policy
        ) -> ColorShade {
            return appearance == ColorAppearance::dark ? policy.dark_brand : policy.light_brand;
        }
    } // namespace

    auto nan_color_scale(const NanHexScale& spec) -> NanColorScale {
        NanColorScale result;
        result.stops[0] = NanColor::from_hex(spec.shade_50);
        result.stops[1] = NanColor::from_hex(spec.shade_100);
        result.stops[2] = NanColor::from_hex(spec.shade_200);
        result.stops[3] = NanColor::from_hex(spec.shade_300);
        result.stops[4] = NanColor::from_hex(spec.shade_400);
        result.stops[5] = NanColor::from_hex(spec.shade_500);
        result.stops[6] = NanColor::from_hex(spec.shade_600);
        result.stops[7] = NanColor::from_hex(spec.shade_700);
        result.stops[8] = NanColor::from_hex(spec.shade_800);
        result.stops[9] = NanColor::from_hex(spec.shade_900);
        result.stops[10] = NanColor::from_hex(spec.shade_950);
        return result;
    }

    auto default_reference_palette() -> NanReferencePalette {
        // Skeleton-compatible OKLCH scales. The warm primary keeps the Phase 7
        // built-in appearance; all semantic defaults below are selected from here.
        return {
            .primary = scale({{
                {0.9700F, 0.02F, 42.00F}, {0.9200F, 0.04F, 41.50F},
                {0.8500F, 0.07F, 41.00F}, {0.7900F, 0.09F, 40.50F},
                {0.7300F, 0.11F, 40.00F}, {0.6803F, 0.12F, 39.30F},
                {0.6000F, 0.14F, 37.50F}, {0.5200F, 0.14F, 35.50F},
                {0.4400F, 0.13F, 34.00F}, {0.3600F, 0.12F, 33.00F},
                {0.2949F, 0.11F, 32.48F},
            }}),
            .secondary = scale({{
                {0.8666F, 0.05F, 300.15F}, {0.7851F, 0.09F, 303.57F},
                {0.7044F, 0.13F, 304.44F}, {0.6283F, 0.17F, 303.81F},
                {0.5548F, 0.20F, 302.75F}, {0.4907F, 0.23F, 300.46F},
                {0.4539F, 0.21F, 299.60F}, {0.4175F, 0.19F, 298.26F},
                {0.3784F, 0.17F, 296.27F}, {0.3408F, 0.15F, 293.97F},
                {0.3018F, 0.13F, 291.16F},
            }}),
            .tertiary = scale({{
                {0.9073F, 0.08F, 328.92F}, {0.8291F, 0.13F, 339.68F},
                {0.7600F, 0.18F, 345.55F}, {0.7027F, 0.23F, 350.68F},
                {0.6648F, 0.25F, 355.85F}, {0.6454F, 0.26F, 2.48F},
                {0.5937F, 0.24F, 1.70F}, {0.5390F, 0.22F, 0.50F},
                {0.4845F, 0.20F, 359.66F}, {0.4269F, 0.17F, 357.71F},
                {0.3693F, 0.15F, 355.34F},
            }}),
            .neutral = scale({{
                {1.0000F, 0.00F, 0.0F}, {0.9067F, 0.00F, 0.0F},
                {0.8141F, 0.00F, 0.0F}, {0.7155F, 0.00F, 0.0F},
                {0.6167F, 0.00F, 0.0F}, {0.5103F, 0.00F, 0.0F},
                {0.4495F, 0.00F, 0.0F}, {0.3867F, 0.00F, 0.0F},
                {0.3211F, 0.00F, 0.0F}, {0.2520F, 0.00F, 0.0F},
                {0.1776F, 0.00F, 0.0F},
            }}),
            .success = scale({{
                {0.9405F, 0.09F, 178.66F}, {0.9162F, 0.10F, 178.60F},
                {0.8944F, 0.11F, 177.16F}, {0.8713F, 0.12F, 176.90F},
                {0.8509F, 0.13F, 175.45F}, {0.8291F, 0.13F, 174.95F},
                {0.7285F, 0.12F, 175.70F}, {0.6240F, 0.10F, 175.99F},
                {0.5126F, 0.08F, 178.28F}, {0.3972F, 0.06F, 179.74F},
                {0.2727F, 0.04F, 185.29F},
            }}),
            .warning = scale({{
                {0.9567F, 0.05F, 84.56F}, {0.9283F, 0.06F, 82.16F},
                {0.9012F, 0.08F, 80.33F}, {0.8759F, 0.10F, 80.01F},
                {0.8503F, 0.12F, 78.35F}, {0.8246F, 0.14F, 76.71F},
                {0.7634F, 0.13F, 72.25F}, {0.7034F, 0.13F, 68.09F},
                {0.6399F, 0.13F, 63.18F}, {0.5791F, 0.13F, 57.97F},
                {0.5169F, 0.13F, 51.44F},
            }}),
            .error = scale({{
                // 注意：本仓库的 error 色阶历史上就是**降序**书写的（shade_50 最深、
                // shade_950 最浅），与另外六条相反；`on_error` 因此取 shade_50。
                // 改动顺序会翻转默认错误色，别"顺手修正"。
                {0.8999F, 0.04F, 14.04F}, {0.8349F, 0.07F, 19.81F},
                {0.7740F, 0.11F, 21.98F}, {0.7213F, 0.15F, 24.90F},
                {0.6739F, 0.19F, 26.71F}, {0.6372F, 0.22F, 28.71F},
                {0.5928F, 0.21F, 28.53F}, {0.5492F, 0.20F, 28.58F},
                {0.5051F, 0.19F, 28.72F}, {0.4622F, 0.18F, 28.88F},
                {0.4186F, 0.17F, 29.23F},
            }}),
        };
    }

    auto make_color_scheme(
        const NanReferencePalette& reference,
        const ColorAppearance appearance,
        const PaletteVariantPolicy policy
    ) -> NanColorScheme {
        return NanColorScheme {reference, appearance, policy, NanColorScheme::GeneratedTag {}};
    }

    NanColorScheme::NanColorScheme():
        NanColorScheme {
            default_reference_palette(),
            ColorAppearance::light,
            PaletteVariantPolicy {},
            GeneratedTag {},
        } {}

    NanColorScheme::NanColorScheme(const SemanticColorSpec& spec):
        background(spec.background),
        foreground(spec.foreground),
        card(spec.card),
        card_foreground(spec.card_foreground),
        popover(spec.popover),
        popover_foreground(spec.popover_foreground),
        primary(spec.primary),
        primary_foreground(spec.primary_foreground),
        secondary(spec.secondary),
        secondary_foreground(spec.secondary_foreground),
        muted(spec.muted),
        muted_foreground(spec.muted_foreground),
        accent(spec.accent),
        accent_foreground(spec.accent_foreground),
        destructive(spec.destructive),
        destructive_foreground(spec.destructive_foreground),
        border(spec.border),
        input(spec.input),
        ring(spec.ring),
        surface(spec.surface),
        surface_foreground(spec.surface_foreground),
        surface_variant(spec.surface_variant),
        surface_variant_foreground(spec.surface_variant_foreground),
        tertiary(spec.tertiary),
        tertiary_foreground(spec.tertiary_foreground),
        success(spec.success),
        success_foreground(spec.success_foreground),
        warning(spec.warning),
        warning_foreground(spec.warning_foreground),
        error(spec.error),
        error_foreground(spec.error_foreground),
        info(spec.info),
        info_foreground(spec.info_foreground),
        focus_ring(spec.ring),
        selection(spec.selection),
        // 兼容别名：与上面的 shadcn 名同值，避免两套字段漂移。
        on_background(spec.foreground),
        on_primary(spec.primary_foreground),
        on_secondary(spec.secondary_foreground),
        on_tertiary(spec.tertiary_foreground),
        on_surface(spec.surface_foreground),
        on_surface_variant(spec.surface_variant_foreground),
        on_muted(spec.muted_foreground),
        outline(spec.border),
        outline_variant(spec.surface_variant),
        on_success(spec.success_foreground),
        on_warning(spec.warning_foreground),
        on_error(spec.error_foreground),
        on_info(spec.info_foreground) {}

    NanColorScheme::NanColorScheme(
        const NanReferencePalette& reference,
        const ColorAppearance appearance,
        const PaletteVariantPolicy policy,
        GeneratedTag
    ) {
        const auto brand = brand_shade(appearance, policy);
        const bool dark = appearance == ColorAppearance::dark;

        const auto& neutral = reference.neutral;
        const auto pick = [&](const ColorShade light, const ColorShade dark_shade) {
            return neutral.at(dark ? dark_shade : light);
        };
        const auto brand_at = [&](const NanColorScale& scale) { return scale.at(brand); };

        // 参考色阶路径下，色阶本身承担了角色分配。这一路径要保持既有主题族（butter /
        // fluent / material）的观感不变，因此**沿用原来的档位语义**，只把新角色接到
        // 最贴近的既有档位上：
        //   * `border` / `input`  ← 原 outline 档（中性 500/600）
        //   * `outline_variant`   ← 原档（中性 300/800）
        //   * `secondary` / `accent` / `muted` ← 原次级档（中性 200/800），
        //     其中 secondary 仍取品牌辅色，以维持遗留策略测试的语义
        //   * `focus_ring`        ← primary（与原行为一致）
        const auto subtle = pick(ColorShade::shade_200, ColorShade::shade_800);
        const auto resolved_border = pick(ColorShade::shade_500, ColorShade::shade_600);
        const auto resolved_outline_variant = pick(ColorShade::shade_300, ColorShade::shade_800);
        const auto resolved_surface = pick(ColorShade::shade_100, ColorShade::shade_900);
        const auto resolved_surface_foreground =
            neutral.at(dark ? ColorShade::shade_50 : ColorShade::shade_950);
        // secondary 在语义层是**中性**次操作色（shadcn 约定），不走品牌辅色色阶：
        // 品牌辅色色阶的第 50 档对中性底只有 2.5:1，且会让"次按钮"带上品牌色。
        // 品牌辅色仍可通过 `tertiary` 取用。
        const auto resolved_secondary =
            neutral.at(dark ? policy.dark_secondary : policy.light_secondary);
        const auto resolved_secondary_foreground =
            neutral.at(dark ? policy.dark_on_secondary : policy.light_on_secondary);

        this->background = pick(ColorShade::shade_50, ColorShade::shade_950);
        this->foreground = neutral.at(dark ? ColorShade::shade_50 : ColorShade::shade_950);

        card = resolved_surface;
        card_foreground = resolved_surface_foreground;
        popover = resolved_surface;
        popover_foreground = resolved_surface_foreground;
        surface = resolved_surface;
        surface_foreground = resolved_surface_foreground;
        surface_variant = subtle;
        surface_variant_foreground =
            neutral.at(dark ? ColorShade::shade_400 : ColorShade::shade_700);

        primary = brand_at(reference.primary);
        primary_foreground =
            reference.primary.at(dark ? policy.dark_on_brand : policy.light_on_brand);
        secondary = resolved_secondary;
        secondary_foreground = resolved_secondary_foreground;
        tertiary = brand_at(reference.tertiary);
        tertiary_foreground = reference.tertiary.at(ColorShade::shade_50);

        // muted / accent 是 hover 与弱化底，取中性次级档；其前景沿用上面的次级文本档。
        muted = subtle;
        // 次要文字必须仍达 4.5:1，故取比 surface_variant 前景更深一档。
        muted_foreground = pick(ColorShade::shade_800, ColorShade::shade_400);
        accent = subtle;
        accent_foreground = neutral.at(dark ? ColorShade::shade_50 : ColorShade::shade_900);

        // 状态色上的前景：在**整个中性色阶**里挑对比度最高的那一档。
        // 固定档位不可行——success / warning / error 三条色阶的明度跨度差别很大
        // （success 0.94→0.27，error 只有 0.90→0.42），且底色明度落在中间区间时
        // 任何固定前景都只能到 ~4.3:1。逐档比较才能拿到该色阶下的最好结果。
        const auto luminance = [](const NanColor& color) {
            const auto rgb = color.to<NanRgb>();
            return nan_srgb_to_linear_channel(rgb.red) * 0.2126F
                 + nan_srgb_to_linear_channel(rgb.green) * 0.7152F
                 + nan_srgb_to_linear_channel(rgb.blue) * 0.0722F;
        };
        const auto contrast_against = [&](const NanColor& fill, const NanColor& candidate) {
            const auto fill_luminance = luminance(fill);
            const auto candidate_luminance = luminance(candidate);
            const auto lighter = std::max(fill_luminance, candidate_luminance);
            const auto darker = std::min(fill_luminance, candidate_luminance);
            return (lighter + 0.05F) / (darker + 0.05F);
        };
        const auto neutral_on = [&](const NanColor& fill) {
            auto best = neutral.at(ColorShade::shade_950);
            auto best_ratio = 0.0F;
            for (const auto& candidate: neutral.stops) {
                const auto ratio = contrast_against(fill, candidate);
                if (ratio > best_ratio) {
                    best_ratio = ratio;
                    best = candidate;
                }
            }
            return best;
        };

        destructive = reference.error.at(ColorShade::shade_500);
        destructive_foreground = neutral_on(destructive);

        border = resolved_border;
        input = resolved_border;
        // focus_ring 与 primary 同色（原行为）；`ring` 是它的新名字。
        // 注意 ring **不能**取边框档：边框本来就该"薄而低调"（对底色 ~1.2:1 正常），
        // 而焦点环是键盘可达性的信号，必须 ≥ 3:1。品牌色是有对比度要求的角色，
        // 用它才符合两者的不同定位。
        focus_ring = primary;
        ring = primary;
        selection = primary.with_alpha(0.32F);

        success = reference.success.at(ColorShade::shade_500);
        // success / warning / error 色阶的明度跨度都不足以稳定达到 4.5:1，
        // 状态色上的文字一律按底色明度取中性极端档（见 destructive_foreground）。
        success_foreground = neutral_on(success);
        warning = reference.warning.at(ColorShade::shade_500);
        warning_foreground = neutral_on(warning);
        error = reference.error.at(ColorShade::shade_500);
        error_foreground = neutral_on(error);
        info = primary;
        info_foreground = primary_foreground;

        // 兼容别名（与 shadcn 名同值）。
        on_background = foreground;
        on_primary = primary_foreground;
        on_secondary = secondary_foreground;
        on_tertiary = tertiary_foreground;
        on_surface = surface_foreground;
        on_surface_variant = surface_variant_foreground;
        on_muted = muted_foreground;
        outline = border;
        outline_variant = resolved_outline_variant;
        on_success = success_foreground;
        on_warning = warning_foreground;
        on_error = error_foreground;
        on_info = info_foreground;
    }

    namespace
    {
        /**
         * 框架默认主题（shadcn/ui 对齐）—— 亮色语义色板。
         *
         * 逐角色取值，基准是 shadcn/ui 的 **neutral 基色 + blue primary**：
         * 中性阶带一点紫调（hue ≈ 285.8），primary 取 shadcn blue-600。
         *
         * 与 shadcn 的两点有意差异：
         *  - shadcn 的 `border` / `input` 与 `muted` 同值（默认主题里都是 gray-200
         *    oklch(0.922 0.004 286.32)）；NandinaUI 把 `border` 调亮一档，因为 Card
         *    的默认边框直接用它，同值会让卡片和 muted 容器糊在一起。
         *  - 多出 `surface` 家族：`surface` = `card`，`surface_variant` = `muted`，
         *    用于通用次级容器，避免所有容器都挤在同一个角色上。
         *
         * @return 默认亮色语义色板
         */
        [[nodiscard]] auto default_light_spec() -> SemanticColorSpec {
            const auto background = NanColor::from(NanOklch {.light = 1.0F});
            // 卡片比页面低 2.5% 明度：既能与 page 分开，又比 muted 块亮，靠 border 收边。
            const auto card = NanColor::from(NanOklch {
                .light = 0.975F,
                .chroma = 0.001F,
                .hue = 286.375F,
            });
            const auto foreground = NanColor::from(NanOklch {
                .light = 0.141F,
                .chroma = 0.005F,
                .hue = 285.823F,
            });
            // shadcn 经典中性默认：primary 是近黑的中性色，品牌色本身不带色相
            // （oklch(0.205 0 0)）。这样换肤时任何一处带色相的改动都很显眼，
            // 也避免框架给出一个"看起来像品牌"的默认蓝。
            const auto primary = NanColor::from(NanOklch {
                .light = 0.205F,
                .chroma = 0.0F,
                .hue = 0.0F,
            });
            const auto primary_foreground = NanColor::from(NanOklch {
                .light = 0.985F,
                .chroma = 0.0F,
                .hue = 0.0F,
            });
            const auto subtle = NanColor::from(NanOklch {
                .light = 0.967F,
                .chroma = 0.001F,
                .hue = 286.375F,
            });
            const auto subtle_foreground = NanColor::from(NanOklch {
                .light = 0.21F,
                .chroma = 0.006F,
                .hue = 285.885F,
            });
            const auto muted_foreground = NanColor::from(NanOklch {
                .light = 0.52F,
                .chroma = 0.016F,
                .hue = 285.938F,
            });
            const auto destructive = NanColor::from(NanOklch {
                .light = 0.53F,
                .chroma = 0.245F,
                .hue = 27.325F,
            });
            const auto border = NanColor::from(NanOklch {
                .light = 0.94F,
                .chroma = 0.004F,
                .hue = 286.32F,
            });
            // 焦点环要在白底和卡片上都看得见（≥ 3:1），shadcn 原来那档偏浅。
            const auto ring = NanColor::from(NanOklch {
                .light = 0.64F,
                .chroma = 0.015F,
                .hue = 286.067F,
            });

            return SemanticColorSpec {
                .background = background,
                .foreground = foreground,
                .card = card,
                .card_foreground = foreground,
                // 浮层比卡片再亮一档：Dialog / Tooltip / Select 弹层落在卡片之上。
                .popover = background,
                .popover_foreground = foreground,
                .primary = primary,
                .primary_foreground = primary_foreground,
                .secondary = subtle,
                .secondary_foreground = subtle_foreground,
                .muted = subtle,
                .muted_foreground = muted_foreground,
                .accent = subtle,
                .accent_foreground = subtle_foreground,
                .destructive = destructive,
                .destructive_foreground = NanColor::from(NanOklch {.light = 0.98F}),
                .border = border,
                .input = border,
                .ring = ring,
                .surface = background,
                .surface_foreground = foreground,
                .surface_variant = subtle,
                .surface_variant_foreground = muted_foreground,
                .tertiary = NanColor::from(NanOklch {
                    .light = 0.558F,
                    .chroma = 0.213F,
                    .hue = 302.321F,
                }),
                .tertiary_foreground = NanColor::from(NanOklch {.light = 0.98F}),
                .success = NanColor::from(NanOklch {
                    .light = 0.627F,
                    .chroma = 0.194F,
                    .hue = 149.214F,
                }),
                // 状态色上的文字统一取中性最深/最浅档：色阶自身的明度跨度不足以
                // 稳定达到 4.5:1，用中性极端档才不会随色相变化而退化。
                .success_foreground = NanColor::from(NanOklch {
                    .light = 0.1776F,
                    .chroma = 0.0F,
                    .hue = 0.0F,
                }),
                .warning = NanColor::from(NanOklch {
                    .light = 0.795F,
                    .chroma = 0.184F,
                    .hue = 86.047F,
                }),
                .warning_foreground = NanColor::from(NanOklch {
                    .light = 0.1776F,
                    .chroma = 0.0F,
                    .hue = 0.0F,
                }),
                .error = destructive,
                .error_foreground = NanColor::from(NanOklch {.light = 0.98F}),
                .info = primary,
                .info_foreground = primary_foreground,
                .selection = primary.with_alpha(0.28F),
            };
        }

        /**
         * 框架默认主题（shadcn/ui 对齐）—— 暗色语义色板。
         *
         * 暗色下 shadcn 用半透明白表达边框（`border` 10%、`input` 15%），而不是固定灰：
         * 这样边框在任何堆叠层级上都保持相对亮度，不会在更亮的卡片上"消失"。
         *
         * @return 默认暗色语义色板
         */
        [[nodiscard]] auto default_dark_spec() -> SemanticColorSpec {
            const auto background = NanColor::from(NanOklch {
                .light = 0.141F,
                .chroma = 0.005F,
                .hue = 285.823F,
            });
            const auto foreground = NanColor::from(NanOklch {.light = 0.985F});
            const auto elevated = NanColor::from(NanOklch {
                .light = 0.21F,
                .chroma = 0.006F,
                .hue = 285.885F,
            });
            // 暗色品牌色提亮到 0.72：与深底拉开 8:1（作为信号色足够），
            // 品牌面上的文字改用深色前景（与 butter / fluent / material 同一约定：
            // 暗色下亮品牌配深字，实测 7:1 以上，远优于亮字方案的 3.3:1）。
            // 暗色对应 shadcn 的 oklch(0.922 0 0)：亮灰底 + 近黑字。
            const auto primary = NanColor::from(NanOklch {
                .light = 0.922F,
                .chroma = 0.0F,
                .hue = 0.0F,
            });
            const auto primary_foreground = NanColor::from(NanOklch {
                .light = 0.205F,
                .chroma = 0.0F,
                .hue = 0.0F,
            });
            const auto subtle = NanColor::from(NanOklch {
                .light = 0.274F,
                .chroma = 0.006F,
                .hue = 286.033F,
            });
            const auto muted_foreground = NanColor::from(NanOklch {
                .light = 0.705F,
                .chroma = 0.015F,
                .hue = 286.067F,
            });
            // 危险色在暗色下取 0.58：与深底拉开 4.15:1（作为信号色足够醒目），
            // 同时浅色文字仍达 4.52:1。再亮一档就会让文字掉到 4.5 以下。
            const auto destructive = NanColor::from(NanOklch {
                .light = 0.58F,
                .chroma = 0.22F,
                .hue = 25.0F,
            });
            const auto ring = NanColor::from(NanOklch {
                .light = 0.552F,
                .chroma = 0.016F,
                .hue = 285.938F,
            });
            // 半透明边框：在任意层级的表面上都保持相对亮度。
            const auto border = NanColor::from(NanOklch {.light = 1.0F, .alpha = 0.10F});
            const auto input = NanColor::from(NanOklch {.light = 1.0F, .alpha = 0.15F});

            return SemanticColorSpec {
                .background = background,
                .foreground = foreground,
                .card = elevated,
                .card_foreground = foreground,
                .popover = elevated,
                .popover_foreground = foreground,
                .primary = primary,
                .primary_foreground = primary_foreground,
                .secondary = subtle,
                .secondary_foreground = foreground,
                .muted = subtle,
                .muted_foreground = muted_foreground,
                .accent = subtle,
                .accent_foreground = foreground,
                .destructive = destructive,
                .destructive_foreground = NanColor::from(NanOklch {.light = 0.98F}),
                .border = border,
                .input = input,
                .ring = ring,
                .surface = background,
                .surface_foreground = foreground,
                .surface_variant = subtle,
                .surface_variant_foreground = muted_foreground,
                .tertiary = NanColor::from(NanOklch {
                    .light = 0.667F,
                    .chroma = 0.295F,
                    .hue = 322.15F,
                }),
                .tertiary_foreground = NanColor::from(NanOklch {.light = 0.98F}),
                .success = NanColor::from(NanOklch {
                    .light = 0.723F,
                    .chroma = 0.219F,
                    .hue = 149.579F,
                }),
                .success_foreground = NanColor::from(NanOklch {
                    .light = 0.266F,
                    .chroma = 0.065F,
                    .hue = 152.934F,
                }),
                .warning = NanColor::from(NanOklch {
                    .light = 0.852F,
                    .chroma = 0.199F,
                    .hue = 91.936F,
                }),
                .warning_foreground = NanColor::from(NanOklch {
                    .light = 0.421F,
                    .chroma = 0.095F,
                    .hue = 57.708F,
                }),
                .error = destructive,
                .error_foreground = NanColor::from(NanOklch {.light = 0.98F}),
                .info = primary,
                .info_foreground = primary_foreground,
                .selection = primary.with_alpha(0.32F),
            };
        }
    } // namespace

    auto default_light_palette() -> NanColorScheme {
        return NanColorScheme {default_light_spec()};
    }

    auto default_dark_palette() -> NanColorScheme {
        return NanColorScheme {default_dark_spec()};
    }

    auto default_theme() -> NanTheme {
        return {.tokens = NanTokens {}, .palette = default_light_palette()};
    }
} // namespace nandina::theme
