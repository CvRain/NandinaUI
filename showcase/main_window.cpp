//
// Created by cvrain on 2026/9/23.
//

#include "main_window.hpp"

#include <nandina/widget/controls.hpp>

#include "pages/avatar_page.hpp"
#include "pages/home_page.hpp"
#include "components/sidebar.hpp"

namespace nandina::showcase
{
    MainWindow::MainWindow(
        nandina::app::NanApplication& application,
        const nandina::app::WindowConfig& config
    ):
        NanWindow(application, config) {}

    void MainWindow::on_setup() {
        NanWindow::on_setup();
        const auto routes = nandina::app::Routes {
            nandina::app::route<ShowcaseHomePage>({.key = "home", .title = "home page"}),
            nandina::app::route<AvatarPage>({.key = "avatar", .title = "avatar page"}),
        };
        auto& router = use_router(routes);

        // 侧边栏把上面的路由表当**数据**用：条目标题 / 图标 / 是否入导航全部来自它，
        // 新增一页只需要在 routes 里加一条，这里一行都不用改。
        //
        // 跳转由应用决定 —— 就是下面这一行 `item.activate(navigation)`。组件只报告
        // "哪一项被点击"，不替应用选择；`RouteEntry::activate` 是 route<PageT>() 生成的
        // 类型擦除跳转，所以这里不需要写 `if (key == ...) navigate<PageT>()` 链。
        set_shell([](nandina::app::ShellContext& context) -> nandina::widget::View {
            return SidebarShell {context}
                .on_activate(
                    [](const nandina::app::Navigation& navigation, const SidebarItem& item) {
                        if (item.activate != nullptr) {
                            (void)item.activate(navigation);
                        }
                    }
                )
                .build_shell();
        });

        (void)router.start<ShowcaseHomePage>();
    }
} // namespace nandina::showcase
