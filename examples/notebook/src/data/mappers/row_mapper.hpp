#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_MAPPERS_ROW_MAPPER_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_MAPPERS_ROW_MAPPER_HPP

#include "../statement.hpp"

#include "../../core/models/note.hpp"
#include "../../core/models/note_group.hpp"
#include "../../core/models/todo.hpp"
#include "../../core/models/user.hpp"

namespace nan_example::notebook::data
{
    // Each mapper assumes the statement is positioned on one row whose selected columns match
    // the documented order in the owning repository.
    [[nodiscard]] auto read_user(const Statement& row) -> models::User;
    [[nodiscard]] auto read_note_group(const Statement& row) -> models::NoteGroup;
    [[nodiscard]] auto read_note(const Statement& row) -> models::Note;
    [[nodiscard]] auto read_todo(const Statement& row) -> models::Todo;
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_MAPPERS_ROW_MAPPER_HPP
