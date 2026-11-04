//
// virtual_list_tests — VirtualListView 虚拟化与选择回归。
//
// 覆盖：可见窗口只物化视口内行、滚动窗口移动与行控件复用、wheel 滚动、
// 点击 / shift / ctrl 选择与事件、键盘漫游与激活、程序化选择滚动、
// 数据替换裁剪选中与活动项、重复键拒绝、脱离树可构建。
//

#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/signal.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/virtual_list_view.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    struct ListItem {
        int key = 0;
        std::string label;
        auto operator==(const ListItem&) const -> bool = default;
    };

    class TestRow: public scene::NanControl {
    public:
        explicit TestRow(const int key, std::string label = {}):
            key_(key),
            label_(std::move(label)) {}
        int key_ = 0;
        std::string label_;
    };

    auto make_items(std::size_t count) -> std::vector<ListItem> {
        std::vector<ListItem> items;
        items.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            items.push_back(
                ListItem {
                    .key = static_cast<int>(index),
                    .label = "row " + std::to_string(index),
                }
            );
        }
        return items;
    }

    auto make_list(reactive::Graph& graph, float row_height = 30.0F)
        -> std::shared_ptr<widget::VirtualListView<ListItem, int, TestRow>> {
        return widget::VirtualListView<ListItem, int, TestRow>::create(
            graph,
            [](const ListItem& item) { return item.key; },
            [](reactive::ReactiveScope&, const ListItem& item) {
                return std::make_shared<TestRow>(item.key);
            },
            {},
            row_height
        );
    }

    auto click_row(
        scene::NanSceneTree& tree,
        const int row,
        const float row_height,
        const scene::KeyModifiers mods = {}
    ) -> void {
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent {
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                foundation::NanPoint(
                    150.0F,
                    static_cast<float>(row) * row_height + row_height / 2.0F
                ),
                mods,
            }
        );
    }
} // namespace

TEST_CASE("VirtualListView materializes only the visible window", "[widget][virtual-list]") {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));

    scene::NanSceneTree tree;
    tree.set_root(list);
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);

    REQUIRE(list->item_count() == 1000);
    // 视口 300 / 行高 30 -> 窗口 0..9；其余 990 行不物化。
    REQUIRE(list->row_node(0) != nullptr);
    REQUIRE(list->row_node(9) != nullptr);
    REQUIRE(list->row_node(10) == nullptr);
    REQUIRE(list->row_node(999) == nullptr);

    auto* first = list->row_node(0);
    REQUIRE(first != nullptr);
    REQUIRE(first->height() == Catch::Approx(30.0F));
    REQUIRE(first->global_bounds().get_top() == Catch::Approx(0.0F));

    // 内容高度 = 1000 × 30，视口 300；行 10 在窗口外。
    REQUIRE(list->content_size().get_height() == Catch::Approx(30000.0F));
    REQUIRE(list->maximum_scroll_offset().get_y() == Catch::Approx(29700.0F));
}

TEST_CASE(
    "VirtualListView scroll moves the window and reuses overlapping rows",
    "[widget][virtual-list]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    auto* row_1 = list->row_node(1);
    REQUIRE(row_1 != nullptr);

    // 下滚一个行高：窗口 1..10，行 0 销毁、行 10 创建、行 1 复用。
    list->set_scroll_offset(foundation::NanPoint(0.0F, 30.0F));
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);

    REQUIRE(list->row_node(0) == nullptr);
    REQUIRE(list->row_node(1) == row_1);
    REQUIRE(list->row_node(10) != nullptr);
    REQUIRE(list->row_node(1)->global_bounds().get_top() == Catch::Approx(0.0F));
    REQUIRE(list->row_node(10)->global_bounds().get_top() == Catch::Approx(270.0F));

    // 滚到底：窗口 990..999。
    list->set_scroll_offset(list->maximum_scroll_offset());
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);
    REQUIRE(list->row_node(990) != nullptr);
    REQUIRE(list->row_node(999) != nullptr);
    REQUIRE(list->row_node(989) == nullptr);

    // 偏移被钳制在最大滚动位置。
    list->set_scroll_offset(foundation::NanPoint(0.0F, 99999.0F));
    REQUIRE(list->scroll_offset().get_y() == Catch::Approx(29700.0F));
}

