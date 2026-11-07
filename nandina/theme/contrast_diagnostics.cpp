// theme/contrast_diagnostics — explicit scenario scoped contrast checks.
#include "contrast_diagnostics.hpp"

#include "theme.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

namespace nandina::theme
{
    namespace
    {
        template<typename Alternative>
        [[nodiscard]] auto operand_role_name(const Alternative& alternative) -> std::string {
            if constexpr (std::same_as<Alternative, ColorToken>) {
                return std::string(color_token_name(alternative));
            }
            else if constexpr (std::same_as<Alternative, NanColor>) {
                return "literal";
            }
            else {
                return alternative.on_accent ? "tone.on_accent" : "tone.accent";
            }
        }

        [[nodiscard]] auto operand_role_name(const ColorOperand& operand) -> std::string {
            return std::visit([](const auto& value) { return operand_role_name(value); }, operand);
        }

        [[nodiscard]] auto color_role_name(const ThemeColor& value) -> std::string {
            return std::visit(
                [](const auto& alternative) -> std::string {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::same_as<Alternative, ColorTransform>) {
                        const auto lhs = operand_role_name(alternative.lhs);
                        switch (alternative.op) {
                            case ColorTransformOp::with_alpha:
                                return "with_alpha(" + lhs + ")";
                            case ColorTransformOp::transparent:
                                return "transparent(" + lhs + ")";
                            case ColorTransformOp::mix:
                                return "mix(" + lhs + ", " + operand_role_name(*alternative.rhs)
                                    + ")";
                        }
                        return "unknown_transform";
                    }
                    else {
                        return operand_role_name(alternative);
                    }
                },
                value.value()
            );
        }

        [[nodiscard]] auto requires_tone(const ThemeColor& value) -> bool {
            if (std::holds_alternative<ToneAccentRef>(value.value())) {
                return true;
            }
            if (const auto* transform = std::get_if<ColorTransform>(&value.value())) {
                return std::holds_alternative<ToneAccentRef>(transform->lhs)
                    || (transform->op == ColorTransformOp::mix
                        && std::holds_alternative<ToneAccentRef>(*transform->rhs));
            }
            return false;
        }

        [[nodiscard]] auto appearance_name(const ColorAppearance appearance) -> std::string_view {
            return appearance == ColorAppearance::dark ? "dark" : "light";
        }

        [[nodiscard]] auto tone_name(const ButtonTone tone) -> std::string_view {
            switch (tone) {
                case ButtonTone::primary:
                    return "primary";
                case ButtonTone::secondary:
                    return "secondary";
                case ButtonTone::neutral:
                    return "neutral";
                case ButtonTone::danger:
                    return "danger";
            }
            return "unknown";
        }

        [[nodiscard]] auto layer_sample(const NanColor& color) -> ContrastLayer {
            return ContrastLayer {.resolved = color, .converted = color.to<NanRgb>()};
        }

