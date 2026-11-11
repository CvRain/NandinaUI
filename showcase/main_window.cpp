//
// Created by cvrain on 2026/9/23.
//

#include "main_window.hpp"

#include "components/sidebar.hpp"
#include "pages/anchors_page.hpp"
#include "pages/home_page.hpp"
#include "pages/image_page.hpp"
#include "pages/input_focus_page.hpp"
#include "pages/label_page.hpp"
#include "pages/typography_page.hpp"

#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{
    MainWindow::MainWindow(app::NanApplication& application, const app::WindowConfig& config):
        NanWindow(application, config) {}

    void MainWindow::on_setup() {
        NanWindow::on_setup();
        const auto routes = app::Routes {
            nandina::app::route<ShowcaseHomePage>(
                app::RouteOptions {.address = "home", .title = "首页", .nav_group = "手册"}
            ),
            nandina::app::route<AnchorsPage>(
                app::RouteOptions {.address = "anchors", .title = "锚点", .nav_group = "手册"}
            ),
            nandina::app::route<LabelPage>(app::RouteOptions {
                .address = "content_and_display/label",
                .title = "标签",
                .nav_group = "内容与展示"
            }),
            nandina::app::route<TypographyPage>(app::RouteOptions {
                .address = "content_and_display/typography",
                .title = "排版",
                .nav_group = "内容与展示"
            }),
            nandina::app::route<ImagePage>(app::RouteOptions {
                .address = "content_and_display/image",
                .title = "图片",
                .nav_group = "内容与展示"
            }),
            nandina::app::route<InputFocusPage>(app::RouteOptions {
                .address = "input_and_focus/focus",
                .title = "输入与焦点",
                .nav_group = "输入与焦点"
            })
        };
        auto& router = use_router(routes);

        set_shell([](const app::ShellContext& context) -> widget::View {
            const Sidebar sidebar {context};

            return sidebar.build_shell();
        });

        (void)router.start<ShowcaseHomePage>();
    }
} // namespace nandina::showcase
