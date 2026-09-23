//
// Popover (non-modal anchored floating container) tests.
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
#include <nandina/widget/label.hpp>
#include <nandina/widget/popover.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;

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
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
    };

    /// 带窗口浮层的 Popover 宿主：内容层里放一个触发按钮，Popover 挂在内容层。
    struct PopoverHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(320.0F, 240.0F));
        std::shared_ptr<widget::Button> trigger = widget::Button::create("Open");
        std::shared_ptr<widget::Label> label;
        std::shared_ptr<widget::Popover> popover;
        scene::NanSceneTree tree;
        foundation::NanSize viewport {320.0F, 240.0F};

        explicit PopoverHarness(
            const foundation::NanPoint popover_position = foundation::NanPoint(0.0F, 80.0F)
        ):
            label(widget::Label::create(graph, "Popover content")) {
            tree.set_theme_manager(themes);
            popover = widget::Popover::create(trigger, label);
            // 触发控件由 Popover 摆在自己的矩形里，所以锚点是 Popover 的位置，不是
            // 触发控件自己的 set_position()。
            popover->set_position(popover_position);
            body->add_child(popover);
            // 第二个子节点阻止 NanControl 的单子拉伸路径把 Popover 撑满内容层，
            // 否则触发控件的锚点就不是它自己的位置了。
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            host->set_content(body);
            tree.set_root(host);
        }

        void layout() {
            (void)tree.layout_root(viewport);
        }
    };

    /// 浮层内容层（order 1000）里的单个浮层，未挂载时返回 nullptr。
    auto overlay_root(scene::OverlayHost& host) -> scene::NanControl* {
        auto* layer = host.layer_at(1);
        auto* surface = layer != nullptr ? layer->layout_root() : nullptr;
        if (surface == nullptr || surface->child_count() == 0) {
            return nullptr;
        }
        auto* child = surface->get_child(0);
        return child != nullptr ? child->as_control() : nullptr;
    }

    /// 浮层里绘制面板的那一层：DismissLayer → FocusScope → PopoverSurface。
    auto overlay_panel(scene::OverlayHost& host) -> scene::NanControl* {
        auto* dismiss = overlay_root(host);
        if (dismiss == nullptr || dismiss->child_count() == 0) {
            return nullptr;
        }
        auto* scope = dismiss->get_child(0)->as_control();
        return scope != nullptr && scope->child_count() > 0 ? scope->get_child(0)->as_control()
                                                            : nullptr;
    }
} // namespace

TEST_CASE("popover is closed and sized to its trigger by default", "[popover]") {
    widget::Popover popover(widget::Button::create("Open"));
    REQUIRE_FALSE(popover.is_open());
    REQUIRE(popover.placement() == widget::internal::OverlayPlacement::bottom);
    REQUIRE(popover.alignment() == widget::internal::OverlayAlignment::start);
    REQUIRE(popover.dismissible());
    REQUIRE(popover.content() == nullptr);

    auto* trigger = popover.trigger().get();
    REQUIRE(trigger != nullptr);
    const auto measured = popover.measure_layout(scene::LayoutConstraints::loose());
    const auto trigger_size = trigger->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(measured.get_width() == Catch::Approx(trigger_size.get_width()));
    REQUIRE(measured.get_height() == Catch::Approx(trigger_size.get_height()));
}

TEST_CASE("popover recipe resolves a popover panel with semantic roles", "[popover][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_popover(design, theme::ColorAppearance::light);
    const auto dark = theme::resolve_popover(design, theme::ColorAppearance::dark);

    REQUIRE(light.panel.fill.oklch().light == Catch::Approx(design.light.popover.oklch().light));
    REQUIRE(light.panel.border.alpha() > 0.0F);
    REQUIRE(light.panel.radius > 0.0F);
    // 默认值引用语义标量：改 token 会跟着变。
    REQUIRE(light.metrics.padding_x == Catch::Approx(design.tokens.spacing.md));
    REQUIRE(light.metrics.gap == Catch::Approx(design.tokens.spacing.sm));
    // 亮暗面板底色不同：默认值只用语义角色，不写字面量。
    REQUIRE_FALSE(light.panel.fill == dark.panel.fill);
}

