//
// DropdownMenu tests: Popover-backed menu over the shared MenuItem model.
//

#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/builtin_component_traits.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/dropdown_menu.hpp>
#include <nandina/widget/popover.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;
        int text_calls = 0;
        int lines = 0;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void
        draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {
            ++rounded_rects;
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++lines;
        }
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++text_calls;
        }
    };

    [[nodiscard]] auto item(
        std::string id,
        std::string label,
        const widget::MenuItemKind kind = widget::MenuItemKind::action,
        const bool disabled = false,
        const bool checked = false,
        std::string shortcut = {}
    ) -> widget::MenuItem {
        widget::MenuItem result;
        result.id = std::move(id);
        result.label = std::move(label);
        result.kind = kind;
        result.disabled = disabled;
        result.checked = checked;
        result.shortcut = std::move(shortcut);
        return result;
    }

    /// 覆盖所有 kind 的一层条目，供多数用例复用。
    [[nodiscard]] auto sample_items() -> std::vector<widget::MenuItem> {
        return {
            item("new", "New File", widget::MenuItemKind::action, false, false, "Ctrl+N"),
            item("sep", "", widget::MenuItemKind::separator),
            item("group", "View", widget::MenuItemKind::label),
            item("wrap", "Word Wrap", widget::MenuItemKind::checkbox, false, true),
            item("ascii", "ASCII Mode", widget::MenuItemKind::radio, false, false),
            item("pro", "Pro Mode", widget::MenuItemKind::radio, false, true),
            item("more", "More Tools", widget::MenuItemKind::submenu),
            item("off", "Unavailable", widget::MenuItemKind::action, true),
        };
    }

    struct MenuHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(360.0F, 260.0F));
        std::shared_ptr<widget::Button> trigger = widget::Button::create("Open");
        std::shared_ptr<widget::DropdownMenu> menu;
        scene::NanSceneTree tree;
        foundation::NanSize viewport {360.0F, 260.0F};

        explicit MenuHarness(
            std::vector<widget::MenuItem> items = sample_items(),
            const foundation::NanPoint menu_position = foundation::NanPoint(0.0F, 80.0F)
        ):
            menu(widget::DropdownMenu::create(trigger, std::move(items))) {
            tree.set_theme_manager(themes);
            menu->set_position(menu_position);
            body->add_child(menu);
            // 第二个子节点阻止 NanControl 的单子拉伸路径把菜单撑满内容层。
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            host->set_content(body);
            tree.set_root(host);
        }

        void layout() {
            (void)tree.layout_root(viewport);
        }

        /// 打开并完成一轮布局；返回浮层里的菜单表面。
        auto open_and_layout() -> scene::NanControl* {
            layout();
            menu->open();
            layout();
            return surface();
        }

        /// 浮层面板（PopoverSurface）：DismissLayer → FocusScope → PopoverSurface。
        [[nodiscard]] auto panel() const -> scene::NanControl* {
            auto* layer = host->layer_at(1);
            auto* overlay_root = layer != nullptr ? layer->layout_root() : nullptr;
            if (overlay_root == nullptr || overlay_root->child_count() == 0) {
                return nullptr;
            }
            auto* dismiss = overlay_root->get_child(0)->as_control();
            auto* scope = dismiss != nullptr && dismiss->child_count() > 0
                ? dismiss->get_child(0)->as_control()
                : nullptr;
            return scope != nullptr && scope->child_count() > 0 ? scope->get_child(0)->as_control()
                                                                : nullptr;
        }

        /// 菜单条目列表容器 = Popover 的 content。
        [[nodiscard]] auto surface() const -> scene::NanControl* {
            auto* popover_surface = panel();
            return popover_surface != nullptr && popover_surface->child_count() > 0
                ? popover_surface->get_child(0)->as_control()
                : nullptr;
        }
    };

    void collect_semantics(const semantics::Node& node, std::vector<const semantics::Node*>& out) {
        out.push_back(&node);
        for (const auto& child: node.children) {
            collect_semantics(child, out);
        }
    }

    [[nodiscard]] auto semantics_nodes(const semantics::Tree& tree)
        -> std::vector<const semantics::Node*> {
        std::vector<const semantics::Node*> out;
        for (const auto& root: tree.roots) {
            collect_semantics(root, out);
        }
        return out;
    }

    [[nodiscard]] auto find_semantics_role(
        const semantics::Tree& tree,
        const semantics::Role role,
        const std::string_view label = {}
    ) -> const semantics::Node* {
        for (const auto* node: semantics_nodes(tree)) {
            if (node->properties.role == role && (label.empty() || node->properties.label == label))
            {
                return node;
            }
        }
        return nullptr;
    }
} // namespace

