#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_REPOSITORY_UTIL_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_REPOSITORY_UTIL_HPP

#include "../database.hpp"
#include "../statement.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace nan_example::notebook::data
{
    [[nodiscard]] auto
    bind_optional_int64(Statement& statement, int index, const std::optional<std::int64_t>& value)
        -> DataResult<void>;

    [[nodiscard]] auto
    bind_optional_text(Statement& statement, int index, const std::optional<std::string>& value)
        -> DataResult<void>;

    [[nodiscard]] auto bind_optional_blob(
        Statement& statement,
        int index,
        const std::optional<std::vector<std::byte>>& value
    ) -> DataResult<void>;

    /// Verify that a note group exists and belongs to `user_id`.
    [[nodiscard]] auto
    require_group_owner(Database& database, std::int64_t group_id, std::int64_t user_id)
        -> DataResult<void>;

    [[nodiscard]] auto invalid_argument(std::string operation, std::string message) -> DataError;
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_REPOSITORY_UTIL_HPP
