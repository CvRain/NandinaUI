#include <nandina/app/nan_application.hpp>

#include "main_window.hpp"

int main() {
    auto window_info = nandina::app::WindowConfig {
        .title = "nandina showcase",
        .width = 720,
        .height = 640,
    };

    nandina::app::NanApplication application(
        nandina::app::NanApplicationConfig::for_process("com.nandina.showcase")
    );

    nandina::showcase::MainWindow main_window {application, window_info};

    return application.run(main_window);
}
