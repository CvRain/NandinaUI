#include "migration.hpp"

#include "resource_keys.hpp"

#include <nandina/resource/resource_manager.hpp>

#include <array>
#include <cstdint>
#include <set>
#include <string_view>
#include <utility>

namespace nan_example::notebook::data
{
    namespace
    {
        namespace keys = nandina::resource_keys::pkg_comDOTnandinaDOTnotebook;

        struct MigrationResource {
            int version = 0;
            std::string_view name;
            std::string_view key;
        };

        // Ordered independently of the filesystem so the application always sees the same
        // sequence. nanres emits these constants from resources.toml.
        constexpr std::array<MigrationResource, 4> kMigrationResources {{
            {
                .version = 1,
                .name = "001_create_user",
                .key = keys::RESOURCE_MIGRATIONS_001_CREATE_USER_SQL,
            },
            {
                .version = 2,
                .name = "002_create_note_group",
                .key = keys::RESOURCE_MIGRATIONS_002_CREATE_NOTE_GROUP_SQL,
            },
            {
                .version = 3,
                .name = "003_create_note",
                .key = keys::RESOURCE_MIGRATIONS_003_CREATE_NOTE_SQL,
            },
            {
                .version = 4,
                .name = "004_create_todo",
                .key = keys::RESOURCE_MIGRATIONS_004_CREATE_TODO_SQL,
            },
        }};

        struct AppliedMigration {
            std::int64_t version = 0;
            std::string name;
        };

        [[nodiscard]] auto resource_error_message(const nandina::resource::ResourceError& error)
            -> std::string {
            if (error.operation.empty()) {
                return error.message;
            }
            return error.operation + ": " + error.message;
        }

        [[nodiscard]] auto load_applied(Database& database)
            -> DataResult<std::vector<AppliedMigration>> {
            auto statement =
                database.prepare("SELECT version, name FROM schema_migrations ORDER BY version;");
            if (!statement) {
                return std::unexpected(std::move(statement.error()));
            }

            std::vector<AppliedMigration> applied;
            while (true) {
                auto step = statement->step();
                if (!step) {
                    return std::unexpected(std::move(step.error()));
                }
                if (*step == StepResult::done) {
                    break;
                }
                applied.push_back({
                    .version = statement->column_int64(0),
                    .name = std::string(statement->column_text(1)),
                });
            }
            return applied;
        }
    } // namespace

