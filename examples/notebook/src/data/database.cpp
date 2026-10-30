#include "database.hpp"

#include <sqlite3.h>

#include <string>
#include <system_error>
#include <utility>

namespace nan_example::notebook::data
{
    namespace
    {
        [[nodiscard]] auto database_error(
            sqlite3* database,
            const DataErrorCode code,
            std::string operation,
            const int sqlite_code
        ) -> DataError {
            const auto* message = database != nullptr ? sqlite3_errmsg(database) : nullptr;
            return {
                .code = code,
                .operation = std::move(operation),
                .message = message != nullptr ? message : sqlite3_errstr(sqlite_code),
                .sqlite_code = sqlite_code,
            };
        }

        [[nodiscard]] auto update_error_code(const int result) -> DataErrorCode {
            return result == SQLITE_CONSTRAINT ? DataErrorCode::constraint_violation
                                               : DataErrorCode::sql_exec_failed;
        }

        void close_connection(sqlite3* database) noexcept {
            if (database == nullptr) {
                return;
            }
            if (sqlite3_close(database) != SQLITE_OK) {
                // Outstanding statements keep the close pending; releasing them later finishes
                // it. Callers own statement lifetime, so this only prevents a leaked handle.
                sqlite3_close_v2(database);
            }
        }
    } // namespace

    Database::Database(sqlite3* database) noexcept: database_(database) {}

    Database::~Database() {
        close_connection(database_);
    }

    Database::Database(Database&& other) noexcept:
        database_(std::exchange(other.database_, nullptr)) {}

    auto Database::operator=(Database&& other) noexcept -> Database& {
        if (this != &other) {
            close_connection(database_);
            database_ = std::exchange(other.database_, nullptr);
        }
        return *this;
    }

    auto Database::open(const std::filesystem::path& path) -> DataResult<Database> {
        if (path.empty()) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "open database",
                    .message = "database path is empty",
                }
            );
        }
        if (path.has_parent_path()) {
            std::error_code error;
            std::filesystem::create_directories(path.parent_path(), error);
            if (error) {
                return std::unexpected(
                    DataError {
                        .code = DataErrorCode::io_error,
                        .operation = "create database directory",
                        .message = error.message(),
                        .sqlite_code = 0,
                    }
                );
            }
        }
        const auto encoded = path.u8string();
        const std::string utf8(reinterpret_cast<const char*>(encoded.data()), encoded.size());
        return open_at(utf8.c_str());
    }

    auto Database::open_memory() -> DataResult<Database> {
        return open_at(":memory:");
    }

    auto Database::open_at(const char* path) -> DataResult<Database> {
        sqlite3* handle = nullptr;
        const int result = sqlite3_open_v2(
            path,
            &handle,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            nullptr
        );
        if (result != SQLITE_OK) {
            auto error =
                database_error(handle, DataErrorCode::open_failed, "open database", result);
            close_connection(handle);
            return std::unexpected(std::move(error));
        }
        Database database(handle);
        if (auto configured = database.configure(); !configured) {
            return std::unexpected(std::move(configured.error()));
        }
        return database;
    }

    auto Database::configure() -> DataResult<void> {
        if (database_ == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::open_failed,
                    .operation = "configure database",
                    .message = "database handle is not open",
                }
            );
        }
        if (const int result = sqlite3_busy_timeout(database_, 5000); result != SQLITE_OK) {
            return std::unexpected(
                database_error(database_, DataErrorCode::open_failed, "set busy timeout", result)
            );
        }
        if (auto result = exec("PRAGMA foreign_keys=ON;"); !result) {
            return result;
        }
        if (auto result = exec("PRAGMA journal_mode=WAL;"); !result) {
            return result;
        }
        return exec("PRAGMA synchronous=NORMAL;");
    }

    auto Database::prepare(const std::string_view sql) -> DataResult<Statement> {
        if (database_ == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "prepare statement",
                    .message = "database handle is not open",
                }
            );
        }
        if (sql.empty()) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "prepare statement",
                    .message = "SQL text is empty",
                }
            );
        }
        const std::string text(sql);
        sqlite3_stmt* statement = nullptr;
        const int result = sqlite3_prepare_v2(database_, text.c_str(), -1, &statement, nullptr);
        if (result != SQLITE_OK) {
            return std::unexpected(database_error(
                database_,
                DataErrorCode::sql_prepare_failed,
                "prepare statement",
                result
            ));
        }
        if (statement == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::sql_prepare_failed,
                    .operation = "prepare statement",
                    .message = "SQL text contains no executable statement",
                }
            );
        }
        return Statement(statement);
    }

    auto Database::exec(const std::string_view sql) -> DataResult<void> {
        if (database_ == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "execute SQL",
                    .message = "database handle is not open",
                }
            );
        }
        const std::string text(sql);
        char* message = nullptr;
        const int result = sqlite3_exec(database_, text.c_str(), nullptr, nullptr, &message);
        std::string detail = message != nullptr ? message : sqlite3_errmsg(database_);
        sqlite3_free(message);
        if (result != SQLITE_OK) {
            return std::unexpected(
                DataError {
                    .code = update_error_code(result),
                    .operation = "execute SQL",
                    .message = std::move(detail),
                    .sqlite_code = result,
                }
            );
        }
        return {};
    }

    auto Database::last_insert_rowid() const noexcept -> std::int64_t {
        return database_ != nullptr ? sqlite3_last_insert_rowid(database_) : 0;
    }

    auto Database::changes() const noexcept -> int {
        return database_ != nullptr ? sqlite3_changes(database_) : 0;
    }

    auto Database::begin_immediate() -> DataResult<void> {
        return exec("BEGIN IMMEDIATE;");
    }

    auto Database::commit() -> DataResult<void> {
        return exec("COMMIT;");
    }

    void Database::rollback() noexcept {
        if (database_ != nullptr) {
            sqlite3_exec(database_, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
    }

    Transaction::Transaction(Database& database) noexcept: database_(&database) {}

    Transaction::~Transaction() {
        rollback();
    }

    Transaction::Transaction(Transaction&& other) noexcept:
        database_(std::exchange(other.database_, nullptr)) {}

    auto Transaction::operator=(Transaction&& other) noexcept -> Transaction& {
        if (this != &other) {
            rollback();
            database_ = std::exchange(other.database_, nullptr);
        }
        return *this;
    }

    auto Transaction::begin(Database& database) -> DataResult<Transaction> {
        if (auto result = database.begin_immediate(); !result) {
            return std::unexpected(std::move(result.error()));
        }
        return Transaction(database);
    }

    auto Transaction::commit() -> DataResult<void> {
        if (database_ == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "commit transaction",
                    .message = "transaction is not active",
                }
            );
        }
        auto result = database_->commit();
        if (result) {
            database_ = nullptr;
        }
        return result;
    }

    void Transaction::rollback() noexcept {
        if (database_ != nullptr) {
            database_->rollback();
            database_ = nullptr;
        }
    }
} // namespace nandina::examples::notebook::data
