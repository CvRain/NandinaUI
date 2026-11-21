// data_table_3a_tests — 批 3a DataTable 内部虚拟化与窗口物化。
//
// DataTable 自持纵向 viewport：表头固定，body 只物化 UniformRowWindow 可见窗口内
// 的单元格。覆盖：窗口物化上限、顶部/中部/底部窗口、offset 夹取、滚动到行（含首布局
// 前挂起）、滚轮、reset/local sort/data_changed 后窗口重算、遍历期 pending 窗口同步、
// 表头位置固定、窗口外 cell_node 为空、无可见列/零视口。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/data_table.hpp>
#include <nandina/widget/label.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
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
        void change(const widget::RowRange range) {
            notify_data_changed(range);
        }
    };

    class UpdateCountingDelegate final: public widget::IItemDelegate {
    public:
        std::size_t create_calls = 0;
        std::size_t update_calls = 0;

        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            ++create_calls;
            return widget::Label::create(ui.graph());
        }

        void update_item(scene::NanControl&, const widget::ItemBinding&) override {
            ++update_calls;
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

    auto name_column() -> widget::DataTableColumn {
        return widget::text_column<TestModel>(
            column_id("name"),
            "Name",
            [](const TestModel& model, const std::size_t row) { return model.names.at(row); },
            widget::TableColumn {.flex = 1.0F}
        );
    }

    auto second_column() -> widget::DataTableColumn {
        return widget::text_column<TestModel>(
            column_id("kind"),
            "Kind",
            [](const TestModel&, const std::size_t row) { return "k" + std::to_string(row); },
            widget::TableColumn {.width = 80.0F}
        );
    }

    auto make_names(const std::size_t count, const std::string& prefix = "n")
        -> std::vector<std::string> {
        std::vector<std::string> names;
        names.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            names.push_back(prefix + std::to_string(index));
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

    // 400×300 视口，默认行高 32、表头 32 → body 268；可见行数 = 9。
    constexpr float kBodyHeight = 268.0F;
    constexpr float kRowHeight = 32.0F;
    constexpr std::size_t kVisibleRows = 9;
    constexpr std::size_t kVisibleCells = kVisibleRows * 2;

    auto make_large_table(Fixture& fixture, TestModel& model, const std::size_t rows = 1000)
        -> std::shared_ptr<widget::DataTable> {
        model.reset(make_names(rows), 1);
        auto table = widget::DataTable::create(fixture.ui, {name_column(), second_column()});
        table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
        return table;
    }

    void require_window(
        const std::shared_ptr<widget::DataTable>& table,
        const std::uint64_t first_id,
        const std::size_t rows,
        const std::size_t columns
    ) {
        INFO("expected window rows=" << rows << " columns=" << columns << " first_id=" << first_id);
        REQUIRE(table->item_count() == rows * columns);
        for (std::size_t index = 0; index < rows; ++index) {
            INFO("window index=" << index);
            REQUIRE(table->cell_node(cell(first_id + index, "name")) != nullptr);
            REQUIRE(table->cell_node(cell(first_id + index, "kind")) != nullptr);
        }
    }
} // namespace

TEST_CASE("DataTable materializes only the visible window", "[data-table][3a][virtualization]") {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    REQUIRE(table->row_count() == 1000);
    REQUIRE(table->column_count() == 2);
    REQUIRE(table->item_count() == kVisibleCells);

    require_window(table, 1, kVisibleRows, 2);
    CHECK(table->cell_node(cell(1 + kVisibleRows, "name")) == nullptr);
    CHECK(table->cell_node(cell(1000, "name")) == nullptr);

    // 表头固定：第一行单元格从表头下方开始，且不随滚动归零。
    auto* header = table->header_node(column_id("name"));
    REQUIRE(header != nullptr);
    CHECK(header->global_bounds().get_top() == Catch::Approx(0.0F));
    auto* first = table->cell_node(cell(1, "name"));
    REQUIRE(first != nullptr);
    CHECK(first->height() == Catch::Approx(kRowHeight));
    CHECK(first->global_bounds().get_top() == Catch::Approx(32.0F));
}

TEST_CASE(
    "DataTable window tracks top, middle and bottom offsets",
    "[data-table][3a][virtualization]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->maximum_scroll_offset() == Catch::Approx(1000.0F * kRowHeight - kBodyHeight));

    // 中部：offset 320 → 窗口 [10, 19)。
    table->set_scroll_offset(320.0F);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(320.0F));
    require_window(table, 11, kVisibleRows, 2); // display 行 10..18 → RowId 11..19
    CHECK(table->cell_node(cell(10, "name")) == nullptr); // display 行 9 已滑出
    CHECK(table->cell_node(cell(20, "name")) == nullptr); // display 行 19 尚未进入

    // 底部：offset = max → 窗口 [991, 1000)。
    table->set_scroll_offset(table->maximum_scroll_offset());
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(1000.0F * kRowHeight - kBodyHeight));
    require_window(table, 992, kVisibleRows, 2);
    CHECK(table->cell_node(cell(991, "name")) == nullptr);

    // 顶部：offset 归零 → 窗口 [0, 9)，中部/底部节点全部移除。
    table->set_scroll_offset(0.0F);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    require_window(table, 1, kVisibleRows, 2);
    CHECK(table->cell_node(cell(992, "name")) == nullptr);
}

