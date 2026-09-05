//
// widget/button — first semantic control built from tone/treatment tokens.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP

#include "../theme/design_system.hpp"
#include "primitives/box_presentation.hpp"
#include "primitives/pressable.hpp"
#include "primitives/text.hpp"
#include "primitives/text_presentation.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{

    class Button: public primitives::Pressable {
    public:
        explicit Button(std::string text, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto
        create(std::string text, theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Button>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        [[nodiscard]] auto text_node() -> primitives::Text&;
        [[nodiscard]] auto text_node() const -> const primitives::Text&;
        [[nodiscard]] auto visual_part(visual::label_t) noexcept
            -> primitives::TextPresentation&;
        [[nodiscard]] auto visual_part(visual::container_t) noexcept
            -> primitives::BoxPresentation&;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> primitives::TextPipeline;
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        void set_font(text::FontRequest request);
        void set_font_family(resource::ResourceKey family);
        void set_font_weight(int weight);
        void set_font_slant(text::FontSlant slant);

        /// 显式字号（逻辑单位），写入共享 label.font_size 实例属性。
        void set_font_size(float size);
        /// 百分比字号：相对按钮自身最终高度解析（如 percent(45) = 高度 × 45%）。
        void set_font_size(scene::PercentLength size);
        /// 清除显式/百分比字号，回退到继承上下文或主题配方。
        void clear_font_size();
        /// 当前生效的字号（最近一次样式应用后文本原语实际持有的值）。
        [[nodiscard]] auto font_size() const -> float;

        void set_text_overflow(primitives::TextOverflow overflow);
        [[nodiscard]] auto text_overflow() const -> primitives::TextOverflow;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（等价整份快照覆盖，不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::ButtonRecipeRule rule);

        void set_tone(theme::ButtonTone tone);
        [[nodiscard]] auto tone() const -> theme::ButtonTone;

        void set_treatment(theme::ButtonTreatment treatment);
        [[nodiscard]] auto treatment() const -> theme::ButtonTreatment;

        void set_button_size(theme::ButtonSize size);
        [[nodiscard]] auto button_size() const -> theme::ButtonSize;

        [[nodiscard]] auto visual_state() const -> theme::ButtonVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedButtonStyle;
        [[nodiscard]] auto ripple_active() const noexcept -> bool;
        [[nodiscard]] auto ripple_progress() const noexcept -> float;

        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_process(float dt) override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        void apply_metrics();
        void apply_text_style(theme::ButtonVisualState state, float reference_height);
        [[nodiscard]] auto resolved_recipe_style(theme::ButtonVisualState state) const
            -> theme::ResolvedButtonStyle;
        [[nodiscard]] auto resolved_font_size(float reference_height, float fallback) const
            -> float;

        primitives::Text text_;
        primitives::TextPresentation label_presentation_;
        primitives::BoxPresentation container_presentation_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::ButtonRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
        bool font_explicit_ = false;
        /// 百分比字号与 label presentation 的固定字号互斥。
        std::optional<scene::PercentLength> font_size_percent_;
        theme::ButtonTone tone_ = theme::ButtonTone::primary;
        theme::ButtonTreatment treatment_ = theme::ButtonTreatment::filled;
        theme::ButtonSize size_ = theme::ButtonSize::medium;
        primitives::TextOverflow text_overflow_ = primitives::TextOverflow::ellipsis;
        std::optional<foundation::NanPoint> ripple_origin_local_;
        float ripple_progress_ = 1.0F;
        bool reduced_motion_ = false;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
