//
// widget/chip - removable pill tag with optional dismiss action.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_CHIP_HPP
#define NANDINA_EXPERIMENT_WIDGET_CHIP_HPP

#include "../reactive/event.hpp"
#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Chip: public scene::NanControl {
    public:
        explicit Chip(
            std::string text,
            bool removable = false,
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            std::string text,
            bool removable = false,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Chip>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        void set_removable(bool removable);
        [[nodiscard]] auto removable() const -> bool;

        void set_on_remove(std::function<void()> callback);
        [[nodiscard]] auto removed() const -> const reactive::Event<>&;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::ChipRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedChipStyle;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& context) -> void override;
        [[nodiscard]] auto is_focusable() const -> bool override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        void apply_text_style();
        void remove();
        [[nodiscard]] auto remove_rect() const -> foundation::NanRect;

        std::string text_string_;
        bool removable_ = false;
        bool focused_ = false;
        primitives::Text text_;
        std::function<void()> on_remove_;
        reactive::Event<> removed_;
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::ChipRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_CHIP_HPP
