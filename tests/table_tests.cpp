// Static Table: track geometry, slot ownership, semantics and layout convergence.
#include <nandina/scene/node_ref.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/table.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace foundation = nandina::foundation;
namespace reactive = nandina::reactive;
namespace scene = nandina::scene;
namespace semantics = nandina::semantics;
namespace theme = nandina::theme;
namespace widget = nandina::widget;

namespace
{
    class Cell final: public scene::NanControl {
    public:
        float natural_width;
        float line_height;
        int measures = 0;
        explicit Cell(float width = 40.0F, float height = 12.0F):
            natural_width(width),
            line_height(height) {}
        void grow(float width) {
            natural_width = width;
            mark_layout_dirty();
        }

    protected:
        auto on_measure(foundation::NanLayoutConstraints c) -> foundation::NanSize override {
            ++measures;
            const float width = std::isfinite(c.max_width) ? c.max_width : natural_width;
            const float lines =
                width > 0.0F ? std::max(1.0F, std::ceil(natural_width / width)) : 1.0F;
            return c.constrain(foundation::NanSize(width, lines * line_height));
        }
    };
    auto value(float width = 40.0F, float height = 12.0F) -> std::shared_ptr<Cell> {
        return std::make_shared<Cell>(width, height);
    }
    void layout(widget::Table& table, float width, float height = 400.0F) {
        (void)table.measure_layout(
            foundation::NanLayoutConstraints::tight(foundation::NanSize(width, height))
        );
        table.layout_to(foundation::NanRect::from_xywh(0, 0, width, height));
    }
} // namespace

TEST_CASE("Table uses fixed and weighted tracks for both header and body", "[table][layout]") {
    auto table = widget::Table::create({{.width = 60.0F}, {.flex = 1}, {.flex = 2}});
    auto header = value();
    auto first = value();
    auto second = value();
    auto third = value();
    table->set_column_gap(10);
    table->set_row_gap(5);
    table->set_header({header, value(), value()});
    table->add_row({first, second, third});
    layout(*table, 320);
    CHECK(header->width() == Catch::Approx(60));
    CHECK(first->width() == header->width());
    CHECK(second->width() == Catch::Approx(80));
    CHECK(third->width() == Catch::Approx(160));
    CHECK(second->global_bounds().get_left() == Catch::Approx(70));
    CHECK(third->global_bounds().get_left() == Catch::Approx(160));
    CHECK(first->global_bounds().get_top() == Catch::Approx(17));
    CHECK(table->header_cell(0) == header.get());
    CHECK(table->cell(0, 2) == third.get());
}

TEST_CASE(
    "Table finite widths remeasure wrapping rows and share the maximum height",
    "[table][layout]"
) {
    auto table = widget::Table::create();
    auto long_cell = value(120, 10);
    auto short_cell = value(10, 8);
    auto next = value();
    table->add_row({long_cell, short_cell});
    table->add_row({next, value()});
    auto measured = table->measure_layout({.max_width = 80});
    CHECK(measured.get_height() == Catch::Approx(42));
    table->layout_to(foundation::NanRect::from_xywh(0, 0, 80, measured.get_height()));
    CHECK(long_cell->height() == Catch::Approx(30));
    CHECK(short_cell->height() == Catch::Approx(30));
    CHECK(next->global_bounds().get_top() == Catch::Approx(30));
    measured = table->measure_layout({.max_width = 240});
    CHECK(measured.get_height() == Catch::Approx(22));
    table->layout_to(foundation::NanRect::from_xywh(0, 0, 240, measured.get_height()));
    CHECK(long_cell->width() == Catch::Approx(120));
    CHECK(short_cell->height() == Catch::Approx(10));
}

TEST_CASE(
    "Table loose measurement preserves natural tracks when arranged at that size",
    "[table][layout]"
) {
    auto table = widget::Table::create();
    auto narrow = value(20, 10);
    auto wide = value(80, 10);
    table->add_row({narrow, wide});
    const auto size = table->measure_layout(foundation::NanLayoutConstraints::loose());
    CHECK(size == foundation::NanSize(100, 10));
    table->layout_to(foundation::NanRect::from_origin_size(foundation::NanPoint {}, size));
    CHECK(narrow->width() == Catch::Approx(20));
    CHECK(wide->width() == Catch::Approx(80));
    CHECK(wide->height() == Catch::Approx(10));
}

TEST_CASE(
    "Table keeps fixed tracks on overflow and clips cell and table boxes",
    "[table][layout]"
) {
    auto table = widget::Table::create({{.width = 120}, {}});
    auto fixed = value();
    auto flexible = value();
    table->set_column_gap(5);
    table->add_row({fixed, flexible});
    layout(*table, 80);
    CHECK(fixed->width() == Catch::Approx(120));
    CHECK(flexible->width() == Catch::Approx(0));
    CHECK(table->overflow() == scene::ControlOverflow::clip);
    CHECK(fixed->parent()->as_control()->overflow() == scene::ControlOverflow::clip);
    CHECK_FALSE(table->is_focusable());
}

TEST_CASE(
    "Table rejects invalid tracks gaps and changed column counts transactionally",
    "[table][validation]"
) {
    auto table = widget::Table::create();
    auto cell = value();
    table->add_row({cell, value()});
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    for (const auto bad: {-1.0F, nan, inf}) {
        CHECK_THROWS_AS(table->set_column_gap(bad), std::invalid_argument);
        CHECK_THROWS_AS(table->set_row_gap(bad), std::invalid_argument);
        CHECK_THROWS_AS(table->set_columns({{.width = bad}, {}}), std::invalid_argument);
    }
    for (const auto bad: {0.0F, -1.0F, nan, inf}) {
        CHECK_THROWS_AS(table->set_columns({{.flex = bad}, {}}), std::invalid_argument);
    }
    CHECK_THROWS_AS(table->set_columns({}), std::invalid_argument);
    CHECK_THROWS_AS(table->set_columns({{}}), std::logic_error);
    CHECK(table->columns().size() == 2);
    CHECK(table->column_gap() == 0);
    CHECK(table->row_gap() == 0);
    CHECK(table->cell(0, 0) == cell.get());
    table->clear_rows();
    table->set_columns({{}});
    CHECK(table->columns().size() == 1);
}

TEST_CASE(
    "Table invalid bulk replacement preserves existing rows and incoming ownership",
    "[table][validation]"
) {
    auto table = widget::Table::create();
    auto original = value();
    table->add_row({original, value()});
    auto incoming = value();
    CHECK_THROWS_AS(table->set_rows({{incoming, value()}, {incoming, value()}}), std::logic_error);
    CHECK(incoming->parent() == nullptr);
    CHECK(table->cell(0, 0) == original.get());
    CHECK_THROWS_AS(table->set_header({value()}), std::invalid_argument);
    CHECK_FALSE(table->has_header());
    CHECK_THROWS_AS(table->set_rows({{value(), nullptr}}), std::invalid_argument);
    CHECK_THROWS_AS(table->add_row({table, value()}), std::logic_error);
    CHECK_THROWS_AS(table->add_row({original, value()}), std::logic_error);
    CHECK_THROWS_AS(table->cell(1, 0), std::out_of_range);
    CHECK_THROWS_AS(table->header_cell(0), std::out_of_range);
    CHECK(table->row_count() == 1);
}

TEST_CASE("Table rejects anchored cells before changing any row", "[table][validation]") {
    auto table = widget::Table::create();
    auto ordinary = value();
    auto anchored = value();
    scene::NodeRef<scene::NanControl> ref(anchored);
    anchored->set_anchors({.left = ref.parent.anchor.left});
    CHECK_THROWS(table->set_rows({{ordinary, anchored}}));
    CHECK(ordinary->parent() == nullptr);
    CHECK(anchored->parent() == nullptr);
    CHECK(table->row_count() == 0);
}

TEST_CASE("Table header replacement preserves row order and drops old subtree", "[table][slots]") {
    auto table = widget::Table::create();
    table->add_row({value(), value()});
    auto old_header = value();
    table->set_header({old_header, value()});
    scene::NanSceneTree tree;
    tree.set_root(table);
    (void)tree.layout_root(foundation::NanSize(100, 100));
    CHECK(old_header->is_inside_tree());
    auto new_header = value();
    table->set_header({new_header, value()});
    CHECK_FALSE(old_header->is_inside_tree());
    CHECK(new_header->is_inside_tree());
    CHECK(table->get_child(0) == new_header->parent()->parent());
    table->clear_header();
    CHECK_FALSE(new_header->is_inside_tree());
    CHECK(table->row_count() == 1);
}

TEST_CASE(
    "Table deferred bulk replacement and header stay ordered and laid out",
    "[table][lifecycle]"
) {
    auto table = widget::Table::create();
    auto old = value();
    table->add_row({old, value()});
    scene::NanSceneTree tree;
    tree.set_root(table);
    (void)tree.layout_root(foundation::NanSize(200, 200));
    auto first = value();
    auto second = value();
    auto header = value();
    {
        auto phase = tree.enter_phase(scene::FramePhase::process);
        table->set_rows({{first, value()}});
        table->add_row({second, value()});
        table->set_header({header, value()});
    }
    tree.flush_tree_mutations();
    (void)tree.layout_root(foundation::NanSize(200, 200));
    CHECK_FALSE(old->is_inside_tree());
    CHECK(first->is_inside_tree());
    CHECK(second->is_inside_tree());
    CHECK(table->child_count() == 3);
    CHECK(table->get_child(0) == header->parent()->parent());
    CHECK(table->get_child(1) == first->parent()->parent());
    CHECK(table->get_child(2) == second->parent()->parent());
    CHECK(second->global_bounds().get_top() > first->global_bounds().get_top());
}

TEST_CASE("Table semantics expose structure and retain content actions", "[table][semantics]") {
    reactive::Graph graph;
    auto table = widget::Table::create({{}});
    auto header = widget::Label::create(graph, "Name");
    auto button = widget::Button::create("Open");
    int clicks = 0;
    button->set_on_click([&] { ++clicks; });
    table->set_header({header});
    table->add_row({button});
    scene::NanSceneTree tree;
    tree.set_root(table);
    (void)tree.layout_root(foundation::NanSize(200, 100));
    REQUIRE(tree.update_semantics());
    const auto* root = tree.semantics_tree().find(table->semantics_id());
    REQUIRE(root);
    CHECK(root->properties.role == semantics::Role::table);
    CHECK_FALSE(root->properties.state.focusable);
    REQUIRE(root->children.size() == 2);
    CHECK(root->children[0].properties.role == semantics::Role::row);
    REQUIRE(root->children[0].children.size() == 1);
    CHECK(root->children[0].children[0].properties.role == semantics::Role::column_header);
    CHECK(root->children[1].children[0].properties.role == semantics::Role::cell);
    REQUIRE(tree.semantics_tree().find(button->semantics_id()));
    CHECK(tree.perform_semantics_action(
        button->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    CHECK(clicks == 1);
    CHECK(semantics::role_name(semantics::Role::table) == "table");
    CHECK(semantics::role_name(semantics::Role::row) == "row");
    CHECK(semantics::role_name(semantics::Role::column_header) == "columnheader");
    CHECK(semantics::role_name(semantics::Role::cell) == "cell");
}

TEST_CASE(
    "Table child invalidation converges and setters do not perform eager layout",
    "[table][layout]"
) {
    auto table = widget::Table::create();
    auto cell = value();
    table->add_row({cell, value()});
    CHECK(cell->measures == 0);
    scene::NanSceneTree tree;
    tree.set_root(table);
    CHECK(cell->measures == 0);
    CHECK(tree.layout_root(foundation::NanSize(100, 200)) == 1);
    const auto measures = cell->measures;
    CHECK(tree.layout_root(foundation::NanSize(100, 200)) == 0);
    CHECK(cell->measures == measures);
    cell->grow(150);
    CHECK(tree.layout_root(foundation::NanSize(100, 200)) == 1);
    CHECK(cell->height() == Catch::Approx(36));
    CHECK(tree.layout_root(foundation::NanSize(100, 200)) == 0);
}

TEST_CASE(
    "Table authoring and child typography follow ambient style and theme",
    "[table][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);
    auto table = ui.make<widget::Table>(std::vector<widget::TableColumn> {{}}).build();
    auto label = ui.make<widget::Label>("A long cell").build();
    table->add_row({label});
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(table);
    (void)tree.layout_root(foundation::NanSize(200, 100));
    auto style = table->style_context();
    style.font_size = theme::StyleValue<float>::explicit_value(25);
    table->set_style_context(style);
    (void)tree.layout_root(foundation::NanSize(200, 100));
    CHECK(label->font_size() == Catch::Approx(25));
    label->set_color_token(theme::ColorToken::primary);
    const auto before = label->color();
    themes.set_preference(theme::ThemePreference::dark);
    (void)tree.layout_root(foundation::NanSize(200, 100));
    CHECK_FALSE(label->color().approx_equals(before));
    CHECK(table->cell(0, 0) == label.get());
}

TEST_CASE(
    "Table builder materializes cells and preserves nested callback scope",
    "[table][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);
    int calls = 0;
    auto button = ui.make<widget::Button>("Open").on_click([&] { ++calls; }).build();
    auto table = ui.make<widget::Table>(std::vector<widget::TableColumn> {{}})
                     .header_cells(ui.make<widget::Label>("Action"))
                     .row_cells(button)
                     .build();
    scene::NanSceneTree tree;
    tree.set_root(table);
    (void)tree.layout_root(foundation::NanSize(100, 100));
    REQUIRE(tree.perform_semantics_action(
        button->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    CHECK(calls == 1);
    scope.clear();
    REQUIRE(tree.perform_semantics_action(
        button->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    CHECK(calls == 1);
    CHECK(table->row_count() == 1);
    CHECK(table->has_header());
}

TEST_CASE(
    "Table wrapped Label grows naturally and composes with ScrollView",
    "[table][text][scroll]"
) {
    reactive::Graph graph;
    auto table = widget::Table::create({{}});
    auto text = widget::Label::create(graph, "alpha beta gamma delta epsilon zeta eta theta");
    text->set_overflow(nandina::text::TextOverflow::wrap);
    text->set_max_lines(100);
    table->add_row({text});
    auto scroll = widget::ScrollView::create();
    scroll->set_child(table);
    scene::NanSceneTree tree;
    tree.set_root(scroll);
    (void)tree.layout_root(foundation::NanSize(90, 25));
    CHECK(text->line_count() > 1);
    CHECK(table->height() > scroll->height());
    CHECK(scroll->maximum_scroll_offset().get_y() > 0);
    const auto old_height = table->height();
    (void)tree.layout_root(foundation::NanSize(400, 25));
    CHECK(table->height() < old_height);
    CHECK(text->width() == Catch::Approx(400));
    CHECK(tree.layout_root(foundation::NanSize(400, 25)) == 0);
}

TEST_CASE("Table empty and header-only tables have no phantom row gaps", "[table][layout]") {
    auto table = widget::Table::create();
    table->set_row_gap(10);
    CHECK(table->measure_layout(foundation::NanLayoutConstraints::loose()).get_height() == 0);
    table->set_header({value(20, 8), value(20, 8)});
    CHECK(table->measure_layout(foundation::NanLayoutConstraints::loose()).get_height() == 8);
    table->clear_rows();
    CHECK(table->has_header());
    table->clear_header();
    CHECK(table->child_count() == 0);
    CHECK(table->measure_layout(foundation::NanLayoutConstraints::loose()).get_height() == 0);
}

TEST_CASE("Table validates the total width of fixed tracks and gaps", "[table][validation]") {
    const auto max = std::numeric_limits<float>::max();
    CHECK_THROWS_AS(widget::Table::create({{.width = max}, {.width = max}}), std::invalid_argument);
    auto table = widget::Table::create({{.width = max}, {}});
    CHECK_THROWS_AS(table->set_column_gap(max), std::invalid_argument);
    CHECK(table->column_gap() == 0);
}

TEST_CASE("Table rejects row gap totals that would overflow the layout", "[table][validation]") {
    const auto max = std::numeric_limits<float>::max();

    // 三行 × max 行距：间距总和超过 float，与列轨道的有限和检查对称，必须拒绝。
    auto table = widget::Table::create();
    for (int i = 0; i < 3; ++i) {
        table->add_row({value(), value()});
    }
    CHECK_THROWS_AS(table->set_row_gap(max), std::invalid_argument);
    CHECK(table->row_gap() == 0);

    // 行距先在小表上合法，随后增长行数会越过上限：add_row 拒绝且不改原状态。
    auto two = widget::Table::create();
    two->add_row({value(), value()});
    two->set_row_gap(max);
    two->add_row({value(), value()});
    CHECK(two->row_count() == 2);
    CHECK_THROWS_AS(two->add_row({value(), value()}), std::invalid_argument);
    CHECK(two->row_count() == 2);
    auto measured = two->measure_layout(foundation::NanLayoutConstraints::loose());
    CHECK(std::isfinite(measured.get_height()));

    // 表头也计入行间距：两行大行距已到上限，再加表头同样拒绝。
    auto headed = widget::Table::create();
    headed->add_row({value(), value()});
    headed->add_row({value(), value()});
    headed->set_row_gap(max);
    CHECK_THROWS_AS(headed->set_header({value(), value()}), std::invalid_argument);
    CHECK_FALSE(headed->has_header());
    CHECK(headed->row_count() == 2);
    CHECK(
        std::isfinite(
            headed->measure_layout(foundation::NanLayoutConstraints::loose()).get_height()
        )
    );
}

namespace
{
    template<typename Builder>
    concept TableSlots = requires(Builder builder, std::shared_ptr<scene::NanControl> cell) {
        builder.header_cells(cell);
        builder.row_cells(cell);
    };
    static_assert(TableSlots<widget::authoring::NodeBuilder<widget::Table>>);
    static_assert(!TableSlots<widget::authoring::NodeBuilder<widget::Label>>);
} // namespace

TEST_CASE("Table rejects content totals beyond finite layout coordinates", "[table][validation]") {
    const float large = std::numeric_limits<float>::max() * 0.75F;
    SECTION("natural column widths") {
        auto table = widget::Table::create();
        table->add_row({value(large, 1), value(large, 1)});
        CHECK_THROWS_AS(
            table->measure_layout(foundation::NanLayoutConstraints::loose()),
            std::invalid_argument
        );
        CHECK(std::isfinite(table->width()));
    }
    SECTION("row heights") {
        auto table = widget::Table::create({{}});
        table->add_row({value(1, large)});
        table->add_row({value(1, large)});
        CHECK_THROWS_AS(
            table->measure_layout(foundation::NanLayoutConstraints::loose()),
            std::invalid_argument
        );
        CHECK_THROWS_AS(layout(*table, 100, 100), std::invalid_argument);
    }
    SECTION("content and a separately valid row gap") {
        auto table = widget::Table::create({{}});
        table->set_header({value(1, large)});
        table->add_row({value(1, 1)});
        table->set_row_gap(large);
        CHECK_THROWS_AS(
            table->measure_layout(foundation::NanLayoutConstraints::loose()),
            std::invalid_argument
        );
    }
}
