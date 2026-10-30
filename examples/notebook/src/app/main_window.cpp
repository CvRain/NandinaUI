//
// Created by cvrain on 2026/10/29.
//

#include "main_window.hpp"

#include "ui/auth/user_login_page.hpp"

namespace nan_example::notebook::app
{

    MainWindow::MainWindow(
        nandina::app::NanApplication& app,
        const nandina::app::WindowConfig& config
    ):
        NanWindow(app, config) {}

    void MainWindow::on_setup() {
        NanWindow::on_setup();

        const auto routers = nandina::app::Routes {
            nandina::app::route<ui::UserLoginPage>(
                nandina::app::RouteOptions {}
            )
        };

        auto& router = this->use_router(routers);

        const auto& _ = router.start<ui::UserLoginPage>();
    }
} // namespace nandina::examples::notebook::app