//
// widget/internal/alert_dismiss_button - Alert 内部的关闭小按钮。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_ALERT_DISMISS_BUTTON_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_ALERT_DISMISS_BUTTON_HPP

#include "../../foundation/nandina_color.hpp"
#include "../../scene/control.hpp"
#include "../primitives/pressable.hpp"

#include <string>
#include <string_view>

namespace nandina::widget::internal
{
    /**
     * Alert 的关闭小按钮：`Pressable` 派生的内部子控件，只在 Alert `dismissible` 时挂载。
     *
     * 键盘可达（Tab 焦点 + Enter / Space 激活）与指针点击走同一条 `Pressable::on_click`
     * 路径，因此辅助技术的 activate 与鼠标点击汇聚到同一个 `Alert::set_on_dismiss()`
     * 回调。关闭本身只回调，不负责把 Alert 从场景树里摘掉——那是调用方的事。
     *
     * 颜色 / 尺寸全部由 Alert 从解析后的配方传入（icon 色、ring 色、`metrics.box_size`），
     * 本控件不持有主题快照，也不写任何颜色字面量。
     */
    class AlertDismissButton final: public primitives::Pressable {
    public:
        AlertDismissButton();

        /// 关闭 × 的字形颜色（Alert 传入解析后的 `icon` 色）。
        void set_glyph_color(foundation::NanColor color);
        [[nodiscard]] auto glyph_color() const -> const foundation::NanColor&;

        /// 焦点环颜色（Alert 传入解析后的 `ColorToken::ring`）。
        void set_focus_ring_color(foundation::NanColor color);
        [[nodiscard]] auto focus_ring_color() const -> const foundation::NanColor&;

        /// 方形边长（Alert 传入解析后的 `metrics.box_size`）。
        void set_affordance_size(float size);
        [[nodiscard]] auto affordance_size() const -> float;

        /// 无障碍标签（默认 "Dismiss"）。
        void set_accessible_label(std::string label);
        [[nodiscard]] auto accessible_label() const -> std::string_view;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        foundation::NanColor glyph_color_ {};
        foundation::NanColor focus_ring_color_ {};
        float size_ = 20.0F;
        std::string label_ = "Dismiss";
    };
} // namespace nandina::widget::internal

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_ALERT_DISMISS_BUTTON_HPP