TEST_CASE("DataTable clamps scroll offsets", "[data-table][3a][virtualization]") {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    table->set_scroll_offset(1.0e9F);
    CHECK(table->scroll_offset() == Catch::Approx(table->maximum_scroll_offset()));
    table->set_scroll_offset(-100.0F);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    table->set_scroll_offset(std::numeric_limits<float>::quiet_NaN());
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));

    // 内容不足一屏时没有可滚动空间。
    model.reset(make_names(3), 1);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->maximum_scroll_offset() == Catch::Approx(0.0F));
    table->set_scroll_offset(999.0F);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->item_count() == 6);
}

TEST_CASE(
    "DataTable scroll_to_row reveals and clamps the target row",
    "[data-table][3a][virtualization]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    // 行 500（display 行 499）：底部 16000 需要贴近 body 底边 268 → offset 15732。
    table->scroll_to_row(499);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(15732.0F));
    CHECK(table->cell_node(cell(500, "name")) != nullptr);

    // 最后一行：夹取到最大 offset。
    table->scroll_to_row(999);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(table->maximum_scroll_offset()));
    CHECK(table->cell_node(cell(1000, "name")) != nullptr);

    // 回到首行：贴顶。
    table->scroll_to_row(0);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->cell_node(cell(1, "name")) != nullptr);
    CHECK(table->cell_node(cell(1000, "name")) == nullptr);

    // 越界目标不改变状态。
    table->scroll_to_row(9999);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
}

TEST_CASE(
    "DataTable keyed reuse never creates off-window cells",
    "[data-table][3a][virtualization][item-host]"
) {
    class CountingDelegate final: public widget::IItemDelegate {
    public:
        int create_calls = 0;

        auto create_item(const widget::BuildContext& ui, const widget::ItemBinding&)
            -> std::shared_ptr<scene::NanControl> override {
            ++create_calls;
            return widget::Label::create(ui.graph());
        }
        void update_item(scene::NanControl&, const widget::ItemBinding&) override {}
    };

    Fixture fixture;
    TestModel model;
    model.reset(make_names(1000), 1);
    auto delegate = std::make_shared<CountingDelegate>();
    auto column = widget::custom_column(
        column_id("name"),
        "Name",
        widget::TableColumn {.flex = 1.0F},
        delegate
    );
    auto table = widget::DataTable::create(fixture.ui, {column});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);
    (void)tree;

    REQUIRE(delegate->create_calls == static_cast<int>(kVisibleRows));
    auto* row_one = table->cell_node(cell(2, "name"));
    REQUIRE(row_one != nullptr);

    table->set_scroll_offset(kRowHeight);
    CHECK(delegate->create_calls == static_cast<int>(kVisibleRows + 1));
    CHECK(table->item_count() == kVisibleRows);
    CHECK(table->cell_node(cell(1, "name")) == nullptr);
    CHECK(table->cell_node(cell(2, "name")) == row_one); // overlap row keyed reuse
    CHECK(table->cell_node(cell(10, "name")) != nullptr);
    CHECK(table->cell_node(cell(11, "name")) == nullptr);
}

TEST_CASE(
    "DataTable pending scroll is superseded by explicit viewport state",
    "[data-table][3a][virtualization]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);

    table->scroll_to_row(499);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    table->set_scroll_offset(0.0F);
    auto tree = mount(fixture, table);
    (void)tree;
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->cell_node(cell(1, "name")) != nullptr);

    auto hidden = name_column();
    hidden.visible = false;
    auto hidden_table = make_large_table(fixture, model);
    hidden_table->scroll_to_row(499);
    hidden_table->set_columns({hidden});
    auto hidden_tree = mount(fixture, hidden_table);
    (void)hidden_tree;
    CHECK(hidden_table->column_count() == 0);
    CHECK(hidden_table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(hidden_table->item_count() == 0);
}