TEST_CASE("dropdown menu is closed and sized to its trigger by default", "[dropdown-menu]") {
    widget::DropdownMenu empty;
    REQUIRE_FALSE(empty.is_open());
    REQUIRE(empty.trigger() == nullptr);
    REQUIRE(empty.item_count() == 0);
    REQUIRE(empty.active_index() == -1);
    REQUIRE(empty.selection_mode() == widget::MenuSelectionMode::none);
    REQUIRE(empty.z_index_hint() == 0);
    const auto measured = empty.measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(measured.get_width() == Catch::Approx(0.0F));
    REQUIRE(measured.get_height() == Catch::Approx(0.0F));

    auto trigger = widget::Button::create("Open");
    auto menu = widget::DropdownMenu::create(trigger, sample_items());
    const auto measured_menu = menu->measure_layout(scene::LayoutConstraints::loose());
    const auto measured_trigger = trigger->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(measured_menu.get_width() == Catch::Approx(measured_trigger.get_width()));
    REQUIRE(measured_menu.get_height() == Catch::Approx(measured_trigger.get_height()));
    REQUIRE(menu->item_count() == 8);
    REQUIRE(menu->items().size() == 8);
}

TEST_CASE("dropdown menu open and close are idempotent", "[dropdown-menu][overlay]") {
    MenuHarness harness;
    harness.layout();

    harness.menu->open();
    REQUIRE(harness.menu->is_open());
    harness.menu->open();
    REQUIRE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 1);
    REQUIRE(harness.surface() != nullptr);

    harness.menu->close();
    REQUIRE_FALSE(harness.menu->is_open());
    harness.menu->close();
    REQUIRE_FALSE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 0);

    harness.menu->toggle();
    REQUIRE(harness.menu->is_open());
    harness.menu->toggle();
    REQUIRE_FALSE(harness.menu->is_open());
}

TEST_CASE("dropdown menu renders every MenuItemKind", "[dropdown-menu][render]") {
    MenuHarness harness;
    auto* surface = harness.open_and_layout();
    REQUIRE(surface != nullptr);
    // 每个条目一个子节点（含结构条目）。
    REQUIRE(surface->child_count() == sample_items().size());

    for (std::size_t i = 0; i < surface->child_count(); ++i) {
        auto* row = surface->get_child(i)->as_control();
        REQUIRE(row != nullptr);
        REQUIRE(row->global_bounds().get_height() > 0.0F);
    }

    RecordingDevice device;
    harness.tree.draw(device);
    // 文本（new / group / checkbox / radio / submenu / disabled）+ 分隔线 + submenu 箭头。
    REQUIRE(device.text_calls >= 7);
    REQUIRE(device.lines >= 1);
    REQUIRE(device.rounded_rects >= 1);
}

TEST_CASE(
    "dropdown menu moves the highlight with arrows skipping structure",
    "[dropdown-menu][keyboard]"
) {
    MenuHarness harness;
    auto* surface = harness.open_and_layout();
    REQUIRE(surface != nullptr);
    REQUIRE(harness.tree.focused_node() == surface);
    // 打开时高亮落在首个可聚焦条目。
    REQUIRE(harness.menu->active_index() == 0);

    // Down 跳过 separator(1) 与 label(2)，落到 Word Wrap(3)。
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.menu->active_index() == 3);
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.menu->active_index() == 4);

    // Up 回到 Word Wrap；方向键绝不能改动任何勾选状态。
    harness.tree.dispatch_key(scene::KeyEvent(265, scene::KeyEvent::Action::press));
    REQUIRE(harness.menu->active_index() == 3);
    REQUIRE(harness.menu->items()[3].checked);
    REQUIRE(harness.menu->items()[5].checked);

    // End / Home / PageDown / PageUp 都由共享漫游设施回答。
    harness.tree.dispatch_key(scene::KeyEvent(269, scene::KeyEvent::Action::press)); // end
    REQUIRE(harness.menu->active_index() == 7); // disabled 条目仍可聚焦
    harness.tree.dispatch_key(scene::KeyEvent(268, scene::KeyEvent::Action::press)); // home
    REQUIRE(harness.menu->active_index() == 0);
    harness.tree.dispatch_key(scene::KeyEvent(267, scene::KeyEvent::Action::press)); // page down
    REQUIRE(harness.menu->active_index() == 7);
    harness.tree.dispatch_key(scene::KeyEvent(266, scene::KeyEvent::Action::press)); // page up
    REQUIRE(harness.menu->active_index() == 0);
}

