#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_DATABASE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_DATABASE_HPP

#include "data_error.hpp"
#include "statement.hpp"

#include <cstdint>
#include <filesystem>
#include <string_view>

struct sqlite3;

namespace nandina::examples::notebook::data
{
    /// RAII owner of one SQLite connection.
    ///
    /// The connection enables foreign keys, a 5 s busy timeout, WAL journaling and normal
    /// synchronous mode. Operations are synchronous and not thread-safe; callers use the
    /// database from one thread (the UI thread in this example). Repositories keep a
    /// reference: keep the Database alive and do not move it while repositories or prepared
    /// statements exist.
    class Database {
    public:
        Database() noexcept = default;
        ~Database();
        Database(Database&& other) noexcept;
        auto operator=(Database&& other) noexcept -> Database&;
        Database(const Database&) = delete;
        auto operator=(const Database&) -> Database& = delete;

        /// Open (or create) a database file and create missing parent directories.
        [[nodiscard]] static auto open(const std::filesystem::path& path) -> DataResult<Database>;
        /// Open a private in-memory database, useful for tests and scratch sessions.
        [[nodiscard]] static auto open_memory() -> DataResult<Database>;

        [[nodiscard]] auto is_open() const noexcept -> bool {
            return database_ != nullptr;
        }
        [[nodiscard]] auto raw() noexcept -> sqlite3* {
            return database_;
        }
        [[nodiscard]] auto raw() const noexcept -> sqlite3* {
            return database_;
        }

        [[nodiscard]] auto prepare(std::string_view sql) -> DataResult<Statement>;
        /// Execute one or more statements. Migration scripts must not contain BEGIN/COMMIT.
        [[nodiscard]] auto exec(std::string_view sql) -> DataResult<void>;

        [[nodiscard]] auto last_insert_rowid() const noexcept -> std::int64_t;
        [[nodiscard]] auto changes() const noexcept -> int;

        [[nodiscard]] auto begin_immediate() -> DataResult<void>;
        [[nodiscard]] auto commit() -> DataResult<void>;
        void rollback() noexcept;

    private:
        explicit Database(sqlite3* database) noexcept;
        [[nodiscard]] static auto open_at(const char* path) -> DataResult<Database>;
        [[nodiscard]] auto configure() -> DataResult<void>;

        sqlite3* database_ = nullptr;
    };

    /// Transaction guard; rolls back if it leaves scope without a successful commit().
    class Transaction {
    public:
        [[nodiscard]] static auto begin(Database& database) -> DataResult<Transaction>;
        ~Transaction();
        Transaction(Transaction&& other) noexcept;
        auto operator=(Transaction&& other) noexcept -> Transaction&;
        Transaction(const Transaction&) = delete;
        auto operator=(const Transaction&) -> Transaction& = delete;

        [[nodiscard]] auto commit() -> DataResult<void>;
        void rollback() noexcept;
        [[nodiscard]] auto active() const noexcept -> bool {
            return database_ != nullptr;
        }

    private:
        explicit Transaction(Database& database) noexcept;

        Database* database_ = nullptr;
    };
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_DATABASE_HPP
