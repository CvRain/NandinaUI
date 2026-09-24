//
// ContextMenu tests: right-click invocation policy over the wrapped DropdownMenu.
//
// ContextMenu 本身不实现菜单：条目模型、勾选、子菜单、键盘与浮层全部来自它包裹的
// DropdownMenu。因此这里只验证"包装层"的职责——目标槽位、指针/键盘调用策略、
// 尺寸镜像与转发，菜单行为本身的契约由 dropdown_menu_tests.cpp 覆盖。
//

#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/builtin_component_traits.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/context_menu.hpp>
#include <nandina/widget/dropdown_menu.hpp>
#include <nandina/widget/key_codes.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

namespace
{
    [[nodiscard]] auto item(
        std::string id,
        std::string label,
        const widget::MenuItemKind kind = widget::MenuItemKind::action,
        const bool disabled = false
    ) -> widget::MenuItem {
        widget::MenuItem result;
        result.id = std::move(id);
        result.label = std::move(label);
        result.kind = kind;
        result.disabled = disabled;
        return result;
    }

    [[nodiscard]] auto sample_items() -> std::vector<widget::MenuItem> {
        return {
            item("rename", "重命名"),
            item("duplicate", "创建副本"),
            item("separator", "", widget::MenuItemKind::separator),
            item("pin", "固定", widget::MenuItemKind::checkbox),
            item("locked", "不可用", widget::MenuItemKind::action, /*disabled=*/true),
        };
    }

    /// 目标控件挂在 ContextMenu 下，ContextMenu 挂在内容层，内容层挂在 OverlayHost 下。
    /// 这样内部 DropdownMenu 的 Popover 能沿祖先链找到浮层宿主（不依赖注入）。
    struct ContextHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(400.0F, 300.0F));
        std::shared_ptr<widget::Button> target = widget::Button::create("Target");
        std::shared_ptr<widget::ContextMenu> menu;
        scene::NanSceneTree tree;
        foundation::NanSize viewport {400.0F, 300.0F};

        explicit ContextHarness(
            std::vector<widget::MenuItem> items = sample_items(),
            const bool with_target = true
        ):
            menu(widget::ContextMenu::create(
                with_target ? std::static_pointer_cast<scene::NanControl>(target) : nullptr,
                std::move(items)
            )) {
            tree.set_theme_manager(themes);
            menu->set_position(foundation::NanPoint(24.0F, 60.0F));
            body->add_child(menu);
            // 第二个子节点阻止 NanControl 的单子拉伸路径把菜单撑满内容层。
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            host->set_content(body);
            tree.set_root(host);
        }

        void layout() {
            (void)tree.layout_root(viewport);
        }

        /// 目标控件中心点：用布局结果算，不猜像素。
        [[nodiscard]] auto target_center() const -> foundation::NanPoint {
            const auto bounds = target->global_bounds();
            return foundation::NanPoint(
                bounds.get_left() + bounds.get_width() * 0.5F,
                bounds.get_top() + bounds.get_height() * 0.5F
            );
        }

        void right_click(const foundation::NanPoint point) {
            tree.dispatch_mouse_button(
                scene::MouseButtonEvent(
                    scene::MouseButtonEvent::Button::right,
                    scene::MouseButtonEvent::Action::press,
                    point
                )
            );
        }

        /// 浮层面板（DismissLayer → FocusScope → PopoverSurface）的屏幕矩形。
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

        void open_and_layout() {
            layout();
            right_click(target_center());
            layout();
        }
    };
} // namespace

TEST_CASE("context menu mirrors its target and rejects a null target", "[context-menu]") {
    ContextHarness harness;
    harness.layout();

    REQUIRE(harness.menu->target() == harness.target);
    // 包装层尺寸镜像目标：目标被布局到包装层自己的矩形里。
    REQUIRE(harness.menu->global_bounds().get_width() > 0.0F);
    REQUIRE(harness.target->global_bounds().get_width() > 0.0F);
    REQUIRE(
        harness.target->global_bounds().get_width()
        == Catch::Approx(harness.menu->global_bounds().get_width()).margin(0.5F)
    );

    // 目标槽位不接受空指针。
    CHECK_THROWS_AS(harness.menu->set_target(nullptr), std::invalid_argument);
}

