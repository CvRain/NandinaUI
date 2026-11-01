#include "user_service.hpp"

#include "auth_service.hpp"
#include "nandina/foundation/nan_logger.hpp"

#include <utility>

namespace nan_example::notebook::core
{
    namespace
    {
        [[nodiscard]] auto trim(const std::string_view text) -> std::string_view {
            constexpr std::string_view whitespace = " \t\r\n";
            const auto begin = text.find_first_not_of(whitespace);
            if (begin == std::string_view::npos) {
                return {};
            }
            const auto end = text.find_last_not_of(whitespace);
            return text.substr(begin, end - begin + 1);
        }

        [[nodiscard]] auto invalid_argument(std::string operation, std::string message)
            -> data::DataError {
            return data::DataError {
                .code = data::DataErrorCode::invalid_argument,
                .operation = std::move(operation),
                .message = std::move(message),
            };
        }
    } // namespace

    UserService::UserService(data::UserRepository& user_repository) noexcept:
        user_repository_(user_repository) {}

    auto UserService::user_count() const -> data::DataResult<int> {
        return user_repository_.length();
    }

    auto UserService::have_user() const -> data::DataResult<bool> {
        auto total = user_repository_.length();
        if (!total) {
            return std::unexpected(std::move(total.error()));
        }
        return *total > 0;
    }

    auto UserService::get_user_by_id(const std::int64_t user_id) const
        -> data::DataResult<std::optional<models::User>> {
        return user_repository_.find_by_id(user_id);
    }

    auto UserService::get_user_by_name(const std::string_view user_name) const
        -> data::DataResult<std::optional<models::User>> {
        return user_repository_.find_by_username(user_name);
    }

    auto
    UserService::register_user(const std::string_view username, const std::string_view password)
        -> data::DataResult<models::User> {
        const auto normalized = trim(username);
        if (normalized.empty()) {
            return std::unexpected(invalid_argument("register user", "username is empty"));
        }
        if (password.size() < AuthService::minimum_password_length) {
            return std::unexpected(invalid_argument(
                "register user",
                "password must be at least " + std::to_string(AuthService::minimum_password_length)
                    + " characters"
            ));
        }

        auto password_hash = AuthService::hash_password(password);
        if (!password_hash) {
            nandina::log::error("register user: password hashing failed");
            return std::unexpected(
                invalid_argument("register user", "password hashing is unavailable")
            );
        }

        auto created = user_repository_.create({
            .username = std::string(normalized),
            .password_hash = std::move(*password_hash),
            .email = std::nullopt,
            .avatar = std::nullopt,
        });
        if (created) {
            nandina::log::info("registered user [{}] (id={})", created->username, created->id);
        }
        return created;
    }

    auto
    UserService::user_login(const std::string_view username, const std::string_view password) const
        -> data::DataResult<std::optional<models::User>> {
        const auto normalized = trim(username);
        if (normalized.empty() || password.empty()) {
            return std::optional<models::User> {};
        }

        auto found = user_repository_.find_by_username(normalized);
        if (!found) {
            nandina::log::error(
                "user login failed: {}::{}",
                found.error().message,
                found.error().sqlite_code
            );
            return std::unexpected(std::move(found.error()));
        }
        if (!found->has_value()) {
            return std::optional<models::User> {};
        }

        const auto verified = AuthService::verify_password(password, (*found)->password_hash);
        if (!verified) {
            nandina::log::error("user login failed: stored password hash is invalid");
            return std::optional<models::User> {};
        }
        if (!*verified) {
            return std::optional<models::User> {};
        }

        nandina::log::info("user [{}] signed in", (*found)->username);
        return *found;
    }
} // namespace nan_example::notebook::core