TEST_CASE("popover open and close are idempotent", "[popover]") {
    PopoverHarness harness;
    harness.layout();

    harness.popover->open();
    REQUIRE(harness.popover->is_open());
    harness.popover->open();
    REQUIRE(harness.popover->is_open());
    REQUIRE(harness.host->overlay_count() == 1);

    harness.popover->close();
    REQUIRE_FALSE(harness.popover->is_open());
    harness.popover->close();
    REQUIRE_FALSE(harness.popover->is_open());
    REQUIRE(harness.host->overlay_count() == 0);

    harness.popover->toggle();
    REQUIRE(harness.popover->is_open());
    harness.popover->toggle();
    REQUIRE_FALSE(harness.popover->is_open());
}

TEST_CASE("popover trigger toggles from pointer and keyboard activation", "[popover][input]") {
    PopoverHarness harness;
    harness.layout();
    const auto center = harness.trigger->global_bounds().get_center();

    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        center,
    });
    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        center,
    });
    REQUIRE(harness.popover->is_open());

    harness.popover->close();
    REQUIRE(harness.tree.focused_node() == harness.trigger.get());
    harness.tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
    REQUIRE(harness.popover->is_open());
}

TEST_CASE("disabled popover trigger does not open", "[popover][input]") {
    PopoverHarness harness;
    harness.trigger->set_disabled(true);
    harness.layout();
    const auto center = harness.trigger->global_bounds().get_center();

    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        center,
    });
    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        center,
    });
    REQUIRE_FALSE(harness.popover->is_open());
}

TEST_CASE("popover presents its content through the overlay host", "[popover][overlay]") {
    PopoverHarness harness;
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);

    harness.popover->open();
    harness.layout();

    REQUIRE(harness.host->overlay_count() == 1);
    auto* panel = overlay_panel(*harness.host);
    REQUIRE(panel != nullptr);
    REQUIRE(panel->child_count() == 1);
    REQUIRE(panel->get_child(0)->as_control() == harness.label.get());
    // 触发控件留在原布局里，内容不在 Popover 子树之下。
    REQUIRE_FALSE(harness.popover->is_ancestor_of(*panel));
    REQUIRE(harness.host->layer_at(1)->is_ancestor_of(*panel));

    // 非模态：浮层承载不阻断其下层（`block_below` 保持 false，层模式仍为 pass）。
    REQUIRE(harness.host->layer_at(1)->input_mode() == scene::LayerInputMode::pass);
}

TEST_CASE("popover honours every placement", "[popover][overlay][placement]") {
    PopoverHarness harness;
    harness.layout();
    const auto trigger_bounds = harness.trigger->global_bounds();
    const auto gap = harness.popover->gap();

    // 这些断言比对的是 placement / alignment 的原始几何；视口内收（viewport_padding）
    // 会让贴边场景整体平移，所以先清零，边界行为交给 flip/shift 用例覆盖。
    harness.popover->set_viewport_padding(0.0F);
    harness.popover->open();
    harness.layout();

    harness.popover->set_placement(widget::internal::OverlayPlacement::bottom);
    harness.layout();
    auto* panel = overlay_panel(*harness.host);
    REQUIRE(panel != nullptr);
    REQUIRE(
        panel->global_bounds().get_top()
        == Catch::Approx(trigger_bounds.get_bottom() + gap).margin(0.5F)
    );

    harness.popover->set_placement(widget::internal::OverlayPlacement::top);
    harness.layout();
    REQUIRE(
        panel->global_bounds().get_bottom()
        == Catch::Approx(trigger_bounds.get_top() - gap).margin(0.5F)
    );

    harness.popover->set_placement(widget::internal::OverlayPlacement::right);
    harness.layout();
    REQUIRE(
        panel->global_bounds().get_left()
        == Catch::Approx(trigger_bounds.get_right() + gap).margin(0.5F)
    );

    harness.popover->set_placement(widget::internal::OverlayPlacement::left);
    harness.layout();
    // 左侧放不下（面板比锚点左侧空间宽），位置器把它收进视口；关键是必须在视口内。
    const auto left_bounds = panel->global_bounds();
    REQUIRE(left_bounds.get_left() >= 0.0F);
    REQUIRE(left_bounds.get_right() <= harness.viewport.get_width());
    // 切换 placement 不能泄漏浮层。
    REQUIRE(harness.host->overlay_count() == 1);
}