    auto load_migrations(const nandina::resource::ResourceManager& resources)
        -> DataResult<std::vector<Migration>> {
        std::vector<Migration> migrations;
        migrations.reserve(kMigrationResources.size());
        for (const auto& source: kMigrationResources) {
            const auto key = nandina::resource::ResourceKey::parse(source.key);
            if (!key) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_invalid_resource,
                        .operation = "load migration " + std::string(source.name),
                        .message = "resource key is not canonical: " + std::string(source.key),
                    }
                );
            }

            auto resolved = resources.require(*key);
            if (!resolved) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_missing_resource,
                        .operation = "load migration " + std::string(source.name),
                        .message = resource_error_message(resolved.error()),
                    }
                );
            }
            if (!resolved->resource) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_missing_resource,
                        .operation = "load migration " + std::string(source.name),
                        .message = "resource lookup returned no payload",
                    }
                );
            }

            const auto bytes = resolved->resource->bytes();
            if (bytes.empty()) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_invalid_resource,
                        .operation = "load migration " + std::string(source.name),
                        .message = "migration resource is empty",
                    }
                );
            }
            std::string sql;
            sql.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            migrations.push_back({
                .version = source.version,
                .name = std::string(source.name),
                .sql = std::move(sql),
            });
        }

        if (auto valid = validate_migrations(migrations); !valid) {
            return std::unexpected(std::move(valid.error()));
        }
        return migrations;
    }

    auto validate_migrations(const std::vector<Migration>& migrations) -> DataResult<void> {
        if (migrations.empty()) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::migration_invalid_resource,
                    .operation = "validate migrations",
                    .message = "no migrations were declared",
                }
            );
        }

        std::set<std::string> names;
        int previous_version = 0;
        for (const auto& migration: migrations) {
            if (migration.version == previous_version) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_duplicate_version,
                        .operation = "validate migrations",
                        .message =
                            "duplicate migration version " + std::to_string(migration.version),
                    }
                );
            }
            if (migration.version != previous_version + 1) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_out_of_order,
                        .operation = "validate migrations",
                        .message = "expected migration version "
                            + std::to_string(previous_version + 1) + ", found "
                            + std::to_string(migration.version),
                    }
                );
            }
            if (migration.name.empty()) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_invalid_resource,
                        .operation = "validate migrations",
                        .message =
                            "migration " + std::to_string(migration.version) + " has no name",
                    }
                );
            }
            if (!names.insert(migration.name).second) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_invalid_resource,
                        .operation = "validate migrations",
                        .message = "duplicate migration name: " + migration.name,
                    }
                );
            }
            if (migration.sql.empty()) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_invalid_resource,
                        .operation = "validate migrations",
                        .message = "migration " + migration.name + " has empty SQL",
                    }
                );
            }
            previous_version = migration.version;
        }
        return {};
    }

    auto apply_migrations(Database& database, const std::vector<Migration>& migrations)
        -> DataResult<MigrationReport> {
        if (auto valid = validate_migrations(migrations); !valid) {
            return std::unexpected(std::move(valid.error()));
        }

        if (auto result = database.exec(
                "CREATE TABLE IF NOT EXISTS schema_migrations ("
                "version INTEGER PRIMARY KEY, "
                "name TEXT NOT NULL, "
                "applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                ");"
            );
            !result)
        {
            return std::unexpected(std::move(result.error()));
        }

        auto applied = load_applied(database);
        if (!applied) {
            return std::unexpected(std::move(applied.error()));
        }
        if (applied->size() > migrations.size()) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::migration_newer_schema,
                    .operation = "check schema",
                    .message = "database has more migrations than this application provides",
                }
            );
        }
        for (std::size_t index = 0; index < applied->size(); ++index) {
            const auto& stored = (*applied)[index];
            const auto& declared = migrations[index];
            if (stored.version != declared.version) {
                const auto code = stored.version > migrations.back().version
                    ? DataErrorCode::migration_newer_schema
                    : DataErrorCode::migration_out_of_order;
                return std::unexpected(
                    DataError {
                        .code = code,
                        .operation = "check schema",
                        .message = "database migration " + std::to_string(stored.version)
                            + " does not match the application sequence",
                    }
                );
            }
            if (stored.name != declared.name) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::migration_name_mismatch,
                        .operation = "check schema",
                        .message = "migration " + std::to_string(stored.version)
                            + " was applied as '" + stored.name + "' but is declared as '"
                            + declared.name + "'",
                    }
                );
            }
        }

        const int previous_version =
            applied->empty() ? 0 : static_cast<int>(applied->back().version);
        MigrationReport report {
            .previous_version = previous_version,
            .current_version = previous_version,
        };
        for (std::size_t index = applied->size(); index < migrations.size(); ++index) {
            const auto& migration = migrations[index];
            auto transaction = Transaction::begin(database);
            if (!transaction) {
                return std::unexpected(std::move(transaction.error()));
            }

            if (auto result = database.exec(migration.sql); !result) {
                auto error = std::move(result.error());
                error.operation = "apply migration " + migration.name;
                return std::unexpected(std::move(error));
            }

            auto insert =
                database.prepare("INSERT INTO schema_migrations (version, name) VALUES (?, ?);");
            if (!insert) {
                return std::unexpected(std::move(insert.error()));
            }
            if (auto result = insert->bind(1, static_cast<std::int64_t>(migration.version));
                !result)
            {
                return std::unexpected(std::move(result.error()));
            }
            if (auto result = insert->bind(2, migration.name); !result) {
                return std::unexpected(std::move(result.error()));
            }
            auto step = insert->step();
            if (!step) {
                return std::unexpected(std::move(step.error()));
            }
            if (*step != StepResult::done) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::sql_step_failed,
                        .operation = "record migration " + migration.name,
                        .message = "insert returned a row instead of finishing",
                    }
                );
            }
            if (auto result = transaction->commit(); !result) {
                return std::unexpected(std::move(result.error()));
            }

            report.current_version = migration.version;
            report.applied.push_back(migration.version);
        }

        const auto user_version =
            "PRAGMA user_version = " + std::to_string(report.current_version) + ";";
        if (auto result = database.exec(user_version); !result) {
            return std::unexpected(std::move(result.error()));
        }
        return report;
    }

    auto bootstrap_database(
        const std::filesystem::path& path,
        const nandina::resource::ResourceManager& resources
    ) -> DataResult<DatabaseBootstrap> {
        if (path.empty()) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "bootstrap database",
                    .message = "database path is empty",
                }
            );
        }

        std::error_code error;
        const bool created = !std::filesystem::exists(path, error);
        if (error) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::io_error,
                    .operation = "inspect database path",
                    .message = error.message(),
                }
            );
        }

        auto database = Database::open(path);
        if (!database) {
            return std::unexpected(std::move(database.error()));
        }
        auto migrations = load_migrations(resources);
        if (!migrations) {
            return std::unexpected(std::move(migrations.error()));
        }
        auto report = apply_migrations(*database, *migrations);
        if (!report) {
            return std::unexpected(std::move(report.error()));
        }

        return DatabaseBootstrap {
            .database = std::move(*database),
            .migrations = std::move(*report),
            .created = created,
        };
    }
} // namespace nandina::examples::notebook::data
