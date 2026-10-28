#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_NOTE_REPOSITORY_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_NOTE_REPOSITORY_HPP

#include "../data_error.hpp"
#include "../database.hpp"

#include "../../core/models/note.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::examples::notebook::data
{
    class NoteRepository {
    public:
        explicit NoteRepository(Database& database) noexcept;

        [[nodiscard]] auto create(
            std::int64_t user_id,
            std::string_view title,
            std::string_view content,
            std::optional<std::int64_t> group_id = std::nullopt
        ) -> DataResult<models::Note>;

        [[nodiscard]] auto find(std::int64_t note_id) -> DataResult<std::optional<models::Note>>;
        /// Anonymous/read-only listing. `group_id` filters to one group when set.
        [[nodiscard]] auto list_all(std::optional<std::int64_t> group_id = std::nullopt)
            -> DataResult<std::vector<models::Note>>;
        [[nodiscard]] auto
        list_for_user(std::int64_t user_id, std::optional<std::int64_t> group_id = std::nullopt)
            -> DataResult<std::vector<models::Note>>;

        /// Update title/content/group by (id, user_id). Returns false when no row matched.
        [[nodiscard]] auto update(const models::Note& note) -> DataResult<bool>;
        [[nodiscard]] auto move_to_group(
            std::int64_t note_id,
            std::int64_t user_id,
            std::optional<std::int64_t> group_id
        ) -> DataResult<bool>;
        [[nodiscard]] auto remove(std::int64_t note_id, std::int64_t user_id) -> DataResult<bool>;

    private:
        Database& database_;
    };
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_NOTE_REPOSITORY_HPP