TEST_CASE("context menu opens on right click at the pointer", "[context-menu][input]") {
    ContextHarness harness;
    harness.layout();
    REQUIRE_FALSE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 0);

    const auto click = harness.target_center();
    harness.right_click(click);
    REQUIRE(harness.menu->is_open());
    REQUIRE(harness.host->overlay_count() == 1);

    harness.layout();
    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    const auto bounds = panel->global_bounds();
    REQUIRE(bounds.is_valid());

    // 锚点是 1x1 的光标矩形：面板起点应贴住点击处（placement=bottom / alignment=start，
    // gap=2），并且被收进视口内。
    REQUIRE(bounds.get_left() == Catch::Approx(click.get_x()).margin(1.0F));
    REQUIRE(bounds.get_top() >= click.get_y());
    REQUIRE(bounds.get_left() >= 0.0F);
    REQUIRE(bounds.get_top() >= 0.0F);
    REQUIRE(bounds.get_right() <= harness.viewport.get_width() + 0.5F);
    REQUIRE(bounds.get_bottom() <= harness.viewport.get_height() + 0.5F);
}

TEST_CASE("context menu open_at is a safe no-op without a target", "[context-menu][input]") {
    ContextHarness harness(sample_items(), /*with_target=*/false);
    harness.layout();

    REQUIRE(harness.menu->target() == nullptr);
    harness.menu->open_at(foundation::NanPoint(50.0F, 50.0F));
    CHECK_FALSE(harness.menu->is_open());
    CHECK(harness.host->overlay_count() == 0);
}

TEST_CASE("menu key opens a context menu at the target corner", "[context-menu][input][keyboard]") {
    ContextHarness harness;
    harness.layout();
    // 键事件沿焦点路径派发，所以键盘手势要求目标先拿到焦点。
    REQUIRE(harness.tree.focus_first_within(*harness.target));
    const auto corner = harness.target->global_bounds().get_bottom_left();

    harness.tree.dispatch_key(scene::KeyEvent(widget::keys::menu, scene::KeyEvent::Action::press));
    REQUIRE(harness.menu->is_open());

    harness.layout();
    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    REQUIRE(
        panel->global_bounds().get_left()
        == Catch::Approx(corner.get_x()).margin(1.0F)
    );
    REQUIRE(panel->global_bounds().get_top() >= corner.get_y());
}

TEST_CASE("shift+F10 opens a context menu but bare F10 does not", "[context-menu][input][keyboard]") {
    ContextHarness harness;
    harness.layout();
    REQUIRE(harness.tree.focus_first_within(*harness.target));

    // 裸 F10 不是上下文菜单手势。
    harness.tree.dispatch_key(scene::KeyEvent(widget::keys::f10, scene::KeyEvent::Action::press));
    CHECK_FALSE(harness.menu->is_open());

    scene::KeyModifiers shift;
    shift.shift = true;
    harness.tree.dispatch_key(
        scene::KeyEvent(widget::keys::f10, scene::KeyEvent::Action::press, shift)
    );
    CHECK(harness.menu->is_open());
}

TEST_CASE("right click outside the target does not open", "[context-menu][input]") {
    ContextHarness harness;
    harness.layout();

    // 点在内容层空白处（目标之外）。
    harness.right_click(foundation::NanPoint(350.0F, 250.0F));
    CHECK_FALSE(harness.menu->is_open());
    CHECK(harness.host->overlay_count() == 0);
}

TEST_CASE("context menu closes programmatically", "[context-menu]") {
    ContextHarness harness;
    harness.open_and_layout();
    REQUIRE(harness.menu->is_open());

    harness.menu->close();
    CHECK_FALSE(harness.menu->is_open());
    CHECK(harness.host->overlay_count() == 0);
}

TEST_CASE("items and selection state pass through to the wrapped menu", "[context-menu][model]") {
    ContextHarness harness;
    harness.layout();
    REQUIRE(harness.menu->item_count() == 5);

    harness.menu->set_items({item("only", "Only", widget::MenuItemKind::checkbox)});
    REQUIRE(harness.menu->item_count() == 1);
    REQUIRE(harness.menu->items().front().id == "only");

    harness.menu->set_selection_mode(widget::MenuSelectionMode::multiple);
    CHECK(harness.menu->selection_mode() == widget::MenuSelectionMode::multiple);

    CHECK(harness.menu->set_checked("only", true));
    CHECK(harness.menu->checked_ids() == std::vector<std::string> {"only"});

    // 非勾选种类不会被写入。
    harness.menu->set_items(sample_items());
    CHECK_FALSE(harness.menu->set_checked("rename", true));
    CHECK(harness.menu->checked_ids().empty());
}

