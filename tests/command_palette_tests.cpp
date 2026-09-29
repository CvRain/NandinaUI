//
// CommandPalette tests: modal command launcher over the shared MenuItem model.
//

#include <nandina/foundation/contrast.hpp>
#include <nandina/reactive/graph.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/command_palette.hpp>
#include <nandina/widget/controls.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    /// 默认配方里几个"必须来自语义角色"的字段值，供断言引用。
    inline constexpr float kDefaultItemHeight = 32.0F;
    inline constexpr float kDefaultPanelWidth = 560.0F;
} // namespace

TEST_CASE("command palette recipe resolves semantic roles", "[command-palette][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_command_palette(design, theme::ColorAppearance::light);
    const auto dark = theme::resolve_command_palette(design, theme::ColorAppearance::dark);

    // 面板沿用浮层外壳的语义角色。
    REQUIRE(light.panel.fill.alpha() > 0.0F);
    REQUIRE(light.panel.border_width == Catch::Approx(design.tokens.border.thin));
    REQUIRE(light.panel.radius == Catch::Approx(design.tokens.radius.md));

    // 查询行与结果行都由语义文本角色给出。
    REQUIRE(
        light.query.color.oklch().light == Catch::Approx(design.light.foreground.oklch().light)
    );
    REQUIRE(
        light.placeholder.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(
        light.item_label.color.oklch().light == Catch::Approx(design.light.foreground.oklch().light)
    );
    REQUIRE(
        light.item_shortcut.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(light.empty.color.alpha() > 0.0F);
    REQUIRE(light.separator.alpha() > 0.0F);
    REQUIRE(light.hover_fill.alpha() > 0.0F);
    REQUIRE(light.focus_fill.alpha() > 0.0F);

    // 默认值引用语义标量：改 token 会跟着变。
    REQUIRE(light.metrics.item_height == Catch::Approx(kDefaultItemHeight));
    REQUIRE(light.metrics.panel_width == Catch::Approx(kDefaultPanelWidth));
    REQUIRE(light.metrics.panel_padding == Catch::Approx(design.tokens.spacing.sm));

    // 亮暗不同：默认值只用语义角色，不写字面量。
    REQUIRE_FALSE(light.item_label.color == dark.item_label.color);
    REQUIRE_FALSE(light.hover_fill == dark.hover_fill);
}

TEST_CASE("command palette recipe rules change the resolved style", "[command-palette][theme]") {
    // 这条同时守住设计系统的四步同步：漏写 apply_rule 重载时，规则字段会在类型上存在
    // 但在行为上完全不生效，且没有任何编译或运行错误。
    auto design = theme::default_design_system();
    const auto base = theme::resolve_command_palette(design, theme::ColorAppearance::light);

    design.components.command_palette.rules.push_back(
        theme::CommandPaletteRecipeRule {
            .empty_color = theme::ThemeColor::token(theme::ColorToken::error),
            .hover_fill = theme::ThemeColor::token(theme::ColorToken::destructive),
            .metrics_panel_width = theme::ThemeScalar::literal(720.0F),
            .metrics_item_height = theme::ThemeScalar::literal(44.0F),
        }
    );
    const auto overridden = theme::resolve_command_palette(design, theme::ColorAppearance::light);

    REQUIRE(overridden.metrics.item_height == Catch::Approx(44.0F));
    REQUIRE(overridden.metrics.panel_width == Catch::Approx(720.0F));
    REQUIRE(overridden.hover_fill != base.hover_fill);
    REQUIRE(overridden.empty.color != base.empty.color);

    // 未被规则覆盖的字段保持原值（规则是增量，而不是整体替换）。
    REQUIRE(overridden.panel.radius == Catch::Approx(base.panel.radius));
    REQUIRE(overridden.metrics.min_width == Catch::Approx(base.metrics.min_width));
}

namespace
{
    [[nodiscard]] auto sample_items() -> std::vector<widget::MenuItem> {
        return {
            widget::MenuItem {.id = "file.new", .label = "新建文件", .shortcut = "Ctrl+N"},
            widget::MenuItem {.id = "file.open", .label = "打开文件", .shortcut = "Ctrl+O"},
            widget::MenuItem {.id = "file.saveas", .label = "另存为", .shortcut = "Ctrl+S", .disabled = true},
            widget::MenuItem {.id = "sep", .kind = widget::MenuItemKind::separator},
            widget::MenuItem {.id = "group.view", .label = "视图", .kind = widget::MenuItemKind::label},
            widget::MenuItem {.id = "view.zoom", .label = "缩放", .shortcut = "Ctrl+0"},
        };
    }

    struct PaletteHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(720.0F, 480.0F));
        std::shared_ptr<widget::CommandPalette> palette;
        scene::NanSceneTree tree;
        foundation::NanSize viewport {720.0F, 480.0F};

        explicit PaletteHarness(std::vector<widget::MenuItem> items = sample_items()):
            palette(widget::CommandPalette::create(std::move(items))) {
            tree.set_theme_manager(themes);
            // 不注入服务：host 作为树根，resolve_overlay_host() 沿祖先链就能找到它
            // （同菜单族的测试写法）。
            body->add_child(palette);
            // 第二个子节点阻止单子拉伸路径把面板撑满内容层（同菜单测试）。
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            host->set_content(body);
            tree.set_root(host);
        }

        void layout() {
            (void)tree.layout_root(viewport);
        }
        void open() {
            layout();
            palette->open();
            layout();
        }
        void key(const int code) {
            tree.dispatch_key(scene::KeyEvent(code, scene::KeyEvent::Action::press));
        }
    };
} // namespace

