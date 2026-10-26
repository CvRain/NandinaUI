#ifndef NANDINAUI_LAB_STORE_HPP
#define NANDINAUI_LAB_STORE_HPP

#include <nandina/app/nan_store.hpp>
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/signal.hpp>

#include <string>
#include <vector>

namespace nandina::lab
{
    struct Note {
        int id = 0;
        std::string title;
        std::string body;

        friend auto operator==(const Note&, const Note&) -> bool = default;
    };

    /// Shared state for the example routes. The store is owned by NanApplication and
    /// deliberately non-persistent: this example validates the application/resource
    /// seams, not database storage.
    class LabStore final: public app::NanStore {
    public:
        explicit LabStore(reactive::Graph& graph);

        reactive::Signal<bool> signed_in;
        reactive::Signal<std::string> display_name;
        reactive::Signal<std::vector<Note>> notes;
        reactive::Signal<int> selected_note;
        reactive::Signal<std::string> notice;

        [[nodiscard]] auto next_note_id() -> int;
        [[nodiscard]] auto selected() const -> const Note*;

    private:
        int next_note_id_ = 1;
    };
} // namespace nandina::lab

#endif // NANDINAUI_LAB_STORE_HPP
