#include "user_service.hpp"

#include <utility>

namespace nan_example::notebook::core
{
    UserService::UserService(data::UserRepository& user_repository) noexcept:
        user_repository_(user_repository) {}

    auto UserService::user_count() const -> data::DataResult<int> {
        return user_repository_.length();
    }

    auto UserService::user_exist() const -> data::DataResult<bool> {
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

    auto UserService::create_user(std::string username, std::string password_hash)
        -> data::DataResult<models::User> {
        return user_repository_.create({
            .username = std::move(username),
            .password_hash = std::move(password_hash),
        });
    }
} // namespace nandina::examples::notebook::core