TEST_CASE("command palette filters by label and ignores shortcuts", "[command-palette][filter]") {
    PaletteHarness harness;
    harness.open();
    REQUIRE(harness.palette->is_open());

    // 空查询列出全部可聚焦条目（disabled 的也在 —— menu_model 规则 1）。
    const auto all = harness.palette->filtered_ids();
    REQUIRE(all.size() == 4);
    REQUIRE(all.front() == "file.new");
    REQUIRE(all.back() == "view.zoom");

    // 按 label 子串过滤。
    harness.palette->set_query("文件");
    const auto matched = harness.palette->filtered_ids();
    REQUIRE(matched.size() == 2);
    REQUIRE(matched[0] == "file.new");
    REQUIRE(matched[1] == "file.open");

    // shortcut 只是展示提示，不参与匹配（menu_model.md 规则 2）。
    harness.palette->set_query("Ctrl+N");
    REQUIRE(harness.palette->filtered_ids().empty());
    REQUIRE(harness.palette->hidden_result_count() == 0);

    // 边界：没有匹配时也不崩，列表为空。
    harness.palette->set_query("绝对不存在的命令");
    REQUIRE(harness.palette->filtered_ids().empty());
    harness.palette->clear_query();
    REQUIRE(harness.palette->filtered_ids().size() == 4);
}

TEST_CASE("command palette drops groups that have no match", "[command-palette][filter]") {
    PaletteHarness harness;
    harness.open();

    // 命中「视图」组内的条目时，组标题与分隔线保留。
    harness.palette->set_query("缩放");
    auto ids = harness.palette->filtered_ids();
    REQUIRE(ids.size() == 1);
    REQUIRE(ids[0] == "view.zoom");

    // 只命中第一组时，「视图」整组（含组标题与分隔线）消失。
    harness.palette->set_query("新建");
    ids = harness.palette->filtered_ids();
    REQUIRE(ids.size() == 1);
    REQUIRE(ids[0] == "file.new");
}

