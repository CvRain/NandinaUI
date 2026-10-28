#ifndef NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_USER_REPOSITORY_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_USER_REPOSITORY_HPP

#include "../data_error.hpp"
#include "../database.hpp"

#include "../../core/models/user.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::examples::notebook::data
{
    class UserRepository {
    public:
        struct NewUser {
            std::string username;
            std::string password_hash;
            std::optional<std::string> email;
            std::optional<std::vector<std::byte>> avatar;
        };

        explicit UserRepository(Database& database) noexcept;

        [[nodiscard]] auto create(const NewUser& user) -> DataResult<models::User>;
        [[nodiscard]] auto find_by_id(std::int64_t user_id) const
            -> DataResult<std::optional<models::User>>;
        [[nodiscard]] auto find_by_username(std::string_view username) const
            -> DataResult<std::optional<models::User>>;
        [[nodiscard]] auto list() const -> DataResult<std::vector<models::User>>;
        [[nodiscard]] auto length() const -> DataResult<int>;

        /// Update a user by id. Returns false when no row matched.
        [[nodiscard]] auto update(const models::User& user) const -> DataResult<bool>;
        [[nodiscard]] auto remove(std::int64_t user_id) const -> DataResult<bool>;

    private:
        Database& database_;
    };
} // namespace nandina::examples::notebook::data

#endif // NANDINA_EXAMPLES_NOTEBOOK_DATA_REPOSITORIES_USER_REPOSITORY_HPP
