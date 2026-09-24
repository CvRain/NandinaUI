//
// Created by cvrain on 2026/9/23.
//

#ifndef NANDINAUI_MAIN_WINDOW_HPP
#define NANDINAUI_MAIN_WINDOW_HPP

#include <nandina/app/nan_window.hpp>

namespace nandina::showcase
{
    class MainWindow final: public nandina::app::NanWindow {
    public:
        explicit MainWindow(
            nandina::app::NanApplication& application,
            const nandina::app::WindowConfig& config
        );

    protected:
        void on_setup() override;

    private:
    };
} // namespace nandina::showcase

#endif // NANDINAUI_MAIN_WINDOW_HPP
