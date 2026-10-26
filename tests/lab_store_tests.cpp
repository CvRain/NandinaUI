#include <catch2/catch_test_macros.hpp>

#include "../examples/lab/lab_store.hpp"

#include <nandina/reactive/graph.hpp>

#include <set>
#include <vector>

TEST_CASE("lab store allocates a unique note id", "[examples][lab]") {
    nandina::reactive::Graph graph;
    nandina::lab::LabStore store {graph};

    std::set<int> ids;
    for (const auto& note: store.notes.peek()) {
        REQUIRE(ids.insert(note.id).second);
    }

    const int next = store.next_note_id();
    REQUIRE_FALSE(ids.contains(next));
    store.notes.update([next](std::vector<nandina::lab::Note>& notes) {
        notes.push_back({
            .id = next,
            .title = "new note",
            .body = {},
        });
    });
    REQUIRE(store.notes.peek().size() == ids.size() + 1);

    store.notes.update([](std::vector<nandina::lab::Note>& notes) {
        notes.push_back({
            .id = 50,
            .title = "external id",
            .body = {},
        });
    });
    REQUIRE(store.next_note_id() == 51);
}

TEST_CASE("lab store selection follows the live note model", "[examples][lab]") {
    nandina::reactive::Graph graph;
    nandina::lab::LabStore store {graph};

    REQUIRE(store.selected_note.peek() == 1);
    const auto* selected = store.selected();
    REQUIRE(selected != nullptr);
    REQUIRE(selected->id == 1);
    REQUIRE(selected->title == "Welcome to Nandina Lab");

    store.selected_note.set(999);
    REQUIRE(store.selected() == nullptr);

    store.notes.update([](std::vector<nandina::lab::Note>& notes) { notes.clear(); });
    store.selected_note.set(1);
    REQUIRE(store.selected() == nullptr);
}

TEST_CASE("lab store never reuses a note id after deletion", "[examples][lab]") {
    nandina::reactive::Graph graph;
    nandina::lab::LabStore store {graph};

    const int next = store.next_note_id();
    store.notes.update([next](std::vector<nandina::lab::Note>& notes) {
        notes.push_back({.id = next, .title = "temporary", .body = {}});
    });
    store.notes.update([next](std::vector<nandina::lab::Note>& notes) {
        std::erase_if(notes, [next](const nandina::lab::Note& note) { return note.id == next; });
    });
    REQUIRE(store.next_note_id() > next);
}
