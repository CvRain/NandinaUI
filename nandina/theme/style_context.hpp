//
// theme/style_context — inheritable four-state style values.
//

#ifndef NANDINA_EXPERIMENT_THEME_STYLE_CONTEXT_HPP
#define NANDINA_EXPERIMENT_THEME_STYLE_CONTEXT_HPP

#include "../foundation/nandina_color.hpp"
#include "../text/font_family.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::theme
{
    enum class StyleValueState { unset, inherit, initial, explicit_value };

    template<typename T>
    class StyleValue {
    public:
        StyleValue() = default;

        [[nodiscard]] static auto inherit() -> StyleValue {
            return StyleValue(StyleValueState::inherit, std::nullopt);
        }

        [[nodiscard]] static auto initial() -> StyleValue {
            return StyleValue(StyleValueState::initial, std::nullopt);
        }

        [[nodiscard]] static auto explicit_value(T value) -> StyleValue {
            return StyleValue(StyleValueState::explicit_value, std::move(value));
        }

        [[nodiscard]] auto state() const noexcept -> StyleValueState {
            return state_;
        }

        [[nodiscard]] auto value() const -> const T& {
            if (!value_) {
                throw std::logic_error("StyleValue does not contain an explicit value");
            }
            return *value_;
        }

    private:
        StyleValue(StyleValueState state, std::optional<T> value):
            state_(state), value_(std::move(value)) {}

        StyleValueState state_ = StyleValueState::unset;
        std::optional<T> value_;
    };

    enum class TextDirection { automatic, left_to_right, right_to_left };

    struct StyleContext {
        StyleValue<text::FontRequest> font;
        StyleValue<float> font_size;
        StyleValue<foundation::NanColor> text_color;
        StyleValue<std::string> locale;
        StyleValue<TextDirection> direction;
    };

    struct ResolvedStyleContext {
        text::FontRequest font;
        float font_size = 16.0F;
        foundation::NanColor text_color = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 255}
        );
        std::string locale = "und";
        TextDirection direction = TextDirection::automatic;
        bool font_from_context = false;
        bool font_size_from_context = false;
        bool text_color_from_context = false;
        bool locale_from_context = false;
        bool direction_from_context = false;
    };

    [[nodiscard]] inline auto resolves_from_context(
        const StyleValueState local,
        const bool inherited_from_context
    ) noexcept -> bool {
        return local != StyleValueState::unset || inherited_from_context;
    }

    template<typename T>
    [[nodiscard]] auto resolve_style_value(
        const StyleValue<T>& local,
        const T& initial,
        const T* inherited,
        const bool inherits_by_default
    ) -> T {
        switch (local.state()) {
            case StyleValueState::explicit_value:
                return local.value();
            case StyleValueState::initial:
                return initial;
            case StyleValueState::inherit:
                return inherited != nullptr ? *inherited : initial;
            case StyleValueState::unset:
                return inherits_by_default && inherited != nullptr ? *inherited : initial;
        }
        return initial;
    }

    [[nodiscard]] inline auto resolve_style_context(
        const StyleContext& local,
        const ResolvedStyleContext* inherited = nullptr
    ) -> ResolvedStyleContext {
        const ResolvedStyleContext initial;
        return {
            .font = resolve_style_value(
                local.font,
                initial.font,
                inherited != nullptr ? &inherited->font : nullptr,
                true
            ),
            .font_size = resolve_style_value(
                local.font_size,
                initial.font_size,
                inherited != nullptr ? &inherited->font_size : nullptr,
                true
            ),
            .text_color = resolve_style_value(
                local.text_color,
                initial.text_color,
                inherited != nullptr ? &inherited->text_color : nullptr,
                true
            ),
            .locale = resolve_style_value(
                local.locale,
                initial.locale,
                inherited != nullptr ? &inherited->locale : nullptr,
                true
            ),
            .direction = resolve_style_value(
                local.direction,
                initial.direction,
                inherited != nullptr ? &inherited->direction : nullptr,
                true
            ),
            .font_from_context = resolves_from_context(
                local.font.state(), inherited != nullptr && inherited->font_from_context
            ),
            .font_size_from_context = resolves_from_context(
                local.font_size.state(),
                inherited != nullptr && inherited->font_size_from_context
            ),
            .text_color_from_context = resolves_from_context(
                local.text_color.state(),
                inherited != nullptr && inherited->text_color_from_context
            ),
            .locale_from_context = resolves_from_context(
                local.locale.state(), inherited != nullptr && inherited->locale_from_context
            ),
            .direction_from_context = resolves_from_context(
                local.direction.state(),
                inherited != nullptr && inherited->direction_from_context
            ),
        };
    }

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_STYLE_CONTEXT_HPP
