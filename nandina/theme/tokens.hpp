//
// theme/tokens — minimal design tokens for v3 widgets.
//

#ifndef NANDINA_EXPERIMENT_THEME_TOKENS_HPP
#define NANDINA_EXPERIMENT_THEME_TOKENS_HPP

namespace nandina::theme
{

    struct NanSpacingTokens {
        float xs = 4.0F;
        float sm = 8.0F;
        float md = 12.0F;
        float lg = 16.0F;
        float xl = 24.0F;
    };

    struct NanRadiusTokens {
        float sm = 6.0F;
        float md = 10.0F;
        float lg = 14.0F;
        float full = 9999.0F;
    };

    struct NanBorderTokens {
        float thin = 1.0F;
        float medium = 2.0F;
        float focus_ring = 2.0F;
    };

    struct NanOpacityTokens {
        float disabled = 0.45F;
        float hover_overlay = 0.10F;
        float pressed_overlay = 0.16F;
    };

    struct NanTypographyTokens {
        float label_sm = 14.0F;
        float label_md = 16.0F;
        float label_lg = 18.0F;
    };

    struct NanMotionTokens {
        float short_duration = 0.12F;
        float medium_duration = 0.20F;
        float long_duration = 0.32F;
    };

    struct NanTokens {
        NanSpacingTokens spacing;
        NanRadiusTokens radius;
        NanBorderTokens border;
        NanOpacityTokens opacity;
        NanTypographyTokens typography;
        NanMotionTokens motion;
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_TOKENS_HPP