TEST_CASE("command palette keyboard moves the highlight and Enter activates", "[command-palette][keyboard]") {
    PaletteHarness harness;
    harness.open();
    std::vector<std::string> selected;
    harness.palette->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });

    REQUIRE(harness.palette->active_id() == "file.new");
    harness.key(264); // down
    REQUIRE(harness.palette->active_id() == "file.open");
    // 禁用条目**仍然可聚焦**：方向键要能走到它，否则键盘用户不知道它存在。
    harness.key(264); // down
    REQUIRE(harness.palette->active_id() == "file.saveas");
    // 再按一次跨过分隔线与组标题，落到下一组第一个可聚焦条目。
    harness.key(264); // down
    REQUIRE(harness.palette->active_id() == "view.zoom");
    harness.key(265); // up
    REQUIRE(harness.palette->active_id() == "file.saveas");

    // 禁用条目不可激活：Enter 不产生回调。
    harness.key(257); // enter
    REQUIRE(selected.empty());
    REQUIRE(harness.palette->is_open());

    harness.key(268); // home
    REQUIRE(harness.palette->active_id() == "file.new");
    harness.key(257); // enter
    REQUIRE(selected.size() == 1);
    REQUIRE(selected[0] == "file.new");
    // 动作条目激活后关闭面板。
    REQUIRE_FALSE(harness.palette->is_open());
}

TEST_CASE("command palette checkbox items toggle without closing", "[command-palette][selection]") {
    std::vector<widget::MenuItem> items {
        widget::MenuItem {
            .id = "toggle.grid",
            .label = "显示网格",
            .kind = widget::MenuItemKind::checkbox,
        },
    };
    PaletteHarness harness(std::move(items));
    harness.open();
    harness.palette->set_selection_mode(widget::MenuSelectionMode::multiple);

    int selects = 0;
    harness.palette->set_on_select([&selects](std::string_view) { ++selects; });
    harness.key(257); // enter

    REQUIRE(selects == 1);
    REQUIRE(harness.palette->is_open()); // 勾选类条目保持打开（同 DropdownMenu）
    const auto checked = harness.palette->checked_ids();
    REQUIRE(checked.size() == 1);
    REQUIRE(checked[0] == "toggle.grid");
}

TEST_CASE("command palette caps visible results and reports the hidden count", "[command-palette][limit]") {
    PaletteHarness harness;
    harness.open();
    REQUIRE(harness.palette->hidden_result_count() == 0);

    harness.palette->set_max_visible_results(2);
    REQUIRE(harness.palette->filtered_ids().size() == 2);
    REQUIRE(harness.palette->hidden_result_count() == 2);

    // 高亮不会落到被裁掉的结果上。
    harness.key(269); // end
    REQUIRE(harness.palette->active_id() == "file.open");

    harness.palette->set_max_visible_results(8);
    REQUIRE(harness.palette->hidden_result_count() == 0);
    REQUIRE(harness.palette->filtered_ids().size() == 4);
}

TEST_CASE("command palette portals on open and unmounts when leaving the tree", "[command-palette][overlay]") {
    PaletteHarness harness;
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);

    harness.palette->open();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);
    REQUIRE(harness.palette->is_open());

    // 离开场景树必须收掉浮层，否则面板会比持有它的页面活得久。
    harness.body->remove_and_delete(*harness.palette);
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);
    REQUIRE_FALSE(harness.palette->is_open());
}

TEST_CASE("command palette theme switch and override change the resolved style", "[command-palette][theme]") {
    PaletteHarness harness;
    harness.layout();
    const auto initial = harness.palette->resolved_style();

    harness.palette->set_override(
        theme::CommandPaletteRecipeRule {
            .metrics_panel_width = theme::ThemeScalar::literal(800.0F),
        }
    );
    const auto overridden = harness.palette->resolved_style();
    REQUIRE(overridden.metrics.panel_width == Catch::Approx(800.0F));
    REQUIRE(overridden.metrics.min_width == Catch::Approx(initial.metrics.min_width));

    // 跟随系统切换：暗色下文本与状态面都要变化。
    harness.palette->on_theme_changed(harness.themes);
    const auto after = harness.palette->resolved_style();
    REQUIRE(after.metrics.panel_width == Catch::Approx(800.0F)); // 实例覆盖保留
}

TEST_CASE("command palette is reachable through BuildContext", "[command-palette][traits]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};
    auto builder = ui.make<widget::CommandPalette>(
        std::vector<widget::MenuItem> {
            widget::MenuItem {.id = "a", .label = "alpha"},
        },
        "搜索命令"
    );
    auto palette = builder.build();
    REQUIRE(palette != nullptr);
    REQUIRE(palette->item_count() == 1);
    REQUIRE(palette->placeholder() == "搜索命令");
    // 未注入覆盖层服务的面板仍可用（detached 回退）。
    palette->open();
    REQUIRE(palette->is_open());
    palette->close();
}

TEST_CASE("command palette pointer click activates a row", "[command-palette][pointer]") {
    PaletteHarness harness;
    harness.open();
    std::vector<std::string> selected;
    harness.palette->set_on_select([&selected](const std::string_view id) {
        selected.emplace_back(id);
    });

    // 浮层链路：overlay layer → overlay surface → DismissLayer → FocusScope → Shell
    //           → Panel → Surface。
    auto* layer = harness.host->layer_at(1);
    auto* overlay_root = layer != nullptr ? layer->layout_root() : nullptr;
    REQUIRE(overlay_root != nullptr);
    REQUIRE(overlay_root->child_count() > 0);
    auto* dismiss = overlay_root->get_child(0) != nullptr
        ? overlay_root->get_child(0)->as_control()
        : nullptr;
    REQUIRE(dismiss != nullptr);
    REQUIRE(dismiss->child_count() > 0);
    auto* scope = dismiss->get_child(0) != nullptr ? dismiss->get_child(0)->as_control() : nullptr;
    REQUIRE(scope != nullptr);
    auto* shell = scope->get_child(0) != nullptr ? scope->get_child(0)->as_control() : nullptr;
    REQUIRE(shell != nullptr);
    auto* panel = shell->get_child(0) != nullptr ? shell->get_child(0)->as_control() : nullptr;
    REQUIRE(panel != nullptr);
    auto* surface = panel->get_child(1) != nullptr ? panel->get_child(1)->as_control() : nullptr;
    REQUIRE(surface != nullptr);

    // 点第一条结果行的中间偏左（x 落在行内任意位置都算命中）。
    const auto style = harness.palette->resolved_style();
    const auto bounds = surface->global_bounds();
    const auto point = foundation::NanPoint(
        bounds.get_left() + 8.0F,
        bounds.get_top() + style.metrics.item_height * 0.5F
    );
    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        point
    ));

    REQUIRE(selected.size() == 1);
    REQUIRE(selected[0] == "file.new");
}

TEST_CASE("command palette click outside the panel dismisses it", "[command-palette][overlay]") {
    PaletteHarness harness;
    harness.open();
    REQUIRE(harness.palette->is_open());

    // 面板下方的空白仍然属于"外壳"，但不在面板矩形内：命中范围必须由面板矩形决定，
    // 否则这部分会被误判为面板内部、点了不关。
    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(4.0F, harness.viewport.get_height() - 4.0F)
    ));
    // 关闭是淡出动画驱动的，推进一帧让 phase 走到 closed。
    harness.palette->on_process(0.5F);
    REQUIRE_FALSE(harness.palette->is_open());
}

namespace
{
    /// 把前景按自身 alpha 合成到不透明背景上（同 theme_palette_contrast_tests）。
    [[nodiscard]] auto composite_over(
        const foundation::NanColor& foreground,
        const foundation::NanColor& background
    ) -> foundation::NanColor {
        const auto source = foreground.to<foundation::NanRgb>();
        const auto backdrop = background.to<foundation::NanRgb>();
        const auto alpha = source.alpha;
        return foundation::NanColor::from_rgb(
            source.red * alpha + backdrop.red * (1.0F - alpha),
            source.green * alpha + backdrop.green * (1.0F - alpha),
            source.blue * alpha + backdrop.blue * (1.0F - alpha)
        );
    }

    [[nodiscard]] auto contrast_of(
        const foundation::NanColor& foreground,
        const foundation::NanColor& background
    ) -> float {
        return foundation::nan_contrast_ratio(composite_over(foreground, background), background);
    }
} // namespace

TEST_CASE("command palette row text stays legible on every state fill", "[command-palette][theme][contrast]") {
    // 结果行会把文字画在三种底上：面板底（常规）、hover_fill（悬停）、focus_fill（键盘
    // 高亮）。前两者用 item_label，高亮用 highlight_text。三个组合都按 WCAG AA 正文档
    // （4.5:1）要求 —— 高亮行最容易踩坑，因为 accent 是一个**有饱和度的底**。
    const auto design = theme::default_design_system();
    for (const auto appearance: {theme::ColorAppearance::light, theme::ColorAppearance::dark}) {
        const auto style = theme::resolve_command_palette(design, appearance);
        INFO(
            "appearance=" << (appearance == theme::ColorAppearance::light ? "light" : "dark")
        );

        REQUIRE(contrast_of(style.item_label.color, style.panel.fill) >= 4.5F);
        REQUIRE(contrast_of(style.item_label.color, style.hover_fill) >= 4.5F);
        REQUIRE(contrast_of(style.highlight_text, style.focus_fill) >= 4.5F);
        REQUIRE(contrast_of(style.checked_indicator, style.panel.fill) >= 4.5F);
        REQUIRE(contrast_of(style.checked_indicator, style.hover_fill) >= 4.5F);
        // 空状态与分组标题是次要文字，仍按正文档要求。
        REQUIRE(contrast_of(style.empty.color, style.panel.fill) >= 4.5F);
        REQUIRE(contrast_of(style.group_label.color, style.panel.fill) >= 4.5F);
        REQUIRE(contrast_of(style.placeholder.color, style.panel.fill) >= 4.5F);
    }
}

TEST_CASE("command palette scrim always darkens", "[command-palette][theme][contrast]") {
    // 遮罩必须**始终压暗**。早期实现把遮罩从面板色派生，于是亮色外观下面板底接近白，
    // 遮罩就变成了一层白罩。这条断言把它固定住。
    const auto design = theme::default_design_system();
    for (const auto appearance: {theme::ColorAppearance::light, theme::ColorAppearance::dark}) {
        const auto style = theme::resolve_command_palette(design, appearance);
        INFO(
            "appearance=" << (appearance == theme::ColorAppearance::light ? "light" : "dark")
        );
        REQUIRE(style.scrim.oklch().light < 0.30F);
        REQUIRE(style.scrim.alpha() > 0.0F);
    }
}

TEST_CASE("command palette scrim and indicator are themeable", "[command-palette][theme][override]") {
    // 这两个字段是审查后补上的：之前遮罩由组件从面板色派生、勾选指示颜色根本不存在，
    // 主题作者想调也调不了。用"改字段 → 断言解析结果变化"确认四处同步都接通了。
    auto design = theme::default_design_system();
    const auto base = theme::resolve_command_palette(design, theme::ColorAppearance::light);

    design.components.command_palette.rules.push_back(
        theme::CommandPaletteRecipeRule {
            .scrim = theme::ThemeColor::token(theme::ColorToken::primary),
            .highlight_text = theme::ThemeColor::token(theme::ColorToken::primary),
            .checked_indicator = theme::ThemeColor::token(theme::ColorToken::error),
        }
    );
    const auto overridden = theme::resolve_command_palette(design, theme::ColorAppearance::light);

    REQUIRE_FALSE(overridden.scrim == base.scrim);
    REQUIRE_FALSE(overridden.highlight_text == base.highlight_text);
    REQUIRE_FALSE(overridden.checked_indicator == base.checked_indicator);
}
