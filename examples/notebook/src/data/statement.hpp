#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_STATEMENT_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_STATEMENT_HPP

#include "data_error.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3_stmt;

namespace nan_example::notebook::data
{
    enum class StepResult : std::uint8_t { row, done };

    /// Owning wrapper around one prepared statement.
    ///
    /// The statement must not outlive its Database. Column text/blob views stay valid until the
    /// next step(), reset() or destruction; copy them before stepping again.
    class Statement {
    public:
        Statement() noexcept = default;
        ~Statement();
        Statement(Statement&& other) noexcept;
        auto operator=(Statement&& other) noexcept -> Statement&;
        Statement(const Statement&) = delete;
        auto operator=(const Statement&) -> Statement& = delete;

        [[nodiscard]] auto is_valid() const noexcept -> bool {
            return statement_ != nullptr;
        }

        [[nodiscard]] auto bind(int index, std::int64_t value) const -> DataResult<void>;
        [[nodiscard]] auto bind(int index, std::string_view value) const -> DataResult<void>;
        [[nodiscard]] auto bind(int index, const std::vector<std::byte>& value) const
            -> DataResult<void>;
        [[nodiscard]] auto bind_null(int index) const -> DataResult<void>;

        [[nodiscard]] auto step() const -> DataResult<StepResult>;
        [[nodiscard]] auto reset() const -> DataResult<void>;
        [[nodiscard]] auto clear_bindings() const -> DataResult<void>;

        [[nodiscard]] auto column_count() const noexcept -> int;
        [[nodiscard]] auto column_is_null(int index) const noexcept -> bool;
        [[nodiscard]] auto column_int64(int index) const noexcept -> std::int64_t;
        [[nodiscard]] auto column_int(int index) const noexcept -> int;
        [[nodiscard]] auto column_text(int index) const -> std::string_view;
        [[nodiscard]] auto column_optional_text(int index) const -> std::optional<std::string>;
        [[nodiscard]] auto column_blob(int index) const -> std::span<const std::byte>;
        [[nodiscard]] auto column_optional_blob(int index) const
            -> std::optional<std::vector<std::byte>>;

    private:
        friend class Database;
        explicit Statement(sqlite3_stmt* statement) noexcept;

        sqlite3_stmt* statement_ = nullptr;
    };
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_STATEMENT_HPP
