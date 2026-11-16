// data_table_2b_tests — 批 2B 全量 DataTable / delegate / ItemHost / 排序与绑定状态机。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/internal/data_table.hpp>
#include <nandina/widget/internal/item_host.hpp>
#include <nandina/widget/label.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace nandina;

namespace
{
    class TestModel final: public widget::TypedRowModelBase<TestModel> {
    public:
        std::vector<std::string> names;
        std::vector<widget::RowId> ids;

        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return names.size();
        }
        [[nodiscard]] auto row_id(const std::size_t row) const -> widget::RowId override {
            return ids.at(row);
        }

        void reset(std::vector<std::string> next_names, const std::uint64_t first_id) {
            names = std::move(next_names);
            ids.clear();
            ids.reserve(names.size());
            for (std::size_t index = 0; index < names.size(); ++index) {
                ids.push_back(widget::RowId {.value = first_id + index});
            }
            notify_reset();
        }
        void change(const widget::RowRange range) {
            notify_data_changed(range);
        }
    };

    class OtherModel final: public widget::TypedRowModelBase<OtherModel> {
    public:
        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return 0;
        }
        [[nodiscard]] auto row_id(std::size_t) const -> widget::RowId override {
            return {};
        }
    };

    struct Fixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
    };

    auto column_id(const char* value) -> widget::ColumnId {
        return widget::ColumnId {.value = value};
    }

    auto name_column(const widget::SortStrategy sort = widget::SortStrategy::none)
        -> widget::DataTableColumn {
        auto column = widget::text_column<TestModel>(
            column_id("name"),
            "Name",
            [](const TestModel& model, const std::size_t row) { return model.names.at(row); },
            widget::TableColumn {.flex = 1.0F}
        );
        if (sort == widget::SortStrategy::local) {
            widget::set_local_sort<TestModel>(
                column,
                [](const TestModel& model, const std::size_t lhs, const std::size_t rhs) {
                    return model.names.at(lhs) <=> model.names.at(rhs);
                }
            );
        }
        else if (sort == widget::SortStrategy::external) {
            column.sort = widget::SortStrategy::external;
        }
        return column;
    }

    auto second_column() -> widget::DataTableColumn {
        return widget::text_column<TestModel>(
            column_id("kind"),
            "Kind",
            [](const TestModel&, const std::size_t row) { return "k" + std::to_string(row); },
            widget::TableColumn {.width = 80.0F}
        );
    }

    auto mount(Fixture& fixture, const std::shared_ptr<widget::DataTable>& table)
        -> std::unique_ptr<scene::NanSceneTree> {
        auto tree = std::make_unique<scene::NanSceneTree>();
        tree->set_root(table);
        (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
        return tree;
    }

    auto cell(const std::uint64_t row, std::string column) -> widget::CellId {
        return widget::CellId {
            .row = widget::RowId {.value = row},
            .column = widget::ColumnId {.value = std::move(column)},
        };
    }
} // namespace

