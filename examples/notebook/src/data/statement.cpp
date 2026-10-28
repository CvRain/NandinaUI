#include "statement.hpp"

#include <sqlite3.h>

#include <limits>
#include <utility>

namespace nandina::examples::notebook::data
{
    namespace
    {
        [[nodiscard]] auto statement_error(
            sqlite3_stmt* statement,
            const DataErrorCode code,
            std::string operation,
            const int sqlite_code
        ) -> DataError {
            auto* database = sqlite3_db_handle(statement);
            return {
                .code = code,
                .operation = std::move(operation),
                .message =
                    database != nullptr ? sqlite3_errmsg(database) : sqlite3_errstr(sqlite_code),
                .sqlite_code = sqlite_code,
            };
        }

        [[nodiscard]] auto bind_index_valid(sqlite3_stmt* statement, const int index) -> bool {
            return statement != nullptr && index > 0
                && index <= sqlite3_bind_parameter_count(statement);
        }
    } // namespace

    Statement::Statement(sqlite3_stmt* statement) noexcept: statement_(statement) {}

    Statement::~Statement() {
        if (statement_ != nullptr) {
            sqlite3_finalize(statement_);
        }
    }

    Statement::Statement(Statement&& other) noexcept:
        statement_(std::exchange(other.statement_, nullptr)) {}

    auto Statement::operator=(Statement&& other) noexcept -> Statement& {
        if (this != &other) {
            if (statement_ != nullptr) {
                sqlite3_finalize(statement_);
            }
            statement_ = std::exchange(other.statement_, nullptr);
        }
        return *this;
    }

    auto Statement::bind(const int index, const std::int64_t value) const -> DataResult<void> {
        if (!bind_index_valid(statement_, index)) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "bind integer",
                    .message = "statement or bind index is invalid",
                }
            );
        }
        if (const int result = sqlite3_bind_int64(statement_, index, value); result != SQLITE_OK) {
            return std::unexpected(
                statement_error(statement_, DataErrorCode::bind_failed, "bind integer", result)
            );
        }
        return {};
    }

    auto Statement::bind(const int index, const std::string_view value) const -> DataResult<void> {
        if (!bind_index_valid(statement_, index)) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "bind text",
                    .message = "statement or bind index is invalid",
                }
            );
        }
        if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "bind text",
                    .message = "text value exceeds SQLite's int limit",
                }
            );
        }
        // A default-constructed string_view has a null data pointer; binding that would
        // produce SQL NULL instead of empty text and break NOT NULL columns.
        const char* data = value.data() != nullptr ? value.data() : "";
        const int result = sqlite3_bind_text(
            statement_,
            index,
            data,
            static_cast<int>(value.size()),
            SQLITE_TRANSIENT
        );
        if (result != SQLITE_OK) {
            return std::unexpected(
                statement_error(statement_, DataErrorCode::bind_failed, "bind text", result)
            );
        }
        return {};
    }

    auto Statement::bind(const int index, const std::vector<std::byte>& value) const
        -> DataResult<void> {
        if (!bind_index_valid(statement_, index)) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "bind blob",
                    .message = "statement or bind index is invalid",
                }
            );
        }
        if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "bind blob",
                    .message = "blob value exceeds SQLite's int limit",
                }
            );
        }
        const int result = value.empty() ? sqlite3_bind_zeroblob(statement_, index, 0)
                                         : sqlite3_bind_blob(
                                               statement_,
                                               index,
                                               value.data(),
                                               static_cast<int>(value.size()),
                                               SQLITE_TRANSIENT
                                           );
        if (result != SQLITE_OK) {
            return std::unexpected(
                statement_error(statement_, DataErrorCode::bind_failed, "bind blob", result)
            );
        }
        return {};
    }

    auto Statement::bind_null(const int index) const -> DataResult<void> {
        if (!bind_index_valid(statement_, index)) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "bind null",
                    .message = "statement or bind index is invalid",
                }
            );
        }
        if (const int result = sqlite3_bind_null(statement_, index); result != SQLITE_OK) {
            return std::unexpected(
                statement_error(statement_, DataErrorCode::bind_failed, "bind null", result)
            );
        }
        return {};
    }

    auto Statement::step() const -> DataResult<StepResult> {
        if (statement_ == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "step",
                    .message = "statement is not initialized",
                }
            );
        }
        const int result = sqlite3_step(statement_);
        if (result == SQLITE_ROW) {
            return StepResult::row;
        }
        if (result == SQLITE_DONE) {
            return StepResult::done;
        }
        const auto code = result == SQLITE_CONSTRAINT ? DataErrorCode::constraint_violation
                                                      : DataErrorCode::sql_step_failed;
        return std::unexpected(statement_error(statement_, code, "step", result));
    }

    auto Statement::reset() const -> DataResult<void> {
        if (statement_ == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "reset",
                    .message = "statement is not initialized",
                }
            );
        }
        if (const int result = sqlite3_reset(statement_); result != SQLITE_OK) {
            return std::unexpected(
                statement_error(statement_, DataErrorCode::sql_step_failed, "reset", result)
            );
        }
        return {};
    }

    auto Statement::clear_bindings() const -> DataResult<void> {
        if (statement_ == nullptr) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::invalid_argument,
                    .operation = "clear bindings",
                    .message = "statement is not initialized",
                }
            );
        }
        if (const int result = sqlite3_clear_bindings(statement_); result != SQLITE_OK) {
            return std::unexpected(
                statement_error(statement_, DataErrorCode::bind_failed, "clear bindings", result)
            );
        }
        return {};
    }

    auto Statement::column_count() const noexcept -> int {
        return statement_ != nullptr ? sqlite3_column_count(statement_) : 0;
    }

    auto Statement::column_is_null(const int index) const noexcept -> bool {
        return statement_ == nullptr || index < 0 || index >= column_count()
            || sqlite3_column_type(statement_, index) == SQLITE_NULL;
    }

    auto Statement::column_int64(const int index) const noexcept -> std::int64_t {
        return statement_ != nullptr ? sqlite3_column_int64(statement_, index) : 0;
    }

    auto Statement::column_int(const int index) const noexcept -> int {
        return statement_ != nullptr ? sqlite3_column_int(statement_, index) : 0;
    }

    auto Statement::column_text(const int index) const -> std::string_view {
        if (statement_ == nullptr) {
            return {};
        }
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(statement_, index));
        if (text == nullptr) {
            return {};
        }
        return {text, static_cast<std::size_t>(sqlite3_column_bytes(statement_, index))};
    }

    auto Statement::column_optional_text(const int index) const -> std::optional<std::string> {
        if (column_is_null(index)) {
            return std::nullopt;
        }
        const auto value = column_text(index);
        return std::string(value);
    }

    auto Statement::column_blob(const int index) const -> std::span<const std::byte> {
        if (statement_ == nullptr) {
            return {};
        }
        const auto* data = static_cast<const std::byte*>(sqlite3_column_blob(statement_, index));
        const int size = sqlite3_column_bytes(statement_, index);
        if (data == nullptr || size <= 0) {
            return {};
        }
        return {data, static_cast<std::size_t>(size)};
    }

    auto Statement::column_optional_blob(const int index) const
        -> std::optional<std::vector<std::byte>> {
        if (column_is_null(index)) {
            return std::nullopt;
        }
        const auto view = column_blob(index);
        return std::vector<std::byte>(view.begin(), view.end());
    }
} // namespace nandina::examples::notebook::data