TEST_CASE(
    "DataTable defers a pre-layout scroll target until the viewport exists",
    "[data-table][3a][virtualization]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);

    table->scroll_to_row(499); // 尚未挂载/布局，视口高度未知 → 挂起
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));

    auto tree = mount(fixture, table);
    CHECK(table->scroll_offset() == Catch::Approx(15732.0F));
    CHECK(table->cell_node(cell(500, "name")) != nullptr);
    CHECK(table->cell_node(cell(1, "name")) == nullptr);
}

TEST_CASE("DataTable wheel scrolls the body viewport", "[data-table][3a][input]") {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    tree->dispatch_mouse_wheel(
        scene::MouseWheelEvent(
            foundation::NanPoint(100.0F, 120.0F),
            foundation::NanPoint(0.0F, -1.0F)
        )
    );
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(40.0F));
    require_window(table, 2, kVisibleRows, 2); // 窗口 [1, 10) → RowId 2..10

    tree->dispatch_mouse_wheel(
        scene::MouseWheelEvent(
            foundation::NanPoint(100.0F, 120.0F),
            foundation::NanPoint(0.0F, 1.0F)
        )
    );
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    require_window(table, 1, kVisibleRows, 2);

    // 顶部继续向上滚：offset 不变；底部继续向下滚：夹取到最大 offset。
    const auto wheel = [&](const float dy) {
        tree->dispatch_mouse_wheel(
            scene::MouseWheelEvent(
                foundation::NanPoint(100.0F, 120.0F),
                foundation::NanPoint(0.0F, dy)
            )
        );
    };
    wheel(1.0F);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    wheel(-1.0e6F);
    CHECK(table->scroll_offset() == Catch::Approx(table->maximum_scroll_offset()));
    require_window(table, 992, kVisibleRows, 2); // 窗口 [991, 1000)
}

TEST_CASE(
    "DataTable rebuilds the window after reset and clamps the old offset",
    "[data-table][3a][virtualization][binding]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    table->set_scroll_offset(table->maximum_scroll_offset());
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    require_window(table, 992, kVisibleRows, 2);
    CHECK(table->cell_node(cell(1000, "name")) != nullptr);

    // reset 后内容变短：旧 offset 夹取到新最大值，旧窗口节点全部移除。
    model.reset(make_names(5), 1);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->row_count() == 5);
    CHECK(table->maximum_scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->item_count() == 10);
    CHECK(table->cell_node(cell(1000, "name")) == nullptr);
    CHECK(table->cell_node(cell(992, "name")) == nullptr);
    require_window(table, 1, 5, 2);
}

TEST_CASE(
    "DataTable defers window sync behind a same-frame model reset",
    "[data-table][3a][virtualization][binding]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        table->set_scroll_offset(table->maximum_scroll_offset());
        model.reset(make_names(5), 1);
        REQUIRE(table->has_pending_notifications());
    }

    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK_FALSE(table->has_pending_notifications());
    CHECK(table->row_count() == 5);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->item_count() == 10);
    require_window(table, 1, 5, 2);
}

TEST_CASE(
    "DataTable rebuilds the window after local sort",
    "[data-table][3a][virtualization][sort]"
) {
    Fixture fixture;
    TestModel model;
    auto column = name_column();
    widget::set_local_sort<TestModel>(
        column,
        [](const TestModel& lhs, const std::size_t left, const std::size_t right) {
            return lhs.names.at(left) <=> lhs.names.at(right);
        }
    );
    model.reset(make_names(1000), 1);
    auto table = widget::DataTable::create(fixture.ui, {column, second_column()});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);
    require_window(table, 1, kVisibleRows, 2);

    REQUIRE(table->set_sort(
        widget::SortSpec {
            .column = column_id("name"),
            .direction = widget::SortDirection::descending
        }
    ));
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    REQUIRE(table->model_row_for_display(0) == std::optional<std::size_t>(999));
    CHECK(table->item_count() == kVisibleCells);
    // 倒序后首个 display 行是 model 行 999 → RowId 1000。
    CHECK(table->cell_node(cell(1000, "name")) != nullptr);
    CHECK(table->cell_node(cell(1, "name")) == nullptr);
    require_window(table, 1000 - (kVisibleRows - 1), kVisibleRows, 2);

    // data_changed 后 local sort 重排，窗口内单元格数量不膨胀。
    model.names[500] = "zzzz";
    model.change({.first = 500, .last_exclusive = 501});
    CHECK(table->item_count() == kVisibleCells);
    CHECK(table->cell_node(cell(501, "name")) != nullptr);
}

TEST_CASE(
    "DataTable applies a traversal-time window sync after layout",
    "[data-table][3a][virtualization][binding]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        table->set_scroll_offset(320.0F);
        REQUIRE(table->has_pending_notifications());
        // 遍历期不能增删节点：旧窗口还在。
        CHECK(table->cell_node(cell(1, "name")) != nullptr);
    }
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK_FALSE(table->has_pending_notifications());
    CHECK(table->scroll_offset() == Catch::Approx(320.0F));
    CHECK(table->item_count() == kVisibleCells);
    CHECK(table->cell_node(cell(1, "name")) == nullptr);
    require_window(table, 11, kVisibleRows, 2);
}

TEST_CASE(
    "DataTable clears deferred window sync across tree exit",
    "[data-table][3a][virtualization][binding]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        table->set_scroll_offset(320.0F);
    }
    REQUIRE(table->has_pending_notifications());
    tree->set_root(nullptr);
    CHECK_FALSE(table->has_pending_notifications());

    tree->set_root(table);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() == Catch::Approx(320.0F));
    require_window(table, 11, kVisibleRows, 2);
}

TEST_CASE(
    "DataTable applies a traversal-time reset window after layout",
    "[data-table][3a][virtualization][binding]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    table->set_scroll_offset(table->maximum_scroll_offset());
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->cell_node(cell(1000, "name")) != nullptr);

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        model.reset(make_names(50), 1);
        REQUIRE(table->has_pending_notifications());
    }
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK_FALSE(table->has_pending_notifications());
    CHECK(table->row_count() == 50);
    CHECK(table->scroll_offset() == Catch::Approx(50.0F * kRowHeight - kBodyHeight));
    CHECK(table->item_count() == kVisibleCells);
    CHECK(table->cell_node(cell(1000, "name")) == nullptr);
    // 夹取后的窗口展示最后 9 行：display 41..49 → RowId 42..50。
    require_window(table, 42, kVisibleRows, 2);
}

TEST_CASE(
    "DataTable keeps the header fixed while rows scroll",
    "[data-table][3a][virtualization]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    auto* header = table->header_node(column_id("name"));
    REQUIRE(header != nullptr);
    REQUIRE(table->cell_node(cell(1, "name")) != nullptr);
    const auto header_top = header->global_bounds().get_top();
    const auto header_height = header->height();

    table->set_scroll_offset(320.0F);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));

    REQUIRE(table->header_node(column_id("name")) == header);
    CHECK(header->global_bounds().get_top() == Catch::Approx(header_top));
    CHECK(header->height() == Catch::Approx(header_height));
    // display 行 10 的顶部仍在表头正下方（offset 恰好 10 行）。
    auto* row = table->cell_node(cell(11, "name"));
    REQUIRE(row != nullptr);
    CHECK(row->global_bounds().get_top() == Catch::Approx(header_top + header_height));
}

TEST_CASE(
    "DataTable with no visible columns has an empty window",
    "[data-table][3a][virtualization]"
) {
    Fixture fixture;
    TestModel model;
    model.reset(make_names(1000), 1);
    auto hidden = name_column();
    hidden.visible = false;
    auto table = widget::DataTable::create(fixture.ui, {hidden, second_column()});
    table->set_columns({hidden});
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);

    CHECK(table->column_count() == 0);
    CHECK(table->item_count() == 0);
    CHECK(table->maximum_scroll_offset() == Catch::Approx(0.0F));
    table->set_scroll_offset(100.0F);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));
    CHECK(table->cell_node(cell(1, "kind")) == nullptr);
}