TEST_CASE("DataTable materializes cells with the default text delegate", "[data-table][2b]") {
    Fixture fixture;
    TestModel model;
    model.reset({"alpha", "beta", "gamma"}, 10);
    auto table = widget::DataTable::create(fixture.ui, {name_column(), second_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    auto tree = mount(fixture, table);
    REQUIRE(table->row_count() == 3);
    REQUIRE(table->column_count() == 2);
    REQUIRE(table->item_count() == 6);
    REQUIRE(table->cell_node(cell(10, "name")) != nullptr);

    auto* label = static_cast<widget::Label*>(table->cell_node(cell(10, "name")));
    REQUIRE(label != nullptr);
    CHECK(label->text() == "alpha");

    // 默认文本 delegate 的单行省略设置已生效。
    CHECK(label->max_lines() == 1);
}

TEST_CASE("DataTable idle notifications apply immediately", "[data-table][2b][binding]") {
    Fixture fixture;
    TestModel model;
    model.reset({"alpha", "beta"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    auto* first = table->cell_node(cell(1, "name"));

    model.reset({"one", "two", "three"}, 1);
    CHECK(table->row_count() == 3);
    CHECK(table->item_count() == 3);
    CHECK_FALSE(table->has_pending_notifications());

    model.change({.first = 0, .last_exclusive = 2});
    CHECK(table->cell_node(cell(1, "name")) == first);
    CHECK(static_cast<widget::Label*>(first)->text() == "one");
}

TEST_CASE(
    "DataTable traversal notifications are pending and coalesced",
    "[data-table][2b][binding]"
) {
    Fixture fixture;
    TestModel model;
    model.reset({"alpha", "beta", "gamma"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        model.change({.first = 0, .last_exclusive = 1});
        model.change({.first = 1, .last_exclusive = 2});
        model.reset({"delta", "epsilon", "zeta", "eta"}, 10);
    }
    REQUIRE(table->has_pending_notifications());
    CHECK(table->row_count() == 3);

    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK_FALSE(table->has_pending_notifications());
    CHECK(table->row_count() == 4);
    CHECK(table->cell_node(cell(10, "name")) != nullptr);
}

TEST_CASE(
    "DataTable unsubscribes on exit and reconciles on re-enter",
    "[data-table][2b][binding]"
) {
    Fixture fixture;
    TestModel model;
    model.reset({"alpha", "beta"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);
    REQUIRE(table->row_count() == 2);

    tree->set_root(nullptr);
    model.reset({"one", "two", "three"}, 5);
    CHECK(table->row_count() == 2);

    tree->set_root(table);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->row_count() == 3);
    CHECK(table->cell_node(cell(5, "name")) != nullptr);
}

TEST_CASE("DataTable clears pending scheduling across tree exit", "[data-table][2b][binding]") {
    Fixture fixture;
    TestModel model;
    model.reset({"alpha", "beta"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        model.change({.first = 0, .last_exclusive = 2});
    }
    REQUIRE(table->has_pending_notifications());
    tree->set_root(nullptr);
    CHECK_FALSE(table->has_pending_notifications());

    tree->set_root(table);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        model.names[0] = "changed";
        model.change({.first = 0, .last_exclusive = 1});
    }
    REQUIRE(table->has_pending_notifications());
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(static_cast<widget::Label*>(table->cell_node(cell(1, "name")))->text() == "changed");
}

TEST_CASE("DataTable column configuration is transactional", "[data-table][2b][columns]") {
    Fixture fixture;
    auto table = widget::DataTable::create(fixture.ui, {name_column(), second_column()});
    const auto before_columns = table->columns().size();

    CHECK_THROWS_AS(table->set_columns({}), std::invalid_argument);

    auto duplicate = second_column();
    duplicate.id = column_id("name");
    CHECK_THROWS_AS(table->set_columns({name_column(), duplicate}), std::invalid_argument);

    auto empty_id = second_column();
    empty_id.id = column_id("");
    CHECK_THROWS_AS(table->set_columns({empty_id}), std::invalid_argument);
    CHECK(table->columns().size() == before_columns);

    // token 与 callback 必须成对出现。
    widget::DataTableColumn token_only;
    token_only.id = column_id("bad");
    token_only.value.model_token = widget::model_type_token_of<TestModel>();
    CHECK_THROWS_AS(table->set_columns({token_only}), std::invalid_argument);

    widget::DataTableColumn callback_only;
    callback_only.id = column_id("bad");
    callback_only.value.text = [](const widget::IRowModel&, std::size_t) { return ""; };
    CHECK_THROWS_AS(table->set_columns({callback_only}), std::invalid_argument);

    widget::DataTableColumn local_without_comparator;
    local_without_comparator.id = column_id("bad");
    local_without_comparator.sort = widget::SortStrategy::local;
    CHECK_THROWS_AS(table->set_columns({local_without_comparator}), std::invalid_argument);

    auto external_with_comparator = second_column();
    external_with_comparator.sort = widget::SortStrategy::external;
    external_with_comparator.comparator = widget::ColumnComparator {
        .model_token = widget::model_type_token_of<TestModel>(),
        .compare = [](const widget::IRowModel&, std::size_t, std::size_t) {
            return std::strong_ordering::equal;
        },
    };
    CHECK_THROWS_AS(table->set_columns({external_with_comparator}), std::invalid_argument);

    auto invalid_strategy = second_column();
    invalid_strategy.sort = static_cast<widget::SortStrategy>(99);
    CHECK_THROWS_AS(table->set_columns({invalid_strategy}), std::invalid_argument);

    auto invalid_track = second_column();
    invalid_track.track.width = -1.0F;
    CHECK_THROWS_AS(table->set_columns({invalid_track}), std::invalid_argument);

    auto invalid_flex = second_column();
    invalid_flex.track.width.reset();
    invalid_flex.track.flex = 0.0F;
    CHECK_THROWS_AS(table->set_columns({invalid_flex}), std::invalid_argument);

    CHECK(table->columns().size() == before_columns);
}

TEST_CASE(
    "DataTable keeps programmatic selection inside the current universe",
    "[data-table][2b][selection]"
) {
    Fixture fixture;
    TestModel model;
    model.reset({"a", "b"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column(), second_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    CHECK(table->selection().mode() == widget::SelectionMode::multiple);

    REQUIRE(table->apply_selection(widget::SelectionOp::replace, cell(1, "name"), {}));
    const auto selected_before = table->selection().selected();
    CHECK_FALSE(table->apply_selection(widget::SelectionOp::replace, cell(99, "name"), {}));
    CHECK(table->selection().selected() == selected_before);

    const std::vector<widget::CellId> invalid_extent {
        cell(1, "name"),
        cell(99, "name"),
    };
    CHECK_FALSE(
        table->apply_selection(widget::SelectionOp::extend, cell(1, "name"), invalid_extent)
    );
    CHECK(table->selection().selected() == selected_before);
    CHECK_FALSE(table->set_current_cell(cell(99, "name")));
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(1, "name")));

    auto single =
        widget::DataTable::create(fixture.ui, {name_column()}, widget::SelectionMode::single);
    CHECK(single->selection().mode() == widget::SelectionMode::single);
}

TEST_CASE("DataTable validates column tokens on set_model", "[data-table][2b][columns]") {
    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    OtherModel other;
    auto table = widget::DataTable::create(fixture.ui, {name_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto* node_before = table->cell_node(cell(1, "name"));
    REQUIRE(node_before != nullptr);

    CHECK_THROWS_AS(
        table->set_model(std::shared_ptr<widget::IRowModel>(&other, [](widget::IRowModel*) {})),
        std::invalid_argument
    );
    CHECK(table->model() == &model);
    CHECK(table->cell_node(cell(1, "name")) == node_before);

    // 已有模型时，set_columns 提交前校验全部 token。
    auto foreign = widget::text_column<OtherModel>(
        column_id("foreign"),
        "Foreign",
        [](const OtherModel&, std::size_t) { return ""; }
    );
    CHECK_THROWS_AS(table->set_columns({foreign}), std::invalid_argument);
    CHECK(table->columns().size() == 1);
}

TEST_CASE(
    "DataTable hidden columns stay sortable but leave the universe",
    "[data-table][2b][columns]"
) {
    Fixture fixture;
    TestModel model;
    model.reset({"b", "a"}, 1);
    auto column = name_column(widget::SortStrategy::local);
    auto table = widget::DataTable::create(fixture.ui, {column, second_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    REQUIRE(table->apply_selection(widget::SelectionOp::replace, cell(1, "name"), {}));
    REQUIRE(table->selection().current().has_value());

    column.visible = false;
    table->set_columns({column, second_column()});
    CHECK(table->column_count() == 1);
    CHECK_FALSE(table->selection().is_selected(cell(1, "name")));
    CHECK_FALSE(table->selection().current().has_value());
    CHECK_FALSE(table->apply_selection(widget::SelectionOp::replace, cell(1, "name"), {}));

    // 隐藏但仍 sortable：排序状态保留。
    column.sort = widget::SortStrategy::local;
    column.visible = false;
    table->set_columns({column, second_column()});
    CHECK(table->set_sort(widget::SortSpec {.column = column_id("name")}));
    CHECK(table->sort_spec().has_value());
    column.visible = true;
    table->set_columns({column, second_column()});
    CHECK(table->column_count() == 2);
}

TEST_CASE("DataTable dropping a sorted column clears sorting", "[data-table][2b][columns]") {
    Fixture fixture;
    auto table = widget::DataTable::create(
        fixture.ui,
        {name_column(widget::SortStrategy::local), second_column()}
    );
    REQUIRE(table->set_sort(widget::SortSpec {.column = column_id("name")}));
    REQUIRE(table->sort_spec().has_value());

    table->set_columns({second_column()});
    CHECK_FALSE(table->sort_spec().has_value());

    auto table2 = widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::local)});
    REQUIRE(table2->set_sort(widget::SortSpec {.column = column_id("name")}));
    auto unsortable = name_column(widget::SortStrategy::none);
    table2->set_columns({unsortable});
    CHECK_FALSE(table2->sort_spec().has_value());
}

TEST_CASE("DataTable local sort reorders display rows", "[data-table][2b][sort]") {
    Fixture fixture;
    TestModel model;
    model.reset({"b", "a", "c"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::local)});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    int requests = 0;
    auto subscription =
        table->sort_requested().subscribe([&requests](widget::SortRequest) { ++requests; });

    REQUIRE(table->set_sort(widget::SortSpec {.column = column_id("name")}));
    CHECK(table->sort_spec()->direction == widget::SortDirection::ascending);
    CHECK(table->model_row_for_display(0) == std::optional<std::size_t>(1));
    CHECK(table->model_row_for_display(1) == std::optional<std::size_t>(0));
    CHECK(table->model_row_for_display(2) == std::optional<std::size_t>(2));

    CHECK_FALSE(table->set_sort(widget::SortSpec {.column = column_id("name")}));
    CHECK(table->set_sort(
        widget::SortSpec {
            .column = column_id("name"),
            .direction = widget::SortDirection::descending
        }
    ));
    CHECK(table->model_row_for_display(0) == std::optional<std::size_t>(2));
    CHECK(requests == 0);

    CHECK(table->set_sort(std::nullopt));
    CHECK_FALSE(table->set_sort(std::nullopt));
    CHECK(table->model_row_for_display(0) == std::optional<std::size_t>(0));
}

TEST_CASE("DataTable local sort keeps state when the comparator throws", "[data-table][2b][sort]") {
    Fixture fixture;
    TestModel model;
    model.reset({"b", "a", "c"}, 1);
    auto column = name_column();
    widget::set_local_sort<TestModel>(
        column,
        [](const TestModel&, const std::size_t lhs, const std::size_t) -> std::weak_ordering {
            if (lhs == 2) {
                throw std::runtime_error("comparator failed");
            }
            return std::strong_ordering::equal;
        }
    );
    auto table = widget::DataTable::create(fixture.ui, {column});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    CHECK_THROWS_AS(
        table->set_sort(widget::SortSpec {.column = column_id("name")}),
        std::runtime_error
    );
    CHECK_FALSE(table->sort_spec().has_value());
    CHECK(table->model_row_for_display(0) == std::optional<std::size_t>(0));
}

TEST_CASE("DataTable re-applies local sort after data changes", "[data-table][2b][sort][binding]") {
    Fixture fixture;
    TestModel model;
    model.reset({"a", "b"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::local)});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);
    REQUIRE(table->set_sort(widget::SortSpec {.column = column_id("name")}));
    REQUIRE(table->model_row_for_display(0) == std::optional<std::size_t>(0));

    model.names[0] = "z";
    model.change({.first = 0, .last_exclusive = 2});
    REQUIRE(table->model_row_for_display(0) == std::optional<std::size_t>(1));

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        model.names[0] = "a";
        model.change({.first = 0, .last_exclusive = 2});
    }
    REQUIRE(table->has_pending_notifications());
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->model_row_for_display(0) == std::optional<std::size_t>(0));
}

TEST_CASE("DataTable external sort commits then emits", "[data-table][2b][sort]") {
    Fixture fixture;
    TestModel model;
    model.reset({"b", "a"}, 1);
    auto table =
        widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::external)});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    std::vector<widget::SortRequest> requests;
    auto subscription =
        table->sort_requested().subscribe([&requests](const widget::SortRequest& request) {
            requests.push_back(request);
        });
    int callbacks = 0;
    table->set_on_sort_requested([&callbacks](const widget::SortRequest&) { ++callbacks; });

    const widget::SortSpec spec {.column = column_id("name")};
    REQUIRE(table->set_sort(spec));
    REQUIRE(requests.size() == 1);
    CHECK(requests.front().sort == std::optional(spec));
    CHECK(requests.front().reason == widget::SortRequestReason::programmatic);
    CHECK(callbacks == 1);
    // external 不重排显示序。
    CHECK(table->model_row_for_display(0) == std::optional<std::size_t>(0));

    CHECK_FALSE(table->set_sort(spec));
    CHECK(requests.size() == 1);

    // 显式清除 external 排序：先提交状态，再发 nullopt 事件。
    REQUIRE(table->set_sort(std::nullopt, widget::SortRequestReason::user_header_click));
    REQUIRE(requests.size() == 2);
    CHECK_FALSE(requests.back().sort.has_value());
    CHECK(requests.back().reason == widget::SortRequestReason::user_header_click);
    CHECK_FALSE(table->sort_spec().has_value());
}

