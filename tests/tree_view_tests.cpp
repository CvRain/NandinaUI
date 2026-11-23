// tree_view_tests — 批 6 TreeView（ITreeModel + 视图持有展开 + 虚拟化 + 行选择）。
//
// 覆盖：折叠子树不物化、展开/折叠、叶子拒绝、程序化 select 展开祖先、指针选择与
// 展开器点击、键盘 Right/Left/Up/Down、data_changed 复用、reset 保留展开/选择、
// ItemState depth/has_children/expanded、虚拟化窗口、行高/缩进校验与脱离树构建。
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/scroll_view.hpp>
#include <nandina/widget/tree_model.hpp>
#include <nandina/widget/tree_view.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace nandina;

namespace
{
    class TestTreeModel final: public widget::TypedTreeModelBase<TestTreeModel> {
    public:
        struct Node {
            widget::RowId id;
            std::string name;
            std::optional<std::size_t> parent_index;
        };

        std::vector<Node> nodes;

        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return nodes.size();
        }
        [[nodiscard]] auto row_id(const std::size_t row) const -> widget::RowId override {
            return nodes.at(row).id;
        }
        [[nodiscard]] auto parent(widget::RowId row) const
            -> std::optional<widget::RowId> override {
            const auto index = index_of(row);
            if (!index.has_value()) {
                return std::nullopt;
            }
            const auto parent_index = nodes[*index].parent_index;
            if (!parent_index.has_value()) {
                return std::nullopt;
            }
            return nodes[*parent_index].id;
        }
        [[nodiscard]] auto child_count(widget::RowId row) const -> std::size_t override {
            const auto index = index_of(row);
            if (!index.has_value()) {
                return 0;
            }
            std::size_t count = 0;
            for (const auto& node: nodes) {
                if (node.parent_index == index) {
                    ++count;
                }
            }
            return count;
        }
        [[nodiscard]] auto child(widget::RowId row, const std::size_t child_index) const
            -> widget::RowId override {
            const auto index = index_of(row);
            if (!index.has_value()) {
                throw std::out_of_range("TestTreeModel child row is unknown");
            }
            std::size_t seen = 0;
            for (const auto& node: nodes) {
                if (node.parent_index == index) {
                    if (seen == child_index) {
                        return node.id;
                    }
                    ++seen;
                }
            }
            throw std::out_of_range("TestTreeModel child index is out of range");
        }

        void reset(std::vector<Node> next_nodes) {
            nodes = std::move(next_nodes);
            notify_reset();
        }

        void change(const widget::RowRange range) {
            notify_data_changed(range);
        }

        void refresh() {
            notify_reset();
        }

        void reset_standard() {
            reset({
                {.id = widget::RowId {.value = 1}, .name = "A", .parent_index = std::nullopt},
                {.id = widget::RowId {.value = 2}, .name = "A.1", .parent_index = 0},
                {.id = widget::RowId {.value = 3}, .name = "A.1.a", .parent_index = 1},
                {.id = widget::RowId {.value = 4}, .name = "A.2", .parent_index = 0},
                {.id = widget::RowId {.value = 5}, .name = "B", .parent_index = std::nullopt},
            });
        }

        void reset_roots(const std::size_t count) {
            std::vector<Node> next;
            next.reserve(count);
            for (std::size_t index = 0; index < count; ++index) {
                next.push_back(
                    Node {
                        .id = widget::RowId {.value = index + 1},
                        .name = "r" + std::to_string(index),
                        .parent_index = std::nullopt,
                    }
                );
            }
            reset(std::move(next));
        }

