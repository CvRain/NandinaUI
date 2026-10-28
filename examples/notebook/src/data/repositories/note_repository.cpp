#include "note_repository.hpp"

#include "../mappers/row_mapper.hpp"
#include "repository_util.hpp"

#include <string_view>
#include <utility>

namespace nandina::examples::notebook::data
{
    namespace
    {
        constexpr std::string_view kSelectColumns =
            "id, user_id, group_id, title, content, created_at, updated_at";

        [[nodiscard]] auto collect_notes(Statement& statement)
            -> DataResult<std::vector<models::Note>> {
            std::vector<models::Note> notes;
            while (true) {
                auto step = statement.step();
                if (!step) {
                    return std::unexpected(std::move(step.error()));
                }
                if (*step == StepResult::done) {
                    return notes;
                }
                notes.push_back(read_note(statement));
            }
        }
    } // namespace

    NoteRepository::NoteRepository(Database& database) noexcept: database_(database) {}

    auto NoteRepository::create(
        const std::int64_t user_id,
        const std::string_view title,
        const std::string_view content,
        const std::optional<std::int64_t> group_id
    ) -> DataResult<models::Note> {
        if (user_id <= 0) {
            return std::unexpected(invalid_argument("create note", "user id is not set"));
        }
        if (group_id) {
            if (auto result = require_group_owner(database_, *group_id, user_id); !result) {
                return std::unexpected(std::move(result.error()));
            }
        }

        auto statement = database_.prepare(
            "INSERT INTO notes (user_id, group_id, title, content) VALUES (?, ?, ?, ?);"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = bind_optional_int64(*statement, 2, group_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(3, title); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(4, content); !result) {
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
                invalid_argument("create note", "inserted note cannot be read back")
            );
        }
        return std::move(**created);
    }

    auto NoteRepository::find(const std::int64_t note_id)
        -> DataResult<std::optional<models::Note>> {
        auto statement = database_.prepare(
            "SELECT " + std::string(kSelectColumns) + " FROM notes WHERE id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, note_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        if (*step == StepResult::done) {
            return std::nullopt;
        }
        return read_note(*statement);
    }

    auto NoteRepository::list_all(const std::optional<std::int64_t> group_id)
        -> DataResult<std::vector<models::Note>> {
        std::string sql = "SELECT " + std::string(kSelectColumns) + " FROM notes";
        if (group_id) {
            sql += " WHERE group_id = ?";
        }
        sql += " ORDER BY updated_at DESC, id DESC;";

        auto statement = database_.prepare(sql);
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (group_id) {
            if (auto result = statement->bind(1, *group_id); !result) {
                return std::unexpected(std::move(result.error()));
            }
        }
        return collect_notes(*statement);
    }

    auto NoteRepository::list_for_user(
        const std::int64_t user_id,
        const std::optional<std::int64_t> group_id
    ) -> DataResult<std::vector<models::Note>> {
        std::string sql = "SELECT " + std::string(kSelectColumns) + " FROM notes WHERE user_id = ?";
        if (group_id) {
            sql += " AND group_id = ?";
        }
        sql += " ORDER BY updated_at DESC, id DESC;";

        auto statement = database_.prepare(sql);
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (group_id) {
            if (auto result = statement->bind(2, *group_id); !result) {
                return std::unexpected(std::move(result.error()));
            }
        }
        return collect_notes(*statement);
    }

    auto NoteRepository::update(const models::Note& note) -> DataResult<bool> {
        if (note.id <= 0) {
            return std::unexpected(invalid_argument("update note", "note id is not set"));
        }
        if (note.user_id <= 0) {
            return std::unexpected(invalid_argument("update note", "user id is not set"));
        }
        if (note.group_id) {
            if (auto result = require_group_owner(database_, *note.group_id, note.user_id); !result)
            {
                return std::unexpected(std::move(result.error()));
            }
        }

        auto statement = database_.prepare(
            "UPDATE notes SET group_id = ?, title = ?, content = ?, "
            "updated_at = CURRENT_TIMESTAMP WHERE id = ? AND user_id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = bind_optional_int64(*statement, 1, note.group_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, note.title); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(3, note.content); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(4, note.id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(5, note.user_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        auto step = statement->step();
        if (!step) {
            return std::unexpected(std::move(step.error()));
        }
        return database_.changes() > 0;
    }

    auto NoteRepository::move_to_group(
        const std::int64_t note_id,
        const std::int64_t user_id,
        const std::optional<std::int64_t> group_id
    ) -> DataResult<bool> {
        if (group_id) {
            if (auto result = require_group_owner(database_, *group_id, user_id); !result) {
                return std::unexpected(std::move(result.error()));
            }
        }

        auto statement = database_.prepare(
            "UPDATE notes SET group_id = ?, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = ? AND user_id = ?;"
        );
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = bind_optional_int64(*statement, 1, group_id); !result) {
            return std::unexpected(std::move(result.error()));
        }
        if (auto result = statement->bind(2, note_id); !result) {
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

    auto NoteRepository::remove(const std::int64_t note_id, const std::int64_t user_id)
        -> DataResult<bool> {
        auto statement = database_.prepare("DELETE FROM notes WHERE id = ? AND user_id = ?;");
        if (!statement) {
            return std::unexpected(std::move(statement.error()));
        }
        if (auto result = statement->bind(1, note_id); !result) {
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
