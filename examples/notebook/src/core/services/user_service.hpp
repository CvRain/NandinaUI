#ifndef NANDINA_EXAMPLES_NOTEBOOK_CORE_SERVICES_USER_SERVICE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_CORE_SERVICES_USER_SERVICE_HPP

#include "../models/user.hpp"
#include "data/repositories/user_repository.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace nan_example::notebook::core
{
    /// User-facing operations over the persistence repository.
    ///
    /// The service does not create or own the database. The composition root
    /// (`NotebookStore`) constructs the repository and injects it here, so tests and
    /// UI pages can share one connection without a global singleton.
    class UserService final {
    public:
        explicit UserService(data::UserRepository& user_repository) noexcept;

        [[nodiscard]] auto user_count() const -> data::DataResult<int>;
        [[nodiscard]] auto user_exist() const -> data::DataResult<bool>;
        [[nodiscard]] auto get_user_by_id(std::int64_t user_id) const
            -> data::DataResult<std::optional<models::User>>;
        [[nodiscard]] auto get_user_by_name(std::string_view user_name) const
            -> data::DataResult<std::optional<models::User>>;

        /// Create a user record. Password hashing belongs to AuthService; this method
        /// deliberately receives the already-hashed value.
        [[nodiscard]] auto create_user(std::string username, std::string password_hash)
            -> data::DataResult<models::User>;

    private:
        data::UserRepository& user_repository_;
    };
} // namespace nandina::examples::notebook::core

#endif // NANDINA_EXAMPLES_NOTEBOOK_CORE_SERVICES_USER_SERVICE_HPP
