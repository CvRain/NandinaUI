#include "notebook_store.hpp"

#include <utility>

namespace nan_example::notebook::app
{
    NotebookStore::NotebookStore(nandina::reactive::Graph& graph, data::Database database):
        database_(std::move(database)),
        user_repository_(database_),
        user_service_(user_repository_),
        current_user_(graph, std::nullopt) {}

    auto NotebookStore::database() noexcept -> data::Database& {
        return database_;
    }

    auto NotebookStore::users() noexcept -> data::UserRepository& {
        return user_repository_;
    }

    auto NotebookStore::user_service() noexcept -> core::UserService& {
        return user_service_;
    }

    auto NotebookStore::current_user() noexcept
        -> nandina::reactive::Signal<std::optional<models::User>>& {
        return current_user_;
    }

    auto NotebookStore::current_user() const noexcept
        -> const nandina::reactive::Signal<std::optional<models::User>>& {
        return current_user_;
    }

    void NotebookStore::sign_in(models::User user) {
        current_user_.set(std::optional<models::User> {std::move(user)});
    }

    void NotebookStore::sign_out() {
        current_user_.set(std::nullopt);
    }
} // namespace nan_example::notebook::app
