//
// Created by cvrain on 2026/9/23.
//

#include "main_window.hpp"

#include "components/sidebar.hpp"
#include "pages/anchors_page.hpp"
#include "pages/avatar_page.hpp"
#include "pages/component_page.hpp"
#include "pages/home_page.hpp"

#include <nandina/widget/controls.hpp>

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
            nandina::app::route<ShowcaseHomePage>({.address = "home", .title = "首页"}),
            nandina::app::route<AvatarPage>({.address = "avatar", .title = "Avatar 示例"}),
            nandina::app::route<AnchorsPage>({.address = "anchors", .title = "Anchors 布局"}),
            nandina::app::route<ComponentPage>(
                {.address = "component", .title = "组件详情", .show_in_nav = false}
            ),
        };
        auto& router = use_router(routes);

        set_shell([](nandina::app::ShellContext& context) -> nandina::widget::View {
            Sidebar sidebar {context};

            return sidebar.build_shell();
        });

        (void)router.start<ShowcaseHomePage>();
    }
} // namespace nandina::showcase
