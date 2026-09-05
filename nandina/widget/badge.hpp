//
// widget/badge - static pill badge for short labels (pure display, no interaction).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BADGE_HPP
#define NANDINA_EXPERIMENT_WIDGET_BADGE_HPP

#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Badge: public scene::NanControl {
    public:
        explicit Badge(std::string text, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(
            std::string text,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Badge>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::BadgeRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedBadgeStyle;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> primitives::TextPipeline;
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
        void apply_metrics();
        void apply_text_style();

        primitives::Text text_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::BadgeRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BADGE_HPP