        [[nodiscard]] auto status_name(const ContrastStatus status) -> std::string_view {
            switch (status) {
                case ContrastStatus::passed:
                    return "passed";
                case ContrastStatus::failed:
                    return "failed";
                case ContrastStatus::insufficient_context:
                    return "insufficient_context";
            }
            return "unknown";
        }
    } // namespace

    auto default_contrast_profile() -> ContrastProfile {
        const auto token = [](const ColorToken value) { return ThemeColor::token(value); };
        const auto text = foundation::nan_contrast_aa_text;
        const auto large = foundation::nan_contrast_aa_large_text;
        return ContrastProfile {
            .rules = {
                {.name = "foreground/background",
                 .foreground = token(ColorToken::foreground),
                 .background = {token(ColorToken::background)},
                 .required_ratio = text},
                {.name = "card_foreground/card",
                 .foreground = token(ColorToken::card_foreground),
                 .background = {token(ColorToken::card)},
                 .required_ratio = text},
                {.name = "popover_foreground/popover",
                 .foreground = token(ColorToken::popover_foreground),
                 .background = {token(ColorToken::popover)},
                 .required_ratio = text},
                {.name = "muted_foreground/background",
                 .foreground = token(ColorToken::muted_foreground),
                 .background = {token(ColorToken::background)},
                 .required_ratio = text},
                {.name = "muted_foreground/muted",
                 .foreground = token(ColorToken::muted_foreground),
                 .background = {token(ColorToken::muted)},
                 .required_ratio = text},
                {.name = "secondary_foreground/secondary",
                 .foreground = token(ColorToken::secondary_foreground),
                 .background = {token(ColorToken::secondary)},
                 .required_ratio = text},
                {.name = "accent_foreground/accent",
                 .foreground = token(ColorToken::accent_foreground),
                 .background = {token(ColorToken::accent)},
                 .required_ratio = text},
                {.name = "primary_foreground/primary",
                 .foreground = token(ColorToken::primary_foreground),
                 .background = {token(ColorToken::primary)},
                 .required_ratio = text},
                {.name = "destructive_foreground/destructive",
                 .foreground = token(ColorToken::destructive_foreground),
                 .background = {token(ColorToken::destructive)},
                 .required_ratio = large},
                {.name = "error_foreground/error",
                 .foreground = token(ColorToken::error_foreground),
                 .background = {token(ColorToken::error)},
                 .required_ratio = large},
                {.name = "success_foreground/success",
                 .foreground = token(ColorToken::success_foreground),
                 .background = {token(ColorToken::success)},
                 .required_ratio = large},
                {.name = "warning_foreground/warning",
                 .foreground = token(ColorToken::warning_foreground),
                 .background = {token(ColorToken::warning)},
                 .required_ratio = large},
                {.name = "ring/background",
                 .foreground = token(ColorToken::ring),
                 .background = {token(ColorToken::background)},
                 .required_ratio = large},
                {.name = "primary/background",
                 .foreground = token(ColorToken::primary),
                 .background = {token(ColorToken::background)},
                 .required_ratio = large},
                {.name = "destructive/background",
                 .foreground = token(ColorToken::destructive),
                 .background = {token(ColorToken::background)},
                 .required_ratio = large},
            },
        };
    }

    auto evaluate_contrast(
        const ThemeSchemeContent& content,
        const ColorAppearance appearance,
        const ContrastProfile& profile,
        const std::optional<ThemeSchemeId> scheme,
        const foundation::CompositeConfig config
    ) -> std::vector<ContrastDiagnostic> {
        const NanTheme theme {content.tokens, content.palette};
        std::vector<ContrastDiagnostic> diagnostics;
        diagnostics.reserve(profile.rules.size());
        for (const auto& rule: profile.rules) {
            ContrastDiagnostic diagnostic {
                .rule = rule.name,
                .state = rule.state,
                .scheme = scheme,
                .appearance = appearance,
                .required = rule.required,
                .required_ratio = rule.required_ratio,
                .config = config,
                .tone = rule.tone,
            };
            diagnostic.foreground_role = color_role_name(rule.foreground);
            if (!rule.tone && requires_tone(rule.foreground)) {
                diagnostic.status = ContrastStatus::insufficient_context;
                diagnostic.message = "foreground requires explicit tone context";
                diagnostics.push_back(std::move(diagnostic));
                continue;
            }
            const auto foreground = resolve_theme_color(theme, rule.foreground, rule.tone);
            diagnostic.foreground = layer_sample(foreground);
            std::vector<NanColor> layers;
            layers.reserve(rule.background.size());
            for (const auto& value: rule.background) {
                diagnostic.background_roles.push_back(color_role_name(value));
                if (!rule.tone && requires_tone(value)) {
                    diagnostic.message = "background requires explicit tone context";
                    break;
                }
                const auto resolved = resolve_theme_color(theme, value, rule.tone);
                diagnostic.background.push_back(layer_sample(resolved));
                layers.push_back(resolved);
                if (resolved.alpha() >= 1.0F) {
                    break;
                }
            }
            if (!diagnostic.message.empty()) {
                diagnostic.status = ContrastStatus::insufficient_context;
                diagnostics.push_back(std::move(diagnostic));
                continue;
            }
            if (!std::isfinite(diagnostic.required_ratio) || diagnostic.required_ratio <= 0.0F) {
                diagnostic.status = ContrastStatus::insufficient_context;
                diagnostic.message = "rule has no usable required ratio";
                diagnostics.push_back(std::move(diagnostic));
                continue;
            }
            auto effective = foundation::nan_composite_over_stack(foreground, layers, config);
            if (!effective) {
                diagnostic.status = ContrastStatus::insufficient_context;
                diagnostic.message = std::string(foundation::describe(effective.error()));
                diagnostics.push_back(std::move(diagnostic));
                continue;
            }
            auto background = foundation::nan_composite_background_stack(layers, config);
            if (!background) {
                // Defensive: over_stack already validated the same stack.
                diagnostic.status = ContrastStatus::insufficient_context;
                diagnostic.message = std::string(foundation::describe(background.error()));
                diagnostics.push_back(std::move(diagnostic));
                continue;
            }
            const float measured = foundation::nan_contrast_ratio(*effective, *background);
            diagnostic.measured_ratio = measured;
            diagnostic.effective_foreground = *effective;
            diagnostic.effective_background = *background;
            if (!std::isfinite(measured)) {
                diagnostic.status = ContrastStatus::insufficient_context;
                diagnostic.message = "measured contrast ratio is not finite";
            }
            else if (measured >= diagnostic.required_ratio) {
                diagnostic.status = ContrastStatus::passed;
            }
            else {
                diagnostic.status = ContrastStatus::failed;
                diagnostic.message = "contrast ratio " + std::to_string(measured) + " below "
                    + std::to_string(diagnostic.required_ratio);
            }
            diagnostics.push_back(std::move(diagnostic));
        }
        return diagnostics;
    }

    auto has_required_contrast_failure(std::span<const ContrastDiagnostic> diagnostics) -> bool {
        return std::ranges::any_of(diagnostics, [](const ContrastDiagnostic& diagnostic) {
            return diagnostic.required && diagnostic.status == ContrastStatus::failed;
        });
    }

    auto has_insufficient_contrast_context(std::span<const ContrastDiagnostic> diagnostics)
        -> bool {
        return std::ranges::any_of(diagnostics, [](const ContrastDiagnostic& diagnostic) {
            return diagnostic.status == ContrastStatus::insufficient_context;
        });
    }

    auto describe(const ContrastDiagnostic& diagnostic) -> std::string {
        std::string result;
        result += diagnostic.rule;
        result += " (";
        result += appearance_name(diagnostic.appearance);
        if (diagnostic.scheme) {
            result += ", ";
            result += diagnostic.scheme->text();
        }
        else {
            result += ", anonymous";
        }
        if (!diagnostic.state.empty()) {
            result += ", ";
            result += diagnostic.state;
        }
        result += "): ";
        result += status_name(diagnostic.status);
        if (diagnostic.measured_ratio) {
            result += " ratio=";
            result += std::to_string(*diagnostic.measured_ratio);
            result += " required=";
            result += std::to_string(diagnostic.required_ratio);
        }
        if (!diagnostic.message.empty()) {
            result += " — ";
            result += diagnostic.message;
        }
        result += " [";
        result += diagnostic.foreground_role.empty() ? "foreground" : diagnostic.foreground_role;
        result += " over ";
        for (std::size_t index = 0; index < diagnostic.background_roles.size(); ++index) {
            if (index != 0) {
                result += " -> ";
            }
            result += diagnostic.background_roles[index];
        }
        result += "]";
        result += " composition=";
        result += diagnostic.config.space == foundation::CompositeSpace::encoded_srgb
            ? "encoded_srgb"
            : "unsupported";
        result += " gamut=linear_rgb_clip quantization=float_no_u8";
        if (diagnostic.tone) {
            result += " tone=";
            result += tone_name(*diagnostic.tone);
        }
        return result;
    }
} // namespace nandina::theme
