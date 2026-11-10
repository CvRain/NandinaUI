//
// theme/typography_scale — Tailwind-compatible type scale and leading tokens.
//
// Authoring sizes are logical (DIP) pixels: the platform content scale always maps them to
// physical pixels (P4.5a/b). The scale itself is expressed as ratios over a themeable root
// base so a theme can shift every step at once; there is no pt conversion and no physical-pixel
// font size. See docs/references/typography_scale.md.
//

#ifndef NANDINA_EXPERIMENT_THEME_TYPOGRAPHY_SCALE_HPP
#define NANDINA_EXPERIMENT_THEME_TYPOGRAPHY_SCALE_HPP

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nandina::theme
{

    /// Tailwind-compatible type steps. `base` is the reference step (1.0 ratio).
    enum class TextSize : std::uint8_t {
        xs,
        sm,
        base,
        lg,
        xl,
        xl2,
        xl3,
        xl4,
        xl5,
        xl6,
        xl7,
        xl8,
        xl9,
    };

    /// Line-height multipliers. `normal` (1.5) is the CJK-first default for body roles;
    /// compact controls may use tighter steps.
    enum class Leading : std::uint8_t {
        none,
        tight,
        snug,
        normal,
        relaxed,
        loose,
    };

    inline constexpr std::size_t text_size_count = 13;
    inline constexpr std::size_t leading_count = 6;

    /// Default root base: 1rem = 16 logical px.
    inline constexpr float canonical_root_base = 16.0F;

    /// Tailwind's default ratios (xs .. 9xl).
    inline constexpr std::array<float, text_size_count> canonical_text_size_ratios {
        0.75F, // xs  -> 12 px
        0.875F, // sm -> 14 px
        1.0F, // base -> 16 px
        1.125F, // lg -> 18 px
        1.25F, // xl  -> 20 px
        1.5F, // 2xl -> 24 px
        1.875F, // 3xl -> 30 px
        2.25F, // 4xl -> 36 px
        3.0F, // 5xl -> 48 px
        3.75F, // 6xl -> 60 px
        4.5F, // 7xl -> 72 px
        6.0F, // 8xl -> 96 px
        8.0F, // 9xl -> 128 px
    };

    /// Tailwind's leading scale (none/tight/snug/normal/relaxed/loose).
    inline constexpr std::array<float, leading_count> canonical_leading_multipliers {
        1.0F,
        1.25F,
        1.375F,
        1.5F,
        1.625F,
        2.0F,
    };

    /**
     * Themeable type scale. Themes override `root_base`, individual ratios or individual
     * leading multipliers; semantic roles (P4.6 Step 2) consume this table.
     */
    struct NanTypeScale {
        float root_base = canonical_root_base;
        std::array<float, text_size_count> ratios = canonical_text_size_ratios;
        std::array<float, leading_count> leading = canonical_leading_multipliers;
    };

    [[nodiscard]] constexpr auto text_size_index(const TextSize size) noexcept -> std::size_t {
        return static_cast<std::size_t>(size);
    }

    [[nodiscard]] constexpr auto leading_index(const Leading leading) noexcept -> std::size_t {
        return static_cast<std::size_t>(leading);
    }

    [[nodiscard]] constexpr auto text_size_name(const TextSize size) noexcept -> std::string_view {
        switch (size) {
            case TextSize::xs:
                return "xs";
            case TextSize::sm:
                return "sm";
            case TextSize::base:
                return "base";
            case TextSize::lg:
                return "lg";
            case TextSize::xl:
                return "xl";
            case TextSize::xl2:
                return "2xl";
            case TextSize::xl3:
                return "3xl";
            case TextSize::xl4:
                return "4xl";
            case TextSize::xl5:
                return "5xl";
            case TextSize::xl6:
                return "6xl";
            case TextSize::xl7:
                return "7xl";
            case TextSize::xl8:
                return "8xl";
            case TextSize::xl9:
                return "9xl";
        }
        return "base";
    }

    [[nodiscard]] constexpr auto leading_name(const Leading leading) noexcept -> std::string_view {
        switch (leading) {
            case Leading::none:
                return "none";
            case Leading::tight:
                return "tight";
            case Leading::snug:
                return "snug";
            case Leading::normal:
                return "normal";
            case Leading::relaxed:
                return "relaxed";
            case Leading::loose:
                return "loose";
        }
        return "normal";
    }

    /// Canonical ratio for a step, independent of any theme override.
    [[nodiscard]] constexpr auto text_size_ratio(const TextSize size) noexcept -> float {
        return canonical_text_size_ratios[text_size_index(size)];
    }

    /// Canonical multiplier for a leading step, independent of any theme override.
    [[nodiscard]] constexpr auto leading_multiplier(const Leading leading) noexcept -> float {
        return canonical_leading_multipliers[leading_index(leading)];
    }

    namespace detail
    {
        [[nodiscard]] constexpr auto
        valid_positive(const float value, const float fallback) noexcept -> float {
            return std::isfinite(value) && value > 0.0F ? value : fallback;
        }
    } // namespace detail

    /**
     * Logical-pixel size of a step under a scale's root base.
     * Non-finite/non-positive root base or ratio falls back to the canonical value rather than
     * producing a NaN size that would break layout.
     */
    [[nodiscard]] constexpr auto
    text_size_px(const NanTypeScale& scale, const TextSize size) noexcept -> float {
        const auto index = text_size_index(size);
        const float root = detail::valid_positive(scale.root_base, canonical_root_base);
        const float ratio =
            detail::valid_positive(scale.ratios[index], canonical_text_size_ratios[index]);
        return root * ratio;
    }

    /// Line-height factor for a leading step under a scale (multiply by the font size).
    [[nodiscard]] constexpr auto
    leading_factor(const NanTypeScale& scale, const Leading leading) noexcept -> float {
        const auto index = leading_index(leading);
        return detail::valid_positive(scale.leading[index], canonical_leading_multipliers[index]);
    }

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_TYPOGRAPHY_SCALE_HPP
