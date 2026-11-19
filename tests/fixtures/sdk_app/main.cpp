#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/foundation/geometry.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/internal/data_table.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/list_view.hpp>

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
        std::vector<nandina::widget::DataTableColumn> columns {std::move(column)};
        auto table = ui.make<nandina::widget::DataTable>(std::move(columns)).build();
        table->set_model(
            std::shared_ptr<nandina::widget::IRowModel>(&model, [](nandina::widget::IRowModel*) {})
        );
        nandina::scene::NanSceneTree tree;
        tree.set_root(table);
        (void)tree.layout_root(nandina::foundation::NanSize(320.0F, 200.0F));
        return table->row_count() == 2 && table->column_count() == 1;
    }

    auto list_api_works(nandina::app::NanApplication& application) -> bool {
        nandina::reactive::ReactiveScope scope {application.graph()};
        nandina::widget::BuildContext ui {application.graph(), scope, application.theme_manager()};
        SdkRowModel model;
        auto list = nandina::widget::ListView::create(ui);
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
    const auto version = SDL_GetVersion();
    std::printf(
        "backend=%.*s sdl=%d.%d.%d decoder=%d data-api=%d list-api=%d\n",
        static_cast<int>(backend.size()),
        backend.data(),
        SDL_VERSIONNUM_MAJOR(version),
        SDL_VERSIONNUM_MINOR(version),
        SDL_VERSIONNUM_MICRO(version),
        decoder != nullptr,
        data_api,
        list_api
    );
    return backend == "sdl3" && decoder != nullptr && data_api && list_api ? 0 : 1;
}
