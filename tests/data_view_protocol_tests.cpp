// Direct unit tests for the batch 2A data-view protocol layer:
// identities, row model notifications/tokens, table selection model, grid navigation.
#include <nandina/widget/data_identity.hpp>
#include <nandina/widget/grid_navigation.hpp>
#include <nandina/widget/row_model.hpp>
#include <nandina/widget/table_selection_model.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace widget = nandina::widget;

namespace
{
    auto cell(const std::uint64_t row, std::string column) -> widget::CellId {
        return widget::CellId {
            .row = widget::RowId {.value = row},
            .column = widget::ColumnId {.value = std::move(column)},
        };
    }

    class TestModel final: public widget::TypedRowModelBase<TestModel> {
    public:
        std::size_t rows = 0;

        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return rows;
        }
        [[nodiscard]] auto row_id(const std::size_t row) const -> widget::RowId override {
            return widget::RowId {.value = row + 1};
        }

        void fire_reset() {
            notify_reset();
        }
        void fire_data_changed(const widget::RowRange range) {
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
} // namespace

TEST_CASE("data identities use explicit default equality", "[data-view][protocol][identity]") {
    CHECK(widget::RowId {.value = 7} == widget::RowId {.value = 7});
    CHECK_FALSE(widget::RowId {.value = 7} == widget::RowId {.value = 8});

    CHECK(widget::ColumnId {.value = "title"} == widget::ColumnId {.value = "title"});
    CHECK_FALSE(widget::ColumnId {.value = "title"} == widget::ColumnId {.value = "body"});

    const auto lhs = cell(1, "title");
    const auto same = cell(1, "title");
    const auto other_column = cell(1, "body");
    const auto other_row = cell(2, "title");
    CHECK(lhs == same);
    CHECK_FALSE(lhs == other_column);
    CHECK_FALSE(lhs == other_row);

    CHECK(widget::ModelIndex {.row = 3, .column = 4} == widget::ModelIndex {.row = 3, .column = 4});
    CHECK_FALSE(
        widget::ModelIndex {.row = 3, .column = 4} == widget::ModelIndex {.row = 4, .column = 3}
    );
}

TEST_CASE("row model token is stable per type", "[data-view][protocol][row-model]") {
    TestModel first;
    TestModel second;
    OtherModel other;

    CHECK(first.model_type_token() == widget::model_type_token_of<TestModel>());
    CHECK(first.model_type_token() == second.model_type_token());
    CHECK(first.model_type_token() == first.model_type_token());

    CHECK(other.model_type_token() == widget::model_type_token_of<OtherModel>());
    CHECK_FALSE(first.model_type_token() == other.model_type_token());
}

TEST_CASE(
    "table selection model is neither copyable nor movable",
    "[data-view][protocol][selection]"
) {
    STATIC_REQUIRE(!std::is_copy_constructible_v<widget::TableSelectionModel>);
    STATIC_REQUIRE(!std::is_move_constructible_v<widget::TableSelectionModel>);
    STATIC_REQUIRE(!std::is_copy_assignable_v<widget::TableSelectionModel>);
    STATIC_REQUIRE(!std::is_move_assignable_v<widget::TableSelectionModel>);
}

TEST_CASE("row model base is neither copyable nor movable", "[data-view][protocol][row-model]") {
    STATIC_REQUIRE(!std::is_copy_constructible_v<widget::RowModelBase>);
    STATIC_REQUIRE(!std::is_move_constructible_v<widget::RowModelBase>);
    STATIC_REQUIRE(!std::is_copy_assignable_v<widget::RowModelBase>);
    STATIC_REQUIRE(!std::is_move_assignable_v<widget::RowModelBase>);
    STATIC_REQUIRE(!std::is_copy_constructible_v<TestModel>);
    STATIC_REQUIRE(!std::is_move_constructible_v<TestModel>);
}

TEST_CASE("row model reset fires synchronously", "[data-view][protocol][row-model]") {
    TestModel model;
    int resets = 0;
    auto subscription = model.reset_event().subscribe([&resets] { ++resets; });

    model.fire_reset();
    CHECK(resets == 1);
    model.fire_reset();
    CHECK(resets == 2);

    subscription.disconnect();
    model.fire_reset();
    CHECK(resets == 2);
}

TEST_CASE(
    "row model data_changed is synchronous and keeps identity",
    "[data-view][protocol][row-model]"
) {
    TestModel model;
    model.rows = 4;
    std::vector<widget::RowRange> received;
    auto subscription =
        model.data_changed_event().subscribe([&received](const widget::RowRange range) {
            received.push_back(range);
        });

    std::vector<widget::RowId> before;
    for (std::size_t row = 0; row < model.row_count(); ++row) {
        before.push_back(model.row_id(row));
    }

    model.fire_data_changed({.first = 1, .last_exclusive = 3});

    REQUIRE(received.size() == 1);
    CHECK(received.front() == widget::RowRange {.first = 1, .last_exclusive = 3});
    CHECK(model.row_count() == 4);
    for (std::size_t row = 0; row < model.row_count(); ++row) {
        CHECK(model.row_id(row) == before[row]);
    }
}

TEST_CASE("row model drops empty data_changed ranges", "[data-view][protocol][row-model]") {
    TestModel model;
    model.rows = 4;
    int events = 0;
    auto subscription =
        model.data_changed_event().subscribe([&events](widget::RowRange) { ++events; });

    model.fire_data_changed({.first = 0, .last_exclusive = 0});
    model.fire_data_changed({.first = 2, .last_exclusive = 2});
    model.fire_data_changed({.first = 4, .last_exclusive = 4});
    CHECK(events == 0);
}

TEST_CASE("row model rejects invalid data_changed ranges", "[data-view][protocol][row-model]") {
    TestModel model;
    model.rows = 4;
    int events = 0;
    auto subscription =
        model.data_changed_event().subscribe([&events](widget::RowRange) { ++events; });

    CHECK_THROWS_AS(
        model.fire_data_changed({.first = 3, .last_exclusive = 2}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        model.fire_data_changed({.first = 0, .last_exclusive = 5}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        model.fire_data_changed({.first = 9, .last_exclusive = 9}),
        std::invalid_argument
    );
    CHECK(events == 0);
}

TEST_CASE(
    "table selection model constructor validates behavior",
    "[data-view][protocol][selection]"
) {
    widget::TableSelectionModel model;
    CHECK(model.mode() == widget::SelectionMode::single);
    CHECK(model.behavior() == widget::SelectionBehavior::cells);
    CHECK_FALSE(model.current().has_value());
    CHECK_FALSE(model.anchor().has_value());
    CHECK(model.selected().empty());

    CHECK_THROWS_AS(
        widget::TableSelectionModel(widget::SelectionMode::single, widget::SelectionBehavior::rows),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        widget::TableSelectionModel(
            widget::SelectionMode::multiple,
            widget::SelectionBehavior::columns
        ),
        std::invalid_argument
    );
}

TEST_CASE("table selection replace normalizes extent", "[data-view][protocol][selection]") {
    widget::TableSelectionModel model(widget::SelectionMode::multiple);
    int changes = 0;
    auto subscription = model.changed().subscribe([&changes] { ++changes; });

    const auto target = cell(2, "c2");
    const std::vector extent {target, cell(1, "c1"), target, cell(3, "c3")};
    model.apply(widget::SelectionOp::replace, target, extent);

    const std::vector expected {target, cell(1, "c1"), cell(3, "c3")};
    CHECK(model.selected() == expected);
    CHECK(model.current() == std::optional(target));
    CHECK(model.anchor() == std::optional(target));
    CHECK(changes == 1);

    // 幂等重复：状态不变则不发事件。
    model.apply(widget::SelectionOp::replace, target, extent);
    CHECK(changes == 1);
}

TEST_CASE("table selection single mode keeps only the target", "[data-view][protocol][selection]") {
    widget::TableSelectionModel model;
    const auto target = cell(2, "c2");
    const std::vector extent {cell(1, "c1"), target, cell(3, "c3")};

    model.apply(widget::SelectionOp::replace, target, extent);
    CHECK(model.selected() == std::vector {target});
    CHECK(model.current() == std::optional(target));
    CHECK(model.anchor() == std::optional(target));

    model.apply(widget::SelectionOp::toggle, target, {});
    CHECK(model.selected().empty());
    CHECK(model.current() == std::optional(target));
    // anchor 定义是 shift 起点，允许指向未选中单元；取消选中后仍落在 target。
    CHECK(model.anchor() == std::optional(target));
}

TEST_CASE("table selection toggle and extend", "[data-view][protocol][selection]") {
    widget::TableSelectionModel model(widget::SelectionMode::multiple);
    const auto c1 = cell(1, "c1");
    const auto c2 = cell(2, "c2");
    const auto c3 = cell(3, "c3");

    model.apply(widget::SelectionOp::replace, c1, {});
    CHECK(model.selected() == std::vector {c1});

    model.apply(widget::SelectionOp::toggle, c2, {});
    CHECK(model.selected() == std::vector {c1, c2});
    CHECK(model.current() == std::optional(c2));
    CHECK(model.anchor() == std::optional(c2));

    model.apply(widget::SelectionOp::toggle, c1, {});
    CHECK(model.selected() == std::vector {c2});
    CHECK(model.anchor() == std::optional(c1));

    // extend 替换为 extent，anchor 保持上一次落点（可以指向未选中单元）。
    model.apply(widget::SelectionOp::replace, c3, {});
    const std::vector extent {c1, c2};
    model.apply(widget::SelectionOp::extend, c1, extent);
    CHECK(model.selected() == std::vector {c1, c2});
    CHECK(model.current() == std::optional(c1));
    CHECK(model.anchor() == std::optional(c3));
    CHECK_FALSE(model.is_selected(c3));
}

TEST_CASE("table selection rejects extent without the target", "[data-view][protocol][selection]") {
    widget::TableSelectionModel model(widget::SelectionMode::multiple);
    const auto c1 = cell(1, "c1");
    const auto c2 = cell(2, "c2");

    model.apply(widget::SelectionOp::replace, c1, {});
    const auto selected_before = model.selected();
    const auto current_before = model.current();
    const auto anchor_before = model.anchor();
    int changes = 0;
    auto subscription = model.changed().subscribe([&changes] { ++changes; });

    const std::vector bad_extent {c2, cell(3, "c3")};
    CHECK_THROWS_AS(
        model.apply(widget::SelectionOp::replace, c1, bad_extent),
        std::invalid_argument
    );
    CHECK(model.selected() == selected_before);
    CHECK(model.current() == current_before);
    CHECK(model.anchor() == anchor_before);
    CHECK(changes == 0);
}

TEST_CASE("table selection rejects invalid operations", "[data-view][protocol][selection]") {
    widget::TableSelectionModel model(widget::SelectionMode::multiple);
    const auto c1 = cell(1, "c1");
    model.apply(widget::SelectionOp::replace, c1, {});

    int changes = 0;
    auto subscription = model.changed().subscribe([&changes] { ++changes; });
    const auto selected_before = model.selected();
    const auto current_before = model.current();
    const auto anchor_before = model.anchor();

    CHECK_THROWS_AS(
        model.apply(static_cast<widget::SelectionOp>(99), c1, {}),
        std::invalid_argument
    );
    CHECK(model.selected() == selected_before);
    CHECK(model.current() == current_before);
    CHECK(model.anchor() == anchor_before);
    CHECK(changes == 0);
}

TEST_CASE("table selection clear and current are independent", "[data-view][protocol][selection]") {
    widget::TableSelectionModel model(widget::SelectionMode::multiple);
    const auto c1 = cell(1, "c1");
    const auto c2 = cell(2, "c2");
    int changes = 0;
    auto subscription = model.changed().subscribe([&changes] { ++changes; });

    model.apply(widget::SelectionOp::replace, c1, {});
    CHECK(changes == 1);

    model.set_current(c2);
    CHECK(model.current() == std::optional(c2));
    CHECK(model.selected() == std::vector {c1});
    CHECK(model.anchor() == std::optional(c1));
    CHECK(changes == 2);

    // 相同 current 幂等。
    model.set_current(c2);
    CHECK(changes == 2);

    model.clear_selection();
    CHECK(model.selected().empty());
    CHECK_FALSE(model.anchor().has_value());
    CHECK(model.current() == std::optional(c2));
    CHECK(changes == 3);

    model.clear_selection();
    CHECK(changes == 3);

    model.set_current(std::nullopt);
    CHECK_FALSE(model.current().has_value());
    CHECK(changes == 4);
}

TEST_CASE(
    "table selection retain prunes and is exception-atomic",
    "[data-view][protocol][selection]"
) {
    widget::TableSelectionModel model(widget::SelectionMode::multiple);
    const auto c1 = cell(1, "c1");
    const auto c2 = cell(2, "c2");
    const auto c3 = cell(3, "c3");

    const std::vector initial {c1, c2, c3};
    model.apply(widget::SelectionOp::replace, c1, initial);
    model.set_current(c2);
    model.apply(widget::SelectionOp::toggle, c3, {}); // c3 已选中 → 移除
    model.apply(widget::SelectionOp::toggle, c3, {}); // 再加回，current/anchor 都是 c3
    model.set_current(c2);
    // 现在 selected {c1,c2,c3}, current c2, anchor c3。
    CHECK(model.selected() == std::vector {c1, c2, c3});

    const auto keep_even_rows = [](const widget::CellId& id) { return id.row.value % 2 == 0; };
    model.retain(keep_even_rows);
    CHECK(model.selected() == std::vector {c2});
    CHECK(model.current() == std::optional(c2));
    // c3 被 keep 拒绝：anchor 与 selected 一起裁剪（anchor 可指向未选中单元，
    // 但不在保留 universe 内时必须清空）。
    CHECK_FALSE(model.anchor().has_value());

    // 恢复多个选中成员，让"先裁剪一部分再抛异常"能真正暴露非原子实现。
    const std::vector rebuilt {c1, c2, c3};
    model.apply(widget::SelectionOp::replace, c1, rebuilt);
    model.set_current(c2);

    int changes = 0;
    auto subscription = model.changed().subscribe([&changes] { ++changes; });
    const auto selected_before = model.selected();
    const auto current_before = model.current();
    const auto anchor_before = model.anchor();

    // 第 1 个 keep 返回 false（应被移除），第 2 个抛异常：实现必须一个都不提交。
    std::size_t calls = 0;
    const auto throwing_keep = [&calls](const widget::CellId&) {
        ++calls;
        if (calls == 2) {
            throw std::runtime_error("keep failed");
        }
        return false;
    };
    CHECK_THROWS_AS(model.retain(throwing_keep), std::runtime_error);
    CHECK(model.selected() == selected_before);
    CHECK(model.current() == current_before);
    CHECK(model.anchor() == anchor_before);
    CHECK(changes == 0);
}

TEST_CASE(
    "grid navigation stays empty until explicitly seeded",
    "[data-view][protocol][navigation]"
) {
    widget::GridNavigation navigation;
    CHECK_FALSE(navigation.current().has_value());
    CHECK_FALSE(navigation.scroll_to_index().has_value());

    navigation.set_current(widget::ModelIndex {.row = 0, .column = 0});
    CHECK_FALSE(navigation.current().has_value());
    CHECK_FALSE(navigation.move(widget::GridDirection::down));
    CHECK_FALSE(navigation.home());
    CHECK_FALSE(navigation.end());
    CHECK_FALSE(navigation.page_up());
    CHECK_FALSE(navigation.page_down());

    // 空 → 非空不自动播种 (0,0)。
    navigation.set_shape(3, 2);
    CHECK_FALSE(navigation.current().has_value());
    CHECK_FALSE(navigation.move(widget::GridDirection::right));
    CHECK_FALSE(navigation.scroll_to_index().has_value());

    navigation.set_current(widget::ModelIndex {.row = 1, .column = 1});
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 1, .column = 1}));
    CHECK(navigation.scroll_to_index() == std::optional<std::size_t>(1));
}

