//
// widget/toggle - two-state button (button appearance, switch semantics).
//
// 外观是按钮、语义是开关：用于工具栏式的开/关（加粗、斜体、静音）。需要"立即生效的
// 设置"时用 Switch，需要"表单里的独立布尔选择"时用 Checkbox。
//
// 结构与 Checkbox 同款：Pressable 提供 hover / pressed / focus / disabled 与
// Enter / Space / 指针点击的激活路径，本类只负责 checked 值、配方解析与绘制差异
// （Checkbox 画指示器，Toggle 画整块按钮容器）。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TOGGLE_HPP
#define NANDINA_EXPERIMENT_WIDGET_TOGGLE_HPP

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
    class ToggleGroup;

    class Toggle: public primitives::Pressable {
    public:
        explicit Toggle(std::string text, theme::NanTheme theme = theme::default_theme());

        /// 组内成员构造：注册到 `group`，并把方向键 / typeahead 转交给组。
        Toggle(
            std::string text,
            std::shared_ptr<ToggleGroup> group,
            theme::NanTheme theme = theme::default_theme()
        );

        ~Toggle() override;

        [[nodiscard]] static auto create(
            std::string text,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Toggle>;

        [[nodiscard]] static auto create(
            std::string text,
            std::shared_ptr<ToggleGroup> group,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Toggle>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;

        /// 程序化赋值：幂等，不发事件（用户操作才发，见 `toggle()`）。
        void set_checked(bool checked);
        [[nodiscard]] auto checked() const -> bool;
        /// 用户激活：切换 checked。组内成员先经过组的模式约束，再发成员事件。
        void toggle();

        /// 语义色家族（复用 ButtonTone），默认 primary。
        void set_tone(theme::ButtonTone tone);
        [[nodiscard]] auto tone() const -> theme::ButtonTone;

        /// 未选中态的处理方式（复用 ButtonTreatment），默认 ghost。
        void set_treatment(theme::ButtonTreatment treatment);
        [[nodiscard]] auto treatment() const -> theme::ButtonTreatment;

        void set_on_change(std::function<void(bool)> callback);
        [[nodiscard]] auto checked_changed() const -> const reactive::Event<bool>&;

        /// 组关联（组内成员才需要；nullptr = 独立控件，方向键不漫游）。
        void set_group(std::shared_ptr<ToggleGroup> group);
        [[nodiscard]] auto group() const -> const std::shared_ptr<ToggleGroup>&;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::ToggleRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::ToggleVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedToggleStyle;

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
        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;
        void on_click() override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        void apply_metrics();
        void apply_text_style();

        primitives::Text text_;
        std::shared_ptr<ToggleGroup> group_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::ToggleRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
        bool checked_ = false;
        theme::ButtonTone tone_ = theme::ButtonTone::primary;
        theme::ButtonTreatment treatment_ = theme::ButtonTreatment::ghost;
        std::function<void(bool)> on_change_;
        reactive::Event<bool> checked_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TOGGLE_HPP
