//
// Created by cvrain on 2026/9/23.
//

#include "main_window.hpp"

#include "components/sidebar.hpp"
#include "pages/anchors_page.hpp"
#include "pages/home_page.hpp"
#include "pages/label_page.hpp"

#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{
    MainWindow::MainWindow(app::NanApplication& application, const app::WindowConfig& config):
        NanWindow(application, config) {}

    void MainWindow::on_setup() {
        NanWindow::on_setup();
        const auto routes = app::Routes {
            nandina::app::route<ShowcaseHomePage>(
                app::RouteOptions {.address = "home", .title = "首页", .type = "手册"}
            ),
            nandina::app::route<AnchorsPage>(
                app::RouteOptions {.address = "anchors", .title = "锚点", .type = "手册"}
            ),
            nandina::app::route<LabelPage>(
                app::RouteOptions {.address = "form/label", .title = "Label", .type = "表单"}
            ),
            // nandina::app::route<AnchorsPage>(
            //     app::RouteOptions {.address = "anchors", .title = "Label", .type = "按钮"}
            // ),
            // nandina::app::route<AnchorsPage>(
            //     app::RouteOptions {.address = "anchors", .title = "Label", .type = "数据"}
            // ),
            // nandina::app::route<AnchorsPage>(
            //     app::RouteOptions {.address = "anchors", .title = "Label", .type = "面板"}
            // ),
            // nandina::app::route<AnchorsPage>(
            //     app::RouteOptions {.address = "anchors", .title = "Label", .type = "浮层"}
            // ),
            // nandina::app::route<AnchorsPage>(
            //     app::RouteOptions {.address = "anchors", .title = "Label", .type = "菜单"}
            // ),
            // nandina::app::route<AnchorsPage>(
            //     app::RouteOptions {.address = "anchors", .title = "Label", .type = "对话框"}
            // ),
            // nandina::app::route<AnchorsPage>(
            //     app::RouteOptions {.address = "anchors", .title = "Label", .type = "杂项"}
            // )

        };
        auto& router = use_router(routes);

        set_shell([](const app::ShellContext& context) -> widget::View {
            const Sidebar sidebar {context};

            return sidebar.build_shell();
        });

        (void)router.start<ShowcaseHomePage>();
    }
} // namespace nandina::showcase