TEST_CASE("dropdown menu typeahead matches labels only", "[dropdown-menu][keyboard]") {
    MenuHarness harness;
    (void)harness.open_and_layout();

    // "p" 命中 Pro Mode（label 起始匹配，ASCII 大小写不敏感）。
    harness.tree.dispatch_text_input(scene::TextInputEvent("p"));
    REQUIRE(harness.menu->active_index() == 5);

    // "w" 命中 Word Wrap。
    harness.tree.dispatch_text_input(scene::TextInputEvent("w"));
    REQUIRE(harness.menu->active_index() == 3);

    // shortcut 不参与匹配："Ctrl+N" 不能让 "c" 命中 New File。
    harness.tree.dispatch_text_input(scene::TextInputEvent("c"));
    REQUIRE(harness.menu->active_index() != 0);
}

TEST_CASE(
    "dropdown menu activates action items with enter and space",
    "[dropdown-menu][keyboard][callback]"
) {
    MenuHarness harness;
    std::vector<std::string> selected;
    harness.menu->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });
    int events = 0;
    auto subscription =
        harness.menu->item_selected().subscribe([&events](const std::string&) { ++events; });
    (void)harness.open_and_layout();

    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(selected == std::vector<std::string> {"new"});
    REQUIRE(events == 1);
    // action 选中即关闭。
    REQUIRE_FALSE(harness.menu->is_open());

    // Space 同样激活。
    (void)harness.open_and_layout();
    REQUIRE(harness.menu->active_index() == 0);
    harness.tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press)); // space
    REQUIRE(selected.size() == 2);
    REQUIRE(selected.back() == "new");
    REQUIRE_FALSE(harness.menu->is_open());
}

TEST_CASE("dropdown menu does not activate a disabled item", "[dropdown-menu][keyboard]") {
    MenuHarness harness;
    std::vector<std::string> selected;
    harness.menu->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });
    (void)harness.open_and_layout();

    harness.tree.dispatch_key(scene::KeyEvent(269, scene::KeyEvent::Action::press)); // end
    REQUIRE(harness.menu->active_index() == 7); // disabled 可聚焦
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE(selected.empty());
    REQUIRE(harness.menu->is_open());
}

TEST_CASE("dropdown menu checkbox toggles and stays open", "[dropdown-menu][selection]") {
    MenuHarness harness;
    harness.menu->set_selection_mode(widget::MenuSelectionMode::multiple);
    std::vector<std::string> selected;
    harness.menu->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });
    (void)harness.open_and_layout();

    // Word Wrap 初始为 checked。
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // down → 3
    REQUIRE(harness.menu->active_index() == 3);
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter
    REQUIRE_FALSE(harness.menu->items()[3].checked);
    REQUIRE(selected == std::vector<std::string> {"wrap"});
    // 勾选菜单不关闭。
    REQUIRE(harness.menu->is_open());

    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter 再切回
    REQUIRE(harness.menu->items()[3].checked);
    REQUIRE(harness.menu->checked_ids() == std::vector<std::string> {"wrap", "pro"});
}