TEST_CASE("DataTable rejects invalid sort enums before mutating state", "[data-table][2b][sort]") {
    Fixture fixture;
    auto table =
        widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::external)});
    CHECK_THROWS_AS(
        table->set_sort(
            widget::SortSpec {
                .column = column_id("name"),
                .direction = static_cast<widget::SortDirection>(99),
            }
        ),
        std::invalid_argument
    );
    CHECK_FALSE(table->sort_spec().has_value());
    CHECK_THROWS_AS(
        table->set_sort(std::nullopt, static_cast<widget::SortRequestReason>(99)),
        std::invalid_argument
    );
}

TEST_CASE(
    "DataTable callback wrapper expires with its build scope",
    "[data-table][2b][sort][scope]"
) {
    Fixture fixture;
    auto table =
        widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::external)});
    int callback_calls = 0;
    int event_calls = 0;
    auto subscription =
        table->sort_requested().subscribe([&event_calls](const widget::SortRequest&) {
            ++event_calls;
        });
    table->set_on_sort_requested([&callback_calls](const widget::SortRequest&) {
        ++callback_calls;
    });

    REQUIRE(table->set_sort(widget::SortSpec {.column = column_id("name")}));
    CHECK(callback_calls == 1);
    fixture.scope.clear();
    REQUIRE(table->set_sort(
        widget::SortSpec {
            .column = column_id("name"),
            .direction = widget::SortDirection::descending,
        }
    ));
    CHECK(callback_calls == 1);
    CHECK(event_calls == 2);
}

