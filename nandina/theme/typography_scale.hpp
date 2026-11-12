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

#include "font_request.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nandina::theme
{

    /// Tailwind-compatible type steps. `base` is the reference step (1.0 ratio).
    ///
    /// C++ identifiers cannot contain `-`, so Tailwind's `2xl … 9xl` are spelled `xl_2 … xl_9`
    /// here; the author-facing names (see `text_size_name`) stay `2xl … 9xl`.
    enum class TextSize : std::uint8_t {
        xs,
        sm,
        base,
        lg,
        xl,
        xl_2,
        xl_3,
        xl_4,
        xl_5,
        xl_6,
        xl_7,
        xl_8,
        xl_9,
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

    /**
     * Semantic typography roles layered on the primitive scale/leading steps.
     *
     * A role names the *purpose* of a run of text (display/heading/body/...); components and
     * pages use a role instead of hard-coding a step, and Step 2b will make the role table
     * theme-overridable. Roles only describe type metrics (size, leading, weight, slant); color
     * stays with the component's semantic color token.
     */
    enum class TypographyRole : std::uint8_t {
        display,
        heading,
        title,
        subtitle,
        body,
        label,
        caption,
    };

    inline constexpr std::size_t typography_role_count = 7;

    struct TypographyRoleStyle {
        TextSize size = TextSize::base;
        Leading leading = Leading::normal;
        int font_weight = 400;
        FontSlant slant = FontSlant::normal;
    };

    [[nodiscard]] constexpr auto typography_role_index(const TypographyRole role) noexcept
        -> std::size_t {
        return static_cast<std::size_t>(role);
    }

    /// Role indexes outside the enum fall back to `body` instead of indexing out of bounds.
    [[nodiscard]] constexpr auto sanitized_typography_role_index(const TypographyRole role) noexcept
        -> std::size_t {
        const auto index = typography_role_index(role);
        return index < typography_role_count ? index : typography_role_index(TypographyRole::body);
    }

    [[nodiscard]] constexpr auto typography_role_name(const TypographyRole role) noexcept
        -> std::string_view {
        switch (role) {
            case TypographyRole::display:
                return "display";
            case TypographyRole::heading:
                return "heading";
            case TypographyRole::title:
                return "title";
            case TypographyRole::subtitle:
                return "subtitle";
            case TypographyRole::body:
                return "body";
            case TypographyRole::label:
                return "label";
            case TypographyRole::caption:
                return "caption";
        }
        return "body";
    }

    /// Canonical role table. Weight/slant become visible through P4.7's synthetic fallback;
    /// themes can override individual roles through `NanTypeScale::roles` (Step 2b).
    inline constexpr std::array<TypographyRoleStyle, typography_role_count>
        canonical_typography_role_styles {
            TypographyRoleStyle {
                .size = TextSize::xl_4,
                .leading = Leading::tight,
                .font_weight = 600,
            },
            TypographyRoleStyle {
                .size = TextSize::xl_2,
                .leading = Leading::snug,
                .font_weight = 600,
            },
            TypographyRoleStyle {
                .size = TextSize::xl,
                .leading = Leading::snug,
                .font_weight = 600,
            },
            TypographyRoleStyle {
                .size = TextSize::lg,
                .leading = Leading::normal,
                .font_weight = 500,
            },
            TypographyRoleStyle {
                .size = TextSize::base,
                .leading = Leading::normal,
                .font_weight = 400,
            },
            TypographyRoleStyle {
                .size = TextSize::sm,
                .leading = Leading::tight,
                .font_weight = 500,
            },
            TypographyRoleStyle {
                .size = TextSize::xs,
                .leading = Leading::snug,
                .font_weight = 400,
            },
    };

    [[nodiscard]] constexpr auto typography_role_style(const TypographyRole role) noexcept
        -> TypographyRoleStyle {
        return canonical_typography_role_styles[sanitized_typography_role_index(role)];
    }

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
        /// Step 2 semantic role table; themes may replace individual roles without changing the
        /// primitive scale. Values stay in terms of TextSize/Leading so root-base overrides apply.
        std::array<TypographyRoleStyle, typography_role_count> roles =
            canonical_typography_role_styles;
        /// Step 3 user/accessibility text scale (rem semantics): token- and role-derived sizes are
        /// multiplied by this factor. Explicit `.font_size(px)` stays absolute/fixed and is not
        /// multiplied. Non-finite/non-positive values fall back to 1.0.
        float text_scale = 1.0F;
        /// Optional cap (logical px) applied to token/role sizes after `text_scale`; 0 disables
        /// the cap. This is where a "never larger than 28 px" policy lives. Applies only to
        /// scale/role-derived sizes, never to explicit px.
        float max_text_px = 0.0F;
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
            case TextSize::xl_2:
                return "2xl";
            case TextSize::xl_3:
                return "3xl";
            case TextSize::xl_4:
                return "4xl";
            case TextSize::xl_5:
                return "5xl";
            case TextSize::xl_6:
                return "6xl";
            case TextSize::xl_7:
                return "7xl";
            case TextSize::xl_8:
                return "8xl";
            case TextSize::xl_9:
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
     * Logical-pixel size of a step under a scale's root base, user text scale and optional cap.
     * Non-finite/non-positive root base, ratio or text scale falls back to the canonical/default
     * value rather than producing a NaN size that would break layout. `max_text_px > 0` caps the
     * scaled result; explicit `.font_size()` never goes through this function and stays fixed.
     */
    [[nodiscard]] constexpr auto
    text_size_px(const NanTypeScale& scale, const TextSize size) noexcept -> float {
        const auto index = text_size_index(size);
        const float root = detail::valid_positive(scale.root_base, canonical_root_base);
        const float ratio =
            detail::valid_positive(scale.ratios[index], canonical_text_size_ratios[index]);
        const float user_scale = detail::valid_positive(scale.text_scale, 1.0F);
        const float scaled = root * ratio * user_scale;
        if (std::isfinite(scale.max_text_px) && scale.max_text_px > 0.0F) {
            return scaled < scale.max_text_px ? scaled : scale.max_text_px;
        }
        return scaled;
    }

    /// Line-height factor for a leading step under a scale (multiply by the font size).
    [[nodiscard]] constexpr auto
    leading_factor(const NanTypeScale& scale, const Leading leading) noexcept -> float {
        const auto index = leading_index(leading);
        return detail::valid_positive(scale.leading[index], canonical_leading_multipliers[index]);
    }

    /// Role lookup through a (possibly theme-overridden) type scale.
    [[nodiscard]] constexpr auto
    typography_role_style(const NanTypeScale& scale, const TypographyRole role) noexcept
        -> TypographyRoleStyle {
        return scale.roles[sanitized_typography_role_index(role)];
    }

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_TYPOGRAPHY_SCALE_HPP
