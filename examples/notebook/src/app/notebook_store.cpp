#include "notebook_store.hpp"

#include <utility>

namespace nan_example::notebook::app
{
    NotebookStore::NotebookStore(nandina::reactive::Graph&, data::Database database):
        database_(std::move(database)),
        user_repository_(database_),
        user_service_(user_repository_) {}

    auto NotebookStore::database() noexcept -> data::Database& {
        return database_;
    }

    auto NotebookStore::users() noexcept -> data::UserRepository& {
        return user_repository_;
    }

    auto NotebookStore::user_service() noexcept -> core::UserService& {
        return user_service_;
    }
} // namespace nandina::examples::notebook::app