TEST_CASE("DataTable header click cycles sort with user reason", "[data-table][2b][sort][header]") {
    Fixture fixture;
    TestModel model;
    model.reset({"b", "a"}, 1);
    auto table =
        widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::external)});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    std::vector<widget::SortRequest> requests;
    auto subscription =
        table->sort_requested().subscribe([&requests](const widget::SortRequest& request) {
            requests.push_back(request);
        });

    auto click_header = [&] {
        auto* header = table->header_node(column_id("name"));
        REQUIRE(header != nullptr);
        const auto bounds = header->global_bounds();
        tree->dispatch_mouse_button(
            scene::MouseButtonEvent {
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                foundation::NanPoint(
                    (bounds.get_left() + bounds.get_right()) * 0.5F,
                    (bounds.get_top() + bounds.get_bottom()) * 0.5F
                ),
            }
        );
    };

    click_header();
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    REQUIRE(requests.size() == 1);
    CHECK(requests.back().reason == widget::SortRequestReason::user_header_click);
    CHECK(table->sort_spec()->direction == widget::SortDirection::ascending);

    click_header();
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    REQUIRE(requests.size() == 2);
    CHECK(table->sort_spec()->direction == widget::SortDirection::descending);

    click_header();
    REQUIRE(requests.size() == 3);
    CHECK_FALSE(requests.back().sort.has_value());
    CHECK_FALSE(table->sort_spec().has_value());
}