TEST_CASE("popover honours every alignment", "[popover][overlay][placement]") {
    // 锚点放在视口中段：面板比触发控件宽得多，贴左边会被位置器收进视口，对齐就观察不到。
    PopoverHarness harness(foundation::NanPoint(120.0F, 80.0F));
    harness.layout();
    const auto trigger_bounds = harness.trigger->global_bounds();

    harness.popover->set_viewport_padding(0.0F);
    harness.popover->set_placement(widget::internal::OverlayPlacement::bottom);
    harness.popover->open();
    harness.layout();
    auto* panel = overlay_panel(*harness.host);
    REQUIRE(panel != nullptr);

    harness.popover->set_alignment(widget::internal::OverlayAlignment::start);
    harness.layout();
    REQUIRE(
        panel->global_bounds().get_left() == Catch::Approx(trigger_bounds.get_left()).margin(0.5F)
    );

    harness.popover->set_alignment(widget::internal::OverlayAlignment::center);
    harness.layout();
    REQUIRE(
        panel->global_bounds().get_center().get_x()
        == Catch::Approx(trigger_bounds.get_center().get_x()).margin(0.5F)
    );

    harness.popover->set_alignment(widget::internal::OverlayAlignment::end);
    harness.layout();
    REQUIRE(
        panel->global_bounds().get_right() == Catch::Approx(trigger_bounds.get_right()).margin(0.5F)
    );
}

TEST_CASE(
    "popover flips and shifts inside the viewport near an edge",
    "[popover][overlay][placement]"
) {
    // 锚点贴着视口右下角：bottom 放不下必须翻到上方，right 放不下必须向左收。
    PopoverHarness harness(foundation::NanPoint(280.0F, 210.0F));
    harness.layout();
    const auto trigger_bounds = harness.trigger->global_bounds();
    REQUIRE(trigger_bounds.get_bottom() > harness.viewport.get_height() - 40.0F);
    REQUIRE(trigger_bounds.get_right() > harness.viewport.get_width() - 60.0F);

    harness.popover->open();
    harness.layout();

    auto* panel = overlay_panel(*harness.host);
    REQUIRE(panel != nullptr);
    const auto bounds = panel->global_bounds();
    // 翻到上方，并且右边缘被收进视口。
    REQUIRE(bounds.get_bottom() <= trigger_bounds.get_top());
    REQUIRE(bounds.get_top() >= 0.0F);
    REQUIRE(bounds.get_right() <= harness.viewport.get_width());
    REQUIRE(bounds.get_left() >= 0.0F);
}

TEST_CASE("outer click closes a dismissible popover", "[popover][overlay][input]") {
    PopoverHarness harness;
    harness.layout();
    harness.popover->open();
    harness.layout();
    REQUIRE(harness.popover->is_open());

    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(300.0F, 20.0F)
        )
    );
    REQUIRE_FALSE(harness.popover->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("clicking inside a popover keeps it open", "[popover][overlay][input]") {
    PopoverHarness harness;
    harness.layout();
    harness.popover->open();
    harness.layout();

    auto* panel = overlay_panel(*harness.host);
    REQUIRE(panel != nullptr);
    // 面板本身不参与命中（内涵控件才有命中语义），但落在面板矩形里的点击不能被
    // 当成外部点击：dismiss 层用面板屏幕矩形判定。
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            panel->global_bounds().get_center()
        )
    );
    REQUIRE(harness.popover->is_open());
}

