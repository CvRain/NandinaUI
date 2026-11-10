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
        apply_color_token();
        apply_label_font_size();
    }

    auto Label::create(reactive::Graph& graph, std::string text, theme::NanTheme theme)
        -> std::shared_ptr<Label> {
        return std::make_shared<Label>(graph, std::move(text), theme);
    }

    void Label::set_color_token(const theme::ColorToken token) {
        color_token_ = token;
        clear_explicit_color();
        const auto& context = resolved_style_context();
        primitives::Text::on_style_context_changed(context);
        if (!context.text_color_from_context) {
            apply_color_token();
        }
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
        if (!resolved_style_context().text_color_from_context) {
            apply_color_token();
        }
        apply_label_font_size();
        apply_leading();
    }

    void Label::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        primitives::Text::on_style_context_changed(context);
        if (!context.text_color_from_context) {
            apply_color_token();
        }
        apply_label_font_size();
    }

    void Label::set_text_size(const theme::TextSize size) {
        text_size_ = size;
        apply_label_font_size();
    }

    auto Label::text_size() const noexcept -> std::optional<theme::TextSize> {
        return text_size_;
    }

    void Label::set_leading(const theme::Leading leading) {
        leading_ = leading;
        line_height_explicit_ = false;
        apply_leading();
    }

    auto Label::leading() const noexcept -> std::optional<theme::Leading> {
        return leading_;
    }

    void Label::set_line_height(const float factor) {
        if (!std::isfinite(factor) || factor < 0.0F) {
            throw std::invalid_argument("Label line height must be finite and non-negative");
        }
        line_height_explicit_ = factor > 0.0F;
        primitives::Text::set_line_height(factor);
        if (!line_height_explicit_) {
            // 0 clears the numeric override; a leading token (if any) becomes authoritative again.
            apply_leading();
        }
    }

    auto Label::line_height() const noexcept -> float {
        return primitives::Text::line_height();
    }

    void Label::apply_label_font_size() {
        const auto& context = resolved_style_context();
        if (context.font_size_from_context) {
            return;
        }
        if (text_size_.has_value()) {
            apply_component_font_size(
                theme::text_size_px(theme_.tokens.typography.scale, *text_size_)
            );
            return;
        }
        apply_component_font_size(theme_.tokens.typography.label_lg);
    }

    void Label::apply_leading() {
        if (line_height_explicit_ || !leading_.has_value()) {
            return;
        }
        primitives::Text::set_line_height(
            theme::leading_factor(theme_.tokens.typography.scale, *leading_)
        );
    }

    void Label::apply_color_token() {
        apply_component_color(
            theme::resolve_theme_color(theme_, theme::ThemeColor::token(color_token_))
        );
    }

} // namespace nandina::widget
