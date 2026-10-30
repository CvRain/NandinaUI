#include "repository_util.hpp"

#include <utility>

namespace nan_example::notebook::data
{
    auto invalid_argument(std::string operation, std::string message) -> DataError {
        return {
            .code = DataErrorCode::invalid_argument,
            .operation = std::move(operation),
            .message = std::move(message),
        };
    }

    auto bind_optional_int64(
        Statement& statement,
        const int index,
        const std::optional<std::int64_t>& value
    ) -> DataResult<void> {
        if (!value) {
            return statement.bind_null(index);
        }
        return statement.bind(index, *value);
    }

    auto bind_optional_text(
        Statement& statement,
        const int index,
        const std::optional<std::string>& value
    ) -> DataResult<void> {
        if (!value) {
            return statement.bind_null(index);
        }
        return statement.bind(index, std::string_view(*value));
    }

    auto bind_optional_blob(
        Statement& statement,
        const int index,
        const std::optional<std::vector<std::byte>>& value
    ) -> DataResult<void> {
        if (!value) {
            return statement.bind_null(index);
        }
        return statement.bind(index, *value);
    }

    auto
    require_group_owner(Database& database, const std::int64_t group_id, const std::int64_t user_id)
        -> DataResult<void> {
        auto statement =
            database.prepare("SELECT 1 FROM note_groups WHERE id = ? AND user_id = ?;");
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, group_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        if (*step == StepResult::done) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::constraint_violation,
                    .operation = "check note group",
                    .message = "note group " + std::to_string(group_id)
                        + " does not belong to user " + std::to_string(user_id),
                }
            );
        }
        return {};
    }
} // namespace nandina::examples::notebook::data