        [[nodiscard]] auto index_of(const widget::RowId row) const -> std::optional<std::size_t> {
            for (std::size_t index = 0; index < nodes.size(); ++index) {
                if (nodes[index].id == row) {
                    return index;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] auto name_of(const std::size_t row) const -> const std::string& {
            return nodes.at(row).name;
        }
    };

    class TestDelegate final: public widget::TypedItemDelegate<TestTreeModel> {
    public:
        std::size_t created = 0;
        std::size_t updated = 0;
        std::unordered_map<std::uint64_t, widget::ItemState> states;

    protected:
        auto create_typed(
            const widget::BuildContext& ui,
            const TestTreeModel& model,
            const widget::ItemBinding& binding
        ) -> std::shared_ptr<scene::NanControl> override {
            ++created;
            states[binding.row.value] = binding.state;
            auto label =
                widget::Label::create(ui.graph(), model.name_of(binding.model_row), ui.theme());
            label->set_typography_role(theme::TypographyRole::label);
            return label;
        }
        void update_typed(
            scene::NanControl& control,
            const TestTreeModel& model,
            const widget::ItemBinding& binding
        ) override {
            ++updated;
            states[binding.row.value] = binding.state;
            static_cast<widget::Label&>(control).set_text(model.name_of(binding.model_row));
        }
    };

    using LayoutCounts = std::unordered_map<std::uint64_t, std::size_t>;

    class LayoutCountingLabel final: public widget::Label {
    public:
        LayoutCountingLabel(
            reactive::Graph& graph,
            std::string text,
            theme::NanTheme theme,
            const widget::RowId row,
            std::shared_ptr<LayoutCounts> counts
        ):
            Label(graph, std::move(text), std::move(theme)),
            row_(row),
            counts_(std::move(counts)) {}

    protected:
        void on_layout() override {
            ++(*counts_)[row_.value];
            Label::on_layout();
        }

    private:
        widget::RowId row_;
        std::shared_ptr<LayoutCounts> counts_;
    };

    class LayoutCountingDelegate final: public widget::TypedItemDelegate<TestTreeModel> {
    public:
        std::shared_ptr<LayoutCounts> layout_counts = std::make_shared<LayoutCounts>();
        std::size_t create_calls = 0;
        std::size_t update_calls = 0;

    protected:
        auto create_typed(
            const widget::BuildContext& ui,
            const TestTreeModel& model,
            const widget::ItemBinding& binding
        ) -> std::shared_ptr<scene::NanControl> override {
            ++create_calls;
            return std::make_shared<LayoutCountingLabel>(
                ui.graph(),
                model.name_of(binding.model_row),
                ui.theme(),
                binding.row,
                layout_counts
            );
        }
        void update_typed(
            scene::NanControl& control,
            const TestTreeModel& model,
            const widget::ItemBinding& binding
        ) override {
            ++update_calls;
            static_cast<widget::Label&>(control).set_text(model.name_of(binding.model_row));
        }
    };

    class OtherTreeModel final: public widget::TypedTreeModelBase<OtherTreeModel> {
    public:
        [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
            return 0;
        }
        [[nodiscard]] auto row_id(std::size_t) const -> widget::RowId override {
            return {};
        }
        [[nodiscard]] auto parent(widget::RowId) const -> std::optional<widget::RowId> override {
            return std::nullopt;
        }
        [[nodiscard]] auto child_count(widget::RowId) const -> std::size_t override {
            return 0;
        }
        [[nodiscard]] auto child(widget::RowId, std::size_t) const -> widget::RowId override {
            throw std::out_of_range("OtherTreeModel has no children");
        }
    };

    class OtherTreeDelegate final: public widget::TypedItemDelegate<OtherTreeModel> {
    protected:
        auto create_typed(
            const widget::BuildContext& ui,
            const OtherTreeModel&,
            const widget::ItemBinding&
        ) -> std::shared_ptr<scene::NanControl> override {
            return widget::Label::create(ui.graph(), "");
        }
        void update_typed(
            scene::NanControl&,
            const OtherTreeModel&,
            const widget::ItemBinding&
        ) override {}
    };

    struct Fixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        widget::BuildContext ui {graph, scope, themes};
    };

    struct Harness {
        std::shared_ptr<widget::TreeView> view;
        std::shared_ptr<TestDelegate> delegate;
    };

    auto make_harness(
        Fixture& fixture,
        TestTreeModel& model,
        const widget::SelectionMode mode = widget::SelectionMode::multiple,
        const float row_height = 32.0F
    ) -> Harness {
        auto view = widget::TreeView::create(fixture.ui, mode, row_height);
        auto delegate = std::make_shared<TestDelegate>();
        view->set_delegate(delegate);
        view->set_model(std::shared_ptr<widget::ITreeModel>(&model, [](widget::ITreeModel*) {}));
        return Harness {.view = std::move(view), .delegate = std::move(delegate)};
    }

