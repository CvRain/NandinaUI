#include "note_group_repository.hpp"

#include "../mappers/row_mapper.hpp"
#include "repository_util.hpp"

#include <string_view>
#include <utility>

namespace nan_example::notebook::data
{
    namespace
    {
        constexpr std::string_view kSelectColumns =
            "id, user_id, name, position, created_at, updated_at";

        [[nodiscard]] auto has_visible_text(const std::string_view text) -> bool {
            return text.find_first_not_of(" \t\r\n") != std::string::npos;
        }
    } // namespace

    NoteGroupRepository::NoteGroupRepository(Database& database) noexcept: database_(database) {}

    auto NoteGroupRepository::create(
        const std::int64_t user_id,
        const std::string_view name,
        const int position
    ) -> DataResult<models::NoteGroup> {
        if (user_id <= 0) {
            return std::unexpected(invalid_argument("create note group", "user id is not set"));
        }
        if (!has_visible_text(name)) {
            return std::unexpected(invalid_argument("create note group", "name is empty"));
        }

        auto statement = database_.prepare(
            "INSERT INTO note_groups (user_id, name, position) VALUES (?, ?, ?);"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, name); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(3, static_cast<std::int64_t>(position)); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }

        auto created = find(database_.last_insert_rowid(), user_id);
        if (!created) {
            return std::unexpected(std::move(created.error()));
        }
        if (!*created) {
            return std::unexpected(
                invalid_argument("create note group", "inserted group cannot be read back")
            );
        }
        return std::move(**created);
    }

    auto NoteGroupRepository::find(const std::int64_t group_id, const std::int64_t user_id)
        -> DataResult<std::optional<models::NoteGroup>> {
        auto statement = database_.prepare(
            "SELECT " + std::string(kSelectColumns)
            + " FROM note_groups WHERE id = ? AND user_id = ?;"
        );
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
            return std::nullopt;
        }
        return read_note_group(*statement);
    }

    auto NoteGroupRepository::list_for_user(const std::int64_t user_id)
        -> DataResult<std::vector<models::NoteGroup>> {
        auto statement = database_.prepare(
            "SELECT " + std::string(kSelectColumns)
            + " FROM note_groups WHERE user_id = ? ORDER BY position, id;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }

        std::vector<models::NoteGroup> groups;
        while (true) {
            auto step = statement->step();
            if (!step) {
                return std::unexpected(std::move(step.error()));
            }
            if (*step == StepResult::done) {
                return groups;
            }
            groups.push_back(read_note_group(*statement));
        }
    }

    auto NoteGroupRepository::update(const models::NoteGroup& group) -> DataResult<bool> {
        if (group.id <= 0) {
            return std::unexpected(invalid_argument("update note group", "group id is not set"));
        }
        if (group.user_id <= 0) {
            return std::unexpected(invalid_argument("update note group", "user id is not set"));
        }
        if (!has_visible_text(group.name)) {
            return std::unexpected(invalid_argument("update note group", "name is empty"));
        }

        auto statement = database_.prepare(
            "UPDATE note_groups SET name = ?, position = ?, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = ? AND user_id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, group.name); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, static_cast<std::int64_t>(group.position)); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(3, group.id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(4, group.user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        return database_.changes() > 0;
    }

    auto NoteGroupRepository::remove(const std::int64_t group_id, const std::int64_t user_id)
        -> DataResult<bool> {
        auto statement = database_.prepare("DELETE FROM note_groups WHERE id = ? AND user_id = ?;");
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
        return database_.changes() > 0;
    }
} // namespace nandina::examples::notebook::data
