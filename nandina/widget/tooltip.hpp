//
// widget/tooltip - hover-triggered floating hint over a trigger control.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TOOLTIP_HPP
#define NANDINA_EXPERIMENT_WIDGET_TOOLTIP_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Tooltip: public scene::NanControl {
    public:
        enum class Placement {
            top,
            bottom,
        };

        explicit Tooltip(
            std::string text,
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            std::string text,
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Tooltip>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        /// 替换触发控件（单子；空指针仅允许作为内部状态，公开接口拒绝）。
        auto set_trigger(std::shared_ptr<scene::NanControl> trigger) -> Tooltip&;
        void set_placement(Placement placement);
        [[nodiscard]] auto placement() const -> Placement;
        /// 悬停多久后显示（秒，≥0）。
        void set_delay(float seconds);
        [[nodiscard]] auto delay() const -> float;

        [[nodiscard]] auto visible() const -> bool;
        void show();
        void hide();

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::TooltipRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedTooltipStyle;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_input(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;
        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        void apply_text_style();

        primitives::Text text_;
        std::weak_ptr<scene::NanControl> trigger_;
        Placement placement_ = Placement::top;
        float delay_ = 0.4F;
        float hover_time_ = 0.0F;
        bool hovered_ = false;
        bool visible_ = false;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::TooltipRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TOOLTIP_HPP