    auto mount(const std::shared_ptr<widget::TreeView>& view)
        -> std::unique_ptr<scene::NanSceneTree> {
        auto tree = std::make_unique<scene::NanSceneTree>();
        tree->set_root(view);
        (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
        return tree;
    }

    auto row_id(const std::uint64_t value) -> widget::RowId {
        return widget::RowId {.value = value};
    }

    void
    click_at(scene::NanSceneTree& tree, scene::NanControl& node, const float x, const float y) {
        const auto bounds = node.global_bounds();
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent {
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                foundation::NanPoint(bounds.get_left() + x, bounds.get_top() + y),
                {},
            }
        );
    }

    void press_key(scene::NanSceneTree& tree, const foundation::LogicalKey key) {
        tree.dispatch_key(scene::KeyEvent(key, scene::KeyEvent::Action::press, {}));
    }
} // namespace

TEST_CASE(
    "TreeView materializes only visible roots and expanded children",
    "[tree-view][virtualization]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    CHECK(harness.view->node_count() == 5);
    CHECK(harness.view->row_count() == 2);
    CHECK(harness.view->item_count() == 2);
    CHECK(harness.view->row_node(row_id(1)) != nullptr);
    CHECK(harness.view->row_node(row_id(2)) == nullptr);

    CHECK(harness.view->expand(row_id(1)));
    CHECK(harness.view->row_count() == 4);
    CHECK(harness.view->row_node(row_id(2)) != nullptr);
    CHECK(harness.view->row_node(row_id(3)) == nullptr);
    CHECK(harness.delegate->states.at(2).depth == 1);
    CHECK(harness.delegate->states.at(2).has_children);
    CHECK_FALSE(harness.delegate->states.at(5).has_children);
}

TEST_CASE("TreeView expand and collapse update the visible window", "[tree-view][expansion]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    int changes = 0;
    harness.view->set_on_expansion_changed([&changes] { ++changes; });

    CHECK(harness.view->expand(row_id(1)));
    CHECK(harness.view->is_expanded(row_id(1)));
    CHECK(changes == 1);
    CHECK(harness.view->expand(row_id(2)));
    CHECK(changes == 2);
    CHECK(harness.view->row_count() == 5);
    CHECK(harness.view->row_node(row_id(3)) != nullptr);

    CHECK(harness.view->collapse(row_id(2)));
    CHECK_FALSE(harness.view->is_expanded(row_id(2)));
    CHECK(harness.view->row_count() == 4);
    CHECK(harness.view->row_node(row_id(3)) == nullptr);

    // 叶子不可展开。
    CHECK_FALSE(harness.view->expand(row_id(3)));
    CHECK_FALSE(harness.view->toggle_expanded(row_id(3)));
    CHECK(changes == 3);
}

TEST_CASE(
    "TreeView commits expansion before reporting a callback failure",
    "[tree-view][expansion][validation]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    harness.view->set_on_expansion_changed([] {
        throw std::runtime_error("expansion callback failed");
    });
    CHECK_THROWS_AS(harness.view->expand(row_id(1)), std::runtime_error);

    CHECK(harness.view->is_expanded(row_id(1)));
    CHECK(harness.view->row_count() == 4);
    CHECK(harness.view->row_node(row_id(2)) != nullptr);
}

TEST_CASE("TreeView programmatic select expands ancestors", "[tree-view][selection]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    CHECK(harness.view->select(row_id(3)));
    CHECK(harness.view->is_expanded(row_id(1)));
    CHECK(harness.view->is_expanded(row_id(2)));
    CHECK(harness.view->selection().contains(row_id(3)));
    REQUIRE(harness.view->display_row_for(row_id(3)).has_value());

    CHECK(harness.view->row_node(row_id(3)) != nullptr);
    CHECK_FALSE(harness.view->select(row_id(99)));
    // 合法但已选中仍返回 true（身份有效语义）。
    CHECK(harness.view->select(row_id(3)));
    CHECK(harness.view->selection().count() == 1);
}

TEST_CASE("TreeView pointer selects rows and the expander toggles", "[tree-view][input]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    // 行内容点击 = 选择（根 A 的内容从 gutter 之后开始）。
    click_at(*tree, *harness.view, 30.0F, 16.0F);
    CHECK(harness.view->selection().selected() == std::vector<widget::RowId> {row_id(1)});
    CHECK(tree->focused_node() == harness.view.get());

    // 展开器点击 = 切换展开，不改变选择。
    click_at(*tree, *harness.view, 9.0F, 16.0F);
    CHECK(harness.view->is_expanded(row_id(1)));
    CHECK(harness.view->selection().selected() == std::vector<widget::RowId> {row_id(1)});

    // 折叠后再次点击展开器。
    click_at(*tree, *harness.view, 9.0F, 16.0F);
    CHECK_FALSE(harness.view->is_expanded(row_id(1)));
}

