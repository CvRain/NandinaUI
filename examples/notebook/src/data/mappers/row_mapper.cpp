#include "row_mapper.hpp"

#include <cstdint>
#include <string>

namespace nan_example::notebook::data
{
    namespace
    {
        [[nodiscard]] auto optional_int64(const Statement& row, const int column)
            -> std::optional<std::int64_t> {
            if (row.column_is_null(column)) {
                return std::nullopt;
            }
            return row.column_int64(column);
        }
    } // namespace

    auto read_user(const Statement& row) -> models::User {
        return {
            .id = row.column_int64(0),
            .username = std::string(row.column_text(1)),
            .password_hash = std::string(row.column_text(2)),
            .email = row.column_optional_text(3),
            .avatar = row.column_optional_blob(4),
            .created_at = std::string(row.column_text(5)),
            .updated_at = std::string(row.column_text(6)),
        };
    }

    auto read_note_group(const Statement& row) -> models::NoteGroup {
        return {
            .id = row.column_int64(0),
            .user_id = row.column_int64(1),
            .name = std::string(row.column_text(2)),
            .position = row.column_int(3),
            .created_at = std::string(row.column_text(4)),
            .updated_at = std::string(row.column_text(5)),
        };
    }

    auto read_note(const Statement& row) -> models::Note {
        return {
            .id = row.column_int64(0),
            .user_id = row.column_int64(1),
            .group_id = optional_int64(row, 2),
            .title = std::string(row.column_text(3)),
            .content = std::string(row.column_text(4)),
            .created_at = std::string(row.column_text(5)),
            .updated_at = std::string(row.column_text(6)),
        };
    }

    auto read_todo(const Statement& row) -> models::Todo {
        return {
            .id = row.column_int64(0),
            .user_id = row.column_int64(1),
            .title = std::string(row.column_text(2)),
            .description = std::string(row.column_text(3)),
            .completed = row.column_int64(4) != 0,
            .due_at = row.column_optional_text(5),
            .created_at = std::string(row.column_text(6)),
            .updated_at = std::string(row.column_text(7)),
        };
    }
} // namespace nandina::examples::notebook::data