TEST_CASE(
    "dropdown menu radio selection clears siblings in single mode",
    "[dropdown-menu][selection]"
) {
    MenuHarness harness;
    harness.menu->set_selection_mode(widget::MenuSelectionMode::single);
    (void)harness.open_and_layout();
    REQUIRE(harness.menu->items()[5].checked); // Pro Mode 初始选中

    // Down 两次到 ASCII Mode(4)，Enter 选中它。
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press));
    REQUIRE(harness.menu->active_index() == 4);
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));

    REQUIRE(harness.menu->items()[4].checked);
    REQUIRE_FALSE(harness.menu->items()[5].checked);
    // single 的互斥只发生在 radio 之间：同层的 checkbox 选择不受影响（见 menu_model.md）。
    REQUIRE(harness.menu->items()[3].checked);
    REQUIRE(harness.menu->checked_ids() == std::vector<std::string> {"wrap", "ascii"});
    // 单选菜单也保持打开，方便连续比较。
    REQUIRE(harness.menu->is_open());
}

TEST_CASE(
    "dropdown menu selection mode none blocks toggling but still notifies",
    "[dropdown-menu][selection]"
) {
    MenuHarness harness;
    REQUIRE(harness.menu->selection_mode() == widget::MenuSelectionMode::none);
    std::vector<std::string> selected;
    harness.menu->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });
    (void)harness.open_and_layout();

    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // → Word Wrap
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
    // none 模式不改变勾选状态，但条目的用户激活仍然通知。
    REQUIRE(harness.menu->items()[3].checked);
    REQUIRE(selected == std::vector<std::string> {"wrap"});
    REQUIRE(harness.menu->is_open());
}

TEST_CASE("dropdown menu submenu fires on_submenu without closing", "[dropdown-menu][submenu]") {
    MenuHarness harness;
    std::vector<std::string> opened;
    harness.menu->set_on_submenu([&opened](const std::string_view id) { opened.emplace_back(id); });
    (void)harness.open_and_layout();

    // End 前先移到 submenu（More Tools 在索引 6）。
    harness.tree.dispatch_key(scene::KeyEvent(269, scene::KeyEvent::Action::press)); // end → 7
    harness.tree.dispatch_key(scene::KeyEvent(265, scene::KeyEvent::Action::press)); // up → 6
    REQUIRE(harness.menu->active_index() == 6);
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
    REQUIRE(opened == std::vector<std::string> {"more"});
    // 本轮不展开嵌套浮层：菜单保持打开，且没有第二个浮层。
    REQUIRE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 1);
}

TEST_CASE("dropdown menu programmatic checked is silent", "[dropdown-menu][value]") {
    auto menu = widget::DropdownMenu::create(nullptr, sample_items());
    REQUIRE(menu->checked_ids() == std::vector<std::string> {"wrap", "pro"});
    int events = 0;
    auto subscription =
        menu->item_selected().subscribe([&events](const std::string&) { ++events; });

    REQUIRE(menu->set_checked("ascii", true));
    // 静默：不触发 item_selected（也不做 single 互斥清理）。
    REQUIRE(events == 0);
    REQUIRE(menu->checked_ids() == std::vector<std::string> {"wrap", "ascii", "pro"});
    REQUIRE_FALSE(menu->set_checked("nope", true));
    REQUIRE_FALSE(menu->set_checked("new", true)); // action 条目不可勾选
}

TEST_CASE("dropdown menu hover and click drive the same highlight", "[dropdown-menu][pointer]") {
    MenuHarness harness;
    std::vector<std::string> selected;
    harness.menu->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });
    auto* surface = harness.open_and_layout();
    REQUIRE(surface != nullptr);
    REQUIRE(harness.menu->active_index() == 0);

    // 悬停到索引 3（Word Wrap）。
    auto* row = surface->get_child(3)->as_control();
    harness.tree.dispatch_mouse_move(
        scene::MouseMoveEvent(row->global_bounds().get_center(), foundation::NanPoint(1.0F, 0.0F))
    );
    REQUIRE(harness.menu->active_index() == 3);

    // 悬停到结构性条目（separator, 索引 1）不改高亮。
    auto* separator = surface->get_child(1)->as_control();
    harness.tree.dispatch_mouse_move(
        scene::MouseMoveEvent(
            separator->global_bounds().get_center(),
            foundation::NanPoint(1.0F, 0.0F)
        )
    );
    REQUIRE(harness.menu->active_index() == 3);

    // 点击 action 条目激活并关闭。
    auto* action_row = surface->get_child(0)->as_control();
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            action_row->global_bounds().get_center()
        )
    );
    REQUIRE(selected == std::vector<std::string> {"new"});
    REQUIRE_FALSE(harness.menu->is_open());
}