TEST_CASE("VirtualListView wheel scrolls the window", "[widget][virtual-list][input]") {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    tree.dispatch_mouse_wheel(
        scene::MouseWheelEvent(
            foundation::NanPoint(10.0F, 10.0F),
            foundation::NanPoint(0.0F, -1.0F)
        )
    );
    // wheel_step 默认 40：偏移 +40，窗口 1..11。
    REQUIRE(list->scroll_offset().get_y() == Catch::Approx(40.0F));
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);
    REQUIRE(list->row_node(1) != nullptr);
    REQUIRE(list->row_node(11) != nullptr);
    REQUIRE(list->row_node(0) == nullptr);
}

TEST_CASE(
    "VirtualListView click selection: exclusive, range and toggle",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));
    int selection_changes = 0;
    auto sub = list->selection_changed().subscribe([&] { ++selection_changes; });

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    // 普通点击：独占选中行 1，并取得键盘焦点。
    click_row(tree, 1, 30.0F);
    REQUIRE(list->is_selected(1));
    REQUIRE(list->selected_count() == 1);
    REQUIRE(tree.focused_node() == list.get());
    REQUIRE(selection_changes == 1);

    // 换行点击：替换而不是累加。
    click_row(tree, 2, 30.0F);
    REQUIRE_FALSE(list->is_selected(1));
    REQUIRE(list->is_selected(2));
    REQUIRE(list->selected_count() == 1);

    // 多选模式 + shift 范围。
    list->set_selection_mode(widget::SelectionMode::multiple);
    click_row(tree, 4, 30.0F);
    REQUIRE(list->is_selected(4));
    click_row(tree, 2, 30.0F, scene::KeyModifiers {.shift = true});
    REQUIRE(list->selected_count() == 3);
    REQUIRE(list->is_selected(2));
    REQUIRE(list->is_selected(3));
    REQUIRE(list->is_selected(4));

    // ctrl 切换：加入 6、移除 3。
    click_row(tree, 6, 30.0F, scene::KeyModifiers {.ctrl = true});
    REQUIRE(list->selected_count() == 4);
    click_row(tree, 3, 30.0F, scene::KeyModifiers {.ctrl = true});
    REQUIRE(list->selected_count() == 3);
    REQUIRE_FALSE(list->is_selected(3));
    REQUIRE(list->is_selected(6));
}

TEST_CASE(
    "VirtualListView keyboard: selection follows in single, toggles in multiple",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    tree.set_focus(list.get());

    // single 模式：方向键移动活动行并跟随选中。
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::down, scene::KeyEvent::Action::press)
    );
    REQUIRE(list->first_selected() == std::optional<int> {1});

    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::down, scene::KeyEvent::Action::press)
    );
    REQUIRE(list->first_selected() == std::optional<int> {2});

    // End：跳到末尾并滚入视口。
    tree.dispatch_key(scene::KeyEvent(foundation::LogicalKey::end, scene::KeyEvent::Action::press));
    REQUIRE(list->first_selected() == std::optional<int> {99});
    REQUIRE(list->row_node(99) != nullptr);

    // Enter 激活当前行。
    int activated = -1;
    auto act_sub = list->activated().subscribe([&](const int key) { activated = key; });
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::enter, scene::KeyEvent::Action::press)
    );
    REQUIRE(activated == 99);

    // multiple 模式：方向键只移动活动行，Space 切换选中。
    list->set_selection_mode(widget::SelectionMode::multiple);
    list->clear_selection();
    tree.set_focus(list.get());
    tree.dispatch_key(scene::KeyEvent(foundation::LogicalKey::up, scene::KeyEvent::Action::press));
    tree.dispatch_key(scene::KeyEvent(foundation::LogicalKey::up, scene::KeyEvent::Action::press));
    REQUIRE(list->selected_count() == 0);
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::space, scene::KeyEvent::Action::press)
    );
    REQUIRE(list->is_selected(97));
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::space, scene::KeyEvent::Action::press)
    );
    REQUIRE_FALSE(list->is_selected(97));
}

TEST_CASE(
    "VirtualListView programmatic select scrolls into view",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    REQUIRE(list->select(500));
    REQUIRE(list->is_selected(500));
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);
    REQUIRE(list->row_node(500) != nullptr);
    REQUIRE(list->row_node(0) == nullptr);

    // 未知键：拒绝且不改动选中。
    REQUIRE_FALSE(list->select(12345));
    REQUIRE(list->selected_count() == 1);
}

