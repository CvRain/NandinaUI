//
// widget/label — semantic text control with optional reactive text binding.
//

#include "label.hpp"
#include "../theme/theme_manager.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{

    Label::Label(reactive::Graph& graph, std::string text, theme::NanTheme theme):
        primitives::Text(std::move(text)),
        scope_(graph),
        theme_(std::move(theme)) {
        reconcile_style(resolved_style_context());
    }

    auto Label::create(reactive::Graph& graph, std::string text, theme::NanTheme theme)
        -> std::shared_ptr<Label> {
        return std::make_shared<Label>(graph, std::move(text), theme);
    }

    void Label::set_color_token(const theme::ColorToken token) {
        color_token_ = token;
        clear_explicit_color();
        reconcile_style(resolved_style_context());
    }

    void Label::clear_color_token() {
        if (color_token_ == theme::ColorToken::on_surface) {
            return;
        }
        color_token_ = theme::ColorToken::on_surface;
        reconcile_style(resolved_style_context());
    }

    auto Label::color_token() const noexcept -> theme::ColorToken {
        return color_token_;
    }

    void Label::unbind_text() noexcept {
        text_property().unbind();
    }

    auto Label::text_is_bound() const noexcept -> bool {
        return text_property().is_bound();
    }

    void Label::on_exit_tree() {
        unbind_text();
        scope_.clear();
        primitives::Text::on_exit_tree();
    }

    void Label::on_theme_changed(const theme::ThemeManager& manager) {
        theme_ = manager.theme();
        reconcile_style(resolved_style_context());
    }

    void Label::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        primitives::Text::on_style_context_changed(context);
    }

    void Label::set_font_size(const float size) {
        primitives::Text::set_font_size(size);
    }

    void Label::set_text_size(const theme::TextSize size) {
        if (text_size_ == size) {
            return;
        }
        text_size_ = size;
        reconcile_style(resolved_style_context());
    }

    void Label::clear_text_size() {
        if (!text_size_.has_value()) {
            return;
        }
        text_size_.reset();
        reconcile_style(resolved_style_context());
    }

    auto Label::text_size() const noexcept -> std::optional<theme::TextSize> {
        return text_size_;
    }

    void Label::set_leading(const theme::Leading leading) {
        leading_ = leading;
        clear_explicit_line_height();
        reconcile_style(resolved_style_context());
    }

    void Label::clear_leading() {
        if (!leading_.has_value()) {
            return;
        }
        leading_.reset();
        reconcile_style(resolved_style_context());
    }

    auto Label::leading() const noexcept -> std::optional<theme::Leading> {
        return leading_;
    }

    void Label::set_line_height(const float factor) {
        if (!std::isfinite(factor) || factor < 0.0F) {
            throw std::invalid_argument("Label line height must be finite and non-negative");
        }
        primitives::Text::set_line_height(factor);
    }

    auto Label::line_height() const noexcept -> float {
        return primitives::Text::line_height();
    }

    void Label::set_typography_role(const theme::TypographyRole role) {
        if (typography_role_ == role) {
            return;
        }
        typography_role_ = role;
        reconcile_style(resolved_style_context());
    }

    void Label::clear_typography_role() {
        if (!typography_role_.has_value()) {
            return;
        }
        typography_role_.reset();
        reconcile_style(resolved_style_context());
    }

    auto Label::typography_role() const noexcept -> std::optional<theme::TypographyRole> {
        return typography_role_;
    }

    void Label::resolve_component_style_fallbacks(
        const theme::ResolvedStyleContext& context,
        text::TextStyle& fallback
    ) const {
        primitives::Text::resolve_component_style_fallbacks(context, fallback);
        const auto& scale = theme_.tokens.typography.scale;

        if (!context.text_color_from_context) {
            fallback.color =
                theme::resolve_theme_color(theme_, theme::ThemeColor::token(color_token_));
        }

        if (!context.font_size_from_context) {
            if (text_size_.has_value()) {
                fallback.font_size = theme::text_size_px(scale, *text_size_);
            }
            else if (typography_role_.has_value()) {
                fallback.font_size = theme::text_size_px(
                    scale,
                    theme::typography_role_style(scale, *typography_role_).size
                );
            }
            else {
                const auto body = theme::typography_role_style(scale, theme::TypographyRole::body);
                fallback.font_size = theme::text_size_px(scale, body.size);
            }
        }

        if (leading_.has_value()) {
            fallback.line_height = theme::leading_factor(scale, *leading_);
        }
        else if (typography_role_.has_value()) {
            fallback.line_height = theme::leading_factor(
                scale,
                theme::typography_role_style(scale, *typography_role_).leading
            );
        }
        else if (
            !text_size_.has_value() && !font_size_is_explicit() && !context.font_size_from_context
        )
        {
            // Step 2b body default: an unstyled Label is body text (base size + normal leading,
            // 1.5 by default for CJK). Explicit/inherited/`set_text_size` sizes keep the backend
            // default leading unless the author chooses a leading token or semantic role.
            const auto body = theme::typography_role_style(scale, theme::TypographyRole::body);
            fallback.line_height = theme::leading_factor(scale, body.leading);
        }
        else {
            fallback.line_height = 0.0F;
        }

        if (!context.font_from_context) {
            const auto role = typography_role_.value_or(theme::TypographyRole::body);
            const auto role_style = theme::typography_role_style(scale, role);
            fallback.font.weight = role_style.font_weight;
            fallback.font.slant = role_style.slant;
        }
    }

} // namespace nandina::widget
