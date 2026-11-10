//
// widget/label — semantic text control with optional reactive text binding.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_LABEL_HPP
#define NANDINA_EXPERIMENT_WIDGET_LABEL_HPP

#include "../reactive/computed.hpp"
#include "../reactive/effect.hpp"
#include "../reactive/graph.hpp"
#include "../reactive/signal.hpp"
#include "../theme/nan_style.hpp"
#include "../theme/typography_scale.hpp"
#include "primitives/text.hpp"

#include <concepts>
#include <memory>
#include <optional>
#include <string>

namespace nandina::widget
{

    class Label: public primitives::Text {
    public:
        explicit Label(
            reactive::Graph& graph,
            std::string text = {},
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            reactive::Graph& graph,
            std::string text = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Label>;

        void set_color_token(theme::ColorToken token);
        [[nodiscard]] auto color_token() const noexcept -> theme::ColorToken;

        /// Tailwind-style size step resolved through the active theme's `NanTypeScale`
        /// (root base × ratio). Explicit `set_font_size()` and an inherited style-context font
        /// size still take precedence over this component-level token.
        void set_text_size(theme::TextSize size);
        [[nodiscard]] auto text_size() const noexcept -> std::optional<theme::TextSize>;

        /// Leading step resolved through the theme scale, applied as a line-height multiplier.
        /// A later explicit `set_line_height()` overrides it; theme changes re-resolve it.
        void set_leading(theme::Leading leading);
        [[nodiscard]] auto leading() const noexcept -> std::optional<theme::Leading>;
        /// Numeric line-height multiplier; `0` restores the backend default. Overrides the
        /// leading token.
        void set_line_height(float factor);
        [[nodiscard]] auto line_height() const noexcept -> float;

        /// Bind immediately. The source must outlive this binding; tree exit unbinds it.
        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        void bind_text(Source& source) {
            text_property().bind(scope_, source);
        }

        /// Page-owned binding: scope clear also stops updates to a retained Label.
        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        void bind_text(reactive::ReactiveScope& scope, Source& source) {
            text_property().bind(scope, source);
        }

        void unbind_text() noexcept;
        [[nodiscard]] auto text_is_bound() const noexcept -> bool;

    protected:
        void on_exit_tree() override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        void apply_color_token();
        /// Component default size: explicit override > inherited context > `text_size()` token
        /// > `typography.label_lg`. Re-run on theme/style-context changes so token steps follow
        /// a theme's scale without freezing the resolved pixel value.
        void apply_label_font_size();
        /// Re-resolve the leading token unless a numeric line height was set.
        void apply_leading();

        reactive::EffectScope scope_;
        theme::NanTheme theme_;
        theme::ColorToken color_token_ = theme::ColorToken::on_surface;
        std::optional<theme::TextSize> text_size_;
        std::optional<theme::Leading> leading_;
        /// A numeric `set_line_height()` wins over the leading token until reset to 0.
        bool line_height_explicit_ = false;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LABEL_HPP