TEST_CASE(
    "DataTable uses bounded track widths and accepts zero-width layout",
    "[data-table][2b][layout]"
) {
    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    auto fixed = widget::text_column<TestModel>(
        column_id("fixed"),
        "Fixed",
        [](const TestModel&, std::size_t) { return std::string("fixed"); },
        widget::TableColumn {.width = 40.0F}
    );
    auto flex = widget::text_column<TestModel>(
        column_id("flex"),
        "Flex",
        [](const TestModel&, std::size_t) { return std::string("flex"); },
        widget::TableColumn {.flex = 1.0F}
    );
    auto table = widget::DataTable::create(fixture.ui, {fixed, flex});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    auto tree = mount(fixture, table);
    REQUIRE(table->cell_node(cell(1, "fixed")) != nullptr);
    REQUIRE(table->cell_node(cell(1, "flex")) != nullptr);
    CHECK(table->cell_node(cell(1, "fixed"))->width() == 40.0F);
    CHECK(table->cell_node(cell(1, "flex"))->width() == 360.0F);

    // Zero is a finite bound; it must not fall back to the 160 DIP unbounded basis.
    tree->layout_root(foundation::NanSize(0.0F, 100.0F));
    CHECK(table->cell_node(cell(1, "fixed"))->width() == 40.0F);
    CHECK(table->cell_node(cell(1, "flex"))->width() == 0.0F);
}

