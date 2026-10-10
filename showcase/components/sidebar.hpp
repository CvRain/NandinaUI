#pragma once

#include <nandina/app/shell_context.hpp>
#include <nandina/reactive/signal.hpp>
#include <nandina/theme/appearance.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/button.hpp>

namespace nandina::showcase
{
    /// 外壳侧边栏：标题 / 分组导航 / 外观切换三段。
    ///
    /// 导航分组来自 `RouteOptions::type`，由 `Routes::nav_sections()` 一次算好 ——
    /// 分段顺序 = 各 type 首次出现的顺序，所以调整顺序只需调整路由声明的位置，
    /// 这里只负责"把段落画出来"。
    class Sidebar {
    public:
        explicit Sidebar(const app::ShellContext& context);

        auto build_shell() const -> widget::View;

    private:
        const float k_item_gap = 4.0F;
        const float k_item_height = 32.0F;
        const float k_item_font_size = 14.0F;

        /// 段与段之间、以及段与上下分隔线之间的距离。比段内的 `k_item_gap` 大一档，
        /// 让"段落"这件事在视觉上先于"条目"被读出来。
        const float k_section_gap = 12.0F;

        /// 段标题与标题字号。字号是字面量而不是排版 role：`Label` 目前没有暴露 role 入口，
        /// 沿用本项目既有做法（见 `docs/components`），值与正文差两档以保证层级。
        const float k_section_label_font_size = 12.0F;
        const float k_title_font_size = 16.0F;

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

        /// 一个分组：段标题 + 该段的条目列。
        ///
        /// 返回 `NanControl` 而不是 `widget::View`：这些结果要交给 `.child()` / `.children()`，
        /// 而容器的 `add()` 要求 `shared_ptr<NanControl>`（`View` 是 `shared_ptr<NanNode2D>`，
        /// 宽了一层，反而塞不进容器）。
        auto generate_section(const app::NavSection& section) const
            -> std::shared_ptr<scene::NanControl>;

        /// 所有分组，段与段之间夹一条分隔线。
        auto generate_navigation_sections() const -> std::shared_ptr<scene::NanControl>;

        /// 标题 + 分组导航 + 外观切换的纵向三段。
        auto generate_sidebar_content() const -> std::shared_ptr<scene::NanControl>;
    };
} // namespace nandina::showcase
