//
// MenuItem model tests — 菜单族共享的条目规则与勾选语义。
//

#include <nandina/widget/menu_item.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace nandina;

namespace
{
    auto make_item(
        std::string id,
        const widget::MenuItemKind kind = widget::MenuItemKind::action,
        std::string label = {}
    ) -> widget::MenuItem {
        widget::MenuItem item;
        item.id = std::move(id);
        item.kind = kind;
        item.label = label.empty() ? item.id : std::move(label);
        return item;
    }

    /// 一层典型菜单：动作项、分隔线、分组标题、两个 radio、一个 checkbox、一个禁用项。
    auto sample_menu() -> std::vector<widget::MenuItem> {
        return std::vector<widget::MenuItem> {
            make_item("new"),
            make_item("sep", widget::MenuItemKind::separator),
            make_item("group", widget::MenuItemKind::label, "编辑"),
            make_item("left", widget::MenuItemKind::radio),
            make_item("center", widget::MenuItemKind::radio),
            make_item("wrap", widget::MenuItemKind::checkbox),
            make_item("locked", widget::MenuItemKind::action),
        };
    }
} // namespace

TEST_CASE("menu item focusability skips structure but keeps disabled items", "[menu][model]") {
    using widget::MenuItemKind;

    // 结构性条目不参与漫游。
    CHECK_FALSE(widget::menu_item_is_focusable(make_item("s", MenuItemKind::separator)));
    CHECK_FALSE(widget::menu_item_is_focusable(make_item("l", MenuItemKind::label)));

    // 其余种类都可聚焦。
    CHECK(widget::menu_item_is_focusable(make_item("a", MenuItemKind::action)));
    CHECK(widget::menu_item_is_focusable(make_item("c", MenuItemKind::checkbox)));
    CHECK(widget::menu_item_is_focusable(make_item("r", MenuItemKind::radio)));
    CHECK(widget::menu_item_is_focusable(make_item("m", MenuItemKind::submenu)));

    // disabled 仍然可聚焦——用户要能用方向键走到它上面，才知道它存在且不可用。
    auto disabled = make_item("d", MenuItemKind::action);
    disabled.disabled = true;
    CHECK(widget::menu_item_is_focusable(disabled));
    CHECK_FALSE(widget::menu_item_is_activatable(disabled));

    // 可聚焦且未禁用才可激活。
    CHECK(widget::menu_item_is_activatable(make_item("a", MenuItemKind::action)));
    CHECK_FALSE(widget::menu_item_is_activatable(make_item("s", MenuItemKind::separator)));
}

TEST_CASE("menu typeahead matches label only, never the shortcut hint", "[menu][model][typeahead]") {
    auto item = make_item("save", widget::MenuItemKind::action, "Save");
    item.shortcut = "Ctrl+S";

    CHECK(widget::menu_item_typeahead_text(item) == "Save");

    // 结构性条目不参与匹配：返回空串，RovingFocus 会跳过空标签。
    CHECK(widget::menu_item_typeahead_text(make_item("s", widget::MenuItemKind::separator)).empty());
    CHECK(widget::menu_item_typeahead_text(make_item("l", widget::MenuItemKind::label)).empty());
}

TEST_CASE("find_menu_item searches one layer only", "[menu][model]") {
    auto submenu = make_item("share", widget::MenuItemKind::submenu);
    submenu.children.push_back(make_item("email"));

    std::vector<widget::MenuItem> items {make_item("new"), std::move(submenu)};

    CHECK(widget::find_menu_item(items, "new") != nullptr);
    CHECK(widget::find_menu_item(items, "share") != nullptr);
    // 子菜单是独立的一层：本层查找不应递归进去。
    CHECK(widget::find_menu_item(items, "email") == nullptr);
    CHECK(widget::find_menu_item(items, "missing") == nullptr);
    CHECK(widget::find_menu_item(items, "email") == nullptr);

    // const 重载同样只查一层。
    const auto& view = items;
    CHECK(widget::find_menu_item(view, "share") != nullptr);
    CHECK(widget::find_menu_item(view, "email") == nullptr);
}

TEST_CASE("none selection mode keeps no checked state", "[menu][selection]") {
    auto items = sample_menu();
    widget::MenuSelection selection {widget::MenuSelectionMode::none};

    int changes = 0;
    const auto subscription = selection.changed().subscribe([&changes](const std::string&) {
        ++changes;
    });

    CHECK_FALSE(selection.toggle(items, "wrap"));
    CHECK_FALSE(widget::MenuSelection::is_checked(items, "wrap"));
    CHECK(changes == 0);
}