TEST_CASE("DataTable accepts empty and all-hidden layouts", "[data-table][2b][layout]") {
    Fixture fixture;
    auto empty = widget::DataTable::create(fixture.ui);
    auto empty_tree = mount(fixture, empty);
    CHECK(empty->row_count() == 0);
    CHECK(empty->column_count() == 0);
    CHECK(empty->header_node(column_id("missing")) == nullptr);

    TestModel model;
    model.reset({"a"}, 1);
    auto hidden = name_column();
    hidden.visible = false;
    auto hidden_table = widget::DataTable::create(fixture.ui, {hidden});
    hidden_table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto hidden_tree = mount(fixture, hidden_table);
    CHECK(hidden_table->column_count() == 0);
    CHECK(hidden_table->item_count() == 0);
    CHECK(hidden_table->header_node(column_id("name")) == nullptr);
}

TEST_CASE("DataTable rejects total content height overflow", "[data-table][2b][layout]") {
    Fixture fixture;
    TestModel model;
    model.reset({"a", "b"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    table->set_row_height(std::numeric_limits<float>::max());
    CHECK_THROWS_AS(mount(fixture, table), std::invalid_argument);
}

TEST_CASE(
    "DataTable exposes visible display column coordinates to delegates",
    "[data-table][2b][delegate]"
) {
    class CapturingDelegate final: public widget::IItemDelegate {
    public:
        explicit CapturingDelegate(std::vector<widget::ModelIndex>& seen): seen_(&seen) {}

        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph());
        }
        void update_item(scene::NanControl&, const widget::ItemBinding& binding) override {
            seen_->push_back(binding.display_index);
        }

    private:
        std::vector<widget::ModelIndex>* seen_;
    };

    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    std::vector<widget::ModelIndex> seen;
    auto delegate = std::make_shared<CapturingDelegate>(seen);
    auto first = widget::custom_column(
        column_id("first"),
        "First",
        widget::TableColumn {.width = 20.0F},
        delegate
    );
    auto second = widget::custom_column(
        column_id("second"),
        "Second",
        widget::TableColumn {.width = 20.0F},
        delegate
    );
    auto table = widget::DataTable::create(fixture.ui, {first, second});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    REQUIRE_FALSE(seen.empty());
    bool saw_second = false;
    for (const auto index: seen) {
        CHECK(index.row == 0);
        saw_second = saw_second || index.column == 1;
    }
    CHECK(saw_second);
}

