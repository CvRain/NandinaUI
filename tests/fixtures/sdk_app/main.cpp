#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/foundation/geometry.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/data_table.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/list_view.hpp>
#include <nandina/widget/tree_view.hpp>

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    class SdkRowModel final: public nandina::widget::TypedRowModelBase<SdkRowModel> {
    public:
        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return 2;
        }

        [[nodiscard]] auto row_id(const std::size_t row) const -> nandina::widget::RowId override {
            return nandina::widget::RowId {.value = row + 1};
        }
    };

    class SdkListDelegate final: public nandina::widget::TypedItemDelegate<SdkRowModel> {
    protected:
        auto create_typed(
            const nandina::widget::BuildContext& ui,
            const SdkRowModel&,
            const nandina::widget::ItemBinding& binding
        ) -> std::shared_ptr<nandina::scene::NanControl> override {
            return nandina::widget::Label::create(
                ui.graph(),
                "row-" + std::to_string(binding.model_row + 1),
                ui.theme()
            );
        }
        void update_typed(
            nandina::scene::NanControl&,
            const SdkRowModel&,
            const nandina::widget::ItemBinding&
        ) override {}
    };

    class SdkTreeModel final: public nandina::widget::TypedTreeModelBase<SdkTreeModel> {
    public:
        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return 2;
        }
        [[nodiscard]] auto row_id(const std::size_t row) const -> nandina::widget::RowId override {
            return nandina::widget::RowId {.value = row + 1};
        }
        [[nodiscard]] auto parent(const nandina::widget::RowId row) const
            -> std::optional<nandina::widget::RowId> override {
            if (row.value == 2) {
                return nandina::widget::RowId {.value = 1};
            }
            return std::nullopt;
        }
        [[nodiscard]] auto child_count(const nandina::widget::RowId row) const
            -> std::size_t override {
            return row.value == 1 ? 1 : 0;
        }
        [[nodiscard]] auto child(const nandina::widget::RowId, const std::size_t) const
            -> nandina::widget::RowId override {
            return nandina::widget::RowId {.value = 2};
        }
    };

    class SdkTreeDelegate final: public nandina::widget::TypedItemDelegate<SdkTreeModel> {
    protected:
        auto create_typed(
            const nandina::widget::BuildContext& ui,
            const SdkTreeModel&,
            const nandina::widget::ItemBinding& binding
        ) -> std::shared_ptr<nandina::scene::NanControl> override {
            return nandina::widget::Label::create(
                ui.graph(),
                "node-" + std::to_string(binding.row.value),
                ui.theme()
            );
        }
        void update_typed(
            nandina::scene::NanControl&,
            const SdkTreeModel&,
            const nandina::widget::ItemBinding&
        ) override {}
    };

    auto data_api_works(nandina::app::NanApplication& application) -> bool {
        nandina::reactive::ReactiveScope scope {application.graph()};
        nandina::widget::BuildContext ui {application.graph(), scope, application.theme_manager()};
        SdkRowModel model;
        auto column = nandina::widget::text_column<SdkRowModel>(
            nandina::widget::ColumnId {.value = "value"},
            "Value",
            [](const SdkRowModel&, const std::size_t row) {
                return std::string {"row-"} + std::to_string(row + 1);
            },
            nandina::widget::TableColumn {.flex = 1.0F}
        );
        nandina::widget::set_local_sort<SdkRowModel>(
            column,
            [](const SdkRowModel&, const std::size_t lhs, const std::size_t rhs) {
                return lhs <=> rhs;
            }
        );
        std::vector<nandina::widget::DataTableColumn> columns {std::move(column)};
        auto table = ui.make<nandina::widget::DataTable>(std::move(columns)).build();
        int selection_events = 0;
        int sort_events = 0;
        table->set_on_selection_changed([&selection_events] { ++selection_events; });
        table->set_on_sort_changed([&sort_events] { ++sort_events; });
        table->set_model(
            std::shared_ptr<nandina::widget::IRowModel>(&model, [](nandina::widget::IRowModel*) {})
        );
        nandina::scene::NanSceneTree tree;
        tree.set_root(table);
        (void)tree.layout_root(nandina::foundation::NanSize(320.0F, 200.0F));
        const bool selected = table->apply_selection(
            nandina::widget::SelectionOp::replace,
            nandina::widget::CellId {
                .row = nandina::widget::RowId {.value = 1},
                .column = nandina::widget::ColumnId {.value = "value"},
            }
        );
        const bool sorted = table->set_sort(
            nandina::widget::SortSpec {
                .column = nandina::widget::ColumnId {.value = "value"},
                .direction = nandina::widget::SortDirection::descending,
            }
        );
        return table->row_count() == 2 && table->column_count() == 1 && selected && sorted
            && selection_events == 1 && sort_events == 1;
    }

    auto list_api_works(nandina::app::NanApplication& application) -> bool {
        nandina::reactive::ReactiveScope scope {application.graph()};
        nandina::widget::BuildContext ui {application.graph(), scope, application.theme_manager()};
        SdkRowModel model;
        auto list = ui.make<nandina::widget::ListView>().build();
        list->set_delegate(std::make_shared<SdkListDelegate>());
        list->set_model(
            std::shared_ptr<nandina::widget::IRowModel>(&model, [](nandina::widget::IRowModel*) {})
        );
        nandina::scene::NanSceneTree tree;
        tree.set_root(list);
        (void)tree.layout_root(nandina::foundation::NanSize(320.0F, 200.0F));
        return list->row_count() == 2 && list->item_count() == 2
            && list->select(nandina::widget::RowId {.value = 2});
    }

    auto tree_api_works(nandina::app::NanApplication& application) -> bool {
        nandina::reactive::ReactiveScope scope {application.graph()};
        nandina::widget::BuildContext ui {application.graph(), scope, application.theme_manager()};
        SdkTreeModel model;
        auto view = ui.make<nandina::widget::TreeView>().build();
        view->set_delegate(std::make_shared<SdkTreeDelegate>());
        view->set_model(
            std::shared_ptr<nandina::widget::ITreeModel>(&model, [](nandina::widget::ITreeModel*) {
            })
        );
        nandina::scene::NanSceneTree tree;
        tree.set_root(view);
        (void)tree.layout_root(nandina::foundation::NanSize(320.0F, 200.0F));
        return view->node_count() == 2 && view->row_count() == 1
            && view->expand(nandina::widget::RowId {.value = 1}) && view->row_count() == 2
            && view->select(nandina::widget::RowId {.value = 2});
    }

    auto accordion_api_works(nandina::app::NanApplication& application) -> bool {
        nandina::reactive::ReactiveScope scope {application.graph()};
        nandina::widget::BuildContext ui {application.graph(), scope, application.theme_manager()};
        auto accordion = ui.make<nandina::widget::Accordion>().build();
        auto content = ui.surface(ui.column(ui.row(
                                      ui.expanded(ui.make<nandina::widget::Label>("section body")),
                                      ui.wrap(ui.make<nandina::widget::Label>("tag"))
                                  )))
                           .build();
        accordion->set_sections({
            {.id = "first", .title = "First", .content = content},
        });
        nandina::scene::NanSceneTree tree;
        tree.set_root(accordion);
        (void)tree.layout_root(nandina::foundation::NanSize(320.0F, 200.0F));
        return content->child() != nullptr && accordion->section_count() == 1
            && accordion->trigger_node("first")->text() == "First" && accordion->toggle("first")
            && accordion->is_open("first");
    }
} // namespace

int main() {
    nandina::app::NanApplication application;
    nandina::app::NanWindow window(application, {});
    window.set_content(
        nandina::widget::Label::create(application.graph(), "SDK consumer", application.theme())
    );
    const auto backend = window.platform_name();
    const auto decoder = application.image_decoder();
    const bool data_api = data_api_works(application);
    const bool list_api = list_api_works(application);
    const bool tree_api = tree_api_works(application);
    const bool accordion_api = accordion_api_works(application);
    const auto version = SDL_GetVersion();
    std::printf(
        "backend=%.*s sdl=%d.%d.%d decoder=%d data-api=%d list-api=%d tree-api=%d "
        "accordion-api=%d\n",
        static_cast<int>(backend.size()),
        backend.data(),
        SDL_VERSIONNUM_MAJOR(version),
        SDL_VERSIONNUM_MINOR(version),
        SDL_VERSIONNUM_MICRO(version),
        decoder != nullptr,
        data_api,
        list_api,
        tree_api,
        accordion_api
    );
    return backend == "sdl3" && decoder != nullptr && data_api && list_api && tree_api
            && accordion_api
        ? 0
        : 1;
}
