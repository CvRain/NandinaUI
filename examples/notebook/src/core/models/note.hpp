#ifndef NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_NOTE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_NOTE_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace nandina::examples::notebook::models
{
    struct Note {
        std::int64_t id = 0;
        std::int64_t user_id = 0;
        std::optional<std::int64_t> group_id;
        std::string title;
        std::string content;
        std::string created_at;
        std::string updated_at;

        friend auto operator==(const Note&, const Note&) -> bool = default;
    };
} // namespace nandina::examples::notebook::models

#endif // NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_NOTE_HPP