TEST_CASE(
    "DataTable selection syncs navigation and prunes on rebuild",
    "[data-table][2b][selection]"
) {
    Fixture fixture;
    TestModel model;
    model.reset({"b", "a", "c"}, 1);
    auto table = widget::DataTable::create(fixture.ui, {name_column(widget::SortStrategy::local)});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    REQUIRE(table->apply_selection(widget::SelectionOp::replace, cell(1, "name"), {}));
    REQUIRE(table->navigation().current().has_value());
    CHECK(table->navigation().current()->row == 0);
    CHECK(table->navigation().current()->column == 0);

    // 排序后按身份重算显示坐标。
    REQUIRE(table->set_sort(widget::SortSpec {.column = column_id("name")}));
    REQUIRE(table->navigation().current().has_value());
    CHECK(table->navigation().current()->row == 1);

    // 行身份消失后 current/anchor 被裁剪。
    model.reset({"x"}, 100);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK_FALSE(table->selection().current().has_value());
    CHECK_FALSE(table->selection().anchor().has_value());
}

TEST_CASE("ItemHost keeps ownership when delegate update fails", "[data-table][2b][item-host]") {
    class FlakyDelegate final: public widget::IItemDelegate {
    public:
        int create_calls = 0;
        int update_calls = 0;
        bool fail_update = false;

        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            ++create_calls;
            return widget::Label::create(ui.graph());
        }
        void update_item(scene::NanControl&, const widget::ItemBinding&) override {
            ++update_calls;
            if (fail_update) {
                throw std::runtime_error("update failed");
            }
        }
    };

    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    widget::ItemHost host(fixture.ui);
    auto delegate = std::make_shared<FlakyDelegate>();
    const widget::ItemHost::ItemRequest request {
        .id = cell(1, "name"),
        .model = &model,
        .model_row = 0,
        .display_index = widget::ModelIndex {.row = 0, .column = 0},
        .row = widget::RowId {.value = 1},
        .column = nullptr,
        .state = {},
        .delegate = delegate,
    };

    host.set_items({request});
    REQUIRE(host.item_count() == 1);
    REQUIRE(host.node_for(cell(1, "name")) != nullptr);
    const auto* node = host.node_for(cell(1, "name"));

    delegate->fail_update = true;
    CHECK_THROWS_AS(host.set_items({request}), std::runtime_error);
    CHECK(host.item_count() == 1);
    CHECK(host.node_for(cell(1, "name")) == node);
}

TEST_CASE("ItemHost leaves no mounted node when create fails", "[data-table][2b][item-host]") {
    class ThrowingCreateDelegate final: public widget::IItemDelegate {
    public:
        auto create_item(const widget::BuildContext&, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            throw std::runtime_error("create failed");
        }
        void update_item(scene::NanControl&, const widget::ItemBinding&) override {}
    };

    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    widget::ItemHost host(fixture.ui);
    const widget::ItemHost::ItemRequest request {
        .id = cell(1, "name"),
        .model = &model,
        .model_row = 0,
        .display_index = widget::ModelIndex {.row = 0, .column = 0},
        .row = widget::RowId {.value = 1},
        .column = nullptr,
        .state = {},
        .delegate = std::make_shared<ThrowingCreateDelegate>(),
    };
    CHECK_THROWS_AS(host.set_items({request}), std::runtime_error);
    CHECK(host.item_count() == 0);
    CHECK(host.child_count() == 0);
}

TEST_CASE(
    "ItemHost rejects duplicate requests before mutating entries",
    "[data-table][2b][item-host]"
) {
    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    widget::ItemHost host(fixture.ui);
    const widget::ItemHost::ItemRequest request {
        .id = cell(1, "name"),
        .model = &model,
        .model_row = 0,
        .display_index = widget::ModelIndex {.row = 0, .column = 0},
        .row = widget::RowId {.value = 1},
        .column = nullptr,
        .state = {},
        .delegate = std::make_shared<widget::TextItemDelegate>(),
    };

    CHECK_THROWS_AS(host.set_items({request, request}), std::invalid_argument);
    CHECK(host.item_count() == 0);
    CHECK(host.child_count() == 0);
}

