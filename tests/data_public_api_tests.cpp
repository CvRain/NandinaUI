// Public/external-style consumption checks for the experimental data-view API.
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/internal/data_identity.hpp>
#include <nandina/widget/internal/data_table.hpp>
#include <nandina/widget/internal/data_table_column.hpp>
#include <nandina/widget/internal/grid_navigation.hpp>
#include <nandina/widget/internal/header_delegate.hpp>
#include <nandina/widget/internal/item_delegate.hpp>
#include <nandina/widget/internal/row_model.hpp>
#include <nandina/widget/internal/table_selection_model.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/list_view.hpp>

#include <catch2/catch_test_macros.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace nandina;

namespace
{
    class PublicModel final: public widget::TypedRowModelBase<PublicModel> {
    public:
        std::vector<std::string> names {"alpha", "beta"};
        std::vector<widget::RowId> ids {
            widget::RowId {.value = 10},
            widget::RowId {.value = 20},
        };

        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return names.size();
        }

        [[nodiscard]] auto row_id(const std::size_t row) const -> widget::RowId override {
            return ids.at(row);
        }
    };

    class PublicItemDelegate final: public widget::TypedItemDelegate<PublicModel> {
    protected:
        auto create_typed(
            const widget::BuildContext& ui,
            const PublicModel& model,
            const widget::ItemBinding& binding
        ) -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), model.names.at(binding.model_row));
        }

        void update_typed(
            scene::NanControl& control,
            const PublicModel& model,
            const widget::ItemBinding& binding
        ) override {
            static_cast<widget::Label&>(control).set_text(model.names.at(binding.model_row));
        }
    };

    class PublicHeaderDelegate final: public widget::IHeaderDelegate {
    public:
        auto create_header(const widget::BuildContext& ui, const widget::HeaderBinding& binding)
            -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), binding.column.title, binding.theme);
        }

        void
        update_header(scene::NanControl& control, const widget::HeaderBinding& binding) override {
            static_cast<widget::Label&>(control).set_text(binding.column.title);
        }
    };

    struct Fixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
    };
} // namespace

TEST_CASE("public identity hashes support standard unordered containers", "[data][public-api]") {
    STATIC_REQUIRE(std::is_default_constructible_v<std::hash<widget::RowId>>);
    STATIC_REQUIRE(std::is_default_constructible_v<std::hash<widget::ColumnId>>);
    STATIC_REQUIRE(std::is_default_constructible_v<std::hash<widget::CellId>>);

    std::unordered_set<widget::RowId> rows;
    CHECK(rows.insert(widget::RowId {.value = 7}).second);
    CHECK_FALSE(rows.insert(widget::RowId {.value = 7}).second);

    std::unordered_map<widget::ColumnId, int> columns;
    columns.emplace(widget::ColumnId {.value = "name"}, 1);
    CHECK(columns.at(widget::ColumnId {.value = "name"}) == 1);

    const widget::CellId cell {
        .row = widget::RowId {.value = 7},
        .column = widget::ColumnId {.value = "name"},
    };
    std::unordered_set<widget::CellId> cells {cell};
    CHECK(cells.contains(cell));
    CHECK(cells.size() == 1);
}

TEST_CASE("authors can define typed item and header delegates", "[data][public-api]") {
    Fixture fixture;
    PublicModel model;
    auto column = widget::text_column<PublicModel>(
        widget::ColumnId {.value = "name"},
        "Name",
        [](const PublicModel& current, const std::size_t row) { return current.names.at(row); },
        widget::TableColumn {.flex = 1.0F}
    );
    widget::set_local_sort<PublicModel>(
        column,
        [](const PublicModel& current, const std::size_t lhs, const std::size_t rhs) {
            return current.names.at(lhs) <=> current.names.at(rhs);
        }
    );
    column.delegate = std::make_shared<PublicItemDelegate>();
    column.header_delegate = std::make_shared<PublicHeaderDelegate>();

    std::vector<widget::DataTableColumn> columns {std::move(column)};
    auto table =
        fixture.ui.make<widget::DataTable>(std::move(columns), widget::SelectionMode::multiple)
            .build();
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    scene::NanSceneTree tree;
    tree.set_root(table);
    (void)tree.layout_root(foundation::NanSize(400.0F, 240.0F));

    CHECK(table->row_count() == 2);
    CHECK(table->column_count() == 1);
    REQUIRE(table->header_node(widget::ColumnId {.value = "name"}) != nullptr);
    REQUIRE(
        table->cell_node(
            widget::CellId {
                .row = widget::RowId {.value = 10},
                .column = widget::ColumnId {.value = "name"},
            }
        )
        != nullptr
    );
}

TEST_CASE("authors can create the public virtualized ListView", "[data][public-api][list-view]") {
    Fixture fixture;
    PublicModel model;
    auto list = widget::ListView::create(fixture.ui);
    list->set_delegate(std::make_shared<PublicItemDelegate>());
    list->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(320.0F, 200.0F));

    CHECK(list->row_count() == 2);
    CHECK(list->item_count() == 2);
    CHECK(list->select(widget::RowId {.value = 20}));
    CHECK(list->selection().contains(widget::RowId {.value = 20}));
}