TEST_CASE(
    "DataTable recalculates the body viewport for zero height and header changes",
    "[data-table][3a][virtualization][layout]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = std::make_unique<scene::NanSceneTree>();
    tree->set_root(table);

    // Body height = 20 - 32 header => zero. No row may materialize and offset must stay 0.
    (void)tree->layout_root(foundation::NanSize(400.0F, 20.0F));
    CHECK(table->item_count() == 0);
    CHECK(table->maximum_scroll_offset() == Catch::Approx(0.0F));
    table->set_scroll_offset(100.0F);
    CHECK(table->scroll_offset() == Catch::Approx(0.0F));

    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    REQUIRE(table->item_count() == kVisibleCells);
    table->set_show_header(false);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));

    // Without a header the full 300 body fits 10 rows; first row starts at the table top.
    CHECK(table->item_count() == 10 * 2);
    REQUIRE(table->cell_node(cell(1, "name")) != nullptr);
    CHECK(table->cell_node(cell(1, "name"))->global_bounds().get_top() == Catch::Approx(0.0F));

    table->set_show_header(true);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->item_count() == kVisibleCells);
    CHECK(table->cell_node(cell(1, "name"))->global_bounds().get_top() == Catch::Approx(32.0F));
}

TEST_CASE(
    "DataTable keeps offset bounded when viewport parameters change",
    "[data-table][3a][virtualization][layout]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    auto tree = mount(fixture, table);

    table->set_scroll_offset(table->maximum_scroll_offset());
    REQUIRE(table->scroll_offset() == Catch::Approx(31732.0F));

    // Hiding the header grows the body by one row; the old bottom offset is now invalid.
    table->set_show_header(false);
    CHECK(table->maximum_scroll_offset() == Catch::Approx(31700.0F));
    CHECK(table->scroll_offset() == Catch::Approx(31700.0F));

    // Smaller rows reduce the content extent and therefore the maximum offset; the old
    // bottom position must be clamped before the next layout.
    table->set_show_header(true);
    table->set_row_height(8.0F);
    CHECK(table->scroll_offset() <= table->maximum_scroll_offset());
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(table->scroll_offset() <= table->maximum_scroll_offset());
}

TEST_CASE(
    "DataTable survives layout pass before deferred shrinking reset flushes",
    "[data-table][3a][virtualization][binding][regression]"
) {
    Fixture fixture;
    TestModel model;
    model.reset(make_names(9), 1);
    auto delegate = std::make_shared<UpdateCountingDelegate>();
    auto table = widget::DataTable::create(
        fixture.ui,
        {
            widget::custom_column(
                column_id("name"),
                "Name",
                widget::TableColumn {.flex = 1.0F},
                delegate
            ),
        }
    );
    table->set_model(std::shared_ptr<widget::IRowModel>(&model, [](widget::IRowModel*) {}));
    auto tree = mount(fixture, table);
    tree->set_theme_manager(fixture.themes);
    REQUIRE(table->item_count() == 9);
    delegate->create_calls = 0;
    delegate->update_calls = 0;

    {
        auto phase = tree->enter_phase(scene::FramePhase::input);
        // Shrink model below the materialized window (9 rows -> 4 rows) during input traversal
        // while the same frame also marks layout dirty (e.g. sibling/inspector update).
        model.reset(make_names(4), 100);
        table->mark_layout_dirty();
        REQUIRE(table->has_pending_notifications());
        // Theme refresh during the pending reset must not index old display rows in the new model.
        fixture.themes.set_text_scaling(1.25F);
    }

    // Pass 1 of layout_root() runs DataTable::on_layout() BEFORE post_layout() flushes the
    // deferred reset; stale display_order_ must not index into the shrunken model.
    REQUIRE_NOTHROW((void)tree->layout_root(foundation::NanSize(300.0F, 200.0F)));
    CHECK_FALSE(table->has_pending_notifications());
    CHECK(table->row_count() == 4);
    CHECK(table->item_count() == 4);
    CHECK(table->cell_node(cell(100, "name")) != nullptr);
    CHECK(table->cell_node(cell(1, "name")) == nullptr);
    // The consumed reset must not run a second stale window-sync materialization.
    CHECK(delegate->create_calls == 4);
    CHECK(delegate->update_calls == 4);
}

TEST_CASE(
    "DataTable detached layout_to materializes a newly visible window",
    "[data-table][3a][virtualization][detached]"
) {
    Fixture fixture;
    TestModel model;
    auto table = make_large_table(fixture, model);
    REQUIRE(table->item_count() == 0);

    table->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 400.0F, 300.0F));
    CHECK(table->item_count() == kVisibleCells);
    CHECK(table->cell_node(cell(1, "name")) != nullptr);
}