TEST_CASE("ItemHost recreates a cell when its delegate changes", "[data-table][2b][item-host]") {
    class PlainDelegate final: public widget::IItemDelegate {
    public:
        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), "plain");
        }
        void update_item(scene::NanControl&, const widget::ItemBinding&) override {}
    };

    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    widget::ItemHost host(fixture.ui);
    auto plain = std::make_shared<PlainDelegate>();
    const widget::ItemHost::ItemRequest request {
        .id = cell(1, "name"),
        .model = &model,
        .model_row = 0,
        .display_index = widget::ModelIndex {.row = 0, .column = 0},
        .row = widget::RowId {.value = 1},
        .column = nullptr,
        .state = {},
        .delegate = plain,
    };

    host.set_items({request});
    auto* before = host.node_for(request.id);
    REQUIRE(before != nullptr);

    auto replacement = std::make_shared<PlainDelegate>();
    auto changed = request;
    changed.delegate = replacement;
    host.set_items({changed});
    CHECK(host.item_count() == 1);
    CHECK(host.node_for(request.id) != before);
}

TEST_CASE(
    "DataTable recreates delegate items after tree re-entry",
    "[data-table][2b][item-host][binding]"
) {
    class ScopeDelegate final: public widget::IItemDelegate {
    public:
        int create_calls = 0;
        int effect_runs = 0;

        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            ++create_calls;
            (void)ui.effect([this] { ++effect_runs; });
            return widget::Label::create(ui.graph(), "scope");
        }
        void update_item(scene::NanControl&, const widget::ItemBinding&) override {}
    };

    Fixture fixture;
    TestModel model;
    model.reset({"a"}, 1);
    auto delegate = std::make_shared<ScopeDelegate>();
    auto column = widget::custom_column(
        column_id("scope"),
        "Scope",
        widget::TableColumn {.width = 40.0F},
        delegate
    );
    auto table = widget::DataTable::create(fixture.ui, {column});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));

    auto tree = mount(fixture, table);
    REQUIRE(delegate->create_calls == 1);
    const int runs_before_exit = delegate->effect_runs;
    tree->set_root(nullptr);
    tree->set_root(table);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));

    CHECK(delegate->create_calls == 2);
    CHECK(delegate->effect_runs > runs_before_exit);
}

TEST_CASE("DataTable custom delegate and custom_column", "[data-table][2b][delegate]") {
    class PrefixDelegate final: public widget::IItemDelegate {
    public:
        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), "prefix");
        }
        void update_item(scene::NanControl& control, const widget::ItemBinding&) override {
            static_cast<widget::Label&>(control).set_text("prefix");
        }
    };

    Fixture fixture;
    TestModel model;
    model.reset({}, 1);
    auto column = widget::custom_column(
        column_id("custom"),
        "Custom",
        widget::TableColumn {.width = 100.0F},
        std::make_shared<PrefixDelegate>()
    );
    auto table = widget::DataTable::create(fixture.ui, {column});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    model.reset({"a", "b"}, 1);
    CHECK(table->item_count() == 2);
    auto* label = static_cast<widget::Label*>(table->cell_node(cell(1, "custom")));
    REQUIRE(label != nullptr);
    CHECK(label->text() == "prefix");
}

TEST_CASE(
    "TypedItemDelegate rechecks the model token at every call",
    "[data-table][2b][delegate]"
) {
    class TestTypedDelegate final: public widget::TypedItemDelegate<TestModel> {
    public:
        auto
        create_typed(const widget::BuildContext& ui, const TestModel&, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), "typed");
        }
        void
        update_typed(scene::NanControl&, const TestModel&, const widget::ItemBinding&) override {}
    };

    Fixture fixture;
    OtherModel other;
    TestTypedDelegate delegate;
    const widget::ItemBinding other_binding {
        .model = other,
        .model_row = 0,
        .display_index = widget::ModelIndex {.row = 0, .column = 0},
        .row = widget::RowId {.value = 1},
        .column = nullptr,
        .state = {},
    };
    CHECK_THROWS_AS(delegate.create_item(fixture.ui, other_binding), std::invalid_argument);

    auto label = widget::Label::create(fixture.graph);
    CHECK_THROWS_AS(delegate.update_item(*label, other_binding), std::invalid_argument);
}
