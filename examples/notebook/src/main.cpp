#include "data/migration.hpp"
#include "nandina/foundation/nan_logger.hpp"

#include <nandina/app/nan_application.hpp>

#include <filesystem>
#include <print>
#include <utility>

using namespace nandina;

int main() {
    auto config = app::NanApplicationConfig::for_process("com.nandina.notebook");
    // Meson writes the resource package beside the executable. The migration SQL is
    // read from this package before any repository is constructed.
    config.resource_package = "resources.db";

    app::NanApplication application(std::move(config));

    // TODO(notebook): move under the platform data root once the app-data helper lands;
    // a CWD-relative file keeps the first data-layer batch easy to inspect.
    const auto database_path = std::filesystem::path {"./notebook.db"};
    auto bootstrap =
        examples::notebook::data::bootstrap_database(database_path, application.resources());

    log::info("current database located in {}", database_path);

    if (!bootstrap) {
        std::println("notebook: {}", examples::notebook::data::describe(bootstrap.error()));
        return 1;
    }

    log::info(
        "notebook: database {} (schema v{}, {} migration(s) applied)",
        bootstrap->created ? "created" : "opened",
        bootstrap->migrations.current_version,
        bootstrap->migrations.applied.size()
    );

    // Upper layers (auth/user services and the window) receive bootstrap->database.
    // The window is intentionally not opened until those layers exist.
    return 0;
}