TEST_CASE("dropdown menu escape closes through the dismiss layer", "[dropdown-menu][keyboard]") {
    MenuHarness harness;
    int closes = 0;
    harness.menu->set_on_close([&closes] { ++closes; });
    (void)harness.open_and_layout();
    REQUIRE(harness.menu->is_open());

    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press)); // escape
    REQUIRE_FALSE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
    REQUIRE(closes == 1);
}

TEST_CASE("dropdown menu exposes menu surface and item semantics", "[dropdown-menu][semantics]") {
    MenuHarness harness;
    (void)harness.open_and_layout();
    REQUIRE(harness.tree.update_semantics());

    const auto& tree = harness.tree.semantics_tree();
    const auto* surface = find_semantics_role(tree, semantics::Role::list, "menu");
    REQUIRE(surface != nullptr);

    const auto* action = find_semantics_role(tree, semantics::Role::list_item, "New File");
    REQUIRE(action != nullptr);
    REQUIRE(semantics::supports(action->properties.actions, semantics::Action::activate));
    REQUIRE(action->properties.hint == "Ctrl+N");
    REQUIRE_FALSE(action->properties.state.disabled);

    const auto* separator = find_semantics_role(tree, semantics::Role::separator);
    REQUIRE(separator != nullptr);

    const auto* group = find_semantics_role(tree, semantics::Role::static_text, "View");
    REQUIRE(group != nullptr);

    const auto* checkbox = find_semantics_role(tree, semantics::Role::checkbox, "Word Wrap");
    REQUIRE(checkbox != nullptr);
    REQUIRE(checkbox->properties.state.checked.has_value());
    REQUIRE(*checkbox->properties.state.checked);

    const auto* radio = find_semantics_role(tree, semantics::Role::radio, "Pro Mode");
    REQUIRE(radio != nullptr);
    REQUIRE(radio->properties.state.checked.has_value());
    REQUIRE(*radio->properties.state.checked);

    const auto* disabled = find_semantics_role(tree, semantics::Role::list_item, "Unavailable");
    REQUIRE(disabled != nullptr);
    REQUIRE(disabled->properties.state.disabled);
    REQUIRE_FALSE(semantics::supports(disabled->properties.actions, semantics::Action::activate));

    // 菜单容器的展开状态在 DropdownMenu 自己身上（与 Popover 同款）。
    const auto* container = tree.find(harness.menu->semantics_id());
    REQUIRE(container != nullptr);
    REQUIRE(container->properties.state.checked.has_value());
    REQUIRE(*container->properties.state.checked);
}

TEST_CASE(
    "dropdown menu set_items while open resyncs without dangling state",
    "[dropdown-menu][mutation]"
) {
    MenuHarness harness;
    (void)harness.open_and_layout();
    // 先取副本：绝不跨 set_items() 持有 items() 的引用（widget 内部存储会被替换）。
    const auto before = harness.menu->items();
    REQUIRE(before.size() == 8);

    std::vector<widget::MenuItem> replacement {
        item("copy", "Copy"),
        item("paste", "Paste", widget::MenuItemKind::action, true),
    };
    harness.menu->set_items(std::move(replacement));
    harness.layout();

    REQUIRE(harness.menu->item_count() == 2);
    REQUIRE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 1);
    // 条目视图已重建（不残留旧节点），高亮重新落在首个可聚焦条目。
    auto* surface = harness.surface();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 2);
    REQUIRE(harness.menu->active_index() == 0);
    REQUIRE(harness.menu->items()[0].id == "copy");

    // 重建后键盘仍然可用，且不会触发旧的条目。
    std::vector<std::string> selected;
    harness.menu->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });
    harness.tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // down → 1
    REQUIRE(harness.menu->active_index() == 1);
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // disabled
    REQUIRE(selected.empty());
    REQUIRE(harness.menu->is_open());
}

