//
// widget/tabs - horizontal tab bar with single selection and roving keyboard focus.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TABS_HPP
#define NANDINA_EXPERIMENT_WIDGET_TABS_HPP

#include "../animation/animated_property.hpp"
#include "../reactive/event.hpp"
#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::widget
{
    class Tabs: public scene::NanControl {
    public:
        explicit Tabs(
            std::vector<std::string> labels = {},
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            std::vector<std::string> labels = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Tabs>;

        void set_labels(std::vector<std::string> labels);
        [[nodiscard]] auto label_count() const -> std::size_t;
        [[nodiscard]] auto label(std::size_t index) const -> std::string_view;

        /// 静默设置选中索引（越界钳制），不触发事件。
        void set_selected_index(int index);
        [[nodiscard]] auto selected_index() const -> int;
        [[nodiscard]] auto selected_label() const -> std::string_view;
        /// 用户激活：选中并触发事件。
        void select(int index);

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;

        void set_on_change(std::function<void(int)> callback);
        [[nodiscard]] auto selection_changed() const -> const reactive::Event<int>&;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::TabsRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::TabsVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedTabsStyle;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        void rebuild_texts();
        void apply_text_styles();
        void measure_labels();
        void sync_indicator(bool animate);
        [[nodiscard]] auto hit_index(float local_x) const -> int;

        std::vector<std::string> labels_;
        std::vector<std::shared_ptr<primitives::Text>> label_texts_;
        std::vector<float> tab_offsets_;
        animation::AnimatedProperty<float> indicator_x_ {0.0F};
        animation::AnimatedProperty<float> indicator_width_ {0.0F};
        int selected_index_ = 0;
        bool disabled_ = false;
        bool focused_ = false;
        bool hovered_ = false;
        std::function<void(int)> on_change_;
        reactive::Event<int> selection_changed_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::TabsRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TABS_HPP