TEST_CASE("escape closes a dismissible popover", "[popover][overlay][input]") {
    PopoverHarness harness;
    harness.layout();
    harness.popover->open();
    harness.layout();
    // 焦点进入浮层，Escape 才沿浮层路径冒泡。
    REQUIRE(harness.tree.focused_node() != nullptr);

    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(harness.popover->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE(
    "a non-dismissible popover ignores outer clicks and escape",
    "[popover][overlay][input]"
) {
    PopoverHarness harness;
    harness.popover->set_dismissible(false);
    harness.layout();
    harness.popover->open();
    harness.layout();

    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(300.0F, 20.0F)
        )
    );
    REQUIRE(harness.popover->is_open());

    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE(harness.popover->is_open());

    // 仍然可以程序化关闭。
    harness.popover->close();
    REQUIRE_FALSE(harness.popover->is_open());
}

TEST_CASE("popover fires on_open and on_close once per transition", "[popover][callback]") {
    PopoverHarness harness;
    int opens = 0;
    int closes = 0;
    harness.popover->set_on_open([&opens] { ++opens; });
    harness.popover->set_on_close([&closes] { ++closes; });
    harness.layout();

    harness.popover->open();
    harness.popover->open();
    REQUIRE(opens == 1);
    REQUIRE(closes == 0);

    harness.popover->close();
    harness.popover->close();
    REQUIRE(closes == 1);

    // 用户关闭路径同样触发 on_close。
    harness.popover->open();
    harness.layout();
    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(harness.popover->is_open());
    REQUIRE(closes == 2);
}

