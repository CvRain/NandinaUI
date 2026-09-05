//
// widget/checkbox - semantic boolean selection control.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_CHECKBOX_HPP
#define NANDINA_EXPERIMENT_WIDGET_CHECKBOX_HPP

#include "../reactive/event.hpp"
#include "../theme/design_system.hpp"
#include "primitives/pressable.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Checkbox: public primitives::Pressable {
    public:
        explicit Checkbox(
            std::string label,
            bool checked = false,
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            std::string label,
            bool checked = false,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Checkbox>;

        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;
        void set_checked(bool checked);
        [[nodiscard]] auto checked() const -> bool;
        void toggle();

        void set_on_change(std::function<void(bool)> callback);
        [[nodiscard]] auto checked_changed() const -> const reactive::Event<bool>&;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::CheckboxRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::CheckboxVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedCheckboxStyle;

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
        void on_click() override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

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
        std::optional<theme::CheckboxRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
        bool checked_ = false;
        std::function<void(bool)> on_change_;
        reactive::Event<bool> checked_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_CHECKBOX_HPP
