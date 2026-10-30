#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_TODO_REPOSITORY_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_TODO_REPOSITORY_HPP

#include "../data_error.hpp"
#include "../database.hpp"

#include "../../core/models/todo.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nan_example::notebook::data
{
    class TodoRepository {
    public:
        explicit TodoRepository(Database& database) noexcept;

        [[nodiscard]] auto create(
            std::int64_t user_id,
            std::string_view title,
            std::string_view description = {},
            std::optional<std::string> due_at = std::nullopt
        ) -> DataResult<models::Todo>;

        [[nodiscard]] auto find(std::int64_t todo_id) -> DataResult<std::optional<models::Todo>>;
        [[nodiscard]] auto
        list_for_user(std::int64_t user_id, std::optional<bool> completed = std::nullopt)
            -> DataResult<std::vector<models::Todo>>;

        /// Update title/description/completion/due date by (id, user_id).
        [[nodiscard]] auto update(const models::Todo& todo) -> DataResult<bool>;
        [[nodiscard]] auto set_completed(std::int64_t todo_id, std::int64_t user_id, bool completed)
            -> DataResult<bool>;
        [[nodiscard]] auto remove(std::int64_t todo_id, std::int64_t user_id) -> DataResult<bool>;
        /// Delete every completed todo for one user; returns the deleted row count.
        [[nodiscard]] auto clear_completed(std::int64_t user_id) -> DataResult<int>;

    private:
        Database& database_;
    };
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_TODO_REPOSITORY_HPP
