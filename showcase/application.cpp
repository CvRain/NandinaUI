#include <nandina/app/nan_application.hpp>

#include "main_window.hpp"

int main() {
    auto window_info = nandina::app::WindowConfig {
        .title = "nandina showcase",
        .width = 1200,
        .height = 800,
    };

    nandina::app::NanApplication application(
        nandina::app::NanApplicationConfig::for_process("com.nandina.showcase")
    );

    nandina::showcase::MainWindow main_window {application, window_info};

    return application.run(main_window);
}