TEST_CASE("activating an item fires on_select and closes the tree", "[context-menu][select]") {
    ContextHarness harness;
    harness.open_and_layout();
    REQUIRE(harness.menu->is_open());

    std::vector<std::string> picked;
    harness.menu->set_on_select([&picked](const std::string_view id) {
        picked.emplace_back(id);
    });

    int close_count = 0;
    harness.menu->set_on_close([&close_count] { ++close_count; });

    // 打开时高亮落在第一个可聚焦条目（rename）。
    REQUIRE(harness.menu->active_index() == 0);
    harness.tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));

    REQUIRE(picked == std::vector<std::string> {"rename"});
    CHECK_FALSE(harness.menu->is_open());
    CHECK(harness.host->overlay_count() == 0);
    CHECK(close_count == 1);
}

TEST_CASE("context menu reports expanded and collapsed semantics", "[context-menu][semantics]") {
    ContextHarness harness;
    harness.layout();

    const auto collapsed = harness.menu->resolved_semantics_properties();
    CHECK(collapsed.value == "collapsed");
    // checked 是 optional<bool>：先确认有值，再比较值本身（直接判 optional 的真值
    // 检查的是 has_value，不是内容）。
    REQUIRE(collapsed.state.checked.has_value());
    CHECK_FALSE(*collapsed.state.checked);

    harness.open_and_layout();
    const auto expanded = harness.menu->resolved_semantics_properties();
    CHECK(expanded.value == "expanded");
    REQUIRE(expanded.state.checked.has_value());
    CHECK(*expanded.state.checked);
}

TEST_CASE("z index hint delegates to the wrapped menu", "[context-menu]") {
    ContextHarness harness;
    harness.layout();

    const auto closed = harness.menu->z_index_hint();
    harness.open_and_layout();
    // 浮层承载时层级由 OverlayLevel 决定；这里只要求打开后不再低于关闭时。
    CHECK(harness.menu->z_index_hint() >= closed);
}

TEST_CASE("theme override passes through to the wrapped menu", "[context-menu][theme]") {
    ContextHarness harness;
    harness.layout();

    const auto initial = harness.menu->resolved_style();
    harness.menu->set_override(
        theme::DropdownMenuRecipeRule {
            .metrics_item_height = theme::ThemeScalar::literal(44.0F),
            .metrics_min_width = theme::ThemeScalar::literal(220.0F),
        }
    );
    const auto overridden = harness.menu->resolved_style();
    CHECK(overridden.metrics.item_height == Catch::Approx(44.0F));
    CHECK(overridden.metrics.min_width == Catch::Approx(220.0F));
    // 未覆盖字段保持默认。
    CHECK(overridden.metrics.padding_x == Catch::Approx(initial.metrics.padding_x));
    CHECK(harness.menu->theme_ref().tokens.spacing.md > 0.0F);
}

TEST_CASE("context menu authoring via ComponentTraits", "[context-menu][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<scene::NanControl>(foundation::NanSize(320.0F, 240.0F)));
    scene::NanSceneTree host_tree;
    host_tree.set_root(host);
    REQUIRE(host_tree.layout_root(foundation::NanSize(320.0F, 240.0F)) >= 1);

    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};

    // 默认构造路径：无目标、无条目。
    auto empty = ui.make<widget::ContextMenu>().build();
    CHECK(empty->target() == nullptr);
    CHECK(empty->item_count() == 0);

    auto target = ui.make<widget::Button>("Right click me").build();
    auto menu = ui.make<widget::ContextMenu>(target, sample_items()).build();
    REQUIRE(menu->target() == target);
    REQUIRE(menu->item_count() == 5);

    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(320.0F, 240.0F));
    root->add_child(menu);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 240.0F)) >= 1);

    // 服务来自 BuildContext 注入：挂在普通控件根下也能 portal。
    menu->open_at(foundation::NanPoint(40.0F, 40.0F));
    REQUIRE(menu->is_open());
    CHECK(host->overlay_count() == 1);
    menu->close();
    CHECK(host->overlay_count() == 0);
}
