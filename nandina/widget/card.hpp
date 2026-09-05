//
// widget/card - semantic surface container for one child (pure container, no interaction).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_CARD_HPP
#define NANDINA_EXPERIMENT_WIDGET_CARD_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"

#include <memory>
#include <optional>

namespace nandina::widget
{
    class Card: public scene::NanControl {
    public:
        explicit Card(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Card>;

        /// 单子内容容器：替换既有子节点（空指针仅允许作为内部状态，公开接口拒绝）。
        auto set_child(std::shared_ptr<scene::NanControl> child) -> Card&;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::CardRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedCardStyle;

        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;

    private:
        void relayout();

        std::weak_ptr<scene::NanControl> child_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::CardRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_CARD_HPP