TEST_CASE(
    "VirtualListView source replacement prunes selection and clamps active",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(10));
    list->set_selection_mode(widget::SelectionMode::multiple);
    (void)list->select(1);
    (void)list->select(2);
    (void)list->select(3);

    int changes = 0;
    auto sub = list->selection_changed().subscribe([&] { ++changes; });

    // 数据缩小：key 3 消失，选中裁剪到 {1, 2}。
    list->set_source(make_items(3));
    REQUIRE(list->selected_keys() == std::vector<int> {1, 2});
    REQUIRE(changes == 1);

    // 再次缩小：key 2 消失，只留 {1}。
    std::vector<ListItem> two {{.key = 0, .label = "a"}, {.key = 1, .label = "b"}};
    list->set_source(two);
    REQUIRE(list->selected_keys() == std::vector<int> {1});
    REQUIRE(changes == 2);
}

TEST_CASE(
    "VirtualListView rejects duplicate keys without touching state",
    "[widget][virtual-list]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(3));
    auto* row_1 = list->row_node(1);

    REQUIRE_THROWS_AS(
        list->set_source(
            std::vector<ListItem> {{.key = 1, .label = "a"}, {.key = 1, .label = "b"}}
        ),
        std::invalid_argument
    );
    REQUIRE(list->item_count() == 3);
    REQUIRE(list->row_node(1) == row_1);
}

TEST_CASE("VirtualListView tracks a bound reactive source", "[widget][virtual-list][reactive]") {
    reactive::Graph graph;
    reactive::Signal<std::vector<ListItem>> items {
        graph,
        make_items(1000),
    };
    auto list = make_list(graph);
    list->set_model(items);

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    REQUIRE(list->item_count() == 1000);
    REQUIRE(list->row_node(5) != nullptr);
    REQUIRE(list->row_node(999) == nullptr);

    // 源缩小：窗口按新数据重算，窗口外行销毁。
    items.set(make_items(5));
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);
    REQUIRE(list->item_count() == 5);
    REQUIRE(list->row_node(4) != nullptr);
    REQUIRE(list->row_node(5) == nullptr);
}

TEST_CASE("VirtualListView works detached before entering a tree", "[widget][virtual-list]") {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));

    // 脱离树时手动给一个视口盒子：窗口照常物化。
    (void)list->measure_layout(
        foundation::NanLayoutConstraints::tight(foundation::NanSize(300.0F, 300.0F))
    );
    list->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 300.0F, 300.0F));

    REQUIRE(list->item_count() == 100);
    REQUIRE(list->row_node(0) != nullptr);
    REQUIRE(list->row_node(9) != nullptr);
    REQUIRE(list->row_node(10) == nullptr);

    list->set_scroll_offset(foundation::NanPoint(0.0F, 30.0F));
    REQUIRE(list->row_node(0) == nullptr);
    REQUIRE(list->row_node(1) != nullptr);
}

TEST_CASE(
    "VirtualListView typeahead navigates and follows selection in single mode",
    "[widget][virtual-list][selection][keyboard]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(
        std::vector<ListItem> {
            {.key = 0, .label = "alpha"},
            {.key = 1, .label = "beta"},
            {.key = 2, .label = "gamma"},
            {.key = 3, .label = "delta"},
            {.key = 4, .label = "epsilon"},
        }
    );
    list->set_label_function([](const ListItem& item) { return item.label; });

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    tree.set_focus(list.get());

    // 键入 "b"：首个以 b 开头的成员（beta, index 1）；single 模式选中跟随。
    tree.dispatch_text_input(scene::TextInputEvent("b"));
    REQUIRE(list->first_selected() == std::optional<int> {1});

    // 键入 "g"：累积缓冲 "bg" 无任何前缀匹配，回落到单字符 "g" → gamma (index 2)。
    tree.dispatch_text_input(scene::TextInputEvent("g"));
    REQUIRE(list->first_selected() == std::optional<int> {2});
}

TEST_CASE(
    "VirtualListView programmatic select_range scrolls to the far end in multiple mode",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));
    list->set_selection_mode(widget::SelectionMode::multiple);

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    REQUIRE(list->select_range(0, 500));
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);
    REQUIRE(list->selected_count() == 501);
    REQUIRE(list->row_node(500) != nullptr);
}

TEST_CASE(
    "VirtualListView select_range scrolls even when the span is already selected",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));
    list->set_selection_mode(widget::SelectionMode::multiple);

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    // 先整段选中（滚动到 500），再滚回顶部，重复同跨度：无选中变化也必须滚动到 to。
    REQUIRE(list->select_range(0, 500));
    REQUIRE(list->selected_count() == 501);
    list->set_scroll_offset(foundation::NanPoint(0.0F, 0.0F));
    REQUIRE_FALSE(list->select_range(0, 500));
    REQUIRE(list->row_node(500) != nullptr);
}

TEST_CASE(
    "VirtualListView set_source resets the typeahead buffer",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(
        std::vector<ListItem> {
            {.key = 0, .label = "alpha"},
            {.key = 1, .label = "beta"},
            {.key = 2, .label = "boar"},
            {.key = 3, .label = "gamma"},
        }
    );
    list->set_label_function([](const ListItem& item) { return item.label; });

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    tree.set_focus(list.get());

    // "b" → beta (index 1)，缓冲 "b"。
    tree.dispatch_text_input(scene::TextInputEvent("b"));
    REQUIRE(list->first_selected() == std::optional<int> {1});

    // 重推同一批数据：缓冲必须清空，否则 "b"+"o" 会拼成 "bo" 命中 boar (index 2)。
    list->set_source(
        std::vector<ListItem> {
            {.key = 0, .label = "alpha"},
            {.key = 1, .label = "beta"},
            {.key = 2, .label = "boar"},
            {.key = 3, .label = "gamma"},
        }
    );
    tree.dispatch_text_input(scene::TextInputEvent("o"));
    REQUIRE(list->first_selected() == std::optional<int> {1});
}

TEST_CASE(
    "VirtualListView focus leave resets the typeahead buffer",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(
        std::vector<ListItem> {
            {.key = 0, .label = "alpha"},
            {.key = 1, .label = "beta"},
            {.key = 2, .label = "boar"},
            {.key = 3, .label = "gamma"},
        }
    );
    list->set_label_function([](const ListItem& item) { return item.label; });

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    tree.set_focus(list.get());

    // "b" → beta (index 1)，缓冲 "b"。
    tree.dispatch_text_input(scene::TextInputEvent("b"));
    REQUIRE(list->first_selected() == std::optional<int> {1});

    // 失焦再聚焦：缓冲必须清空，否则 "b"+"o" 会拼成 "bo" 命中 boar (index 2)。
    tree.set_focus(nullptr);
    tree.set_focus(list.get());
    tree.dispatch_text_input(scene::TextInputEvent("o"));
    REQUIRE(list->first_selected() == std::optional<int> {1});
}

TEST_CASE(
    "VirtualListView set_row_height relayouts rows to the new height",
    "[widget][virtual-list]"
) {
    reactive::Graph graph;
    auto list = make_list(graph, 30.0F);
    list->set_source(make_items(100));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    REQUIRE(list->row_node(0) != nullptr);
    REQUIRE(list->row_node(9) != nullptr);
    REQUIRE(list->row_node(10) == nullptr);

    // 行高 30 → 60：视口只能容纳 5 行，窗口收缩，行按新行高落位。
    list->set_row_height(60.0F);
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);
    REQUIRE(list->row_node(0) != nullptr);
    REQUIRE(list->row_node(4) != nullptr);
    REQUIRE(list->row_node(5) == nullptr);
    // 第 2 行（window 内 index 2）占据 y 120..180。
    auto* row = list->row_node(2);
    REQUIRE(row != nullptr);
    REQUIRE(row->position().get_y() == Catch::Approx(120.0F));
    REQUIRE(row->height() == Catch::Approx(60.0F));
}

TEST_CASE(
    "VirtualListView wheel scrolls upward and clamps at the boundaries",
    "[widget][virtual-list][input]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    // 底部：滚到底后偏移 == 内容高 - 视口高。
    for (int i = 0; i < 100; ++i) {
        tree.dispatch_mouse_wheel(
            scene::MouseWheelEvent(
                foundation::NanPoint(10.0F, 10.0F),
                foundation::NanPoint(0.0F, -1.0F)
            )
        );
    }
    REQUIRE(list->scroll_offset().get_y() == Catch::Approx(3000.0F - 300.0F));
    // 越界下滚：夹在最大值。
    tree.dispatch_mouse_wheel(
        scene::MouseWheelEvent(
            foundation::NanPoint(10.0F, 10.0F),
            foundation::NanPoint(0.0F, -1.0F)
        )
    );
    REQUIRE(list->scroll_offset().get_y() == Catch::Approx(2700.0F));

    // 上滚：偏移回落。
    tree.dispatch_mouse_wheel(
        scene::MouseWheelEvent(foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint(0.0F, 1.0F))
    );
    REQUIRE(list->scroll_offset().get_y() == Catch::Approx(2660.0F));

    // 顶部：滚到 0 后越界上滚夹在 0。
    for (int i = 0; i < 100; ++i) {
        tree.dispatch_mouse_wheel(
            scene::MouseWheelEvent(
                foundation::NanPoint(10.0F, 10.0F),
                foundation::NanPoint(0.0F, 1.0F)
            )
        );
    }
    REQUIRE(list->scroll_offset().get_y() == Catch::Approx(0.0F));
}

TEST_CASE(
    "VirtualListView mouse release and non-left buttons do not select",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    // 左键释放：不选择、不抢焦点。
    REQUIRE(tree.focused_node() == nullptr);
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::release,
            foundation::NanPoint(15.0F, 15.0F),
        }
    );
    REQUIRE(list->selected_count() == 0);
    REQUIRE(tree.focused_node() == nullptr);

    // 右键按下：列表不选择（焦点跟随按下是场景层规则，与列表无关）。
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::right,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(15.0F, 15.0F),
        }
    );
    REQUIRE(list->selected_count() == 0);
    REQUIRE(tree.focused_node() == list.get());
}

TEST_CASE(
    "VirtualListView Enter and Space act only on the initial press",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    tree.set_focus(list.get());

    // multiple 模式、活动行 0：Space 只在初次按下切换（与 DropdownMenu /
    // Pagination 的"按住不放不重复提交"约定一致），repeat 不重复切换。
    list->set_selection_mode(widget::SelectionMode::multiple);
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::space, scene::KeyEvent::Action::repeat)
    );
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::space, scene::KeyEvent::Action::repeat)
    );
    REQUIRE_FALSE(list->is_selected(0));
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::space, scene::KeyEvent::Action::press)
    );
    REQUIRE(list->is_selected(0));
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::space, scene::KeyEvent::Action::repeat)
    );
    REQUIRE(list->is_selected(0));

    // Enter 只在初次按下激活。
    int activations = 0;
    auto act_sub = list->activated().subscribe([&](const int) { ++activations; });
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::enter, scene::KeyEvent::Action::repeat)
    );
    REQUIRE(activations == 0);
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::enter, scene::KeyEvent::Action::press)
    );
    REQUIRE(activations == 1);

    // 方向键 repeat 仍逐次移动活动行（漫游的重复语义不受影响）。
    list->set_selection_mode(widget::SelectionMode::single);
    list->clear_selection();
    tree.set_focus(list.get());
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::down, scene::KeyEvent::Action::repeat)
    );
    REQUIRE(list->first_selected() == std::optional<int> {1});
}

TEST_CASE(
    "VirtualListView PageUp and PageDown jump to the row edges",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    tree.set_focus(list.get());

    // PageDown：跳到末尾行并跟随选中（single 模式）。
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::page_down, scene::KeyEvent::Action::press)
    );
    REQUIRE(list->first_selected() == std::optional<int> {99});
    REQUIRE(list->row_node(99) != nullptr);

    // PageUp：跳回首行。
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::page_up, scene::KeyEvent::Action::press)
    );
    REQUIRE(list->first_selected() == std::optional<int> {0});
    REQUIRE(list->row_node(0) != nullptr);
}

TEST_CASE(
    "VirtualListView empty source is inert for input and selection",
    "[widget][virtual-list][input]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source({});

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    tree.set_focus(list.get());

    REQUIRE(list->item_count() == 0);
    REQUIRE(list->selected_count() == 0);
    REQUIRE(list->row_node(0) == nullptr);

    // 点击 / 滚轮 / 方向键 / 输入 / Enter 全部无效果也不崩溃。
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(15.0F, 15.0F),
        }
    );
    tree.dispatch_mouse_wheel(
        scene::MouseWheelEvent(
            foundation::NanPoint(10.0F, 10.0F),
            foundation::NanPoint(0.0F, -1.0F)
        )
    );
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::down, scene::KeyEvent::Action::press)
    );
    tree.dispatch_text_input(scene::TextInputEvent("a"));
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::enter, scene::KeyEvent::Action::press)
    );
    REQUIRE(list->selected_count() == 0);
    REQUIRE(list->scroll_offset().get_y() == Catch::Approx(0.0F));
}

