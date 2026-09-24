//
// Created by cvrain on 2026/9/23.
//

#include "main_window.hpp"

namespace nandina::showcase
{

    MainWindow::MainWindow(
        nandina::app::NanApplication& application,
        const nandina::app::WindowConfig& config
    ):
        NanWindow(application, config) {}

    void MainWindow::on_setup() {
        NanWindow::on_setup();
    }
} // namespace nandina::showcase