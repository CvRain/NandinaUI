//
// widget/label — semantic text control with optional reactive text binding.
//

#include "label.hpp"
#include "../theme/theme_manager.hpp"

#include <utility>

namespace nandina::widget
{

    Label::Label(reactive::Graph& graph, std::string text, theme::NanTheme theme):
        primitives::Text(std::move(text)),
        scope_(graph),
        theme_(std::move(theme)) {
        apply_color_token();
        apply_component_font_size(theme_.tokens.typography.label_lg);
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
        if (!resolved_style_context().font_size_from_context) {
            apply_component_font_size(theme_.tokens.typography.label_lg);
        }
    }

    void Label::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        primitives::Text::on_style_context_changed(context);
        if (!context.text_color_from_context) {
            apply_color_token();
        }
        if (!context.font_size_from_context) {
            apply_component_font_size(theme_.tokens.typography.label_lg);
        }
    }

    void Label::apply_color_token() {
        apply_component_color(
            theme::resolve_theme_color(theme_, theme::ThemeColor::token(color_token_))
        );
    }

} // namespace nandina::widget
