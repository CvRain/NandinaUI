#include "lab_store.hpp"
#include "lab_window.hpp"

#include <nandina/app/nan_application.hpp>

#include <utility>

int main() {
    const auto window_info = nandina::app::WindowConfig {
        .title = "Nandina Lab",
        .width = 1180,
        .height = 820,
    };

    auto config = nandina::app::NanApplicationConfig::for_process("com.nandina.lab");
    // Meson writes the package beside the executable; the locator resolves
    // `<exe-dir>/resources` and falls back to a direct resources.db lookup in
    // portable staging where resource-location.json is intentionally absent.
    config.resource_package = "resources.db";

    nandina::app::NanApplication application(std::move(config));
    application.use_store<nandina::lab::LabStore>();
    nandina::lab::LabWindow window {application, window_info};
    return application.run(window);
}
