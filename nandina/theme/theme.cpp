//
// theme/theme — built-in reference scales and semantic palette generation.
//

#include "theme.hpp"

namespace nandina::theme
{
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

    NanColorScheme::NanColorScheme(
        const NanReferencePalette& reference,
        const ColorAppearance appearance,
        const PaletteVariantPolicy policy,
        GeneratedTag
    ) {
        const auto brand = brand_shade(appearance, policy);
        const bool dark = appearance == ColorAppearance::dark;

        background = reference.neutral.at(dark ? ColorShade::shade_950 : ColorShade::shade_50);
        on_background = reference.neutral.at(dark ? ColorShade::shade_50 : ColorShade::shade_950);
        surface = reference.neutral.at(dark ? ColorShade::shade_900 : ColorShade::shade_100);
        on_surface = on_background;
        surface_variant = reference.neutral.at(dark ? ColorShade::shade_800 : ColorShade::shade_200);
        on_surface_variant = reference.neutral.at(dark ? ColorShade::shade_400 : ColorShade::shade_700);
        outline = reference.neutral.at(dark ? ColorShade::shade_600 : ColorShade::shade_500);
        outline_variant = reference.neutral.at(dark ? ColorShade::shade_800 : ColorShade::shade_300);

        primary = reference.primary.at(brand);
        on_primary = reference.primary.at(dark ? policy.dark_on_brand : policy.light_on_brand);
        secondary = reference.secondary.at(brand);
        on_secondary = reference.secondary.at(ColorShade::shade_50);
        tertiary = reference.tertiary.at(brand);
        on_tertiary = reference.tertiary.at(ColorShade::shade_50);
        success = reference.success.at(ColorShade::shade_500);
        on_success = reference.success.at(ColorShade::shade_950);
        warning = reference.warning.at(ColorShade::shade_500);
        on_warning = reference.warning.at(ColorShade::shade_950);
        error = reference.error.at(ColorShade::shade_500);
        on_error = reference.error.at(ColorShade::shade_50);
        focus_ring = primary;
        selection = primary.with_alpha(0.32F);
    }

    auto default_light_palette() -> NanColorScheme {
        return make_color_scheme(default_reference_palette(), ColorAppearance::light);
    }

    auto default_dark_palette() -> NanColorScheme {
        return make_color_scheme(default_reference_palette(), ColorAppearance::dark);
    }

    auto default_theme() -> NanTheme {
        return {.tokens = NanTokens {}, .palette = default_light_palette()};
    }
} // namespace nandina::theme
