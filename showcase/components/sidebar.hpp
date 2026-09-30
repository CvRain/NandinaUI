#pragma once

#include <nandina/app/shell_context.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/button.hpp>

#include <functional>
#include <string>
#include <vector>

namespace nandina::showcase
{
    /// 侧边栏的一项 —— 从路由表投影出来的**只读描述**（相当于 ListView 的 model 一行）。
    ///
    /// 它不是另一份页面清单：字段全部来自 `app::RouteEntry`，删掉一条路由这一项就消失。
    struct SidebarItem {
        /// 页面身份（路由表里的类型键）。高亮就按它比对。
        app::NanTypeKey page_key = nullptr;
        /// 展示文字：`RouteOptions::title`，空则回退 `key`。
        std::string label;
        /// `RouteOptions::icon`：资源名，交给 delegate 决定怎么画（组件自己不解析资源）。
        std::string icon;
        /// 由 `route<PageT>()` 生成的"进入这一页"。应用可以直接调用它，也可以无视它去做别的事。
        bool (*activate)(const app::Navigation& navigation) = nullptr;
    };

    /**
     * 侧边栏外壳。
     *
     * 分工（照 QML `ListView` 的 data / delegate 分法）：
     *
     *   * **data** —— `app::Routes`，经 `ShellContext` 传入。组件按 `show_in_nav` 过滤、
     *     按 `title` / `icon` 投影成 `SidebarItem`，**不自己维护页面清单**。新增一页
     *     只需要在 router 的路由表里加一条。
     *   * **绘制** —— 由本组件负责（面板、条目、当前项高亮），观感通过
     *     `set_active_treatment()` / `set_idle_treatment()` 调整。
     *
     *     刻意**没有**做"每条目 delegate"：delegate 返回的是任意 `View`，而高亮绑定
     *     需要具体控件类型（`ui.bind` 的 setter 参数类型要和节点一致）。那样写出来的
     *     delegate 无法自动跟随当前路由，会得到一个"点了不亮"的静默陷阱。等条目形状
     *     真的需要分化时，正确做法是让 delegate 连响应式来源一起拿到、自己绑。
     *   * **跳转** —— **不由组件决定**。点击只回调 `on_activate`，应用自己决定是否
     *     调用 `item.activate(navigation)`（也可以什么都不做）。组件不替应用做选择。
     *   * **高亮** —— 派生自 `ShellContext::current_page()`，即 Router 发布的当前路由。
     *     因此首屏 `start()` 与任何程序化导航都会自动跟随，应用不需要手动同步。
     *
     * 另外：外壳在任何页面存在之前就建好了（`set_shell()` 早于 `start()`），这也是高亮
     * 必须来自 Router 观察、而不能在构建时读一次的原因。
     */
    class SidebarShell {
    public:
        /// 激活回调。应用在这里决定跳转（通常就一行 `item.activate(navigation)`）。
        using ActivateHandler =
            std::function<void(const app::Navigation& navigation, const SidebarItem& item)>;

        explicit SidebarShell(app::ShellContext& context);

        /// 侧边栏宽度（默认 200）。
        auto set_width(float width) -> SidebarShell&;
        /// 侧边栏顶部标题；空字符串表示不要标题区。
        auto set_title(std::string title) -> SidebarShell&;
        /// 当前项 / 非当前项的按钮语气（默认 `filled` / `ghost`）。
        auto set_active_treatment(theme::ButtonTreatment treatment) -> SidebarShell&;
        auto set_idle_treatment(theme::ButtonTreatment treatment) -> SidebarShell&;
        /// 点击某项时的回调。**不设置就什么也不发生** —— 组件不会自己跳转。
        auto on_activate(ActivateHandler handler) -> SidebarShell&;

        /// 从路由表投影出的条目（已按 `show_in_nav` 过滤）。
        [[nodiscard]] auto items() const -> const std::vector<SidebarItem>&;

        /// 构建外壳：`Row[ 侧边栏, Expanded(outlet) ]`。
        [[nodiscard]] auto build_shell() -> widget::View;

    private:
        /// 把 `context_->routes()` 投影成条目；跳过 `show_in_nav == false`、标题全空、
        /// 以及无法凭类型键进入（参数不可默认构造）的页面。
        void project_routes();
        /// 构造一个条目按钮并接上激活回调。
        [[nodiscard]] auto make_item(const SidebarItem& item) -> std::shared_ptr<widget::Button>;

        app::ShellContext* context_;
        std::vector<SidebarItem> items_;
        ActivateHandler on_activate_;
        std::string title_ = "NandinaUI";
        float width_ = 200.0F;
        theme::ButtonTreatment active_treatment_ = theme::ButtonTreatment::filled;
        theme::ButtonTreatment idle_treatment_ = theme::ButtonTreatment::ghost;
    };
} // namespace nandina::showcase
