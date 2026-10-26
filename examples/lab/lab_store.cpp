#include "lab_store.hpp"

namespace nandina::lab
{
    LabStore::LabStore(reactive::Graph& graph):
        signed_in(graph, false),
        display_name(graph, std::string {}),
        notes(
            graph,
            std::vector<Note> {
                {.id = 1,
                 .title = "Welcome to Nandina Lab",
                 .body = "This example is deliberately small, but it exercises the seams that "
                         "matter in a desktop application: routing, form validation, reactive "
                         "state, a timer driven by the frame loop, a keyed list, a long scrolling "
                         "document, an image loaded from the resource package, and a modal "
                         "confirmation.\n\n"
                         "Try editing this note. The body is an ordinary TextArea, so wrapping, "
                         "selection, caret movement and scrolling all go through the framework's "
                         "text pipeline rather than a page-local imitation."},
                {.id = 2,
                 .title = "Long content check",
                 .body =
                     "A note that is long enough to force vertical scrolling. Text layout is "
                     "hard to judge from short strings: line breaking depends on the available "
                     "width, the font metrics and the chosen overflow policy. Resize the "
                     "window or change the theme while this note is open.\n\n"
                     "Paragraph two adds more lines so the editor and the page both have "
                     "something to scroll. The example intentionally avoids a second text "
                     "engine or a custom caret; if something looks wrong here, it is a "
                     "framework issue worth reproducing in a smaller test.\n\n"
                     "Paragraph three: The quick brown fox jumps over the lazy dog. 0123456789. "
                     "The five boxing wizards jump quickly. Sphinx of black quartz, judge my "
                     "vow. Pack my box with five dozen liquor jugs."},
                {.id = 3,
                 .title = "Focus session notes",
                 .body =
                     "Use the Focus route for a 25/5 focus/break sequence. The timer is driven by "
                     "NanControl::on_process, not by a wall-clock thread. That makes pause, "
                     "resume and reset deterministic with respect to the application's frame "
                     "clock, and keeps every signal write on the UI thread.\n\n"
                     "The timer continues while you browse other routes? No: the page is "
                     "destroyed on navigation in this first version. That is a deliberate "
                     "boundary, not a persistence claim."},
                {.id = 4,
                 .title = "Image preview",
                 .body = "Open the Preview route from the toolbar. The image source is generated "
                         "from resources.toml into a C++ constant header, then resolved through "
                         "the page's ResourceManager as res://images/lab-cover.png."},
            }
        ),
        selected_note(graph, 1),
        notice(graph, std::string {}) {}

    auto LabStore::next_note_id() -> int {
        // Seed notes already occupy ids 1..4, and callers may also set the notes
        // signal directly. Derive the next id from the live model so a keyed list
        // never receives a duplicate key.
        for (const auto& note: notes.peek()) {
            if (note.id >= next_note_id_) {
                next_note_id_ = note.id + 1;
            }
        }
        return next_note_id_++;
    }

    auto LabStore::selected() const -> const Note* {
        const auto& all = notes.peek();
        for (const auto& note: all) {
            if (note.id == selected_note.peek()) {
                return &note;
            }
        }
        return nullptr;
    }
} // namespace nandina::lab
