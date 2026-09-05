//
// theme/nan_style — component rules backed by theme tokens or literals.
//

#ifndef NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP
#define NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP

#include "theme.hpp"
#include "visual_state.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace nandina::theme
{
    enum class ColorToken {
        background,
        on_background,
        primary,
        on_primary,
        secondary,
        on_secondary,
        tertiary,
        on_tertiary,
        surface,
        on_surface,
        surface_variant,
        on_surface_variant,
        outline,
        outline_variant,
        success,
        on_success,
        warning,
        on_warning,
        error,
        on_error,
        focus_ring,
        selection,
    };

    enum class ScalarToken {
        spacing_xs,
        spacing_sm,
        spacing_md,
        spacing_lg,
        spacing_xl,
        radius_sm,
        radius_md,
        radius_lg,
        radius_full,
        border_thin,
        border_medium,
        border_focus_ring,
        opacity_disabled,
        opacity_hover_overlay,
        opacity_pressed_overlay,
        typography_label_sm,
        typography_label_md,
        typography_label_lg,
        motion_short_duration,
        motion_medium_duration,
        motion_long_duration,
    };

    template<typename T, typename Token>
    class ThemeValue {
    public:
        [[nodiscard]] static auto token(Token token) -> ThemeValue {
            return ThemeValue(token);
        }

        [[nodiscard]] static auto literal(T value) -> ThemeValue {
            return ThemeValue(std::move(value));
        }

        [[nodiscard]] auto value() const -> const std::variant<T, Token>& {
            return value_;
        }

    private:
        explicit ThemeValue(Token token): value_(token) {}
        explicit ThemeValue(T value): value_(std::move(value)) {}

        std::variant<T, Token> value_;
    };

    using ThemeScalar = ThemeValue<float, ScalarToken>;

    // ─── 颜色表达式 ────────────────────────────────────────────────────────────
    //
    // ThemeColor 除 token / 字面量外，还支持浅层变换（with_alpha / transparent /
    // mix），以及「当前 tone 强调色」引用（accent / on_accent），使配方规则能表达
    // 「primary 的 hover 覆盖层」「surface 透明」「filled 的反色文本」等语义。
    // 变换的操作数不嵌套表达式（保持浅层、可静态推导）。

    /** 引用当前 Button tone 的强调色：on_accent=false 取 accent，否则取 on_accent。 */
    using ToneAccentRef = struct ToneAccentRef {
        bool on_accent = false;
    };

    inline constexpr auto accent_ref = ToneAccentRef {};
    inline constexpr auto on_accent_ref = ToneAccentRef {.on_accent = true};

    /** 颜色变换的操作数：字面量、颜色 token 或 tone 强调色引用。 */
    using ColorOperand = std::variant<NanColor, ColorToken, ToneAccentRef>;

    /** 颜色变换类型。 */
    enum class ColorTransformOp {
        with_alpha,  // lhs.with_alpha(factor)
        transparent, // lhs.with_alpha(0)
        mix,         // lhs.mix(rhs, factor)
    };

    /** 浅层颜色变换（操作数不嵌套表达式）。 */
    using ColorTransform = struct ColorTransform {
        ColorTransformOp op;
        ColorOperand lhs;
        std::optional<ColorOperand> rhs; // mix 的右操作数；with_alpha / transparent 忽略
        ThemeScalar factor;              // with_alpha 的 alpha；mix 的混合因子；transparent 忽略
    };

    /** 颜色值：字面量 | 颜色 token | tone 强调色引用 | 浅层变换。 */
    class ThemeColor {
    public:
        [[nodiscard]] static auto token(ColorToken token) -> ThemeColor {
            return ThemeColor(token);
        }

        [[nodiscard]] static auto literal(NanColor value) -> ThemeColor {
            return ThemeColor(std::move(value));
        }

        /** @return 当前 tone 的强调色引用（随解析时的 tone 解析）。 */
        [[nodiscard]] static auto accent() -> ThemeColor {
            return ThemeColor(accent_ref);
        }

        /** @return 当前 tone 的反色（强调色上的文本色）引用。 */
        [[nodiscard]] static auto on_accent() -> ThemeColor {
            return ThemeColor(on_accent_ref);
        }

        /** @return source.with_alpha(factor) 的颜色表达式。 */
        [[nodiscard]] static auto with_alpha(ColorOperand source, ThemeScalar factor) -> ThemeColor {
            return ThemeColor(ColorTransform {
                .op = ColorTransformOp::with_alpha,
                .lhs = std::move(source),
                .factor = std::move(factor),
            });
        }

        /** @return source.with_alpha(0)，即完全透明。 */
        [[nodiscard]] static auto transparent(ColorOperand source) -> ThemeColor {
            return ThemeColor(ColorTransform {
                .op = ColorTransformOp::transparent,
                .lhs = std::move(source),
                .factor = ThemeScalar::literal(0.0F),
            });
        }

        /** @return lhs.mix(rhs, factor) 的颜色表达式。 */
        [[nodiscard]] static auto mix(ColorOperand lhs, ColorOperand rhs, ThemeScalar factor)
            -> ThemeColor {
            return ThemeColor(ColorTransform {
                .op = ColorTransformOp::mix,
                .lhs = std::move(lhs),
                .rhs = std::move(rhs),
                .factor = std::move(factor),
            });
        }

        [[nodiscard]] auto value() const
            -> const std::variant<NanColor, ColorToken, ToneAccentRef, ColorTransform>& {
            return value_;
        }

    private:
        explicit ThemeColor(ColorToken token): value_(token) {}
        explicit ThemeColor(NanColor value): value_(std::move(value)) {}
        explicit ThemeColor(ToneAccentRef accent): value_(accent) {}
        explicit ThemeColor(ColorTransform transform): value_(std::move(transform)) {}

        std::variant<NanColor, ColorToken, ToneAccentRef, ColorTransform> value_;
    };

    /**
     * 解析当前 tone 的强调色对（accent, on_accent）。
     * tone 为 nullopt 时按 primary 处理（非 Button 语境下 accent 引用无意义）。
     */
    [[nodiscard]] auto
    button_accent(const NanColorScheme& palette, std::optional<ButtonTone> tone)
        -> std::pair<NanColor, NanColor>;

    [[nodiscard]] auto resolve_theme_color(
        const NanTheme& theme,
        const ThemeColor& value,
        std::optional<ButtonTone> tone = std::nullopt
    ) -> NanColor;
    [[nodiscard]] auto resolve_theme_scalar(const NanTheme& theme, const ThemeScalar& value)
        -> float;

    struct ButtonRuleSelector {
        std::optional<ButtonTone> tone;
        std::optional<ButtonTreatment> treatment;
        std::optional<ButtonSize> size;
        std::optional<ButtonVisualState> state;

        [[nodiscard]] auto matches(
            ButtonTone current_tone,
            ButtonTreatment current_treatment,
            ButtonSize current_size,
            ButtonVisualState current_state
        ) const noexcept -> bool;
    };

    struct ButtonStyleRule {
        ButtonRuleSelector selector;
        std::optional<ThemeColor> background;
        std::optional<ThemeColor> foreground;
        std::optional<ThemeColor> border_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> border_width;
        std::optional<ThemeScalar> radius;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> height;
        std::optional<ThemeScalar> padding_x;
        std::optional<ThemeScalar> font_size;
    };

    struct TextFieldStyleRule {
        std::optional<TextFieldVisualState> state;
        std::optional<ThemeColor> background;
        std::optional<ThemeColor> foreground;
        std::optional<ThemeColor> placeholder;
        std::optional<ThemeColor> border_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeColor> selection;
        std::optional<ThemeScalar> border_width;
        std::optional<ThemeScalar> radius;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> height;
        std::optional<ThemeScalar> padding_x;
        std::optional<ThemeScalar> font_size;
    };

    class NanStyle {
    public:
        virtual ~NanStyle() = default;

        void add_button_rule(ButtonStyleRule rule);
        [[nodiscard]] auto button_rules() const noexcept -> const std::vector<ButtonStyleRule>&;

        void add_text_field_rule(TextFieldStyleRule rule);
        [[nodiscard]] auto text_field_rules() const noexcept
            -> const std::vector<TextFieldStyleRule>&;

    private:
        std::vector<ButtonStyleRule> button_rules_;
        std::vector<TextFieldStyleRule> text_field_rules_;
    };

    [[nodiscard]] auto default_style() -> std::shared_ptr<const NanStyle>;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP
