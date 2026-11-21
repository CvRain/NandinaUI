// data_table_3b_tests — 批 3b 第一增量：指针/键盘网格选择、导航与表头语义。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/data_table.hpp>
#include <nandina/widget/label.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
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

    auto kind_column(const widget::SortStrategy sort = widget::SortStrategy::none)
        -> widget::DataTableColumn {
        auto column = widget::text_column<TestModel>(
            column_id("kind"),
            "Kind",
            [](const TestModel&, const std::size_t row) { return "k" + std::to_string(row); },
            widget::TableColumn {.width = 120.0F}
        );
        column.sort = sort;
        return column;
    }

    auto make_names(const std::size_t count) -> std::vector<std::string> {
        std::vector<std::string> names;
        names.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            names.push_back("n" + std::to_string(index));
        }
        return names;
    }

    auto mount(Fixture&, const std::shared_ptr<widget::DataTable>& table)
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

    auto make_table(
        Fixture& fixture,
        TestModel& model,
        const std::size_t rows = 100,
        const widget::SelectionMode mode = widget::SelectionMode::multiple
    ) -> std::shared_ptr<widget::DataTable> {
        model.reset(make_names(rows), 1);
        auto table = widget::DataTable::create(
            fixture.ui,
            {name_column(widget::SortStrategy::local), kind_column()},
            mode
        );
        table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
        return table;
    }

    void click_node(
        scene::NanSceneTree& tree,
        scene::NanControl& node,
        const scene::KeyModifiers mods = {}
    ) {
        const auto bounds = node.global_bounds();
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent {
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                foundation::NanPoint(
                    (bounds.get_left() + bounds.get_right()) * 0.5F,
                    (bounds.get_top() + bounds.get_bottom()) * 0.5F
                ),
                mods,
            }
        );
    }

    void click_cell(
        scene::NanSceneTree& tree,
        const std::shared_ptr<widget::DataTable>& table,
        const widget::CellId& target,
        const scene::KeyModifiers mods = {}
    ) {
        auto* node = table->cell_node(target);
        REQUIRE(node != nullptr);
        click_node(tree, *node, mods);
    }

    void press_key(
        scene::NanSceneTree& tree,
        const foundation::LogicalKey key,
        const scene::KeyModifiers mods = {}
    ) {
        tree.dispatch_key(scene::KeyEvent(key, scene::KeyEvent::Action::press, mods));
    }
} // namespace

TEST_CASE("DataTable pointer click selects a cell and takes focus", "[data-table][3b][input]") {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model);
    auto tree = mount(fixture, table);

    click_cell(*tree, table, cell(1, "kind"));
    CHECK(table->selection().selected() == std::vector<widget::CellId> {cell(1, "kind")});
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(1, "kind")));
    CHECK(table->selection().anchor() == std::optional<widget::CellId>(cell(1, "kind")));
    CHECK(table->navigation().current().has_value());
    CHECK(table->navigation().current()->row == 0);
    CHECK(table->navigation().current()->column == 1);
    CHECK(tree->focused_node() == table.get());

    // 普通点击独占选中：只保留新单元。
    click_cell(*tree, table, cell(3, "name"));
    CHECK(table->selection().selected() == std::vector<widget::CellId> {cell(3, "name")});
}

TEST_CASE(
    "DataTable shift-click extends a cell rectangle and ctrl-click toggles",
    "[data-table][3b][input]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model);
    auto tree = mount(fixture, table);

    click_cell(*tree, table, cell(1, "name"));
    click_cell(*tree, table, cell(3, "kind"), scene::KeyModifiers {.shift = true});

    const std::vector<widget::CellId> expected {
        cell(1, "name"),
        cell(1, "kind"),
        cell(2, "name"),
        cell(2, "kind"),
        cell(3, "name"),
        cell(3, "kind"),
    };
    CHECK(table->selection().selected() == expected);
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(3, "kind")));
    CHECK(table->selection().anchor() == std::optional<widget::CellId>(cell(1, "name")));

    // ctrl 点击已选单元 = 取消该单元；current 仍落在目标。
    click_cell(*tree, table, cell(2, "name"), scene::KeyModifiers {.ctrl = true});
    CHECK_FALSE(table->selection().is_selected(cell(2, "name")));
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(2, "name")));
}

TEST_CASE(
    "DataTable keyboard moves current, keeps multiple selection and reveals rows",
    "[data-table][3b][input][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model);
    auto tree = mount(fixture, table);

    click_cell(*tree, table, cell(1, "name"));
    for (int step = 0; step < 15; ++step) {
        press_key(*tree, foundation::LogicalKey::down);
    }
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));

    REQUIRE(table->navigation().current().has_value());
    CHECK(table->navigation().current()->row == 15);
    CHECK(table->navigation().current()->column == 0);
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(16, "name")));
    // multiple 模式：移动 current 不清空已有选择。
    CHECK(table->selection().selected() == std::vector<widget::CellId> {cell(1, "name")});
    // 目标行已滚入窗口并物化。
    CHECK(table->scroll_offset() > 0.0F);
    CHECK(table->cell_node(cell(16, "name")) != nullptr);

    press_key(*tree, foundation::LogicalKey::right);
    CHECK(table->navigation().current()->column == 1);
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(16, "kind")));
}

TEST_CASE(
    "DataTable single mode selection follows keyboard current",
    "[data-table][3b][input][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model, 20, widget::SelectionMode::single);
    auto tree = mount(fixture, table);

    click_cell(*tree, table, cell(1, "name"));
    press_key(*tree, foundation::LogicalKey::down);
    CHECK(table->selection().selected() == std::vector<widget::CellId> {cell(2, "name")});
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(2, "name")));
}

TEST_CASE(
    "DataTable keyboard Home/End/PageUp/PageDown stay inside the row and page",
    "[data-table][3b][input][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model);
    auto tree = mount(fixture, table);

    click_cell(*tree, table, cell(1, "name"));
    press_key(*tree, foundation::LogicalKey::end);
    CHECK(table->navigation().current()->column == 1);
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(1, "kind")));
    press_key(*tree, foundation::LogicalKey::home);
    CHECK(table->navigation().current()->column == 0);

    press_key(*tree, foundation::LogicalKey::page_down);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->navigation().current()->row == 9); // 视口 268 / 行高 32 → 9 行
    press_key(*tree, foundation::LogicalKey::page_up);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->navigation().current()->row == 0);
}

TEST_CASE(
    "DataTable keyboard seeds current when navigation has none",
    "[data-table][3b][input][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model);
    auto tree = mount(fixture, table);

    // 未点击时无 current；首个方向键播种到当前窗口首行首列。
    tree->set_focus(table.get());
    press_key(*tree, foundation::LogicalKey::down);
    REQUIRE(table->navigation().current().has_value());
    CHECK(table->navigation().current()->row == 0);
    CHECK(table->navigation().current()->column == 0);
    CHECK(table->selection().selected() == std::vector<widget::CellId> {cell(1, "name")});
}

TEST_CASE(
    "DataTable only direction keys seed an empty navigation state",
    "[data-table][3b][input][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model);
    auto tree = mount(fixture, table);

    tree->set_focus(table.get());
    press_key(*tree, foundation::LogicalKey::home);
    press_key(*tree, foundation::LogicalKey::end);
    press_key(*tree, foundation::LogicalKey::page_down);
    CHECK_FALSE(table->navigation().current().has_value());
    CHECK(table->selection().selected().empty());

    press_key(*tree, foundation::LogicalKey::right);
    REQUIRE(table->navigation().current().has_value());
    CHECK(table->navigation().current()->row == 0);
    CHECK(table->navigation().current()->column == 0);
    CHECK(table->selection().selected() == std::vector<widget::CellId> {cell(1, "name")});
}

TEST_CASE(
    "DataTable consumes boundary navigation without changing selection",
    "[data-table][3b][input][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model, 20, widget::SelectionMode::single);
    auto tree = mount(fixture, table);

    click_cell(*tree, table, cell(1, "name"));
    press_key(*tree, foundation::LogicalKey::up);
    press_key(*tree, foundation::LogicalKey::left);
    CHECK(table->navigation().current()->row == 0);
    CHECK(table->navigation().current()->column == 0);
    CHECK(table->selection().selected() == std::vector<widget::CellId> {cell(1, "name")});
}

TEST_CASE(
    "DataTable keyboard shift extends from the selection anchor",
    "[data-table][3b][input][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model);
    auto tree = mount(fixture, table);

    click_cell(*tree, table, cell(1, "name"));
    press_key(*tree, foundation::LogicalKey::down, scene::KeyModifiers {.shift = true});
    press_key(*tree, foundation::LogicalKey::down, scene::KeyModifiers {.shift = true});

    const std::vector<widget::CellId> expected {
        cell(1, "name"),
        cell(2, "name"),
        cell(3, "name"),
    };
    CHECK(table->selection().selected() == expected);
    CHECK(table->selection().current() == std::optional<widget::CellId>(cell(3, "name")));
    CHECK(table->selection().anchor() == std::optional<widget::CellId>(cell(1, "name")));
}

TEST_CASE(
    "DataTable header is focusable and keyboard activation sorts",
    "[data-table][3b][header][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    model.reset(make_names(20), 1);
    auto table = widget::DataTable::create(
        fixture.ui,
        {name_column(widget::SortStrategy::local), kind_column(widget::SortStrategy::external)}
    );
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    std::vector<widget::SortRequest> requests;
    auto subscription =
        table->sort_requested().subscribe([&requests](const widget::SortRequest& request) {
            requests.push_back(request);
        });

    auto* header_cell = table->header_node(column_id("name"));
    REQUIRE(header_cell != nullptr);
    auto* header = static_cast<scene::NanControl*>(header_cell->parent());
    REQUIRE(header != nullptr);
    tree->set_focus(header);
    REQUIRE(tree->focused_node() == header);
    REQUIRE(header->child_count() == 2);

    // Enter 触发焦点列表头（local 列）：排序状态改变，但不发 external 请求。
    press_key(*tree, foundation::LogicalKey::enter);
    CHECK(requests.empty());
    REQUIRE(table->sort_spec().has_value());
    CHECK(table->sort_spec()->column == column_id("name"));
    CHECK(table->sort_spec()->direction == widget::SortDirection::ascending);

    // 右移焦点到第二列（external）；Space 以 user_header_click 发出请求。
    press_key(*tree, foundation::LogicalKey::right);
    press_key(*tree, foundation::LogicalKey::space);
    REQUIRE(requests.size() == 1);
    CHECK(requests.back().reason == widget::SortRequestReason::user_header_click);
    REQUIRE(requests.back().sort.has_value());
    CHECK(requests.back().sort->column == column_id("kind"));
    CHECK(requests.back().sort->direction == widget::SortDirection::ascending);
}

TEST_CASE(
    "DataTable header focus follows a column across reordering",
    "[data-table][3b][header][keyboard]"
) {
    Fixture fixture;
    TestModel model;
    model.reset(make_names(20), 1);
    auto table = widget::DataTable::create(
        fixture.ui,
        {kind_column(widget::SortStrategy::external), name_column(widget::SortStrategy::local)}
    );
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    auto* header_cell = table->header_node(column_id("kind"));
    REQUIRE(header_cell != nullptr);
    auto* header = static_cast<scene::NanControl*>(header_cell->parent());
    REQUIRE(header != nullptr);
    tree->set_focus(header);
    press_key(*tree, foundation::LogicalKey::right);

    // Focus "name" at the second visible column, then move it to the first. Focus must
    // follow its ColumnId rather than remaining at the old numeric index.
    table->set_columns(
        {name_column(widget::SortStrategy::local), kind_column(widget::SortStrategy::external)}
    );
    REQUIRE(header->child_count() == 2);
    CHECK(header->get_child(0) == table->header_node(column_id("name")));
    CHECK(header->get_child(1) == table->header_node(column_id("kind")));
    std::vector<widget::SortRequest> requests;
    auto subscription =
        table->sort_requested().subscribe([&requests](const widget::SortRequest& request) {
            requests.push_back(request);
        });
    press_key(*tree, foundation::LogicalKey::enter);

    CHECK(requests.empty()); // focused name is local sort; old index would hit kind/external
}

TEST_CASE("DataTable exposes table and header-column semantics", "[data-table][3b][semantics]") {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model, 20);
    auto tree = mount(fixture, table);

    REQUIRE(tree->update_semantics());
    const auto* root = tree->semantics_tree().find(table->semantics_id());
    REQUIRE(root != nullptr);
    CHECK(root->properties.role == semantics::Role::table);
    CHECK(root->properties.state.focusable);

    tree->set_focus(table.get());
    REQUIRE(tree->update_semantics());
    root = tree->semantics_tree().find(table->semantics_id());
    REQUIRE(root != nullptr);
    CHECK(root->properties.state.focused);

    const semantics::Node* header_row = nullptr;
    for (const auto& child: root->children) {
        if (child.properties.role == semantics::Role::row) {
            header_row = &child;
            break;
        }
    }
    REQUIRE(header_row != nullptr);
    REQUIRE(header_row->children.size() == 2);
    CHECK(header_row->children[0].properties.role == semantics::Role::column_header);
    CHECK(header_row->children[1].properties.role == semantics::Role::column_header);

    auto* header_cell = table->header_node(column_id("name"));
    REQUIRE(header_cell != nullptr);
    tree->set_focus(static_cast<scene::NanControl*>(header_cell->parent()));
    REQUIRE(tree->update_semantics());
    root = tree->semantics_tree().find(table->semantics_id());
    REQUIRE(root != nullptr);
    CHECK_FALSE(root->properties.state.focused);
    header_row = nullptr;
    for (const auto& child: root->children) {
        if (child.properties.role == semantics::Role::row) {
            header_row = &child;
            break;
        }
    }
    REQUIRE(header_row != nullptr);
    CHECK(header_row->properties.state.focused);

    // 排序后 aria-sort 通过 semantics value 暴露。
    REQUIRE(table->set_sort(
        widget::SortSpec {
            .column = column_id("name"),
            .direction = widget::SortDirection::ascending
        }
    ));
    CHECK(tree->semantics_dirty());
    REQUIRE(tree->update_semantics());
    root = tree->semantics_tree().find(table->semantics_id());
    REQUIRE(root != nullptr);
    bool saw_ascending = false;
    for (const auto& child: root->children) {
        if (child.properties.role != semantics::Role::row) {
            continue;
        }
        for (const auto& column_header: child.children) {
            saw_ascending = saw_ascending || column_header.properties.value == "ascending";
        }
    }
    CHECK(saw_ascending);
}

TEST_CASE(
    "DataTable does not select when clicking the header or outside the body",
    "[data-table][3b][input]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_table(fixture, model, 20);
    auto tree = mount(fixture, table);

    auto* header = table->header_node(column_id("name"));
    REQUIRE(header != nullptr);
    click_node(*tree, *header);
    CHECK(table->selection().selected().empty());

    tree->dispatch_mouse_button(
        scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(-20.0F, -20.0F),
        }
    );
    CHECK(table->selection().selected().empty());
}

TEST_CASE(
    "DataTable zero-width columns are not pointer targets",
    "[data-table][3b][input][layout]"
) {
    Fixture fixture;
    TestModel model;
    model.reset(make_names(20), 1);
    auto zero = widget::text_column<TestModel>(
        column_id("zero"),
        "Zero",
        [](const TestModel&, std::size_t) { return std::string("zero"); },
        widget::TableColumn {.width = 0.0F}
    );
    auto table = widget::DataTable::create(fixture.ui, {zero});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    tree->dispatch_mouse_button(
        scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(0.0F, 120.0F),
        }
    );
    CHECK(table->selection().selected().empty());
    CHECK_FALSE(table->selection().current().has_value());
}