TEST_CASE(
    "grid navigation clamps current and moves without wrapping",
    "[data-view][protocol][navigation]"
) {
    widget::GridNavigation navigation;
    navigation.set_shape(3, 3);

    navigation.set_current(widget::ModelIndex {.row = 99, .column = 99});
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 2, .column = 2}));

    CHECK_FALSE(navigation.move(widget::GridDirection::down));
    CHECK_FALSE(navigation.move(widget::GridDirection::right));
    CHECK(navigation.move(widget::GridDirection::left));
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 2, .column = 1}));
    CHECK(navigation.move(widget::GridDirection::up));
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 1, .column = 1}));
    CHECK(navigation.move(widget::GridDirection::left));
    CHECK_FALSE(navigation.move(widget::GridDirection::left));

    navigation.set_current(widget::ModelIndex {.row = 0, .column = 0});
    CHECK_FALSE(navigation.move(widget::GridDirection::up));
    CHECK_FALSE(navigation.move(widget::GridDirection::left));
}

TEST_CASE(
    "grid navigation home and end stay on the current row",
    "[data-view][protocol][navigation]"
) {
    widget::GridNavigation navigation;
    navigation.set_shape(4, 3);
    navigation.set_current(widget::ModelIndex {.row = 2, .column = 1});

    CHECK(navigation.home());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 2, .column = 0}));
    CHECK(navigation.home()); // 已在端点仍视为已处理
    CHECK(navigation.end());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 2, .column = 2}));

    navigation.clear_current();
    CHECK_FALSE(navigation.home());
    CHECK_FALSE(navigation.end());
}

TEST_CASE("grid navigation pages keep the column", "[data-view][protocol][navigation]") {
    widget::GridNavigation navigation;
    navigation.set_shape(6, 2);
    navigation.set_page_rows(2);
    navigation.set_current(widget::ModelIndex {.row = 0, .column = 1});

    CHECK(navigation.page_down());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 2, .column = 1}));
    CHECK(navigation.page_down());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 4, .column = 1}));
    CHECK(navigation.page_down());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 5, .column = 1}));
    CHECK_FALSE(navigation.page_down());

    CHECK(navigation.page_up());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 3, .column = 1}));
    CHECK(navigation.page_up());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 1, .column = 1}));
    CHECK(navigation.page_up());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 0, .column = 1}));
    CHECK_FALSE(navigation.page_up());

    navigation.set_page_rows(0);
    CHECK_FALSE(navigation.page_down());
    CHECK_FALSE(navigation.page_up());
}

TEST_CASE("grid navigation zero dimensions clear current", "[data-view][protocol][navigation]") {
    widget::GridNavigation navigation;
    navigation.set_shape(3, 2);
    navigation.set_current(widget::ModelIndex {.row = 1, .column = 1});

    navigation.set_shape(3, 0);
    CHECK_FALSE(navigation.current().has_value());

    navigation.set_shape(3, 2);
    CHECK_FALSE(navigation.current().has_value());
    navigation.set_current(widget::ModelIndex {.row = 2, .column = 1});
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 2, .column = 1}));

    navigation.set_shape(0, 3);
    CHECK_FALSE(navigation.current().has_value());
}

