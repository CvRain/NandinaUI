//
// widget/avatar - circular initials badge (pure display; image later).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_AVATAR_HPP
#define NANDINA_EXPERIMENT_WIDGET_AVATAR_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Avatar: public scene::NanControl {
    public:
        explicit Avatar(std::string name = {}, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto
        create(std::string name = {}, theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Avatar>;

        void set_name(std::string name);
        [[nodiscard]] auto name() const -> std::string_view;
        [[nodiscard]] auto initials() const -> std::string;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::AvatarRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedAvatarStyle;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        void apply_text_style();

        std::string name_;
        primitives::Text text_;
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::AvatarRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_AVATAR_HPP
