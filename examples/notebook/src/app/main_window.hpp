//
// Created by cvrain on 2026/10/29.
//

#pragma once

#include <nandina/app/nan_window.hpp>

namespace nan_example::notebook::app
{
    class MainWindow final: public nandina::app::NanWindow {
    public:
        explicit MainWindow(
            nandina::app::NanApplication& app,
            const nandina::app::WindowConfig& config
        );
        ~MainWindow() override = default;

    protected:
        void on_setup() override;
    };
} // namespace nandina::examples::notebook::app
