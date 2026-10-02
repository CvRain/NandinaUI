#pragma once

#include <nandina/app/shell_context.hpp>
#include <nandina/reactive/signal.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/button.hpp>

namespace nandina::showcase
{
    class Sidebar {
    public:
        explicit Sidebar(const app::ShellContext& context);

        auto build_shell() const -> widget::View;

    private:
        const float k_item_gap = 4.0F;
        const float k_item_height = 32.0F;
        const float k_item_font_size = 14.0F;

        /// 悬浮 / 按下时在**主题圆角之上**外扩的像素量。
        ///
        /// 默认配方的按钮圆角是 `radius_md`（8px），所以增量落在 8→10→12 之间：
        /// 看得出在动，又不至于把 32px 高的条目挤成胶囊。
        const float k_hover_radius_gain = 2.0F;
        const float k_pressed_radius_gain = 4.0F;

        const app::ShellContext& context;

        /// 构建一个导航条目按钮。
        ///
        /// `hovered` / `pressed` 由调用方创建并传入，**不是**在这里面新建：这两个信号同时
        /// 被条目的交互回调（写）和圆角动画（读）使用，各建一份就会接成两条互不相干的线 ——
        /// 表现为"动画永远不触发"。
        auto generate_nav_item_builder(
            const std::string& item_title,
            const app::PageKey& page_key,
            reactive::Signal<bool>& hovered,
            reactive::Signal<bool>& pressed
        ) const -> auto;
        auto generate_navigation_buttons() const -> auto;
    };
} // namespace nandina::showcase
