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
    /// User-facing account operations over the persistence repository.
    ///
    /// The service does not create or own the database. The composition root
    /// (`NotebookStore`) constructs the repository and injects it here, so tests and
    /// UI pages can share one connection without a global singleton.
    class UserService final {
    public:
        explicit UserService(data::UserRepository& user_repository) noexcept;

        [[nodiscard]] auto user_count() const -> data::DataResult<int>;

        /// Whether the database already contains at least one account.
        [[nodiscard]] auto have_user() const -> data::DataResult<bool>;
        [[nodiscard]] auto get_user_by_id(std::int64_t user_id) const
            -> data::DataResult<std::optional<models::User>>;
        [[nodiscard]] auto get_user_by_name(std::string_view user_name) const
            -> data::DataResult<std::optional<models::User>>;

        /// Register an account. The password is hashed by `AuthService`; callers never
        /// see or provide the stored hash.
        [[nodiscard]] auto register_user(std::string_view username, std::string_view password)
            -> data::DataResult<models::User>;

        /// Authenticate an account.
        ///
        /// The outer error is a database/verification failure. An empty optional means
        /// the credentials were rejected, deliberately without distinguishing an
        /// unknown username from a wrong password.
        [[nodiscard]] auto user_login(std::string_view username, std::string_view password) const
            -> data::DataResult<std::optional<models::User>>;

    private:
        data::UserRepository& user_repository_;
    };
} // namespace nan_example::notebook::core

#endif // NANDINA_EXAMPLES_NOTEBOOK_CORE_SERVICES_USER_SERVICE_HPP
