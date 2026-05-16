module;

#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module nandina.app.authoring;

export import nandina.app.application;
export import nandina.app.page;
export import nandina.app.router;
export import nandina.app.page_host;

import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.widgets.sidebar;
import nandina.widgets.sidebar_menu_button;
import nandina.widgets.icon;
import nandina.runtime.nan_widget;

export namespace nandina::app {

    /**
     * ShellConfig — 应用壳配置
     *
     * 控制侧边栏宽度、标题、用户信息等顶层壳属性。
     */
    struct ShellConfig {
        float sidebar_width = 260.0f;
        std::string_view header_title = "NandinaUI";
        std::string_view user_name;
        std::string_view user_role;
    };

    /**
     * create_shell — 自动构建标准应用壳（侧边栏 + 页面宿主）
     *
     * 从 NanRouter 已注册的页面自动生成侧边栏导航按钮，
     * 搭配 NanPageHost 形成侧边栏 + 内容区的标准布局。
     *
     * 用法：
     *   auto router = NanRouter::create();
     *   router->register_page(std::make_unique<MyPage>());
     *   window.set_root(create_shell(std::move(router),
     *       {.sidebar_width = 260, .header_title = "My App"}));
     */
    inline auto create_shell(NanRouter::Ptr router, const ShellConfig& config = {}) -> Node {
        // ── 1. 从 router 页面自动构建侧边栏 ────────────────────
        auto sidebar = nandina::widgets::Sidebar::create();
        sidebar->set_header_title(config.header_title);
        if (!config.user_name.empty()) {
            sidebar->set_user_name(config.user_name);
            sidebar->set_user_role(config.user_role);
        }

        std::vector<nandina::widgets::SidebarMenuButton*> nav_buttons;
        std::vector<std::string> nav_keys;

        for (const auto& page : router->pages()) {
            auto btn = nandina::widgets::SidebarMenuButton::create();
            btn->set_label(page->title())
                .set_icon_type(page->icon_type())
                .set_active(page->route_key() == router->current_key());

            nav_buttons.push_back(btn.get());
            nav_keys.emplace_back(page->route_key());

            btn->on_click([router, key = std::string{page->route_key()}] {
                router->navigate_to(key);
            });

            sidebar->add_menu_item(std::move(btn));
        }

        // 导航激活态同步
        router->on_navigate([
            nav_buttons = std::move(nav_buttons),
            nav_keys = std::move(nav_keys)
        ](std::string_view new_key) {
            for (std::size_t i = 0; i < nav_buttons.size(); ++i) {
                nav_buttons[i]->set_active(nav_keys[i] == new_key);
            }
        });

        // ── 2. 构建壳布局 ─────────────────────────────────────
        return row(children(
            sized_box(adopt(std::move(sidebar)))
                .width(config.sidebar_width),
            expanded(adopt(std::make_unique<NanPageHost>(std::move(router))))
        )).align_items(nandina::layout::LayoutAlignment::stretch);
    }

    /**
     * setup_shell — 一行设置应用壳（极简入口）
     *
     * 自动创建 NanRouter、注册所有页面、构建 Sidebar + PageHost 并设为窗口根组件。
     *
     * 用法（单个页面）：
     *   setup_shell(*this, {.header_title = "My App"},
     *       std::make_unique<OverviewPage>());
     *
     * 用法（多个页面）：
     *   setup_shell(*this, {.header_title = "My App"},
     *       std::make_unique<OverviewPage>(),
     *       std::make_unique<SettingsPage>());
     */
    template<typename... PagePtrs>
    auto setup_shell(NanAppWindow& window, const ShellConfig& config, PagePtrs... pages) -> void {
        auto router = NanRouter::create();
        (router->register_page(std::move(pages)), ...);
        window.set_root(create_shell(std::move(router), config));
    }

} // namespace nandina::app