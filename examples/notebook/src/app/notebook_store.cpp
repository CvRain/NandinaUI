#include "notebook_store.hpp"

#include <algorithm>
#include <utility>

namespace nan_example::notebook::app
{
    namespace
    {
        [[nodiscard]] auto no_session_error(const std::string_view operation) -> data::DataError {
            return data::DataError {
                .code = data::DataErrorCode::invalid_argument,
                .operation = std::string {operation},
                .message = "no active user session",
            };
        }

        [[nodiscard]] auto not_found_error(const std::string_view operation) -> data::DataError {
            return data::DataError {
                .code = data::DataErrorCode::invalid_argument,
                .operation = std::string {operation},
                .message = "row does not exist or is not owned by the current user",
            };
        }
    } // namespace

    NotebookStore::NotebookStore(nandina::reactive::Graph& graph, data::Database database):
        database_(std::move(database)),
        user_repository_(database_),
        note_group_repository_(database_),
        note_repository_(database_),
        user_service_(user_repository_),
        current_user_(graph, std::nullopt),
        group_list_(graph, {}),
        note_list_(graph, {}),
        selected_group_(graph, std::nullopt),
        selected_note_(graph, std::nullopt) {}

    auto NotebookStore::database() noexcept -> data::Database& {
        return database_;
    }

    auto NotebookStore::users() noexcept -> data::UserRepository& {
        return user_repository_;
    }

    auto NotebookStore::note_groups() noexcept -> data::NoteGroupRepository& {
        return note_group_repository_;
    }

    auto NotebookStore::notes() noexcept -> data::NoteRepository& {
        return note_repository_;
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
        clear_workspace();
        current_user_.set(std::optional<models::User> {std::move(user)});
    }

    void NotebookStore::sign_out() {
        clear_workspace();
        current_user_.set(std::nullopt);
    }

    auto NotebookStore::group_list() noexcept
        -> nandina::reactive::Signal<std::vector<models::NoteGroup>>& {
        return group_list_;
    }

    auto NotebookStore::note_list() noexcept
        -> nandina::reactive::Signal<std::vector<models::Note>>& {
        return note_list_;
    }

    auto NotebookStore::selected_group() noexcept
        -> nandina::reactive::Signal<std::optional<std::int64_t>>& {
        return selected_group_;
    }

    auto NotebookStore::selected_note() noexcept
        -> nandina::reactive::Signal<std::optional<std::int64_t>>& {
        return selected_note_;
    }

    auto NotebookStore::load_workspace() -> data::DataResult<void> {
        const auto user = current_user_.peek();
        if (!user.has_value()) {
            clear_workspace();
            return std::unexpected(no_session_error("load_workspace"));
        }

        auto groups = note_group_repository_.list_for_user(user->id);
        if (!groups) {
            return std::unexpected(groups.error());
        }
        group_list_.set(std::move(*groups));

        // A filter whose group disappeared (for example deleted elsewhere) falls back to
        // "all notes" instead of silently keeping an impossible id.
        if (const auto filter = selected_group_.peek(); filter.has_value()
            && std::ranges::none_of(group_list_.peek(),
                                    [filter](const models::NoteGroup& group) {
                                        return group.id == *filter;
                                    }))
        {
            selected_group_.set(std::nullopt);
        }
        return reload_notes();
    }

    auto NotebookStore::select_group(const std::optional<std::int64_t> group_id)
        -> data::DataResult<void> {
        selected_group_.set(group_id);
        // reload_notes() clears the note selection only when it is no longer visible.
        return reload_notes();
    }

    void NotebookStore::select_note(const std::optional<std::int64_t> note_id) {
        selected_note_.set(note_id);
    }

    auto NotebookStore::create_note(const std::string_view title, const std::string_view content)
        -> data::DataResult<models::Note> {
        const auto user = current_user_.peek();
        if (!user.has_value()) {
            return std::unexpected(no_session_error("create_note"));
        }
        auto created = note_repository_.create(user->id, title, content, selected_group_.peek());
        if (!created) {
            return std::unexpected(created.error());
        }
        const auto note = *created;
        if (auto reloaded = reload_notes(); !reloaded) {
            return std::unexpected(reloaded.error());
        }
        selected_note_.set(note.id);
        return note;
    }

    auto NotebookStore::save_note(
        const std::int64_t note_id,
        const std::string_view title,
        const std::string_view content
    ) -> data::DataResult<void> {
        const auto user = current_user_.peek();
        if (!user.has_value()) {
            return std::unexpected(no_session_error("save_note"));
        }
        auto existing = note_repository_.find(note_id);
        if (!existing) {
            return std::unexpected(existing.error());
        }
        if (!existing->has_value() || (*existing)->user_id != user->id) {
            return std::unexpected(not_found_error("save_note"));
        }
        auto note = **existing;
        note.title = std::string {title};
        note.content = std::string {content};
        auto updated = note_repository_.update(note);
        if (!updated) {
            return std::unexpected(updated.error());
        }
        if (!*updated) {
            return std::unexpected(not_found_error("save_note"));
        }
        return reload_notes();
    }

    auto NotebookStore::delete_note(const std::int64_t note_id) -> data::DataResult<void> {
        const auto user = current_user_.peek();
        if (!user.has_value()) {
            return std::unexpected(no_session_error("delete_note"));
        }
        auto removed = note_repository_.remove(note_id, user->id);
        if (!removed) {
            return std::unexpected(removed.error());
        }
        if (!*removed) {
            return std::unexpected(not_found_error("delete_note"));
        }
        if (selected_note_.peek() == note_id) {
            selected_note_.set(std::nullopt);
        }
        return reload_notes();
    }

    auto NotebookStore::create_group(const std::string_view name)
        -> data::DataResult<models::NoteGroup> {
        const auto user = current_user_.peek();
        if (!user.has_value()) {
            return std::unexpected(no_session_error("create_group"));
        }
        auto created = note_group_repository_.create(user->id, name, 0);
        if (!created) {
            return std::unexpected(created.error());
        }
        const auto group = *created;
        if (auto reloaded = load_workspace(); !reloaded) {
            return std::unexpected(reloaded.error());
        }
        return group;
    }

    auto NotebookStore::current_note() const -> std::optional<models::Note> {
        const auto note_id = selected_note_.peek();
        if (!note_id.has_value()) {
            return std::nullopt;
        }
        const auto& notes = note_list_.peek();
        const auto found = std::ranges::find_if(notes, [note_id](const models::Note& note) {
            return note.id == *note_id;
        });
        if (found == notes.end()) {
            return std::nullopt;
        }
        return *found;
    }

    auto NotebookStore::reload_notes() -> data::DataResult<void> {
        const auto user = current_user_.peek();
        if (!user.has_value()) {
            clear_workspace();
            return std::unexpected(no_session_error("reload_notes"));
        }
        auto notes = note_repository_.list_for_user(user->id, selected_group_.peek());
        if (!notes) {
            return std::unexpected(notes.error());
        }
        note_list_.set(std::move(*notes));
        // Keep the selection only while the note remains visible in the active filter.
        if (const auto selected = selected_note_.peek(); selected.has_value()
            && std::ranges::none_of(note_list_.peek(),
                                    [selected](const models::Note& note) {
                                        return note.id == *selected;
                                    }))
        {
            selected_note_.set(std::nullopt);
        }
        return {};
    }

    void NotebookStore::clear_workspace() {
        group_list_.set({});
        note_list_.set({});
        selected_group_.set(std::nullopt);
        selected_note_.set(std::nullopt);
    }
} // namespace nan_example::notebook::app