TEST_CASE(
    "dropdown menu survives set_items from inside an input phase",
    "[dropdown-menu][mutation]"
) {
    MenuHarness harness;
    harness.menu->set_selection_mode(widget::MenuSelectionMode::multiple);
    auto* menu = harness.menu.get();
    bool replaced = false;
    menu->set_on_select([&](const std::string_view id) {
        if (id == "wrap" && !replaced) {
            replaced = true;
            menu->set_items({item("x", "Xenon"), item("y", "Yttrium")});
        }
    });
    (void)harness.open_and_layout();

    // 在输入阶段里替换条目：浮层的同步移除会被延迟，重建必须排到安全提交点。
    {
        auto phase = harness.tree.enter_phase(scene::FramePhase::input);
        harness.tree.dispatch_key(
            scene::KeyEvent(264, scene::KeyEvent::Action::press)
        ); // down → Word Wrap
        harness.tree.dispatch_key(
            scene::KeyEvent(257, scene::KeyEvent::Action::press)
        ); // 回调里 set_items
    }
    REQUIRE(replaced);
    harness.tree.flush_tree_mutations();
    harness.layout();

    REQUIRE(menu->item_count() == 2);
    REQUIRE(menu->is_open());
    REQUIRE(harness.host->overlay_count() == 1);
    auto* surface = harness.surface();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 2);
    REQUIRE(menu->active_index() == 0);
}

TEST_CASE("destroying an open dropdown menu releases its overlay", "[dropdown-menu][lifetime]") {
    MenuHarness harness;
    (void)harness.open_and_layout();
    REQUIRE(harness.host->overlay_count() == 1);

    // 把菜单从场景树里摘下来再销毁：浮层必须跟着释放，不留下指向已消失锚点的孤儿。
    auto menu = harness.menu;
    (void)harness.body->remove_child(*menu);
    menu.reset();
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("dropdown menu works without an overlay host", "[dropdown-menu][fallback]") {
    auto trigger = widget::Button::create("Open");
    auto menu = widget::DropdownMenu::create(trigger, sample_items());
    std::vector<std::string> selected;
    menu->set_on_select([&selected](const std::string_view id) { selected.emplace_back(id); });

    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(300.0F, 240.0F));
    root->add_child(menu);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 240.0F)) >= 1);

    REQUIRE(menu->z_index_hint() == 0);
    menu->open();
    root->mark_layout_dirty();
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 240.0F)) >= 1);
    REQUIRE(menu->is_open());
    REQUIRE(menu->z_index_hint() == 1);
    // 树内回退时内容确实在菜单子树里，并且焦点落到了菜单表面。
    REQUIRE(menu->is_visible_in_tree());
    REQUIRE(tree.focused_node() != nullptr);
    REQUIRE(menu->active_index() == 0);

    // 高亮与激活在回退模式下同样工作。
    tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // down → 3
    REQUIRE(menu->active_index() == 3);
    tree.dispatch_key(scene::KeyEvent(265, scene::KeyEvent::Action::press)); // up → 0
    REQUIRE(menu->active_index() == 0);
    tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter → action 关闭
    REQUIRE_FALSE(menu->is_open());
    REQUIRE(selected == std::vector<std::string> {"new"});
}

TEST_CASE("dropdown menu authoring via ComponentTraits", "[dropdown-menu][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 160.0F)));
    scene::NanSceneTree host_tree;
    host_tree.set_root(host);
    REQUIRE(host_tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};

    // 默认构造路径（测试门槛里的 ui.make<DropdownMenu>()）。
    auto empty = ui.make<widget::DropdownMenu>().build();
    REQUIRE(empty->trigger() == nullptr);
    REQUIRE_FALSE(empty->is_open());
    REQUIRE(empty->item_count() == 0);

    auto trigger = ui.make<widget::Button>("Open").build();
    auto menu = ui.make<widget::DropdownMenu>(trigger, sample_items()).build();
    REQUIRE(menu->trigger() == trigger);
    REQUIRE(menu->item_count() == 8);

    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 160.0F));
    root->add_child(menu);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    // 服务来自 BuildContext 注入：挂在普通控件根下也能 portal。
    menu->open();
    REQUIRE(host->overlay_count() == 1);
    menu->close();
    REQUIRE(host->overlay_count() == 0);
}

