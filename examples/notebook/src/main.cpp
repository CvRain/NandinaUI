#include "app/main_window.hpp"
#include "app/notebook_store.hpp"
#include "data/migration.hpp"
#include "showcase/main_window.hpp"

#include <nandina/app/nan_application.hpp>
#include <nandina/foundation/nan_logger.hpp>

#include <utility>

int main() {
    auto config = nandina::app::NanApplicationConfig::for_process("com.nandina.notebook");
    // This is a repository demo, not an installed application: keep its database in the
    // disposable build tree instead of polluting the user's data directory. The path is
    // derived from the executable so the app behaves the same from any working directory.
    const auto database_path = config.executable_path.parent_path() / "notebook.db";
    // Meson writes the resource package beside the executable. Migration SQL is read
    // from that package before the application store is built.
    config.resource_package = "resources.db";

    nandina::app::NanApplication application(std::move(config));
    auto bootstrap =
        nan_example::notebook::data::bootstrap_database(database_path, application.resources());
    if (!bootstrap) {
        // Keep startup errors on the framework logger: std::println pulls terminal
        // symbols that are not available from every Windows toolchain link line.
        nandina::log::error(
            "notebook: {}",
            nan_example::notebook::data::describe(bootstrap.error())
        );
        return 1;
    }

    nandina::log::info(
        "notebook: database {} at {} (schema v{}, {} migration(s) applied)",
        bootstrap->created ? "created" : "opened",
        database_path.string(),
        bootstrap->migrations.current_version,
        bootstrap->migrations.applied.size()
    );

    // Composition root: the application owns the store for the whole process.
    // Pages resolve NotebookStore through PageContext instead of constructing services.
    application.use_store<nan_example::notebook::app::NotebookStore>(
        std::move(bootstrap->database)
    );

    const auto window_info = nandina::app::WindowConfig {
        .title = "Nandina Notebook",
        .width = 1100,
        .height = 760,
    };

    auto window = nan_example::notebook::app::MainWindow(application, window_info);

    return application.run(window);
}
