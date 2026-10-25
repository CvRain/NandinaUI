#include <nandina/app/nan_application.hpp>
#include <nandina/foundation/nan_logger.hpp>
#include <nandina/theme/builtin_themes.hpp>

#include <cstdlib>
#include <string_view>
#include <utility>

#include "main_window.hpp"

int main() {
    const auto window_info = nandina::app::WindowConfig {
        .title = "nandina showcase",
        .width = 1200,
        .height = 800,
    };

    auto config = nandina::app::NanApplicationConfig::for_process("com.nandina.showcase");
    // The Meson target writes the package beside the executable; the locator resolves
    // `<exe-dir>/resources` and reads resource-location.json from there.
    config.resource_package = "resources.db";
    nandina::app::NanApplication application(std::move(config));
    // 内置主题族（butter/fluent/material）默认不注册；人工验收用环境变量选择，
    // 例如 butter 的卡片软阴影与按钮涟漪圆角裁剪需要在真实主题下目检。
    nandina::theme::register_default_theme_families(application.theme_manager());
    if (const char* requested = std::getenv("NANDINA_SHOWCASE_THEME");
        requested != nullptr && *requested != '\0')
    {
        const std::string_view family {requested};
        if (!application.theme_manager().activate_family(family)) {
            nandina::log::get("showcase")
                .warn(
                    "NANDINA_SHOWCASE_THEME: unknown theme family \"{}\" (known: butter, fluent, material)",
                    family
                );
        }
    }

    nandina::showcase::MainWindow main_window {application, window_info};

    return application.run(main_window);
}
