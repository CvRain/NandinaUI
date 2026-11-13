#ifndef NANDINA_EXAMPLES_NOTEBOOK_APP_NOTEBOOK_STORE_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_APP_NOTEBOOK_STORE_HPP

#include "core/services/user_service.hpp"
#include "data/data_error.hpp"
#include "data/database.hpp"
#include "data/repositories/note_group_repository.hpp"
#include "data/repositories/note_repository.hpp"
#include "data/repositories/user_repository.hpp"

#include "core/models/note.hpp"
#include "core/models/note_group.hpp"

#include <nandina/app/nan_store.hpp>
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/signal.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nan_example::notebook::app
{
    /// Application composition root and service container.
    ///
    /// The store is created by `NanApplication::use_store()` after migrations have
    /// initialized the database. It owns the long-lived database and exposes the
    /// repository/service graph to every page through `PageContext::store()`.
    ///
    /// The signed-in workspace state (group filter, note list and current note) also
    /// lives here: pages bind `group_list()` / `note_list()` to list widgets and call
    /// the CRUD entry points, so switching pages does not duplicate SQL calls or
    /// reload logic.
    ///
    /// Lifetime: database -> repositories -> service -> session. Destructors run in
    /// reverse, so the service and repositories never outlive the connection they
    /// reference.
    class NotebookStore final: public nandina::app::NanStore {
    public:
        NotebookStore(nandina::reactive::Graph& graph, data::Database database);

        [[nodiscard]] auto database() noexcept -> data::Database&;
        [[nodiscard]] auto users() noexcept -> data::UserRepository&;
        [[nodiscard]] auto note_groups() noexcept -> data::NoteGroupRepository&;
        [[nodiscard]] auto notes() noexcept -> data::NoteRepository&;
        [[nodiscard]] auto user_service() noexcept -> core::UserService&;

        /// The account signed in for this process. `nullopt` means no active session.
        [[nodiscard]] auto current_user() noexcept
            -> nandina::reactive::Signal<std::optional<models::User>>&;
        [[nodiscard]] auto current_user() const noexcept
            -> const nandina::reactive::Signal<std::optional<models::User>>&;

        void sign_in(models::User user);
        void sign_out();

        // ---- Signed-in workspace state ----

        /// Groups of the signed-in user, in repository order.
        [[nodiscard]] auto group_list() noexcept
            -> nandina::reactive::Signal<std::vector<models::NoteGroup>>&;
        /// Notes of the current group filter; empty when signed out.
        [[nodiscard]] auto note_list() noexcept
            -> nandina::reactive::Signal<std::vector<models::Note>>&;
        /// Active group filter; `nullopt` means "all notes".
        [[nodiscard]] auto selected_group() noexcept
            -> nandina::reactive::Signal<std::optional<std::int64_t>>&;
        /// Note currently open in the editor; it must exist in `note_list()`.
        [[nodiscard]] auto selected_note() noexcept
            -> nandina::reactive::Signal<std::optional<std::int64_t>>&;

        /// Load groups and notes for the signed-in user. Failure leaves the previous
        /// signals untouched so the page can keep showing the last good state.
        [[nodiscard]] auto load_workspace() -> data::DataResult<void>;
        /// Change the group filter (`nullopt` = all notes) and reload the note list.
        [[nodiscard]] auto select_group(std::optional<std::int64_t> group_id)
            -> data::DataResult<void>;
        /// Mark a note as open in the editor. The id is not validated here; the page
        /// resolves it from `note_list()`.
        void select_note(std::optional<std::int64_t> note_id);

        /// Create a note in the active group filter, reload the list and select it.
        [[nodiscard]] auto create_note(std::string_view title, std::string_view content)
            -> data::DataResult<models::Note>;
        /// Persist title/content of an existing note and reload the list.
        [[nodiscard]] auto
        save_note(std::int64_t note_id, std::string_view title, std::string_view content)
            -> data::DataResult<void>;
        /// Delete a note and clear the selection when it was open.
        [[nodiscard]] auto delete_note(std::int64_t note_id) -> data::DataResult<void>;
        /// Create a group for the signed-in user and reload the workspace lists.
        [[nodiscard]] auto create_group(std::string_view name)
            -> data::DataResult<models::NoteGroup>;

        /// The selected note resolved from the current list, or nullopt.
        [[nodiscard]] auto current_note() const -> std::optional<models::Note>;

    private:
        /// Reload `note_list()` for the current user/group filter without touching
        /// group data. Clears the note selection when that note disappeared.
        [[nodiscard]] auto reload_notes() -> data::DataResult<void>;
        /// Reset every workspace signal (sign out, or a failed workspace load).
        void clear_workspace();

        data::Database database_;
        data::UserRepository user_repository_;
        data::NoteGroupRepository note_group_repository_;
        data::NoteRepository note_repository_;
        core::UserService user_service_;
        nandina::reactive::Signal<std::optional<models::User>> current_user_;
        nandina::reactive::Signal<std::vector<models::NoteGroup>> group_list_;
        nandina::reactive::Signal<std::vector<models::Note>> note_list_;
        nandina::reactive::Signal<std::optional<std::int64_t>> selected_group_;
        nandina::reactive::Signal<std::optional<std::int64_t>> selected_note_;
    };
} // namespace nan_example::notebook::app

#endif // NANDINA_EXAMPLES_NOTEBOOK_APP_NOTEBOOK_STORE_HPP
