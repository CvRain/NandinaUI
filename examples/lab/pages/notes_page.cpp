#include "notes_page.hpp"

#include "preview_page.hpp"

#include "lab_style.hpp"

#include "../lab_store.hpp"

#include <nandina/widget/controls.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nandina::lab
{
    namespace
    {
        [[nodiscard]] auto first_note(const LabStore& store) -> const Note* {
            const auto& all = store.notes.peek();
            return all.empty() ? nullptr : &all.front();
        }

        [[nodiscard]] auto labeled(
            const widget::BuildContext& ui,
            const std::string_view text,
            std::shared_ptr<scene::NanControl> field
        ) -> widget::authoring::NodeBuilder<widget::Column> {
            return ui.column()
                .width(widget::authoring::fill)
                .gap(6.0F)
                .children(caption(ui, text))
                .children(std::move(field));
        }
    } // namespace

    auto NotesPage::build(app::PageContext& context) -> widget::View {
        auto ui = context.ui();
        auto* store = &context.store<LabStore>();
        const auto navigation = context.navigation();

        const auto* initial = store->selected();
        if (initial == nullptr) {
            initial = first_note(*store);
        }
        if (initial != nullptr && store->selected_note.peek() != initial->id) {
            // Keep the store selection in sync with the fallback, so Save/Delete act on
            // the note the editor actually shows.
            store->selected_note.set(initial->id);
        }
        auto& draft_title = ui.signal_value(initial != nullptr ? initial->title : std::string {});
        auto& draft_body = ui.signal_value(initial != nullptr ? initial->body : std::string {});
        auto& feedback = ui.signal_value(std::string {"Editing the selected note."});

        const auto select_note = std::make_shared<std::function<void(int)>>(
            [store, &draft_title, &draft_body](const int id) {
                store->selected_note.set(id);
                for (const auto& note: store->notes.peek()) {
                    if (note.id == id) {
                        draft_title.set(note.title);
                        draft_body.set(note.body);
                        return;
                    }
                }
                draft_title.set({});
                draft_body.set({});
            }
        );

        const auto save_note = [store, &draft_title, &draft_body, &feedback] {
            const int id = store->selected_note.peek();
            bool saved = false;
            store->notes.update([id, &draft_title, &draft_body, &saved](std::vector<Note>& all) {
                for (auto& note: all) {
                    if (note.id == id) {
                        note.title = draft_title.peek();
                        note.body = draft_body.peek();
                        saved = true;
                        return;
                    }
                }
            });
            feedback.set(saved ? "Saved to the in-memory store." : "No note is selected.");
        };

        const auto add_note = [store, &feedback, select_note] {
            const int id = store->next_note_id();
            store->notes.update([id](std::vector<Note>& all) {
                all.push_back({
                    .id = id,
                    .title = "Untitled note",
                    .body = "Start writing here.",
                });
            });
            (*select_note)(id);
            feedback.set("Created a new note.");
        };

        const auto remove_selected = [store, &draft_title, &draft_body, &feedback, select_note] {
            const int id = store->selected_note.peek();
            store->notes.update([id](std::vector<Note>& all) {
                std::erase_if(all, [id](const Note& note) { return note.id == id; });
            });
            if (store->notes.peek().empty()) {
                store->selected_note.set(0);
                draft_title.set({});
                draft_body.set({});
                feedback.set("No notes left. Add one to continue.");
            }
            else {
                (*select_note)(store->notes.peek().front().id);
                feedback.set("Deleted the selected note.");
            }
        };

        const auto alert = ui.make<widget::AlertDialog>(
                                 "Delete this note?",
                                 "The note is removed from the in-memory store immediately."
        )
                               .configure([&, remove_selected](widget::AlertDialog& dialog) {
                                   dialog.set_confirm_text("Delete");
                                   dialog.set_confirm_tone(theme::ButtonTone::danger);
                                   dialog.set_on_confirm([remove_selected] { remove_selected(); });
                               })
                               .build();

        const auto note_list = ui.for_each(
            store->notes,
            &Note::id,
            [store, select_note](widget::BuildContext item, const Note& note) {
                auto button = item.make<widget::Button>(note.title)
                                  .width(widget::authoring::fill)
                                  .height(40.0F)
                                  .treatment(theme::ButtonTreatment::outlined)
                                  .label_align(theme::TextAlign::start)
                                  .on_click([select_note, id = note.id] { (*select_note)(id); });
                const auto built = button.build();
                auto& active = item.computed([store, id = note.id] {
                    return store->selected_note.get() == id;
                });
                item.bind(
                    built,
                    [](widget::Button& target, const bool selected) {
                        target.set_treatment(
                            selected ? theme::ButtonTreatment::filled
                                     : theme::ButtonTreatment::outlined
                        );
                    },
                    active
                );
                return built;
            },
            [](widget::Button& button, const Note& note) { button.set_text(note.title); }
        );

        const auto list_panel =
            ui.column()
                .width(280.0F)
                .height(widget::authoring::fill)
                .gap(8.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(section_title(ui, "All notes"))
                .children(ui.expanded().child(
                    ui.scroll_view().width(widget::authoring::fill).child(note_list)
                ))
                .children(ui.make<widget::Button>("New note")
                              .treatment(theme::ButtonTreatment::tonal)
                              .on_click([add_note] { add_note(); }));

        // Session/notice line: the account pages write this signal, the notes page is
        // where the result becomes visible after navigation.
        const auto notice = ui.make<widget::Label>(std::string {})
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(12.0F)
                                .width(widget::authoring::fill)
                                .build();
        ui.bind(notice, &widget::Label::set_text, store->notice);

        auto& session = ui.computed([store] {
            return store->signed_in.get()
                ? std::string {"Signed in as "} + store->display_name.get()
                : std::string {"Local demo session"};
        });
        const auto session_label = ui.make<widget::Label>(std::string {})
                                       .color_token(theme::ColorToken::muted_foreground)
                                       .font_size(12.0F)
                                       .width(widget::authoring::fill)
                                       .build();
        ui.bind(session_label, &widget::Label::set_text, session);

        auto& has_notes = ui.computed([store] { return !store->notes.get().empty(); });
        const auto save_button = ui.make<widget::Button>("Save")
                                     .treatment(theme::ButtonTreatment::filled)
                                     .on_click([save_note] { save_note(); })
                                     .build();
        ui.bind(
            save_button,
            [](widget::Button& button, const bool enabled) { button.set_disabled(!enabled); },
            has_notes
        );
        const auto delete_button = ui.make<widget::Button>("Delete")
                                       .treatment(theme::ButtonTreatment::outlined)
                                       .tone(theme::ButtonTone::danger)
                                       .on_click([alert] { alert->open(); })
                                       .build();
        ui.bind(
            delete_button,
            [](widget::Button& button, const bool enabled) { button.set_disabled(!enabled); },
            has_notes
        );

        const auto title_field = ui.make<widget::TextField>(draft_title, "Note title")
                                     .width(widget::authoring::fill)
                                     .build();
        const auto body_field = ui.make<widget::TextArea>(draft_body, "Write the note body")
                                    .width(widget::authoring::fill)
                                    .configure([](widget::TextArea& area) { area.set_rows(14); })
                                    .build();

        const auto editor =
            ui.column()
                .width(widget::authoring::fill)
                .gap(10.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(section_title(ui, "Editor"))
                .children(labeled(ui, "Title", title_field))
                .children(labeled(ui, "Body", body_field))
                .children(ui.make<widget::Label>(feedback)
                              .color_token(theme::ColorToken::muted_foreground)
                              .font_size(12.0F)
                              .width(widget::authoring::fill))
                .children(ui.row()
                              .gap(8.0F)
                              .cross_alignment(widget::LayoutAlignment::center)
                              .children(save_button)
                              .children(ui.make<widget::Button>("Preview image")
                                            .treatment(theme::ButtonTreatment::outlined)
                                            .on_click([navigation] {
                                                (void)navigation.navigate<PreviewPage>();
                                            }))
                              .children(delete_button));

        auto body = page_column(ui)
                        .children(page_title(ui, "Notes"))
                        .children(body_text(
                            ui,
                            "A keyed list, a reactive editor and a modal confirmation. The list "
                            "keeps node identity when notes are added or removed; the editor "
                            "writes only when Save is pressed.",
                            4
                        ))
                        .children(session_label)
                        .children(notice)
                        .children(ui.row()
                                      .width(widget::authoring::fill)
                                      .height(600.0F)
                                      .gap(16.0F)
                                      .cross_alignment(widget::LayoutAlignment::stretch)
                                      .children(list_panel)
                                      .children(ui.expanded().child(editor)))
                        .children(alert);

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(PAGE_PADDING)).child(body))
            .build();
    }
} // namespace nandina::lab
