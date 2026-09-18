//
// theme/nan_style — component rules backed by theme tokens or literals.
//

#include "nan_style.hpp"

namespace nandina::theme
{
    namespace
    {
        /**
         * 解析颜色操作数（字面量、颜色 token 或 tone 强调色引用）。
         *
         * @param tone 当前 Button tone；accent 引用依赖它（缺省按 primary）。
         */
        [[nodiscard]] auto resolve_color_operand(
            const NanTheme& theme,
            const ColorOperand& operand,
            const std::optional<ButtonTone> tone = std::nullopt
        ) -> NanColor {
            if (const auto* literal = std::get_if<NanColor>(&operand)) {
                return *literal;
            }
            if (const auto* accent = std::get_if<ToneAccentRef>(&operand)) {
                const auto [accent_color, on_accent] = button_accent(theme.palette, tone);
                return accent->on_accent ? on_accent : accent_color;
            }
            switch (std::get<ColorToken>(operand)) {
                case ColorToken::background:
                    return theme.palette.background;
                case ColorToken::foreground:
                    return theme.palette.foreground;
                case ColorToken::card:
                    return theme.palette.card;
                case ColorToken::card_foreground:
                    return theme.palette.card_foreground;
                case ColorToken::popover:
                    return theme.palette.popover;
                case ColorToken::popover_foreground:
                    return theme.palette.popover_foreground;
                case ColorToken::primary:
                    return theme.palette.primary;
                case ColorToken::primary_foreground:
                    return theme.palette.primary_foreground;
                case ColorToken::secondary:
                    return theme.palette.secondary;
                case ColorToken::secondary_foreground:
                    return theme.palette.secondary_foreground;
                case ColorToken::muted:
                    return theme.palette.muted;
                case ColorToken::muted_foreground:
                    return theme.palette.muted_foreground;
                case ColorToken::accent:
                    return theme.palette.accent;
                case ColorToken::accent_foreground:
                    return theme.palette.accent_foreground;
                case ColorToken::destructive:
                    return theme.palette.destructive;
                case ColorToken::destructive_foreground:
                    return theme.palette.destructive_foreground;
                case ColorToken::border:
                    return theme.palette.border;
                case ColorToken::input:
                    return theme.palette.input;
                case ColorToken::ring:
                    return theme.palette.ring;
                case ColorToken::surface:
                    return theme.palette.surface;
                case ColorToken::surface_foreground:
                    return theme.palette.surface_foreground;
                case ColorToken::surface_variant:
                    return theme.palette.surface_variant;
                case ColorToken::surface_variant_foreground:
                    return theme.palette.surface_variant_foreground;
                case ColorToken::tertiary:
                    return theme.palette.tertiary;
                case ColorToken::tertiary_foreground:
                    return theme.palette.tertiary_foreground;
                case ColorToken::success:
                    return theme.palette.success;
                case ColorToken::success_foreground:
                    return theme.palette.success_foreground;
                case ColorToken::warning:
                    return theme.palette.warning;
                case ColorToken::warning_foreground:
                    return theme.palette.warning_foreground;
                case ColorToken::error:
                    return theme.palette.error;
                case ColorToken::error_foreground:
                    return theme.palette.error_foreground;
                case ColorToken::info:
                    return theme.palette.info;
                case ColorToken::info_foreground:
                    return theme.palette.info_foreground;
                case ColorToken::on_background:
                    return theme.palette.on_background;
                case ColorToken::on_primary:
                    return theme.palette.on_primary;
                case ColorToken::on_secondary:
                    return theme.palette.on_secondary;
                case ColorToken::on_tertiary:
                    return theme.palette.on_tertiary;
                case ColorToken::on_surface:
                    return theme.palette.on_surface;
                case ColorToken::on_surface_variant:
                    return theme.palette.on_surface_variant;
                case ColorToken::on_muted:
                    return theme.palette.on_muted;
                case ColorToken::outline:
                    return theme.palette.outline;
                case ColorToken::outline_variant:
                    return theme.palette.outline_variant;
                case ColorToken::on_success:
                    return theme.palette.on_success;
                case ColorToken::on_warning:
                    return theme.palette.on_warning;
                case ColorToken::on_error:
                    return theme.palette.on_error;
                case ColorToken::on_info:
                    return theme.palette.on_info;
                case ColorToken::focus_ring:
                    return theme.palette.focus_ring;
                case ColorToken::selection:
                    return theme.palette.selection;
            }
            return theme.palette.primary;
        }
    } // namespace

