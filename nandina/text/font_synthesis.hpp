//
// text/font_synthesis — synthetic bold/oblique for single-face font families.
//

#ifndef NANDINA_EXPERIMENT_TEXT_FONT_SYNTHESIS_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_SYNTHESIS_HPP

#include "../theme/font_request.hpp"

#include <algorithm>

namespace nandina::text
{
    /// Embolden amount as a fraction of the em. Matches FreeType's default
    /// `FT_GlyphSlot_Embolden()` strength (0x0AAA / 0x10000) and HarfBuzz's synthetic bold
    /// convention, so layout advances and rasterized outlines grow by the same amount.
    inline constexpr float synthetic_bold_em = static_cast<float>(0x0AAA) / 65536.0F;

    /// Oblique shear (about 12 degrees), matching FreeType's `FT_GlyphSlot_Oblique()`.
    inline constexpr float synthetic_oblique_shear = static_cast<float>(0x0366A) / 65536.0F;

    /// Synthetic styles applied when the nearest resolved face cannot satisfy a weight/slant
    /// request (the builtin family ships a single Regular face, so `bold`/`italic` would otherwise
    /// render identically).
    struct FontSynthesis {
        bool bold = false;
        bool oblique = false;
        /// Embolden strength in em units; meaningful only when `bold` is set. Lighter requests
        /// (500/600) get a proportionally weaker stroke than 700+, so the default family can
        /// express Medium / Semibold / Bold without real faces.
        float bold_em = synthetic_bold_em;

        [[nodiscard]] auto any() const noexcept -> bool {
            return bold || oblique;
        }
        auto operator==(const FontSynthesis&) const -> bool = default;
    };

    /// Requests at/above medium (500) are synthesized when the chosen face is lighter than the
    /// request; italic/oblique requests are synthesized when the chosen face is upright. Thinning
    /// a heavier face is not supported, and real faces that already match win over synthesis.
    [[nodiscard]] inline auto synthetic_font_style(
        const theme::FontRequest& request,
        const int face_weight,
        const theme::FontSlant face_slant
    ) noexcept -> FontSynthesis {
        FontSynthesis result;
        if (request.weight >= 500 && face_weight < request.weight) {
            // 500→~0.014em, 600→~0.028em, 700+→0.042em: enough contrast between steps while
            // keeping the synthetic stroke close to a real medium/bold face at UI sizes.
            const float scaled =
                synthetic_bold_em * static_cast<float>(request.weight - face_weight) / 300.0F;
            result.bold = true;
            result.bold_em = std::clamp(scaled, 0.012F, synthetic_bold_em);
        }
        result.oblique =
            request.slant != theme::FontSlant::normal && face_slant == theme::FontSlant::normal;
        return result;
    }

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_FONT_SYNTHESIS_HPP
