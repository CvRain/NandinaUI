//
// foundation/contrast - WCAG 2.x color contrast helpers.
//

#ifndef NANDINA_EXPERIMENT_FOUNDATION_CONTRAST_HPP
#define NANDINA_EXPERIMENT_FOUNDATION_CONTRAST_HPP

#include "nandina_color.hpp"

namespace nandina::foundation
{
    /// WCAG 2.x minimum contrast ratio for normal text (AA).
    inline constexpr float nan_contrast_aa_text = 4.5F;
    /// WCAG 2.x minimum contrast ratio for large text (AA).
    inline constexpr float nan_contrast_aa_large_text = 3.0F;

    /// WCAG 2.x relative luminance of an opaque color, in [0, 1].
    /// Alpha is ignored by contract; callers compose against a background first.
    [[nodiscard]] inline auto nan_relative_luminance(const NanColor& color) -> float {
        const auto rgb = color.to<NanRgb>();
        return 0.2126F * nan_srgb_to_linear_channel(rgb.red)
             + 0.7152F * nan_srgb_to_linear_channel(rgb.green)
             + 0.0722F * nan_srgb_to_linear_channel(rgb.blue);
    }

    /// WCAG 2.x contrast ratio in [1, 21], symmetric in its arguments.
    [[nodiscard]] inline auto
    nan_contrast_ratio(const NanColor& first, const NanColor& second) -> float {
        const float first_luminance = nan_relative_luminance(first);
        const float second_luminance = nan_relative_luminance(second);
        const float lighter = std::max(first_luminance, second_luminance);
        const float darker = std::min(first_luminance, second_luminance);
        return (lighter + 0.05F) / (darker + 0.05F);
    }
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_FOUNDATION_CONTRAST_HPP