TEST_CASE("popover exposes expanded / collapsed semantics", "[popover][semantics]") {
    PopoverHarness harness;
    harness.layout();

    REQUIRE(harness.tree.update_semantics());
    const auto* node = harness.tree.semantics_tree().find(harness.popover->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::generic);
    REQUIRE(node->properties.state.checked.has_value());
    REQUIRE_FALSE(*node->properties.state.checked);

    harness.popover->open();
    harness.layout();
    REQUIRE(harness.tree.update_semantics());
    node = harness.tree.semantics_tree().find(harness.popover->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(*node->properties.state.checked);
    // 触发控件自己仍是独立节点，暴露可激活的 action。
    const auto* trigger_node = harness.tree.semantics_tree().find(harness.trigger->semantics_id());
    REQUIRE(trigger_node != nullptr);
    REQUIRE(semantics::supports(trigger_node->properties.actions, semantics::Action::activate));
}

TEST_CASE(
    "popover resolves both theme switches and instance overrides",
    "[popover][theme][override]"
) {
    PopoverHarness harness;
    harness.layout();

    const auto initial = harness.popover->resolved_style();

    // 实例覆盖：只改指定字段。
    harness.popover->set_override(
        theme::PopoverRecipeRule {
            .panel_radius = theme::ThemeScalar::literal(2.0F),
            .metrics_padding_x = theme::ThemeScalar::literal(20.0F),
        }
    );
    const auto overridden = harness.popover->resolved_style();
    REQUIRE(overridden.panel.radius == Catch::Approx(2.0F));
    REQUIRE(overridden.metrics.padding_x == Catch::Approx(20.0F));
    // 未覆盖的字段保持默认。
    REQUIRE(overridden.metrics.gap == Catch::Approx(initial.metrics.gap));

    // 主题切换：DesignSystem 的规则层同样作用到解析结果（守住 apply_rule 四步同步）。
    auto design = theme::default_design_system();
    design.components.popover.rules.push_back(
        theme::PopoverRecipeRule {
            .panel_fill = theme::ThemeColor::token(theme::ColorToken::error),
            .metrics_gap = theme::ThemeScalar::literal(19.0F),
        }
    );
    harness.themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    const auto themed = harness.popover->resolved_style();
    REQUIRE(
        themed.panel.fill.oklch().light
        == Catch::Approx(harness.themes.design_system().light.error.oklch().light)
    );
    REQUIRE(themed.metrics.gap == Catch::Approx(19.0F));
    // 实例覆盖在系统切换后保留。
    REQUIRE(themed.panel.radius == Catch::Approx(2.0F));

    // 显式 NanTheme 覆盖（不再跟随系统切换）。先清掉实例覆盖，否则它会继续压过主题。
    harness.popover->set_override(theme::PopoverRecipeRule {});
    auto explicit_theme = theme::default_theme();
    explicit_theme.tokens.spacing.md = 33.0F;
    harness.popover->set_theme(explicit_theme);
    REQUIRE(harness.popover->theme_ref().tokens.spacing.md == Catch::Approx(33.0F));
    REQUIRE(harness.popover->resolved_style().metrics.padding_x == Catch::Approx(33.0F));
}

TEST_CASE("popover replaces its content slot safely", "[popover][overlay][content]") {
    PopoverHarness harness;
    harness.layout();
    harness.popover->open();
    harness.layout();

    auto* first = overlay_panel(*harness.host);
    REQUIRE(first != nullptr);
    REQUIRE(first->child_count() == 1);

    auto replacement = widget::Label::create(harness.graph, "Replaced");
    harness.popover->set_content(replacement);
    harness.layout();

    REQUIRE(harness.popover->content() == replacement.get());
    REQUIRE(harness.popover->is_open());
    REQUIRE(harness.host->overlay_count() == 1);
    auto* panel = overlay_panel(*harness.host);
    REQUIRE(panel != nullptr);
    REQUIRE(panel->child_count() == 1);
    REQUIRE(panel->get_child(0)->as_control() == replacement.get());
    // 旧槽位节点已脱离场景树，不会与替换后的内容并存。
    REQUIRE(harness.label->parent() == nullptr);
    // 旧内容已经脱离场景树，不会留下孤儿。
    REQUIRE(harness.label->parent() == nullptr);
}

TEST_CASE("popover paints its panel while open", "[popover][overlay][paint]") {
    PopoverHarness harness;
    harness.popover->open();
    harness.layout();

    RecordingDevice device;
    harness.tree.draw(device);
    // 面板的圆角填充至少画了一次（内容层是纯 NanControl，不画圆角）。
    REQUIRE(device.rounded_rects >= 1);
}

TEST_CASE("popover restores focus to its opener when it closes", "[popover][overlay][focus]") {
    PopoverHarness harness;
    harness.layout();
    harness.tree.set_focus(harness.trigger.get());
    REQUIRE(harness.tree.focused_node() == harness.trigger.get());

    harness.popover->open();
    harness.layout();
    // 焦点进入浮层，Tab / Escape 才可达。
    REQUIRE(harness.tree.focused_node() != harness.trigger.get());

    harness.popover->close();
    REQUIRE(harness.tree.focused_node() == harness.trigger.get());
}

TEST_CASE("popover traps tab focus inside its content", "[popover][overlay][focus]") {
    PopoverHarness harness;
    auto first = widget::Button::create("First");
    auto second = widget::Button::create("Second");
    auto row = widget::Row::create();
    row->add(first);
    row->add(second);
    harness.popover->set_content(row);
    harness.layout();
    harness.popover->open();
    harness.layout();

    harness.tree.set_focus(first.get());
    REQUIRE(harness.tree.focused_node() == first.get());
    harness.tree.dispatch_key(scene::KeyEvent(258, scene::KeyEvent::Action::press));
    REQUIRE(harness.tree.focused_node() == second.get());
    // Tab 循环留在浮层内部，不会跑到页面其它控件。
    harness.tree.dispatch_key(scene::KeyEvent(258, scene::KeyEvent::Action::press));
    REQUIRE(harness.tree.focused_node() == first.get());
}

TEST_CASE("destroying an open popover releases its overlay", "[popover][overlay][lifetime]") {
    PopoverHarness harness;
    harness.layout();
    harness.popover->open();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);

    // 外部仍持有内容；先把 Popover 从场景树里摘下来再销毁，浮层必须跟着释放。
    auto content = harness.label;
    auto popover = harness.popover;
    (void)harness.body->remove_child(*popover);
    popover.reset();
    REQUIRE(harness.host->overlay_count() == 0);
    REQUIRE(content != nullptr);
}

