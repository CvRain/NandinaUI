#ifndef NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_NOTE_GROUP_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_NOTE_GROUP_HPP

#include <cstdint>
#include <string>

namespace nan_example::notebook::models
{
    struct NoteGroup {
        std::int64_t id = 0;
        std::int64_t user_id = 0;
        std::string name;
        int position = 0;
        std::string created_at;
        std::string updated_at;

        friend auto operator==(const NoteGroup&, const NoteGroup&) -> bool = default;
    };
} // namespace nandina::examples::notebook::models

#endif // NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_NOTE_GROUP_HPP