TEST_CASE("single selection mode clears sibling radios only", "[menu][selection]") {
    auto items = sample_menu();
    widget::MenuSelection selection {widget::MenuSelectionMode::single};

    std::vector<std::string> observed;
    const auto subscription = selection.changed().subscribe(
        [&observed](const std::string& id) { observed.push_back(id); }
    );

    CHECK(selection.toggle(items, "left"));
    CHECK(widget::MenuSelection::is_checked(items, "left"));

    // 选中另一个 radio 会清掉同层的其他 radio。
    CHECK(selection.toggle(items, "center"));
    CHECK(widget::MenuSelection::is_checked(items, "center"));
    CHECK_FALSE(widget::MenuSelection::is_checked(items, "left"));

    // checkbox 与 radio 互不干扰：single 的互斥只发生在 radio 之间。
    CHECK(selection.toggle(items, "wrap"));
    CHECK(widget::MenuSelection::is_checked(items, "center"));
    CHECK(widget::MenuSelection::is_checked(items, "wrap"));

    CHECK(observed == std::vector<std::string> {"left", "center", "wrap"});

    // 再次切换同一个 radio 是取消勾选，不会去清别人。
    CHECK(selection.toggle(items, "center"));
    CHECK_FALSE(widget::MenuSelection::is_checked(items, "center"));
    CHECK(widget::MenuSelection::is_checked(items, "wrap"));
}

TEST_CASE("multiple selection mode keeps items independent", "[menu][selection]") {
    auto items = sample_menu();
    widget::MenuSelection selection {widget::MenuSelectionMode::multiple};

    CHECK(selection.toggle(items, "left"));
    CHECK(selection.toggle(items, "center"));
    CHECK(widget::MenuSelection::is_checked(items, "left"));
    CHECK(widget::MenuSelection::is_checked(items, "center"));

    CHECK(widget::MenuSelection::checked_ids(items) == std::vector<std::string> {"left", "center"});
}

TEST_CASE("selection rejects structural, disabled and unknown targets", "[menu][selection]") {
    auto items = sample_menu();
    widget::MenuSelection selection {widget::MenuSelectionMode::single};

    int changes = 0;
    const auto subscription = selection.changed().subscribe([&changes](const std::string&) {
        ++changes;
    });

    // 结构性条目没有勾选语义。
    CHECK_FALSE(selection.toggle(items, "sep"));
    CHECK_FALSE(selection.toggle(items, "group"));
    // 非勾选种类（action / submenu）同样拒绝。
    CHECK_FALSE(selection.toggle(items, "new"));
    // 未知 id。
    CHECK_FALSE(selection.toggle(items, "nope"));

    // disabled 的勾选项不可被用户改变。
    auto disabled = make_item("frozen", widget::MenuItemKind::checkbox);
    disabled.disabled = true;
    std::vector<widget::MenuItem> mixed {disabled};
    CHECK_FALSE(selection.toggle(mixed, "frozen"));
    CHECK_FALSE(widget::MenuSelection::is_checked(mixed, "frozen"));

    CHECK(changes == 0);

    // 空列表不崩溃。
    std::vector<widget::MenuItem> empty;
    CHECK_FALSE(selection.toggle(empty, "anything"));
    CHECK(widget::MenuSelection::checked_ids(empty).empty());
}

TEST_CASE("set_checked writes silently without exclusivity or events", "[menu][selection]") {
    auto items = sample_menu();
    widget::MenuSelection selection {widget::MenuSelectionMode::single};

    int changes = 0;
    const auto subscription = selection.changed().subscribe([&changes](const std::string&) {
        ++changes;
    });

    CHECK(widget::MenuSelection::set_checked(items, "left", true));
    CHECK(widget::MenuSelection::set_checked(items, "center", true));

    // 静默设置不做 single 互斥清理：两个 radio 可以同时为真，供程序侧同步使用。
    CHECK(widget::MenuSelection::is_checked(items, "left"));
    CHECK(widget::MenuSelection::is_checked(items, "center"));
    CHECK(changes == 0);

    // 非勾选种类与未知 id 不写入。
    CHECK_FALSE(widget::MenuSelection::set_checked(items, "new", true));
    CHECK_FALSE(widget::MenuSelection::set_checked(items, "missing", true));

    // checked_ids 按条目顺序返回，且只包含 checkbox / radio。
    CHECK(widget::MenuSelection::checked_ids(items) == std::vector<std::string> {"left", "center"});
}

TEST_CASE("selection mode can be changed at runtime", "[menu][selection]") {
    auto items = sample_menu();
    widget::MenuSelection selection {widget::MenuSelectionMode::single};
    CHECK(selection.mode() == widget::MenuSelectionMode::single);

    CHECK(selection.toggle(items, "left"));
    CHECK(selection.toggle(items, "center"));
    CHECK_FALSE(widget::MenuSelection::is_checked(items, "left"));

    // 切到 multiple 后不再互斥。
    selection.set_mode(widget::MenuSelectionMode::multiple);
    CHECK(selection.mode() == widget::MenuSelectionMode::multiple);
    CHECK(selection.toggle(items, "left"));
    CHECK(widget::MenuSelection::is_checked(items, "left"));
    CHECK(widget::MenuSelection::is_checked(items, "center"));
}