TEST_CASE("TreeView keyboard expands, collapses and moves", "[tree-view][keyboard]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model, widget::SelectionMode::single);
    auto tree = mount(harness.view);

    click_at(*tree, *harness.view, 30.0F, 16.0F); // 选中根 A
    REQUIRE(harness.view->selection().selected() == std::vector<widget::RowId> {row_id(1)});

    press_key(*tree, foundation::LogicalKey::right);
    CHECK(harness.view->is_expanded(row_id(1)));
    CHECK(harness.view->row_count() == 4);

    press_key(*tree, foundation::LogicalKey::right);
    CHECK(harness.delegate->states.at(2).current);

    press_key(*tree, foundation::LogicalKey::down);
    CHECK(harness.delegate->states.at(4).current);

    press_key(*tree, foundation::LogicalKey::left);
    CHECK(harness.delegate->states.at(1).current);
    press_key(*tree, foundation::LogicalKey::left);
    CHECK_FALSE(harness.view->is_expanded(row_id(1)));
    CHECK(harness.view->row_count() == 2);
}

TEST_CASE(
    "TreeView keyboard ignores Enter and Space for a hidden current row",
    "[tree-view][keyboard][selection]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model, widget::SelectionMode::single);
    auto tree = mount(harness.view);

    CHECK(harness.view->select(row_id(3)));
    tree->set_focus(harness.view.get());
    CHECK(harness.view->collapse(row_id(2)));
    REQUIRE_FALSE(harness.view->display_row_for(row_id(3)).has_value());

    std::optional<widget::RowId> activated;
    harness.view->set_on_activated([&activated](const widget::RowId row) { activated = row; });
    press_key(*tree, foundation::LogicalKey::enter);
    CHECK_FALSE(activated.has_value());
    CHECK(harness.view->selection().contains(row_id(3)));

    press_key(*tree, foundation::LogicalKey::space);
    CHECK(harness.view->selection().contains(row_id(3)));
}

TEST_CASE("TreeView data_changed reuses visible rows", "[tree-view][delegate][reuse]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    auto* node = harness.view->row_node(row_id(1));
    REQUIRE(node != nullptr);
    model.nodes[0].name = "renamed";
    // data_changed 的行号是模型的先序行号：根 A = 0。
    model.change(widget::RowRange {.first = 0, .last_exclusive = 1});

    CHECK(harness.view->row_node(row_id(1)) == node);
    CHECK(static_cast<widget::Label*>(node)->text() == "renamed");
    CHECK(harness.delegate->created == 2);
}

TEST_CASE(
    "TreeView delegate replacement rebuilds the visible window",
    "[tree-view][delegate][reuse]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    const auto old_created = harness.delegate->created;
    const auto old_updated = harness.delegate->updated;
    auto next = std::make_shared<TestDelegate>();
    harness.view->set_delegate(next);

    CHECK(next->created == 2);
    CHECK(harness.delegate->created == old_created);

    model.nodes[0].name = "renamed";
    model.change(widget::RowRange {.first = 0, .last_exclusive = 1});
    CHECK(next->updated >= 1);
    CHECK(harness.delegate->updated == old_updated);
}

TEST_CASE("TreeView reset retains expansion and selection", "[tree-view][model]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    CHECK(harness.view->expand(row_id(1)));
    CHECK(harness.view->select(row_id(4)));
    REQUIRE(harness.view->is_expanded(row_id(1)));

    // 重合同一树：展开与选择保留。
    model.refresh();
    CHECK(harness.view->is_expanded(row_id(1)));
    CHECK(harness.view->selection().contains(row_id(4)));

    // 替换为只剩根 B：失效的展开/选择被裁剪。
    model.reset({
        {.id = row_id(5), .name = "B", .parent_index = std::nullopt},
    });
    CHECK_FALSE(harness.view->is_expanded(row_id(1)));
    CHECK(harness.view->selection().count() == 0);
    CHECK(harness.view->row_count() == 1);
}

TEST_CASE(
    "TreeView defers reset while the scene tree is traversing",
    "[tree-view][model][virtualization]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);
    CHECK(harness.view->expand(row_id(1)));

    {
        auto phase = tree->enter_phase(scene::FramePhase::process);
        model.reset({
            {.id = row_id(5), .name = "B", .parent_index = std::nullopt},
        });
        CHECK(harness.view->has_pending_notifications());
        auto next = std::make_shared<TestDelegate>();
        CHECK_NOTHROW(harness.view->set_delegate(next));
        harness.delegate = std::move(next);
    }

    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK_FALSE(harness.view->has_pending_notifications());
    CHECK(harness.view->node_count() == 1);
    CHECK(harness.view->row_count() == 1);
    CHECK(harness.view->item_count() == 1);
    CHECK(harness.view->row_node(row_id(5)) != nullptr);
    CHECK(harness.view->row_node(row_id(1)) == nullptr);
    CHECK(harness.delegate->created == 1);
}

TEST_CASE(
    "TreeView recovers selection refresh suppression after a callback failure",
    "[tree-view][selection][validation]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model, widget::SelectionMode::single);
    auto tree = mount(harness.view);

    CHECK(harness.view->select(row_id(3)));
    CHECK(harness.view->collapse(row_id(2)));
    int callback_count = 0;
    harness.view->set_on_selection_changed([&callback_count] {
        ++callback_count;
        throw std::runtime_error("selection callback failed");
    });

    // The selected descendant disappears during reset. The callback throws after
    // SelectionModel::retain has committed its new state.
    CHECK_THROWS_AS(
        model.reset({
            {.id = row_id(1), .name = "A", .parent_index = std::nullopt},
            {.id = row_id(2), .name = "A.1", .parent_index = 0},
            {.id = row_id(99), .name = "A.3", .parent_index = 0},
            {.id = row_id(98), .name = "A.4", .parent_index = 0},
            {.id = row_id(5), .name = "B", .parent_index = std::nullopt},
        }),
        std::runtime_error
    );
    INFO("selection callback count: " << callback_count);
    CHECK(callback_count == 1);
    harness.view->set_on_selection_changed({});

    click_at(*tree, *harness.view, 30.0F, 16.0F);
    REQUIRE(harness.delegate->states.at(1).selected);
    press_key(*tree, foundation::LogicalKey::space);
    CHECK_FALSE(harness.delegate->states.at(1).selected);
}

TEST_CASE("TreeView virtualizes a wide root list", "[tree-view][virtualization]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_roots(1000);
    auto harness = make_harness(fixture, model, widget::SelectionMode::single, 32.0F);
    auto tree = mount(harness.view);

    CHECK(harness.view->node_count() == 1000);
    CHECK(harness.view->row_count() == 1000);
    CHECK(harness.view->item_count() == 10);
    CHECK(harness.view->row_node(row_id(1)) != nullptr);
    CHECK(harness.view->row_node(row_id(500)) == nullptr);

    harness.view->set_scroll_offset(32.0F * 100.0F);
    CHECK(harness.view->row_node(row_id(101)) != nullptr);
    CHECK(harness.view->row_node(row_id(1)) == nullptr);
}

TEST_CASE("TreeView item state exposes depth, children and expansion", "[tree-view][theme]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    CHECK(harness.view->expand(row_id(1)));
    const auto& state = harness.delegate->states.at(2);
    CHECK(state.depth == 1);
    CHECK(state.has_children);
    CHECK_FALSE(state.expanded);
    CHECK(state.theme_revision == fixture.themes.revision());

    CHECK(harness.view->expand(row_id(2)));
    CHECK(harness.delegate->states.at(2).expanded);
}

TEST_CASE("TreeView refreshes visible item states on theme revision", "[tree-view][theme]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = std::make_unique<scene::NanSceneTree>();
    tree->set_theme_manager(fixture.themes);
    tree->set_root(harness.view);
    (void)tree->layout_root(foundation::NanSize(400.0F, 300.0F));

    const auto initial_revision = fixture.themes.revision();
    REQUIRE(harness.delegate->states.at(1).theme_revision == initial_revision);

    fixture.themes.set_text_scaling(1.25F);
    const auto new_revision = fixture.themes.revision();
    REQUIRE(new_revision > initial_revision);
    CHECK(harness.delegate->states.at(1).theme_revision == new_revision);
}

TEST_CASE("TreeView builds detached and materializes after mounting", "[tree-view][lifecycle]") {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);

    CHECK(harness.view->item_count() == 0);
    auto tree = mount(harness.view);
    CHECK(harness.view->item_count() == 2);
}

TEST_CASE("TreeView model is non-copyable and non-movable", "[tree-view][model]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<TestTreeModel>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<TestTreeModel>);
    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<TestTreeModel>);
    STATIC_REQUIRE_FALSE(std::is_move_assignable_v<TestTreeModel>);
}

TEST_CASE(
    "TreeView reset clears expansion for nodes that became leaves",
    "[tree-view][expansion][model]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    CHECK(harness.view->expand(row_id(1)));
    CHECK(harness.view->expand(row_id(2)));
    REQUIRE(harness.view->is_expanded(row_id(2)));

    int changes = 0;
    harness.view->set_on_expansion_changed([&changes] { ++changes; });

    // A.1 (id 2) 仍存在但 reset 后变成叶子；A (id 1) 仍有子节点。
    model.reset({
        {.id = row_id(1), .name = "A", .parent_index = std::nullopt},
        {.id = row_id(2), .name = "A.1", .parent_index = 0},
        {.id = row_id(5), .name = "B", .parent_index = std::nullopt},
    });

    CHECK_FALSE(harness.view->is_expanded(row_id(2)));
    CHECK(harness.view->is_expanded(row_id(1)));
    CHECK(changes == 1);
}

TEST_CASE(
    "TreeView shift click falls back when the anchor is hidden",
    "[tree-view][selection][input]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    auto tree = mount(harness.view);

    CHECK(harness.view->expand(row_id(1)));
    CHECK(harness.view->expand(row_id(2)));
    REQUIRE(harness.view->select(row_id(3)));
    REQUIRE(harness.view->selection().anchor() == std::optional<widget::RowId>(row_id(3)));

    CHECK(harness.view->collapse(row_id(2)));
    REQUIRE_FALSE(harness.view->display_row_for(row_id(3)).has_value());

    // shift-click 可见的 A.2 (id 4)：锚点 3 不可见 → 回退到点击行。
    const auto display = harness.view->display_row_for(row_id(4));
    REQUIRE(display.has_value());
    const float y = static_cast<float>(*display) * 32.0F + 16.0F;
    const auto bounds = harness.view->global_bounds();
    tree->dispatch_mouse_button(
        scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(bounds.get_left() + 30.0F, bounds.get_top() + y),
            scene::KeyModifiers {.shift = true},
        }
    );
    CHECK(harness.view->selection().selected() == std::vector<widget::RowId> {row_id(4)});
}

TEST_CASE("TreeView validates row height, indent and gutter", "[tree-view][validation]") {
    Fixture fixture;
    CHECK_THROWS_AS(
        widget::TreeView::create(fixture.ui, widget::SelectionMode::single, 0.0F),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        widget::TreeView::create(
            fixture.ui,
            widget::SelectionMode::single,
            std::numeric_limits<float>::quiet_NaN()
        ),
        std::invalid_argument
    );

    TestTreeModel model;
    model.reset_standard();
    auto harness = make_harness(fixture, model);
    CHECK_THROWS_AS(harness.view->set_row_height(-1.0F), std::invalid_argument);
    CHECK_THROWS_AS(harness.view->set_indent_width(-1.0F), std::invalid_argument);
    CHECK_THROWS_AS(harness.view->set_expander_gutter(-1.0F), std::invalid_argument);
}

TEST_CASE(
    "TreeView rejects delegate model token mismatches transactionally",
    "[tree-view][validation]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_standard();
    auto model_ptr = std::shared_ptr<widget::ITreeModel>(&model, [](widget::ITreeModel*) {});

    auto with_other_delegate = widget::TreeView::create(fixture.ui);
    with_other_delegate->set_delegate(std::make_shared<OtherTreeDelegate>());
    CHECK_THROWS_AS(with_other_delegate->set_model(model_ptr), std::invalid_argument);
    CHECK(with_other_delegate->model() == nullptr);

    auto with_other_model = widget::TreeView::create(fixture.ui);
    with_other_model->set_model(model_ptr);
    CHECK_THROWS_AS(
        with_other_model->set_delegate(std::make_shared<OtherTreeDelegate>()),
        std::invalid_argument
    );
    CHECK(with_other_model->delegate() == nullptr);
}

TEST_CASE(
    "TreeView survives layout pass before deferred shrinking reset flushes",
    "[tree-view][virtualization][binding][regression]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_roots(50);
    auto view = widget::TreeView::create(fixture.ui);
    auto delegate = std::make_shared<LayoutCountingDelegate>();
    view->set_delegate(delegate);
    view->set_model(std::shared_ptr<widget::ITreeModel>(&model, [](widget::ITreeModel*) {}));
    auto tree = mount(view);
    tree->set_theme_manager(fixture.themes);
    REQUIRE(view->row_count() == 50);
    REQUIRE(view->select(row_id(2)));
    const auto row_one_layouts = delegate->layout_counts->at(1);
    delegate->create_calls = 0;
    delegate->update_calls = 0;

    {
        auto phase = tree->enter_phase(scene::FramePhase::input);
        model.reset({
            {.id = row_id(100), .name = "Only", .parent_index = std::nullopt},
        });
        view->mark_layout_dirty();
        REQUIRE(view->has_pending_notifications());
        // Pointer input must not operate on the old visible-row ledger during the pending reset.
        click_at(*tree, *view, 30.0F, 16.0F);
        CHECK(view->selection().selected() == std::vector<widget::RowId> {row_id(2)});
        // Theme propagation during the pending reset must not update old model_row bindings.
        fixture.themes.set_text_scaling(1.25F);
    }

    REQUIRE_NOTHROW((void)tree->layout_root(foundation::NanSize(360.0F, 200.0F)));
    CHECK_FALSE(view->has_pending_notifications());
    CHECK(view->row_count() == 1);
    CHECK(view->item_count() == 1);
    CHECK(view->row_node(row_id(100)) != nullptr);
    CHECK(view->row_node(row_id(1)) == nullptr);
    CHECK(delegate->layout_counts->at(1) == row_one_layouts);
    CHECK(delegate->create_calls == 1);
    CHECK(delegate->update_calls == 1);
}

TEST_CASE(
    "TreeView detached layout_to materializes a newly visible window",
    "[tree-view][virtualization][detached]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_roots(50);
    auto harness = make_harness(fixture, model);
    REQUIRE(harness.view->item_count() == 0);

    harness.view->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 400.0F, 300.0F));
    CHECK(harness.view->item_count() == 10);
    CHECK(harness.view->row_node(row_id(1)) != nullptr);
}

TEST_CASE(
    "fill-height TreeView wrapped in detached Padding does not materialize full model",
    "[tree-view][virtualization][off-tree]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_roots(500);
    auto harness = make_harness(fixture, model);
    harness.view->set_width(scene::fill);
    harness.view->set_height(scene::fill);

    auto frame = widget::Padding::create(foundation::NanInsets::all(6.0F));
    frame->set_child(harness.view);

    CHECK(harness.view->item_count() == 0);
    CHECK(harness.delegate->created == 0);

    scene::NanSceneTree tree;
    tree.set_root(frame);
    // on_ready() 的 loose relayout 仍属预排：首次真实布局前不得物化全树
    // （6px 上下 inset → 12px，而不是 500 行 × 32px）。
    CHECK(harness.view->item_count() == 0);
    CHECK(harness.delegate->created == 0);
    CHECK(frame->measured_size().get_height() == Catch::Approx(12.0F));
    (void)tree.layout_root(foundation::NanSize(412.0F, 312.0F));
    CHECK(harness.view->item_count() == 10);
    CHECK(harness.delegate->created == 10);
}

TEST_CASE(
    "in-tree unbounded parents keep the fill-height TreeView measured from content",
    "[tree-view][virtualization][pre-layout]"
) {
    Fixture fixture;
    TestTreeModel model;
    model.reset_roots(40);
    auto harness = make_harness(fixture, model);
    harness.view->set_width(scene::fill);
    harness.view->set_height(scene::fill);

    // 真实布局 pass 里的无界纵向测量（ScrollView）必须保持既有内容高语义，
    // 不能把 fill 高度在布局期收缩成 min_height（见 data_components_and_virtualization.md §9.9）。
    auto scroll = widget::ScrollView::create();
    scroll->set_child(harness.view);
    scene::NanSceneTree tree;
    tree.set_root(scroll);
    (void)tree.layout_root(foundation::NanSize(400.0F, 300.0F));
    CHECK(harness.view->item_count() == 40);
    CHECK(harness.delegate->created == 40);
}
