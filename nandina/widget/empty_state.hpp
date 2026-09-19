//
// widget/empty_state - empty collection placeholder (title + description + optional slots).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_EMPTY_STATE_HPP
#define NANDINA_EXPERIMENT_WIDGET_EMPTY_STATE_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    /**
     * 空状态：列表/集合暂无内容时占据内容区，给出标题、可选描述与可选操作。
     *
     * 结构约定与 Dialog 的命名槽位一致：`icon` / `action` 两个槽位接受尚未挂载的普通控件，
     * 缺省为空且不占高度；标题为空且没有图标时整体测量为 0，便于条件挂载而不留下空隙。
     * 组件自身不可交互，交互由 `action` 槽位内的控件承载。
     */
    class EmptyState: public scene::NanControl {
    public:
        explicit EmptyState(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<EmptyState>;

        void set_title(std::string title);
        [[nodiscard]] auto title() const -> std::string_view;

        void set_description(std::string description);
        [[nodiscard]] auto description() const -> std::string_view;

        /// 可选图标槽位；传入控件必须尚未挂载，替换时旧内容被移除。
        void set_icon(std::shared_ptr<scene::NanControl> icon);
        /// 可选操作槽位；同上。
        void set_action(std::shared_ptr<scene::NanControl> action);
        [[nodiscard]] auto icon() const -> scene::NanControl*;
        [[nodiscard]] auto action() const -> scene::NanControl*;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::EmptyStateRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::EmptyStateVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedEmptyStateStyle;

        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        /// 组件当前是否没有任何可展示内容：标题为空且没有图标。
        [[nodiscard]] auto is_empty() const -> bool;

        /// 按可用宽度计算标题 / 描述 / 图标的纵向堆叠（逻辑坐标，内容水平居中）。
        struct Placement {
            std::optional<foundation::NanRect> icon;
            std::optional<foundation::NanRect> title;
            std::optional<foundation::NanRect> description;
            std::optional<foundation::NanRect> action;
            float inner_width = 0.0F;
            float total_height = 0.0F;
        };
        [[nodiscard]] auto placement(float available_width) -> Placement;

        void apply_text_styles();
        void relayout();

        primitives::Text title_text_;
        primitives::Text description_text_;
        std::weak_ptr<scene::NanControl> icon_;
        std::weak_ptr<scene::NanControl> action_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::EmptyStateRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_EMPTY_STATE_HPP
