#ifndef NANDINA_EXAMPLES_NOTEBOOK_APP_NOTEBOOK_STORE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_APP_NOTEBOOK_STORE_HPP

#include "core/services/user_service.hpp"
#include "data/database.hpp"
#include "data/repositories/user_repository.hpp"

#include <nandina/app/nan_store.hpp>
#include <nandina/reactive/graph.hpp>

namespace nan_example::notebook::app
{
    /// Application composition root and service container.
    ///
    /// The store is created by `NanApplication::use_store()` after migrations have
    /// initialized the database. It owns the long-lived database and exposes the
    /// repository/service graph to every page through `PageContext::store()`.
    ///
    /// Lifetime: database -> repository -> service. Destructors run in reverse, so
    /// the service and repository never outlive the connection they reference.
    class NotebookStore final: public nandina::app::NanStore {
    public:
        NotebookStore(nandina::reactive::Graph& graph, data::Database database);

        [[nodiscard]] auto database() noexcept -> data::Database&;
        [[nodiscard]] auto users() noexcept -> data::UserRepository&;
        [[nodiscard]] auto user_service() noexcept -> core::UserService&;

    private:
        data::Database database_;
        data::UserRepository user_repository_;
        core::UserService user_service_;
    };
} // namespace nan_example::notebook::app

#endif // NANDINA_EXAMPLES_NOTEBOOK_APP_NOTEBOOK_STORE_HPP
