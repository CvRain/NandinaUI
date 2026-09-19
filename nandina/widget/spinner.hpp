//
// widget/spinner - indeterminate progress indicator (pure display, no interaction).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_SPINNER_HPP
#define NANDINA_EXPERIMENT_WIDGET_SPINNER_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Spinner: public scene::NanControl {
    public:
        explicit Spinner(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Spinner>;

        /// 无障碍标签（可选，仅进 semantics）。
        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::SpinnerRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::SpinnerVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedSpinnerStyle;

        /// 当前弧线起始角（弧度）。测试用；生产代码通过 on_process 推进。
        [[nodiscard]] auto rotation() const -> float;

        void on_theme_changed(const theme::ThemeManager& manager) override;
        void on_process(float dt) override;
        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        std::string label_;
        bool disabled_ = false;
        /// 弧线起始角（弧度），每帧按 rotation_speed 推进。
        float rotation_ = 0.0F;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::SpinnerRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_SPINNER_HPP
