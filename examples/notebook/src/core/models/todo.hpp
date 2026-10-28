#ifndef NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_TODO_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_TODO_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace nandina::examples::notebook::models
{
    struct Todo {
        std::int64_t id = 0;
        std::int64_t user_id = 0;
        std::string title;
        std::string description;
        bool completed = false;
        /// Optional ISO-8601 instant supplied by the caller. The data layer stores it verbatim.
        std::optional<std::string> due_at;
        std::string created_at;
        std::string updated_at;

        friend auto operator==(const Todo&, const Todo&) -> bool = default;
    };
} // namespace nandina::examples::notebook::models

#endif // NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_TODO_HPP
