#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_MIGRATION_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_MIGRATION_HPP

#include "data_error.hpp"
#include "database.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nandina::resource
{
    class ResourceManager;
} // namespace nandina::resource

namespace nandina::examples::notebook::data
{
    /// One SQL migration loaded from the resource package.
    struct Migration {
        int version = 0;
        std::string name;
        std::string sql;
    };

    struct MigrationReport {
        int previous_version = 0;
        int current_version = 0;
        std::vector<int> applied;
    };

    struct DatabaseBootstrap {
        Database database;
        MigrationReport migrations;
        /// True when the database file did not exist before this call.
        bool created = false;
    };

    /// Read every migration declared in resources.toml through the mounted package.
    ///
    /// The generated resource key header is the compile-time link between the policy and this
    /// code: renaming or deleting a migration declaration fails the build at the call site.
    [[nodiscard]] auto load_migrations(const resource::ResourceManager& resources)
        -> DataResult<std::vector<Migration>>;

    /// Require ascending, contiguous versions starting at 1 with unique names and non-empty SQL.
    [[nodiscard]] auto validate_migrations(const std::vector<Migration>& migrations)
        -> DataResult<void>;

    /// Apply pending migrations in individual transactions and record them in
    /// `schema_migrations`. A database newer than the application is rejected.
    [[nodiscard]] auto
    apply_migrations(Database& database, const std::vector<Migration>& migrations)
        -> DataResult<MigrationReport>;

    /// Open (or create) the app database, load the SQL from nanres resources, and migrate it.
    [[nodiscard]] auto bootstrap_database(
        const std::filesystem::path& path,
        const resource::ResourceManager& resources
    ) -> DataResult<DatabaseBootstrap>;
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_MIGRATION_HPP
