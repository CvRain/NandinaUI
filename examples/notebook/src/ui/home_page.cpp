#include "home_page.hpp"

#include "app/notebook_store.hpp"
#include "data/data_error.hpp"
#include "ui/auth/user_login_page.hpp"

#include <nandina/widget/authoring.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/list_view.hpp>
#include <nandina/widget/text_area.hpp>
#include <nandina/widget/text_field.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nan_example::notebook::ui
{
    namespace
    {
        using app::NotebookStore;
        using models::Note;
        using models::NoteGroup;

        /// 行模型：把 store 的 vector 信号快照进 IRowModel，RowId = 数据库主键。
        template<typename Item>
        class VectorRowModel final:
            public nandina::widget::TypedRowModelBase<VectorRowModel<Item>> {
        public:
            using IdFunction = std::function<std::int64_t(const Item&)>;

            explicit VectorRowModel(IdFunction id): id_(std::move(id)) {}

            void set_items(std::vector<Item> next) {
                items_ = std::move(next);
                this->notify_reset();
            }

            [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
                return items_.size();
            }
            [[nodiscard]] auto row_id(const std::size_t row) const
                -> nandina::widget::RowId override {
                return nandina::widget::RowId {
                    .value = static_cast<std::uint64_t>(id_(items_.at(row))),
                };
            }
            [[nodiscard]] auto at(const std::size_t row) const -> const Item& {
                return items_.at(row);
            }

        private:
            IdFunction id_;
            std::vector<Item> items_;
        };

        /// ListView 行 delegate 的 lambda 适配器：create/update 保持页面内的组合逻辑。
        template<typename ModelT>
        class LambdaRowDelegate final: public nandina::widget::TypedItemDelegate<ModelT> {
        public:
            using Create = std::function<std::shared_ptr<nandina::scene::NanControl>(
                const nandina::widget::BuildContext&,
                const ModelT&,
                const nandina::widget::ItemBinding&
            )>;
            using Update = std::function<void(
                nandina::scene::NanControl&,
                const ModelT&,
                const nandina::widget::ItemBinding&
            )>;

            LambdaRowDelegate(Create create, Update update):
                create_(std::move(create)),
                update_(std::move(update)) {}

        protected:
            auto create_typed(
                const nandina::widget::BuildContext& ui,
                const ModelT& model,
                const nandina::widget::ItemBinding& binding
            ) -> std::shared_ptr<nandina::scene::NanControl> override {
                return create_(ui, model, binding);
            }
            void update_typed(
                nandina::scene::NanControl& control,
                const ModelT& model,
                const nandina::widget::ItemBinding& binding
            ) override {
                update_(control, model, binding);
            }

        private:
            Create create_;
            Update update_;
        };

        [[nodiscard]] auto row_title(const Note& note) -> std::string {
            return note.title.empty() ? "(未命名笔记)" : note.title;
        }

        void set_feedback(
            nandina::widget::Label& label,
            const std::string_view message,
            const bool error
        ) {
            label.set_text(error ? "⚠ " + std::string {message} : std::string {message});
        }
    } // namespace

    auto HomePage::build(nandina::app::PageContext& context) -> nandina::widget::View {
        auto ui = context.ui();
        auto* store = &context.store<NotebookStore>();
        const auto navigation = context.navigation();
        const auto user = store->current_user().peek();

        if (!user.has_value()) {
            auto fallback = ui.make<nandina::widget::Label>("Sign in to open your notebook.")
                                .width(nandina::widget::authoring::fill)
                                .font_size(18)
                                .color_token(nandina::theme::ColorToken::muted_foreground)
                                .build();
            auto action =
                ui.make<nandina::widget::Button>("Go to sign in")
                    .treatment(nandina::theme::ButtonTreatment::filled)
                    .on_click([navigation] { (void)navigation.navigate<UserLoginPage>(); })
                    .build();
            return ui.center()
                .child(ui.make<nandina::widget::Card>().width(520).height(220).child(
                    ui.padding(nandina::foundation::NanInsets::all(28))
                        .width(nandina::widget::authoring::fill)
                        .child(ui.column()
                                   .width(nandina::widget::authoring::fill)
                                   .gap(16)
                                   .children(fallback)
                                   .children(action))
                ))
                .build();
        }

        // ---- Header ----
        auto feedback_label = ui.make<nandina::widget::Label>("")
                                  .width(nandina::widget::authoring::fill)
                                  .font_size(13)
                                  .color_token(nandina::theme::ColorToken::muted_foreground)
                                  .build();
        auto* feedback = feedback_label.get();

        auto load_result = store->load_workspace();
        if (!load_result) {
            set_feedback(*feedback, "加载笔记失败：" + data::describe(load_result.error()), true);
        }

        auto title_label = ui.make<nandina::widget::Label>("我的笔记")
                               .font_size(24)
                               .color_token(nandina::theme::ColorToken::card_foreground)
                               .build();
        auto user_label = ui.make<nandina::widget::Label>("已登录：" + user->username)
                              .font_size(14)
                              .color_token(nandina::theme::ColorToken::muted_foreground)
                              .build();
        auto logout_button = ui.make<nandina::widget::Button>("退出登录")
                                 .treatment(nandina::theme::ButtonTreatment::tonal)
                                 .on_click([store, navigation] {
                                     store->sign_out();
                                     (void)navigation.navigate<UserLoginPage>();
                                 })
                                 .build();

        // ---- Editor controls (built first so list rows can capture them) ----
        auto title_input = ui.make<nandina::widget::TextField>("", "笔记标题")
                               .width(nandina::widget::authoring::fill)
                               .height(40)
                               .build();
        auto body_area_builder =
            nandina::widget::authoring::from(std::make_shared<nandina::widget::TextArea>());
        auto body_area = body_area_builder.width(nandina::widget::authoring::fill)
                             .height(nandina::widget::authoring::fill)
                             .configure([](nandina::widget::TextArea& area) {
                                 area.set_placeholder("在这里写下你的想法…");
                                 area.set_rows(14);
                             })
                             .build();

        auto editor_hint =
            ui.make<nandina::widget::Label>("选择左侧笔记，或点击「新建笔记」开始编辑。")
                .width(nandina::widget::authoring::fill)
                .font_size(14)
                .color_token(nandina::theme::ColorToken::muted_foreground)
                .build();
        auto* editor_hint_ptr = editor_hint.get();

        const auto clear_editor = [title_input, body_area, editor_hint_ptr] {
            title_input->set_value({});
            body_area->set_value({});
            editor_hint_ptr->set_text("选择左侧笔记，或点击「新建笔记」开始编辑。");
        };
        const auto fill_editor = [title_input, body_area, editor_hint_ptr](const Note& note) {
            title_input->set_value(note.title);
            body_area->set_value(note.content);
            editor_hint_ptr->set_text(
                "正在编辑：" + (note.title.empty() ? std::string {"(未命名笔记)"} : note.title)
            );
        };

        auto save_button =
            ui.make<nandina::widget::Button>("保存")
                .treatment(nandina::theme::ButtonTreatment::filled)
                .on_click([store, title_input, body_area, feedback] {
                    const auto note_id = store->selected_note().peek();
                    if (!note_id.has_value()) {
                        set_feedback(*feedback, "先选择或新建一篇笔记。", true);
                        return;
                    }
                    auto saved =
                        store->save_note(*note_id, title_input->value(), body_area->value());
                    if (!saved) {
                        set_feedback(*feedback, "保存失败：" + data::describe(saved.error()), true);
                        return;
                    }
                    set_feedback(*feedback, "已保存。", false);
                })
                .build();

        auto delete_button = ui.make<nandina::widget::Button>("删除")
                                 .treatment(nandina::theme::ButtonTreatment::outlined)
                                 .on_click([store, feedback, clear_editor] {
                                     const auto note_id = store->selected_note().peek();
                                     if (!note_id.has_value()) {
                                         set_feedback(*feedback, "先选择一篇笔记。", true);
                                         return;
                                     }
                                     auto removed = store->delete_note(*note_id);
                                     if (!removed) {
                                         set_feedback(
                                             *feedback,
                                             "删除失败：" + data::describe(removed.error()),
                                             true
                                         );
                                         return;
                                     }
                                     clear_editor();
                                     set_feedback(*feedback, "笔记已删除。", false);
                                 })
                                 .build();

        auto new_button = ui.make<nandina::widget::Button>("新建笔记")
                              .treatment(nandina::theme::ButtonTreatment::filled)
                              .on_click([store, feedback, fill_editor] {
                                  auto created = store->create_note("", "");
                                  if (!created) {
                                      set_feedback(
                                          *feedback,
                                          "新建失败：" + data::describe(created.error()),
                                          true
                                      );
                                      return;
                                  }
                                  fill_editor(*created);
                                  set_feedback(*feedback, "已新建一篇笔记。", false);
                              })
                              .build();

        // ---- Note list ----
        auto note_model =
            std::make_shared<VectorRowModel<Note>>([](const Note& note) { return note.id; });
        auto notes_list =
            nandina::widget::ListView::create(ui, nandina::widget::SelectionMode::single, 38.0F);
        notes_list->set_delegate(
            std::make_shared<LambdaRowDelegate<VectorRowModel<Note>>>(
                [store, fill_editor, feedback](
                    const nandina::widget::BuildContext& item_ui,
                    const VectorRowModel<Note>& model,
                    const nandina::widget::ItemBinding& binding
                ) -> std::shared_ptr<nandina::scene::NanControl> {
                    const auto note_id = model.at(binding.model_row).id;
                    return item_ui
                        .make<nandina::widget::Button>(row_title(model.at(binding.model_row)))
                        .treatment(nandina::theme::ButtonTreatment::ghost)
                        .label_align(nandina::theme::TextAlign::start)
                        .width(nandina::widget::authoring::fill)
                        .height(38)
                        .on_click([store, fill_editor, feedback, note_id] {
                            store->select_note(note_id);
                            if (const auto current = store->current_note(); current.has_value()) {
                                fill_editor(*current);
                                set_feedback(*feedback, "正在编辑：" + row_title(*current), false);
                            }
                        })
                        .build();
                },
                [](nandina::scene::NanControl& control,
                   const VectorRowModel<Note>& model,
                   const nandina::widget::ItemBinding& binding) {
                    auto& button = static_cast<nandina::widget::Button&>(control);
                    button.set_text(row_title(model.at(binding.model_row)));
                    button.set_treatment(
                        binding.state.selected ? nandina::theme::ButtonTreatment::tonal
                                               : nandina::theme::ButtonTreatment::ghost
                    );
                }
            )
        );
        notes_list->set_model(note_model);
        (void)ui.effect([note_model, store] { note_model->set_items(store->note_list().get()); });
        (void)ui.effect([notes_list, store] {
            if (const auto selected = store->selected_note().get(); selected.has_value()) {
                (void)notes_list->select(
                    nandina::widget::RowId {
                        .value = static_cast<std::uint64_t>(*selected),
                    }
                );
            }
            else {
                notes_list->clear_selection();
            }
        });
        auto notes_list_node = nandina::widget::authoring::from(notes_list)
                                   .width(nandina::widget::authoring::fill)
                                   .height(nandina::widget::authoring::fill)
                                   .build();

        auto notes_hint = ui.make<nandina::widget::Label>("")
                              .width(nandina::widget::authoring::fill)
                              .font_size(14)
                              .color_token(nandina::theme::ColorToken::muted_foreground)
                              .build();
        auto* notes_hint_ptr = notes_hint.get();
        const auto refresh_notes_hint = [store, notes_hint_ptr] {
            notes_hint_ptr->set_text(
                store->note_list().peek().empty() ? "当前分组还没有笔记。" : ""
            );
        };
        refresh_notes_hint();

        // ---- Group list and creation ----
        auto group_name_input = ui.make<nandina::widget::TextField>("", "新分组名称")
                                    .width(nandina::widget::authoring::fill)
                                    .height(32)
                                    .build();
        auto create_group_button =
            ui.make<nandina::widget::Button>("新建")
                .treatment(nandina::theme::ButtonTreatment::tonal)
                .on_click([store, group_name_input, feedback, refresh_notes_hint, clear_editor] {
                    const auto name = std::string {group_name_input->value()};
                    if (name.empty()) {
                        set_feedback(*feedback, "分组名称不能为空。", true);
                        return;
                    }
                    auto created = store->create_group(name);
                    if (!created) {
                        set_feedback(
                            *feedback,
                            "新建分组失败：" + data::describe(created.error()),
                            true
                        );
                        return;
                    }
                    group_name_input->set_value({});
                    clear_editor();
                    refresh_notes_hint();
                    set_feedback(*feedback, "分组「" + created->name + "」已创建。", false);
                })
                .build();

        auto group_model = std::make_shared<VectorRowModel<NoteGroup>>([](const NoteGroup& group) {
            return group.id;
        });
        auto groups_list =
            nandina::widget::ListView::create(ui, nandina::widget::SelectionMode::single, 32.0F);
        groups_list->set_delegate(
            std::make_shared<LambdaRowDelegate<VectorRowModel<NoteGroup>>>(
                [store, feedback, refresh_notes_hint, clear_editor](
                    const nandina::widget::BuildContext& item_ui,
                    const VectorRowModel<NoteGroup>& model,
                    const nandina::widget::ItemBinding& binding
                ) -> std::shared_ptr<nandina::scene::NanControl> {
                    const auto group_id = model.at(binding.model_row).id;
                    return item_ui.make<nandina::widget::Button>(model.at(binding.model_row).name)
                        .treatment(nandina::theme::ButtonTreatment::ghost)
                        .label_align(nandina::theme::TextAlign::start)
                        .width(nandina::widget::authoring::fill)
                        .height(32)
                        .on_click([store, feedback, refresh_notes_hint, clear_editor, group_id] {
                            auto selected = store->select_group(group_id);
                            if (!selected) {
                                set_feedback(
                                    *feedback,
                                    "切换分组失败：" + data::describe(selected.error()),
                                    true
                                );
                                return;
                            }
                            clear_editor();
                            refresh_notes_hint();
                        })
                        .build();
                },
                [](nandina::scene::NanControl& control,
                   const VectorRowModel<NoteGroup>& model,
                   const nandina::widget::ItemBinding& binding) {
                    auto& button = static_cast<nandina::widget::Button&>(control);
                    button.set_text(model.at(binding.model_row).name);
                    button.set_treatment(
                        binding.state.selected ? nandina::theme::ButtonTreatment::tonal
                                               : nandina::theme::ButtonTreatment::ghost
                    );
                }
            )
        );
        groups_list->set_model(group_model);
        (void)ui.effect([group_model, store] {
            group_model->set_items(store->group_list().get());
        });
        (void)ui.effect([groups_list, store] {
            if (const auto selected = store->selected_group().get(); selected.has_value()) {
                (void)groups_list->select(
                    nandina::widget::RowId {
                        .value = static_cast<std::uint64_t>(*selected),
                    }
                );
            }
            else {
                groups_list->clear_selection();
            }
        });
        auto groups_list_node = nandina::widget::authoring::from(groups_list)
                                    .width(nandina::widget::authoring::fill)
                                    .height(nandina::widget::authoring::fill)
                                    .build();

        auto all_notes_button = ui.make<nandina::widget::Button>("全部笔记")
                                    .treatment(nandina::theme::ButtonTreatment::tonal)
                                    .label_align(nandina::theme::TextAlign::start)
                                    .width(nandina::widget::authoring::fill)
                                    .height(32)
                                    .on_click([store, feedback, refresh_notes_hint, clear_editor] {
                                        auto selected = store->select_group(std::nullopt);
                                        if (!selected) {
                                            set_feedback(
                                                *feedback,
                                                "切换分组失败：" + data::describe(selected.error()),
                                                true
                                            );
                                            return;
                                        }
                                        clear_editor();
                                        refresh_notes_hint();
                                    })
                                    .build();

        // ---- Layout ----
        auto header = ui.row()
                          .width(nandina::widget::authoring::fill)
                          .height(40)
                          .gap(12)
                          .children(title_label)
                          .children(user_label)
                          .children(ui.expanded())
                          .children(logout_button)
                          .build();

        auto sidebar = ui.column()
                           .width(200)
                           .height(nandina::widget::authoring::fill)
                           .gap(8)
                           .children(ui.make<nandina::widget::Label>("分组")
                                         .font_size(15)
                                         .color_token(nandina::theme::ColorToken::card_foreground)
                                         .build())
                           .children(all_notes_button)
                           .children(groups_list_node)
                           .children(ui.row()
                                         .width(nandina::widget::authoring::fill)
                                         .gap(6)
                                         .children(group_name_input)
                                         .children(create_group_button))
                           .build();

        auto middle =
            ui.column()
                .width(280)
                .height(nandina::widget::authoring::fill)
                .gap(8)
                .children(
                    ui.row()
                        .width(nandina::widget::authoring::fill)
                        .gap(8)
                        .children(ui.make<nandina::widget::Label>("笔记")
                                      .font_size(15)
                                      .color_token(nandina::theme::ColorToken::card_foreground)
                                      .build())
                        .children(ui.expanded())
                        .children(new_button)
                )
                .children(notes_hint)
                .children(notes_list_node)
                .build();

        auto editor = ui.column()
                          .width(nandina::widget::authoring::fill)
                          .height(nandina::widget::authoring::fill)
                          .gap(10)
                          .children(editor_hint)
                          .children(title_input)
                          .children(body_area)
                          .children(ui.row()
                                        .width(nandina::widget::authoring::fill)
                                        .gap(8)
                                        .children(save_button)
                                        .children(delete_button)
                                        .children(ui.expanded())
                                        .children(feedback_label))
                          .build();

        auto body = ui.row()
                        .width(nandina::widget::authoring::fill)
                        .height(nandina::widget::authoring::fill)
                        .gap(16)
                        .children(sidebar)
                        .children(middle)
                        .children(editor)
                        .build();

        auto root = ui.column()
                        .width(nandina::widget::authoring::fill)
                        .height(nandina::widget::authoring::fill)
                        .gap(12)
                        .children(header)
                        .children(body)
                        .build();

        if (const auto current = store->current_note(); current.has_value()) {
            fill_editor(*current);
        }
        return root;
    }
} // namespace nan_example::notebook::ui
