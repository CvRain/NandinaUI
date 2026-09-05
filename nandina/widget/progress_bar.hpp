//
// widget/progress_bar - determinate progress bar (pure display, no interaction).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PROGRESS_BAR_HPP
#define NANDINA_EXPERIMENT_WIDGET_PROGRESS_BAR_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class ProgressBar: public scene::NanControl {
    public:
        /// @param value 进度分数 [0,1]（越界/非有限值会被钳制）。
        explicit ProgressBar(float value = 0.0F, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(
            float value = 0.0F,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<ProgressBar>;

        /// 无障碍标签（可选，仅进 semantics）。
        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;

        /// 静默设置进度（[0,1] 钳制），不触发事件。
        void set_value(float value);
        [[nodiscard]] auto value() const -> float;

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::ProgressBarRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::ProgressBarVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedProgressBarStyle;

        void on_theme_changed(const theme::ThemeManager& manager) override;
        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        std::string label_;
        float value_ = 0.0F;
        bool disabled_ = false;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::ProgressBarRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_PROGRESS_BAR_HPP