TEST_CASE("popover tolerates an expired injected overlay service", "[popover][overlay][lifetime]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};
    auto trigger = ui.make<widget::Button>("Open").build();
    auto popover = ui.make<widget::Popover>(trigger).build();
    host.reset();

    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 120.0F));
    root->add_child(popover);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 120.0F)) >= 1);

    // 服务已失效：不得崩溃，且回退到树内展示。
    popover->open();
    REQUIRE(popover->is_open());
    REQUIRE(popover->is_visible_in_tree());
}

TEST_CASE("popover falls back to in-tree hosting without an overlay host", "[popover][fallback]") {
    reactive::Graph graph;
    auto trigger = widget::Button::create("Open");
    auto content = widget::Label::create(graph, "Detached content");
    auto popover = widget::Popover::create(trigger, content);

    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 160.0F));
    root->add_child(popover);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 160.0F)) >= 1);

    // 收起时触发控件仍然可见可点，内容既不在树里也不挂在面板上。
    REQUIRE(trigger->is_visible_in_tree());
    REQUIRE_FALSE(content->is_inside_tree());
    REQUIRE(content->parent() == nullptr);
    // 唯一子节点仍是触发控件。
    REQUIRE(popover->child_count() == 1);
    REQUIRE(popover->get_child(0) == trigger.get());

    popover->open();
    // 布局是"根脏了才跑"的模型（`_layout_root_once` 只看根自身），所以组件自己调用
    // mark_layout_dirty() 并不足以让下一次 layout_root() 真的排布它；窗口里由帧循环
    // 持续驱动，测试里要显式让根变脏。
    root->mark_layout_dirty();
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 160.0F)) >= 1);
    REQUIRE(popover->is_open());
    REQUIRE(popover->is_visible_in_tree());
    REQUIRE(popover->z_index_hint() == 1);
    // 内容确实在树内可见，并且落到了锚点下方。
    REQUIRE(content->is_visible_in_tree());
    REQUIRE(popover->is_ancestor_of(*content));
    REQUIRE(content->global_bounds().get_top() >= trigger->global_bounds().get_bottom());

    popover->close();
    REQUIRE_FALSE(popover->is_open());
    REQUIRE(popover->content() == content.get());
    REQUIRE(popover->z_index_hint() == 0);
}

TEST_CASE("popover authoring via ComponentTraits", "[popover][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 120.0F)));
    scene::NanSceneTree host_tree;
    host_tree.set_root(host);
    REQUIRE(host_tree.layout_root(foundation::NanSize(200.0F, 120.0F)) >= 1);

    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};

    // 默认构造路径（测试门槛里的 `ui.make<Popover>()`）。
    auto empty = ui.make<widget::Popover>().build();
    REQUIRE(empty->trigger() == nullptr);
    REQUIRE_FALSE(empty->is_open());

    auto trigger = ui.make<widget::Button>("Open").build();
    auto content = ui.make<widget::Label>("Content").build();
    auto popover = ui.make<widget::Popover>(trigger, content).build();
    REQUIRE(popover->trigger() == trigger);
    REQUIRE(popover->content() == content.get());

    // 服务来自 BuildContext 注入：挂在普通控件根下也能 portal。
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 120.0F));
    root->add_child(popover);
    root->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 120.0F)) >= 1);

    popover->open();
    REQUIRE(host->overlay_count() == 1);
    popover->close();
    REQUIRE(host->overlay_count() == 0);
}
