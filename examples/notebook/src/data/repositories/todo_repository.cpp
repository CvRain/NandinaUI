#include "todo_repository.hpp"

#include "../mappers/row_mapper.hpp"
#include "repository_util.hpp"

#include <utility>

namespace nandina::examples::notebook::data
{
    namespace
    {
        constexpr std::string_view kSelectColumns =
            "id, user_id, title, description, completed, due_at, created_at, updated_at";

        [[nodiscard]] auto has_visible_text(const std::string_view text) -> bool {
            return text.find_first_not_of(" \t\r\n") != std::string_view::npos;
        }

        [[nodiscard]] auto collect_todos(Statement& statement)
            -> DataResult<std::vector<models::Todo>> {
            std::vector<models::Todo> todos;
            while (true) {
                auto step = statement.step();
                if (!step) {
                    return std::unexpected(std::move(step.error()));
                }
                if (*step == StepResult::done) {
                    return todos;
                }
                todos.push_back(read_todo(statement));
            }
        }
    } // namespace

    TodoRepository::TodoRepository(Database& database) noexcept: database_(database) {}

    auto TodoRepository::create(
        const std::int64_t user_id,
        const std::string_view title,
        const std::string_view description,
        std::optional<std::string> due_at
    ) -> DataResult<models::Todo> {
        if (user_id <= 0) {
            return std::unexpected(invalid_argument("create todo", "user id is not set"));
        }
        if (!has_visible_text(title)) {
            return std::unexpected(invalid_argument("create todo", "title is empty"));
        }

        auto statement = database_.prepare(
            "INSERT INTO todos (user_id, title, description, due_at) VALUES (?, ?, ?, ?);"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, title); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(3, description); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = bind_optional_text(*statement, 4, due_at); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }

        auto created = find(database_.last_insert_rowid());
        if (!created) {
            return std::unexpected(std::move(created.error()));
        }
        if (!*created) {
            return std::unexpected(
                invalid_argument("create todo", "inserted todo cannot be read back")
            );
        }
        return std::move(**created);
    }

    auto TodoRepository::find(const std::int64_t todo_id)
        -> DataResult<std::optional<models::Todo>> {
        auto statement = database_.prepare(
            "SELECT " + std::string(kSelectColumns) + " FROM todos WHERE id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, todo_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        if (*step == StepResult::done) {
            return std::nullopt;
        }
        return read_todo(*statement);
    }

    auto
    TodoRepository::list_for_user(const std::int64_t user_id, const std::optional<bool> completed)
        -> DataResult<std::vector<models::Todo>> {
        std::string sql = "SELECT " + std::string(kSelectColumns) + " FROM todos WHERE user_id = ?";
        if (completed) {
            sql += " AND completed = ?";
        }
        sql += " ORDER BY completed, id;";

        auto statement = database_.prepare(sql);
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (completed) {
            if (auto result = statement->bind(2, static_cast<std::int64_t>(*completed ? 1 : 0));
                !result)
            {
                return std::unexpected(std::move(result.error()));
            }
        }
        return collect_todos(*statement);
    }

    auto TodoRepository::update(const models::Todo& todo) -> DataResult<bool> {
        if (todo.id <= 0) {
            return std::unexpected(invalid_argument("update todo", "todo id is not set"));
        }
        if (todo.user_id <= 0) {
            return std::unexpected(invalid_argument("update todo", "user id is not set"));
        }
        if (!has_visible_text(todo.title)) {
            return std::unexpected(invalid_argument("update todo", "title is empty"));
        }

        auto statement = database_.prepare(
            "UPDATE todos SET title = ?, description = ?, completed = ?, due_at = ?, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = ? AND user_id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, todo.title); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, todo.description); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(3, static_cast<std::int64_t>(todo.completed ? 1 : 0));
            !result)
        {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = bind_optional_text(*statement, 4, todo.due_at); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(5, todo.id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(6, todo.user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        return database_.changes() > 0;
    }

    auto TodoRepository::set_completed(
        const std::int64_t todo_id,
        const std::int64_t user_id,
        const bool completed
    ) -> DataResult<bool> {
        auto statement = database_.prepare(
            "UPDATE todos SET completed = ?, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = ? AND user_id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, static_cast<std::int64_t>(completed ? 1 : 0)); !result)
        {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, todo_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(3, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        return database_.changes() > 0;
    }

    auto TodoRepository::remove(const std::int64_t todo_id, const std::int64_t user_id)
        -> DataResult<bool> {
        auto statement = database_.prepare("DELETE FROM todos WHERE id = ? AND user_id = ?;");
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, todo_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        return database_.changes() > 0;
    }

    auto TodoRepository::clear_completed(const std::int64_t user_id) -> DataResult<int> {
        auto statement =
            database_.prepare("DELETE FROM todos WHERE user_id = ? AND completed = 1;");
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        return database_.changes();
    }
} // namespace nandina::examples::notebook::data