TEST_CASE("dropdown menu recipe resolves semantic roles", "[dropdown-menu][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_dropdown_menu(design, theme::ColorAppearance::light);
    const auto dark = theme::resolve_dropdown_menu(design, theme::ColorAppearance::dark);

    REQUIRE(
        light.item_label.color.oklch().light == Catch::Approx(design.light.foreground.oklch().light)
    );
    REQUIRE(
        light.item_shortcut.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(
        light.group_label.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(light.separator.alpha() > 0.0F);
    REQUIRE(light.hover_fill.alpha() > 0.0F);
    REQUIRE(light.checked_indicator.alpha() > 0.0F);
    // 默认值引用语义标量：改 token 会跟着变。
    REQUIRE(light.metrics.item_height == Catch::Approx(32.0F));
    REQUIRE(light.metrics.padding_x == Catch::Approx(design.tokens.spacing.xs));
    REQUIRE(light.metrics.min_width == Catch::Approx(160.0F));
    // 亮暗文本 / 状态面不同：默认值只用语义角色，不写字面量。
    REQUIRE_FALSE(light.item_label.color == dark.item_label.color);
    REQUIRE_FALSE(light.hover_fill == dark.hover_fill);
}

TEST_CASE(
    "dropdown menu theme switch and instance override change resolved style",
    "[dropdown-menu][theme][override]"
) {
    MenuHarness harness;
    harness.layout();
    auto* menu = harness.menu.get();
    const auto& design = harness.themes.design_system();
    const auto initial = menu->resolved_style();

    menu->set_override(
        theme::DropdownMenuRecipeRule {
            .item_label_color = theme::ThemeColor::token(theme::ColorToken::error),
            .metrics_item_height = theme::ThemeScalar::literal(21.0F),
        }
    );
    const auto overridden = menu->resolved_style();
    REQUIRE(
        overridden.item_label.color.oklch().light == Catch::Approx(design.light.error.oklch().light)
    );
    REQUIRE(overridden.metrics.item_height == Catch::Approx(21.0F));
    // 未覆盖的字段保持默认。
    REQUIRE(overridden.metrics.padding_x == Catch::Approx(initial.metrics.padding_x));
    REQUIRE(overridden.item_shortcut.color.approx_equals(initial.item_shortcut.color));

    // 主题切换：DesignSystem 的规则层同样作用到解析结果（守住 apply_rule 四步同步）。
    auto next = theme::default_design_system();
    next.components.dropdown_menu.rules.push_back(
        theme::DropdownMenuRecipeRule {
            .focus_fill = theme::ThemeColor::token(theme::ColorToken::warning),
            .metrics_padding_y = theme::ThemeScalar::literal(19.0F),
        }
    );
    harness.themes.apply(std::make_shared<const theme::DesignSystem>(std::move(next)));
    const auto themed = menu->resolved_style();
    REQUIRE(
        themed.focus_fill.oklch().light
        == Catch::Approx(harness.themes.design_system().light.warning.oklch().light)
    );
    REQUIRE(themed.metrics.padding_y == Catch::Approx(19.0F));
    // 实例覆盖在系统切换后保留并压过规则层。
    REQUIRE(themed.metrics.item_height == Catch::Approx(21.0F));
    REQUIRE(
        themed.item_label.color.oklch().light
        == Catch::Approx(harness.themes.design_system().light.error.oklch().light)
    );

    // 显式 NanTheme 覆盖（不再跟随系统切换）。先清掉实例覆盖，否则它会继续压过主题。
    menu->set_override(theme::DropdownMenuRecipeRule {});
    auto explicit_theme = theme::default_theme();
    explicit_theme.tokens.spacing.xs = 7.0F;
    menu->set_theme(explicit_theme);
    REQUIRE(menu->theme_ref().tokens.spacing.xs == Catch::Approx(7.0F));
    REQUIRE(menu->resolved_style().metrics.padding_x == Catch::Approx(7.0F));
}

TEST_CASE("dropdown menu recipe rule covers every field", "[dropdown-menu][theme][override]") {
    MenuHarness harness;
    harness.layout();
    auto* menu = harness.menu.get();
    const auto& ds = harness.themes.design_system();

    menu->set_override(
        theme::DropdownMenuRecipeRule {
            .item_label_color = theme::ThemeColor::token(theme::ColorToken::error),
            .item_label_font_size = theme::ThemeScalar::literal(11.0F),
            .item_shortcut_color = theme::ThemeColor::token(theme::ColorToken::warning),
            .item_shortcut_font_size = theme::ThemeScalar::literal(12.0F),
            .group_label_color = theme::ThemeColor::token(theme::ColorToken::success),
            .group_label_font_size = theme::ThemeScalar::literal(13.0F),
            .disabled_label = theme::ThemeColor::token(theme::ColorToken::info),
            .hover_fill = theme::ThemeColor::token(theme::ColorToken::primary),
            .focus_fill = theme::ThemeColor::token(theme::ColorToken::tertiary),
            .checked_indicator = theme::ThemeColor::token(theme::ColorToken::destructive),
            .separator = theme::ThemeColor::token(theme::ColorToken::ring),
            .metrics_item_height = theme::ThemeScalar::literal(41.0F),
            .metrics_padding_x = theme::ThemeScalar::literal(12.0F),
            .metrics_padding_y = theme::ThemeScalar::literal(13.0F),
            .metrics_gap = theme::ThemeScalar::literal(14.0F),
            .metrics_item_radius = theme::ThemeScalar::literal(15.0F),
            .metrics_separator_thickness = theme::ThemeScalar::literal(1.5F),
            .metrics_min_width = theme::ThemeScalar::literal(222.0F),
        }
    );

    const auto style = menu->resolved_style();
    REQUIRE(style.item_label.color.oklch().light == Catch::Approx(ds.light.error.oklch().light));
    REQUIRE(style.item_label.font_size == Catch::Approx(11.0F));
    REQUIRE(
        style.item_shortcut.color.oklch().light == Catch::Approx(ds.light.warning.oklch().light)
    );
    REQUIRE(style.item_shortcut.font_size == Catch::Approx(12.0F));
    REQUIRE(style.group_label.color.oklch().light == Catch::Approx(ds.light.success.oklch().light));
    REQUIRE(style.group_label.font_size == Catch::Approx(13.0F));
    REQUIRE(style.disabled_label.oklch().light == Catch::Approx(ds.light.info.oklch().light));
    REQUIRE(style.hover_fill.oklch().light == Catch::Approx(ds.light.primary.oklch().light));
    REQUIRE(style.focus_fill.oklch().light == Catch::Approx(ds.light.tertiary.oklch().light));
    REQUIRE(
        style.checked_indicator.oklch().light == Catch::Approx(ds.light.destructive.oklch().light)
    );
    REQUIRE(style.separator.oklch().light == Catch::Approx(ds.light.ring.oklch().light));
    REQUIRE(style.metrics.item_height == Catch::Approx(41.0F));
    REQUIRE(style.metrics.padding_x == Catch::Approx(12.0F));
    REQUIRE(style.metrics.padding_y == Catch::Approx(13.0F));
    REQUIRE(style.metrics.gap == Catch::Approx(14.0F));
    REQUIRE(style.metrics.item_radius == Catch::Approx(15.0F));
    REQUIRE(style.metrics.separator_thickness == Catch::Approx(1.5F));
    REQUIRE(style.metrics.min_width == Catch::Approx(222.0F));
}

TEST_CASE(
    "dropdown menu delegates placement and alignment to its popover",
    "[dropdown-menu][overlay]"
) {
    // 两条短条目：面板够矮，视口也留了上下翻转的空间。
    MenuHarness harness(
        {item("a", "Alpha"), item("b", "Beta")},
        foundation::NanPoint(120.0F, 200.0F)
    );
    harness.viewport = foundation::NanSize(360.0F, 420.0F);
    harness.menu->set_gap(16.0F);
    harness.layout();
    const auto anchor = harness.menu->global_bounds();

    harness.menu->open();
    harness.layout();
    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    REQUIRE(
        panel->global_bounds().get_top() == Catch::Approx(anchor.get_bottom() + 16.0F).margin(1.0F)
    );

    harness.menu->set_placement(widget::internal::OverlayPlacement::top);
    harness.layout();
    panel = harness.panel();
    REQUIRE(
        panel->global_bounds().get_bottom() == Catch::Approx(anchor.get_top() - 16.0F).margin(1.0F)
    );

    harness.menu->set_alignment(widget::internal::OverlayAlignment::center);
    harness.layout();
    panel = harness.panel();
    REQUIRE(
        panel->global_bounds().get_center().get_x()
        == Catch::Approx(anchor.get_center().get_x()).margin(1.0F)
    );
    REQUIRE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 1);
}
