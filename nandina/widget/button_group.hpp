//
// widget/button_group - horizontal/vertical row of related buttons with one shared rhythm.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUTTON_GROUP_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUTTON_GROUP_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "layout.hpp"

#include <memory>
#include <optional>

namespace nandina::widget
{
    /**
     * 按钮组：把一组相关按钮按**统一间距**排成一行或一列。
     *
     * 与 `Row` / `Column` 同款：本类是真实场景节点，子按钮通过 `add_button()`（内部走
     * `add_child`）挂载，方向与间距由组统一决定，因此不会出现"每个调用点各写一个
     * gap"的重复样板。组自身不接受输入、不承载焦点——键盘可达性由子按钮自己负责
     * （组不是一个 `role`，语义上保持透明容器）。
     *
     * 方向复用布局系统的 `LayoutAxis`（horizontal / vertical），不另立枚举。
     *
     * ## 关于 attached（相邻按钮共边）——刻意未实现
     *
     * 需求里的 `attached` 造型要求"相邻按钮共边、只有最外侧两角圆角"。当前配方 /
     * 绘制模型**无法表达**它：
     *   * 每个子按钮各自解析自己的配方（圆角来自自己的 `container_radius`），父容器
     *     没有"按子项位置覆盖圆角"的通道；`set_override()` 是逐控件实例覆盖，组既
     *     不知道子控件的具体类型，也不应该在容器里对子控件做类型嗅探；
     *   * 即使强行把中间项圆角改成 0，`BoxPainter` 对每个子项都会画**完整**的圆角矩形
     *     边框，相邻边会出现两条 1px 边线（双边框接缝），且 hover / focus 状态层同样
     *     会在接缝处重叠——这些都不是圆角一个字段能解决的。
     * 强行实现只能得到"看起来像共边、实际有双边框与错位状态层"的半成品，因此这里
     * **只提供带间距的变体**，并把限制如实记录在文档与报告中。
     */
    class ButtonGroup: public scene::NanControl {
    public:
        explicit ButtonGroup(
            LayoutAxis axis = LayoutAxis::horizontal,
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            LayoutAxis axis = LayoutAxis::horizontal,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<ButtonGroup>;

        /// 挂载一个（尚未入树的）按钮控件；返回自身以便链式调用（与 `Row::add` 一致）。
        auto add_button(std::shared_ptr<scene::NanControl> button) -> ButtonGroup&;
        /// 移除并销毁全部子控件。
        void clear();
        /// 当前子控件数量（含通过继承的 `add_child` 直接挂载的控件）。
        [[nodiscard]] auto button_count() const -> std::size_t;

        /// 主轴方向；复用布局系统的 `LayoutAxis`。
        auto set_orientation(LayoutAxis axis) -> ButtonGroup&;
        [[nodiscard]] auto orientation() const -> LayoutAxis;
        /// 覆盖成员间距；未显式设置时使用配方 `metrics.gap`。
        auto set_gap(float gap) -> ButtonGroup&;
        /// 清除显式间距，回到配方 `metrics.gap`。
        void clear_gap();
        [[nodiscard]] auto gap() const -> float;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::ButtonGroupRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::ButtonGroupVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedButtonGroupStyle;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        void on_ready() override;

    private:
        /// 显式间距优先，否则取配方 `metrics.gap`。
        [[nodiscard]] auto resolved_gap() const -> float;
        void relayout();

        LayoutAxis axis_ = LayoutAxis::horizontal;
        std::optional<float> gap_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::ButtonGroupRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUTTON_GROUP_HPP
