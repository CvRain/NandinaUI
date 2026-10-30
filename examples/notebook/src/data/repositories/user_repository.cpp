#include "user_repository.hpp"

#include "../mappers/row_mapper.hpp"
#include "repository_util.hpp"

#include <utility>

namespace nan_example::notebook::data
{
    namespace
    {
        constexpr std::string_view K_SELECT_COLUMNS =
            "id, username, password_hash, email, avatar, created_at, updated_at";
    } // namespace

    UserRepository::UserRepository(Database& database) noexcept: database_(database) {}

    auto UserRepository::create(const NewUser& user) -> DataResult<models::User> {
        if (user.username.empty()) {
            return std::unexpected(invalid_argument("create user", "username is empty"));
        }
        if (user.password_hash.empty()) {
            return std::unexpected(invalid_argument("create user", "password hash is empty"));
        }

        auto statement = database_.prepare(
            "INSERT INTO users (username, password_hash, email, avatar) VALUES (?, ?, ?, ?);"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user.username); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, user.password_hash); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = bind_optional_text(*statement, 3, user.email); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = bind_optional_blob(*statement, 4, user.avatar); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        if (*step != StepResult::done) {
            return std::unexpected(
                invalid_argument("create user", "insert returned a row instead of finishing")
            );
        }

        auto created = find_by_id(database_.last_insert_rowid());
        if (!created) {
            return std::unexpected(std::move(created.error()));
        }
        if (!*created) {
            return std::unexpected(
                invalid_argument("create user", "inserted user cannot be read back")
            );
        }
        return std::move(**created);
    }

    auto UserRepository::find_by_id(const std::int64_t user_id) const
        -> DataResult<std::optional<models::User>> {
        auto statement = database_.prepare(
            "SELECT " + std::string(K_SELECT_COLUMNS) + " FROM users WHERE id = ?;"
        );
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
        if (*step == StepResult::done) {
            return std::nullopt;
        }
        return read_user(*statement);
    }

    auto UserRepository::find_by_username(const std::string_view username) const
        -> DataResult<std::optional<models::User>> {
        auto statement = database_.prepare(
            "SELECT " + std::string(K_SELECT_COLUMNS) + " FROM users WHERE username = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, username); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        if (*step == StepResult::done) {
            return std::nullopt;
        }
        return read_user(*statement);
    }

    auto UserRepository::list() const -> DataResult<std::vector<models::User>> {
        auto statement = database_.prepare(
            "SELECT " + std::string(K_SELECT_COLUMNS) + " FROM users ORDER BY id;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }

        std::vector<models::User> users;
        while (true) {
            auto step = statement->step();
            if (!step) {
                return std::unexpected(std::move(step.error()));
            }
            if (*step == StepResult::done) {
                return users;
            }
            users.push_back(read_user(*statement));
        }
    }

    auto UserRepository::length() const -> DataResult<int> {
        auto statement = database_.prepare("SELECT COUNT(*) FROM users;");
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }

        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        if (*step != StepResult::row) {
            return std::unexpected(
                DataError {
                    .code = DataErrorCode::sql_step_failed,
                    .operation = "count users",
                    .message = "COUNT(*) query returned no row",
                }
            );
        }
        return static_cast<int>(statement->column_int64(0));
    }

    auto UserRepository::update(const models::User& user) const -> DataResult<bool> {
        if (user.id <= 0) {
            return std::unexpected(invalid_argument("update user", "user id is not set"));
        }
        if (user.username.empty()) {
            return std::unexpected(invalid_argument("update user", "username is empty"));
        }
        if (user.password_hash.empty()) {
            return std::unexpected(invalid_argument("update user", "password hash is empty"));
        }

        auto statement = database_.prepare(
            "UPDATE users SET username = ?, password_hash = ?, email = ?, avatar = ?, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user.username); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, user.password_hash); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = bind_optional_text(*statement, 3, user.email); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = bind_optional_blob(*statement, 4, user.avatar); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(5, user.id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        return database_.changes() > 0;
    }

    auto UserRepository::remove(const std::int64_t user_id) const -> DataResult<bool> {
        auto statement = database_.prepare("DELETE FROM users WHERE id = ?;");
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
        return database_.changes() > 0;
    }
} // namespace nandina::examples::notebook::data