TEST_CASE("grid navigation clamps width-only shape changes", "[data-view][protocol][navigation]") {
    widget::GridNavigation navigation;
    navigation.set_shape(5, 5);
    navigation.set_current(widget::ModelIndex {.row = 3, .column = 4});

    // 只缩小列数：row 保持不变，column 夹取。
    navigation.set_shape(5, 2);
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 3, .column = 1}));
}

TEST_CASE("grid navigation handles oversized page counts", "[data-view][protocol][navigation]") {
    widget::GridNavigation navigation;
    navigation.set_shape(3, 1);
    navigation.set_page_rows(100);
    navigation.set_current(widget::ModelIndex {.row = 0, .column = 0});

    CHECK(navigation.page_down());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 2, .column = 0}));
    CHECK_FALSE(navigation.page_down());

    CHECK(navigation.page_up());
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 0, .column = 0}));
    CHECK_FALSE(navigation.page_up());
}

TEST_CASE("grid navigation rejects invalid directions", "[data-view][protocol][navigation]") {
    widget::GridNavigation navigation;
    navigation.set_shape(3, 3);
    navigation.set_current(widget::ModelIndex {.row = 1, .column = 1});

    CHECK_FALSE(navigation.move(static_cast<widget::GridDirection>(99)));
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 1, .column = 1}));
}

TEST_CASE(
    "grid navigation shape changes clamp or clear current",
    "[data-view][protocol][navigation]"
) {
    widget::GridNavigation navigation;
    navigation.set_shape(5, 3);
    navigation.set_current(widget::ModelIndex {.row = 4, .column = 2});

    navigation.set_shape(2, 2);
    CHECK(navigation.current() == std::optional(widget::ModelIndex {.row = 1, .column = 1}));

    navigation.set_shape(0, 2);
    CHECK_FALSE(navigation.current().has_value());
    CHECK_FALSE(navigation.scroll_to_index().has_value());

    navigation.set_shape(3, 2);
    CHECK_FALSE(navigation.current().has_value());
}