TEST_CASE("VirtualListView clips rows to its viewport box", "[widget][virtual-list]") {
    reactive::Graph graph;
    auto list = make_list(graph);
    // 自身是滚动视口：滚出盒子的行不得绘制到视口外（与 ScrollView 同语义）。
    REQUIRE(list->overflow() == scene::ControlOverflow::clip);
}

TEST_CASE(
    "VirtualListView same-window source replacement refreshes reused rows",
    "[widget][virtual-list]"
) {
    reactive::Graph graph;
    auto list = widget::VirtualListView<ListItem, int, TestRow>::create(
        graph,
        [](const ListItem& item) { return item.key; },
        [](reactive::ReactiveScope&, const ListItem& item) {
            return std::make_shared<TestRow>(item.key, item.label);
        },
        [](TestRow& row, const ListItem& item) { row.label_ = item.label; },
        30.0F
    );
    list->set_source(std::vector<ListItem> {{.key = 0, .label = "old"}});

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    REQUIRE(list->row_node(0) != nullptr);
    REQUIRE(list->row_node(0)->label_ == "old");

    // 同数量同 key 换内容：窗口区间不变也必须推送 set_items（update_ 生效）。
    list->set_source(std::vector<ListItem> {{.key = 0, .label = "new"}});
    REQUIRE(list->row_node(0)->label_ == "new");
}

TEST_CASE(
    "VirtualListView same-window source replacement swaps row keys",
    "[widget][virtual-list]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(3));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    REQUIRE(list->row_node(0) != nullptr);

    // 同数量不同 key：窗口区间不变也必须对账（旧行销毁、新行创建）。
    list->set_source(
        std::vector<ListItem> {
            {.key = 10, .label = "a"},
            {.key = 11, .label = "b"},
            {.key = 12, .label = "c"},
        }
    );
    REQUIRE(list->row_node(0) == nullptr);
    REQUIRE(list->row_node(10) != nullptr);
}

TEST_CASE(
    "VirtualListView pre-layout programmatic select scrolls the target into view",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));

    scene::NanSceneTree tree;
    tree.set_root(list);

    // 首布局前 select：偏移不得按零高度视口跳到行底，布局后目标行必须可见。
    REQUIRE(list->select(500));
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 300.0F)) >= 1);
    REQUIRE(list->row_node(500) != nullptr);
}

TEST_CASE(
    "VirtualListView click on the bottom edge hits the last visible row",
    "[widget][virtual-list][selection]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(100));

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    // 视口 300 = 行高 30 × 10：点击底边 y==300 命中最后可见行 9，而不是未物化的行 10。
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(150.0F, 300.0F),
        }
    );
    REQUIRE(list->is_selected(9));
    REQUIRE_FALSE(list->is_selected(10));
}

TEST_CASE(
    "VirtualListView inside Expanded virtualizes to the allotted viewport",
    "[widget][virtual-list]"
) {
    reactive::Graph graph;
    auto list = make_list(graph);
    list->set_source(make_items(1000));
    auto expanded = widget::Expanded::create();
    expanded->set_child(list);

    scene::NanSceneTree tree;
    tree.set_root(expanded);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));

    // Expanded 把列表摆进 300 高盒子：只物化视口内行，不按内容高全量物化。
    REQUIRE(list->height() == Catch::Approx(300.0F));
    REQUIRE(list->row_node(0) != nullptr);
    REQUIRE(list->row_node(9) != nullptr);
    REQUIRE(list->row_node(10) == nullptr);
}

TEST_CASE(
    "VirtualListView binding is not restored after re-entering a tree",
    "[widget][virtual-list][reactive]"
) {
    reactive::Graph graph;
    reactive::Signal<std::vector<ListItem>> items {
        graph,
        make_items(5),
    };
    auto list = make_list(graph);
    list->set_model(items);

    scene::NanSceneTree tree;
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    REQUIRE(list->item_count() == 5);

    // 移除树后绑定已解绑：源变化不再追踪（与 ForEach::bind 一致）。
    tree.set_root(nullptr);
    items.set(make_items(9));
    REQUIRE(list->item_count() == 5);

    // 重入树：绑定不自动恢复，需重新 set_model。
    tree.set_root(list);
    (void)tree.layout_root(foundation::NanSize(300.0F, 300.0F));
    REQUIRE(list->item_count() == 5);
}
