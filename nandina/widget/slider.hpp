//
// widget/slider - semantic continuous numeric input control.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_SLIDER_HPP
#define NANDINA_EXPERIMENT_WIDGET_SLIDER_HPP

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
    class Slider: public scene::NanControl {
    public:
        explicit Slider(
            std::string label,
            float value = 0.0F,
            float minimum = 0.0F,
            float maximum = 1.0F,
            float step = 0.01F,
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            std::string label,
            float value = 0.0F,
            float minimum = 0.0F,
            float maximum = 1.0F,
            float step = 0.01F,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Slider>;

        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;
        void set_value(float value);
        [[nodiscard]] auto value() const -> float;
        void set_range(float minimum, float maximum);
        [[nodiscard]] auto minimum() const -> float;
        [[nodiscard]] auto maximum() const -> float;
        void set_step(float step);
        [[nodiscard]] auto step() const -> float;

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;
        void set_on_change(std::function<void(float)> callback);
        [[nodiscard]] auto value_changed() const -> const reactive::Event<float>&;

        /// 在轨道上方显示当前值文本标签；关闭时不占额外高度（默认关闭）。
        void set_show_value_label(bool show);
        [[nodiscard]] auto show_value_label() const -> bool;
        /// 值标签当前渲染的数字文本（未开启时仍随 value 更新）。
        [[nodiscard]] auto value_label_text() const -> std::string_view;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> primitives::TextPipeline;
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::SliderRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::SliderVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedSliderStyle;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        [[nodiscard]] auto normalized(float value) const -> float;
        [[nodiscard]] auto fraction() const -> float;
        void set_user_value(float value);
        void update_from_pointer(foundation::NanPoint screen_position);
        void update_visual_state();
        void apply_value_label_style();

        primitives::Text value_text_;
        std::string label_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::SliderRecipeRule> override_;
        float value_ = 0.0F;
        float minimum_ = 0.0F;
        float maximum_ = 1.0F;
        float step_ = 0.01F;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
        bool disabled_ = false;
        bool hovered_ = false;
        bool dragging_ = false;
        bool focused_ = false;
        bool show_value_label_ = false;
        std::function<void(float)> on_change_;
        reactive::Event<float> value_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_SLIDER_HPP
