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
#include "primitives/text.hpp"

#include <concepts>
#include <functional>
#include <memory>
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

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        void bind_text(Source& source) {
            binding_ = [&source](reactive::EffectScope& scope, primitives::Text& text) {
                text.text_property().bind(scope, source);
            };
            if (is_inside_tree()) {
                activate_binding();
            }
        }

    protected:
        void on_ready() override;
        void on_exit_tree() override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        void activate_binding();
        void apply_color_token();

        reactive::EffectScope scope_;
        std::function<void(reactive::EffectScope&, primitives::Text&)> binding_;
        theme::NanTheme theme_;
        theme::ColorToken color_token_ = theme::ColorToken::on_surface;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LABEL_HPP
