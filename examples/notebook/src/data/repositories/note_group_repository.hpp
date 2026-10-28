#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_NOTE_GROUP_REPOSITORY_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_NOTE_GROUP_REPOSITORY_HPP

#include "../data_error.hpp"
#include "../database.hpp"

#include "../../core/models/note_group.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::examples::notebook::data
{
    class NoteGroupRepository {
    public:
        explicit NoteGroupRepository(Database& database) noexcept;

        [[nodiscard]] auto create(std::int64_t user_id, std::string_view name, int position = 0)
            -> DataResult<models::NoteGroup>;

        [[nodiscard]] auto find(std::int64_t group_id, std::int64_t user_id)
            -> DataResult<std::optional<models::NoteGroup>>;
        [[nodiscard]] auto list_for_user(std::int64_t user_id)
            -> DataResult<std::vector<models::NoteGroup>>;

        /// Update name/position by (id, user_id). Returns false when no row matched.
        [[nodiscard]] auto update(const models::NoteGroup& group) -> DataResult<bool>;
        /// Notes in the group are retained and become ungrouped through ON DELETE SET NULL.
        [[nodiscard]] auto remove(std::int64_t group_id, std::int64_t user_id) -> DataResult<bool>;

    private:
        Database& database_;
    };
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_NOTE_GROUP_REPOSITORY_HPP