    auto button_accent(const NanColorScheme& palette, const std::optional<ButtonTone> tone)
        -> std::pair<NanColor, NanColor> {
        switch (tone.value_or(ButtonTone::primary)) {
            case ButtonTone::primary:
                return {palette.primary, palette.on_primary};
            case ButtonTone::secondary:
                return {palette.secondary, palette.on_secondary};
            case ButtonTone::neutral:
                return {palette.surface_variant, palette.on_surface_variant};
            case ButtonTone::danger:
                return {palette.error, palette.on_error};
        }
        return {palette.primary, palette.on_primary};
    }

    auto resolve_theme_color(
        const NanTheme& theme,
        const ThemeColor& value,
        const std::optional<ButtonTone> tone
    ) -> NanColor {
        if (const auto* literal = std::get_if<NanColor>(&value.value())) {
            return *literal;
        }
        if (const auto* accent = std::get_if<ToneAccentRef>(&value.value())) {
            const auto [accent_color, on_accent] = button_accent(theme.palette, tone);
            return accent->on_accent ? on_accent : accent_color;
        }
        if (const auto* transform = std::get_if<ColorTransform>(&value.value())) {
            const auto lhs = resolve_color_operand(theme, transform->lhs, tone);
            switch (transform->op) {
                case ColorTransformOp::transparent:
                    return lhs.with_alpha(0.0F);
                case ColorTransformOp::with_alpha:
                    return lhs.with_alpha(resolve_theme_scalar(theme, transform->factor));
                case ColorTransformOp::mix:
                    return lhs.mix(
                        resolve_color_operand(theme, *transform->rhs, tone),
                        resolve_theme_scalar(theme, transform->factor)
                    );
            }
        }
        return resolve_color_operand(theme, std::get<ColorToken>(value.value()), tone);
    }

    auto resolve_theme_scalar(const NanTheme& theme, const ThemeScalar& value) -> float {
        if (const auto* literal = std::get_if<float>(&value.value())) {
            return *literal;
        }
        switch (std::get<ScalarToken>(value.value())) {
            case ScalarToken::spacing_xs:
                return theme.tokens.spacing.xs;
            case ScalarToken::spacing_sm:
                return theme.tokens.spacing.sm;
            case ScalarToken::spacing_md:
                return theme.tokens.spacing.md;
            case ScalarToken::spacing_lg:
                return theme.tokens.spacing.lg;
            case ScalarToken::spacing_xl:
                return theme.tokens.spacing.xl;
            case ScalarToken::radius_sm:
                return theme.tokens.radius.sm;
            case ScalarToken::radius_md:
                return theme.tokens.radius.md;
            case ScalarToken::radius_lg:
                return theme.tokens.radius.lg;
            case ScalarToken::radius_full:
                return theme.tokens.radius.full;
            case ScalarToken::border_thin:
                return theme.tokens.border.thin;
            case ScalarToken::border_medium:
                return theme.tokens.border.medium;
            case ScalarToken::border_focus_ring:
                return theme.tokens.border.focus_ring;
            case ScalarToken::opacity_disabled:
                return theme.tokens.opacity.disabled;
            case ScalarToken::opacity_hover_overlay:
                return theme.tokens.opacity.hover_overlay;
            case ScalarToken::opacity_pressed_overlay:
                return theme.tokens.opacity.pressed_overlay;
            case ScalarToken::typography_label_sm:
                return theme.tokens.typography.label_sm;
            case ScalarToken::typography_label_md:
                return theme.tokens.typography.label_md;
            case ScalarToken::typography_label_lg:
                return theme.tokens.typography.label_lg;
            case ScalarToken::motion_short_duration:
                return theme.tokens.motion.short_duration;
            case ScalarToken::motion_medium_duration:
                return theme.tokens.motion.medium_duration;
            case ScalarToken::motion_long_duration:
                return theme.tokens.motion.long_duration;
        }
        return 0.0F;
    }

    auto ButtonRuleSelector::matches(
        const ButtonTone current_tone,
        const ButtonTreatment current_treatment,
        const ButtonSize current_size,
        const ButtonVisualState current_state
    ) const noexcept -> bool {
        return (!tone || *tone == current_tone) && (!treatment || *treatment == current_treatment)
            && (!size || *size == current_size) && (!state || *state == current_state);
    }

    void NanStyle::add_button_rule(ButtonStyleRule rule) {
        button_rules_.push_back(std::move(rule));
    }

    auto NanStyle::button_rules() const noexcept -> const std::vector<ButtonStyleRule>& {
        return button_rules_;
    }

    void NanStyle::add_text_field_rule(TextFieldStyleRule rule) {
        text_field_rules_.push_back(std::move(rule));
    }

    auto NanStyle::text_field_rules() const noexcept -> const std::vector<TextFieldStyleRule>& {
        return text_field_rules_;
    }

    auto default_style() -> std::shared_ptr<const NanStyle> {
        static const auto style = std::make_shared<const NanStyle>();
        return style;
    }

} // namespace nandina::theme
