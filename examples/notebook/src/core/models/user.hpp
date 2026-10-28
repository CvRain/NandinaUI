#ifndef NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_USER_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_USER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace nandina::examples::notebook::models
{
    struct User {
        std::int64_t id = 0;
        std::string username;
        /// Hash produced by the upper auth service; the data layer never hashes or verifies.
        std::string password_hash;
        std::optional<std::string> email;
        std::optional<std::vector<std::byte>> avatar;
        std::string created_at;
        std::string updated_at;

        friend auto operator==(const User&, const User&) -> bool = default;
    };
} // namespace nandina::examples::notebook::models

#endif // NANDINA_EXAMPLES_NOTEBOOK_CORE_MODELS_USER_HPP
