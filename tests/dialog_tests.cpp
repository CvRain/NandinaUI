//
// Dialog (modal overlay) tests.
//

#include <nandina/widget/dialog.hpp>

#include <nandina/foundation/geometry.hpp>
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
#include <nandina/widget/layout.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string_view>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
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

    struct DialogHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<widget::Dialog> dialog = widget::Dialog::create();
        std::shared_ptr<scene::NanControl> root =
            std::make_shared<scene::NanControl>(foundation::NanSize(800.0F, 600.0F));
        scene::NanSceneTree tree;

        DialogHarness() {
            tree.set_theme_manager(themes);
            root->add_child(dialog);
            tree.set_root(root);
        }

        void layout() {
            (void)tree.layout_root(foundation::NanSize(800.0F, 600.0F));
        }

        /// 树内回退：Dialog → DismissLayer → FocusScope → DialogPanel
        [[nodiscard]] auto panel() const -> scene::NanControl* {
            if (dialog->child_count() == 0) {
                return nullptr;
            }
            auto* dismiss = dialog->get_child(0)->as_control();
            if (dismiss == nullptr || dismiss->child_count() == 0) {
                return nullptr;
            }
            auto* scope = dismiss->get_child(0)->as_control();
            return scope != nullptr && scope->child_count() > 0
                ? scope->get_child(0)->as_control()
                : nullptr;
        }
    };

    /// 模拟一帧：process（状态机完成转换）→ animation（Host 推进淡入淡出）。
    void tick(DialogHarness& harness, const float dt) {
        harness.dialog->on_process(dt);
        {
            auto phase = harness.tree.enter_phase(scene::FramePhase::animation);
            harness.tree.advance_animations(dt);
        }
    }
} // namespace

TEST_CASE("dialog recipe resolves a translucent scrim and centered panel", "[dialog][theme]") {
    const auto system = theme::default_design_system();
    const auto light = theme::resolve_dialog(system, theme::ColorAppearance::light);
    const auto dark = theme::resolve_dialog(system, theme::ColorAppearance::dark);

    REQUIRE(light.scrim.alpha() > 0.0F);
    REQUIRE(light.scrim.alpha() < 1.0F);
    REQUIRE(light.metrics.panel_width == Catch::Approx(360.0F));
    REQUIRE(light.panel.radius > 0.0F);
    // 亮暗面板底色不同。
    REQUIRE_FALSE(light.panel.fill == dark.panel.fill);
}

TEST_CASE("dialog open toggles z-order, focusability and semantics", "[dialog]") {
    DialogHarness harness;
    harness.dialog->set_title("Discard changes?");
    harness.layout();

    REQUIRE_FALSE(harness.dialog->is_open());
    REQUIRE(harness.dialog->z_index_hint() == 0);
    REQUIRE_FALSE(harness.dialog->is_focusable());

    harness.dialog->open();
    harness.layout();
    REQUIRE(harness.dialog->is_open());
    REQUIRE(harness.dialog->z_index_hint() == 1);
    // 焦点不再落在 Dialog 节点上：FocusScope 负责限制焦点，内容为空时用它自己兜底，
    // 这样 Escape 与 Tab 才能沿浮层路径冒泡。
    REQUIRE(harness.tree.focused_node() != nullptr);
    REQUIRE(harness.tree.focused_node() != harness.dialog.get());

    // dialog 语义挂在面板上：浮层承载时 Dialog 节点只是不可见的锚点，在那里报告
    // 零尺寸的 dialog 会误导辅助技术，所以两种承载方式统一由面板暴露。
    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    REQUIRE(harness.tree.update_semantics());
    const auto* node = harness.tree.semantics_tree().find(panel->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::dialog);
    REQUIRE(node->properties.label == "Discard changes?");

    harness.dialog->close();
    REQUIRE_FALSE(harness.dialog->is_open());
}

TEST_CASE("dialog fade-in advances without error", "[dialog][animation]") {
    DialogHarness harness;
    harness.dialog->open();
    harness.layout();
    tick(harness, 0.5F); // 越过 long_duration，淡入结束
    tick(harness, 0.5F); // 空闲，无操作
    REQUIRE(harness.dialog->is_open());
}

TEST_CASE("dialog hides its content when closed", "[dialog][visibility]") {
    DialogHarness harness;
    auto button = widget::Button::create("OK");
    (void)harness.dialog->set_content(button);
    harness.layout();

    // 初始关闭：对话框及其内容子节点均不可见（回归：按钮不得常驻左上角）。
    REQUIRE_FALSE(harness.dialog->is_visible_in_tree());
    REQUIRE_FALSE(button->is_visible_in_tree());

    harness.dialog->open();
    harness.layout();
    REQUIRE(harness.dialog->is_visible_in_tree());
    REQUIRE(button->is_visible_in_tree());

    harness.dialog->close();
    // 淡出期间仍可见（模态），淡出完成后才隐藏。
    REQUIRE(harness.dialog->is_visible_in_tree());
    REQUIRE(button->is_visible_in_tree());
    tick(harness, 0.5F); // 完成淡出
    tick(harness, 0.5F); // closing → closed，隐藏内容
    REQUIRE_FALSE(harness.dialog->is_visible_in_tree());
    REQUIRE_FALSE(button->is_visible_in_tree());
}

TEST_CASE("dialog escape and scrim click dismiss it", "[dialog][input]") {
    DialogHarness harness;
    harness.dialog->open();
    harness.layout();
    harness.tree.set_focus(harness.dialog.get());

    // Escape 关闭。
    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(harness.dialog->is_open());

    // 遮罩（面板外）点击关闭；面板内点击不关闭。
    harness.dialog->open();
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(10.0F, 10.0F)
        )
    );
    REQUIRE_FALSE(harness.dialog->is_open());

    harness.dialog->open();
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(400.0F, 300.0F) // 面板中心
        )
    );
    REQUIRE(harness.dialog->is_open());
}

TEST_CASE("non-dismissible dialog ignores escape and scrim click", "[dialog][input]") {
    DialogHarness harness;
    harness.dialog->set_dismissible(false);
    harness.dialog->open();
    harness.layout();
    harness.tree.set_focus(harness.dialog.get());

    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE(harness.dialog->is_open());

    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(10.0F, 10.0F)
        )
    );
    REQUIRE(harness.dialog->is_open());
}

TEST_CASE("dialog traps focus within its panel", "[dialog][focus]") {
    DialogHarness harness;
    auto first = widget::Button::create("First");
    auto second = widget::Button::create("Second");
    auto row = widget::Row::create();
    row->add(first);
    row->add(second);
    (void)harness.dialog->set_content(row);
    harness.dialog->open();
    harness.layout();
    harness.tree.set_focus(first.get());
    REQUIRE(harness.tree.focused_node() == first.get());

    // Tab 前进到第二个按钮。
    harness.tree.dispatch_key(scene::KeyEvent(258, scene::KeyEvent::Action::press));
    REQUIRE(harness.tree.focused_node() == second.get());

    // Shift+Tab 回到第一个。
    harness.tree.dispatch_key(
        scene::KeyEvent(258, scene::KeyEvent::Action::press, {.shift = true})
    );
    REQUIRE(harness.tree.focused_node() == first.get());
}

TEST_CASE("dialog override adjusts scrim and close callback fires", "[dialog][override]") {
    DialogHarness harness;
    int closes = 0;
    harness.dialog->set_on_close([&closes] { ++closes; });
    harness.dialog->set_override(
        theme::DialogRecipeRule {
            .scrim = theme::ThemeColor::literal(foundation::NanColor::from_hex(0x000000, 0.6F)),
            .metrics_panel_width = theme::ThemeScalar::literal(500.0F),
        }
    );

    const auto style = harness.dialog->resolved_style();
    REQUIRE(style.metrics.panel_width == Catch::Approx(500.0F));
    REQUIRE(style.scrim.alpha() == Catch::Approx(0.6F));

    harness.dialog->open();
    harness.dialog->close();
    REQUIRE(closes == 0); // 淡出完成前不触发 on_close
    tick(harness, 0.5F);  // 完成淡出
    tick(harness, 0.5F);  // closing → closed，触发 on_close
    REQUIRE(closes == 1);
}

TEST_CASE("dialog paints without error when open", "[dialog][paint]") {
    DialogHarness harness;
    harness.dialog->set_title("Confirm");
    harness.dialog->open();
    harness.layout();
    RecordingDevice device;
    harness.tree.draw(device);
    SUCCEED();
}

TEST_CASE("dialog authoring via ComponentTraits", "[dialog][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);

    auto content = std::make_shared<widget::Button>("OK");
    auto dialog = ui.make<widget::Dialog>("Heads up", content).build();
    REQUIRE(dialog->title() == "Heads up");
    // 内容进入面板槽位而不是直接挂在 Dialog 之下：没有浮层服务时，遮罩层在打开时
    // 才作为子节点挂上来，因此 Dialog 自身始终只有一个模态壳。
    REQUIRE_FALSE(content->is_visible_in_tree());

    dialog->open();
    REQUIRE(dialog->is_open());
    REQUIRE(dialog->child_count() == 1);
    REQUIRE(content->is_visible_in_tree());
}

namespace
{
    /// 带窗口浮层的对话框宿主：内容层里放一个裁剪容器，对话框挂在其中。
    struct OverlayDialogHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(800.0F, 600.0F));
        std::shared_ptr<widget::Dialog> dialog = widget::Dialog::create();
        /// 非空时对话框挂在其中，用来验证浮层托管不受父级裁剪影响。
        std::shared_ptr<scene::NanControl> clip;
        scene::NanSceneTree tree;

        explicit OverlayDialogHarness(const bool inside_clip = false) {
            tree.set_theme_manager(themes);
            host->set_content(body);
            tree.set_root(host);
            // 占位兄弟：NanControl 只有一个可见子节点时会把它拉伸铺满，这里要让 body
            // 走多子布局，裁剪容器才能保持自己的 120x80。
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            if (inside_clip) {
                clip = std::make_shared<scene::NanControl>(foundation::NanSize(120.0F, 80.0F));
                clip->set_position(foundation::NanPoint(10.0F, 10.0F));
                clip->set_overflow(scene::ControlOverflow::clip);
                clip->add_child(dialog);
                body->add_child(clip);
            }
            else {
                body->add_child(dialog);
            }
        }

        void layout() {
            (void)tree.layout_root(foundation::NanSize(800.0F, 600.0F));
        }

        /// overlay layer → surface → DismissLayer → FocusScope → DialogPanel
        [[nodiscard]] auto panel() const -> scene::NanControl* {
            auto* surface = host->layer_at(1)->layout_root();
            if (surface == nullptr || surface->child_count() == 0) {
                return nullptr;
            }
            auto* dismiss = surface->get_child(0)->as_control();
            if (dismiss == nullptr || dismiss->child_count() == 0) {
                return nullptr;
            }
            auto* scope = dismiss->get_child(0)->as_control();
            return scope != nullptr && scope->child_count() > 0
                ? scope->get_child(0)->as_control()
                : nullptr;
        }
    };

    /// 推进淡入淡出直到状态机稳定。
    void settle(OverlayDialogHarness& harness) {
        for (int i = 0; i < 8; ++i) {
            harness.dialog->on_process(0.1F);
            {
                auto phase = harness.tree.enter_phase(scene::FramePhase::animation);
                harness.tree.advance_animations(0.1F);
            }
        }
    }
} // namespace

TEST_CASE("dialog hosts its panel in the window overlay", "[dialog][overlay]") {
    OverlayDialogHarness harness;
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);

    harness.dialog->open();
    harness.layout();

    REQUIRE(harness.dialog->is_open());
    REQUIRE(harness.host->overlay_count() == 1);

    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    // 面板按视口居中，而不是按父容器。
    const auto bounds = panel->global_bounds();
    REQUIRE(bounds.get_center().get_x() == Catch::Approx(400.0F).margin(1.0F));
    REQUIRE(bounds.get_center().get_y() == Catch::Approx(300.0F).margin(1.0F));

    harness.dialog->close();
    settle(harness);
    REQUIRE_FALSE(harness.dialog->is_open());
    REQUIRE(harness.host->overlay_count() == 0);
}

TEST_CASE("dialog escapes a clipping container", "[dialog][overlay][clip]") {
    // 对话框挂在 120x80 的裁剪容器里：树内绘制会被截断，浮层托管不应受影响。
    OverlayDialogHarness harness(true);
    harness.layout();

    harness.dialog->open();
    harness.layout();

    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    const auto bounds = panel->global_bounds();
    REQUIRE(bounds.get_center().get_x() == Catch::Approx(400.0F).margin(1.0F));
    REQUIRE(bounds.get_center().get_y() == Catch::Approx(300.0F).margin(1.0F));
    // 面板完整落在裁剪容器之外：中心不在容器内，且比容器更宽，说明没有被截断。
    const auto clip_bounds = harness.clip->global_bounds();
    REQUIRE(clip_bounds.is_valid());
    REQUIRE_FALSE(clip_bounds.contains_point(bounds.get_center()));
    REQUIRE(bounds.get_width() > clip_bounds.get_width());
}

TEST_CASE("dialog blocks input below while open", "[dialog][overlay][modal]") {
    OverlayDialogHarness harness;
    auto behind = std::make_shared<widget::Button>("Behind");
    behind->set_position(foundation::NanPoint(20.0F, 20.0F));
    harness.body->add_child(behind);
    harness.layout();

    const auto behind_point = foundation::NanPoint(30.0F, 30.0F);
    REQUIRE(harness.tree.hit_test(behind_point) == behind.get());

    harness.dialog->open();
    harness.layout();

    // 命中被模态壳接管，不再落到下层按钮。
    auto* hit = harness.tree.hit_test(behind_point);
    REQUIRE(hit != nullptr);
    REQUIRE(hit != behind.get());
}

TEST_CASE("dialog dismisses on scrim click and escape through the overlay", "[dialog][overlay][input]")
{
    OverlayDialogHarness harness;
    (void)harness.dialog->set_content(std::make_shared<widget::Button>("OK"));
    harness.dialog->open();
    harness.layout();

    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(10.0F, 10.0F)
    ));
    REQUIRE_FALSE(harness.dialog->is_open());

    harness.dialog->open();
    harness.layout();
    REQUIRE(harness.dialog->is_open());
    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(harness.dialog->is_open());
}

TEST_CASE("non-dismissible dialog swallows escape and scrim clicks", "[dialog][overlay][input]") {
    OverlayDialogHarness harness;
    harness.dialog->set_dismissible(false);
    harness.dialog->open();
    harness.layout();

    int presses = 0;
    auto behind = std::make_shared<widget::Button>("Behind");
    behind->set_position(foundation::NanPoint(10.0F, 10.0F));
    behind->set_on_click([&presses] { ++presses; });
    harness.body->add_child(behind);
    harness.layout();

    harness.tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE(harness.dialog->is_open());

    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(15.0F, 15.0F)
    ));
    REQUIRE(harness.dialog->is_open());
    // 模态层吞掉输入，下层按钮不响应。
    REQUIRE(presses == 0);
}

TEST_CASE("dialog restores focus to the opener on close", "[dialog][overlay][focus]") {
    OverlayDialogHarness harness;
    auto opener = std::make_shared<widget::Button>("Open");
    harness.body->add_child(opener);
    harness.layout();
    harness.tree.set_focus(opener.get());
    REQUIRE(harness.tree.focused_node() == opener.get());

    (void)harness.dialog->set_content(std::make_shared<widget::Button>("OK"));
    harness.dialog->open();
    harness.layout();
    REQUIRE(harness.tree.focused_node() != opener.get());

    harness.dialog->close();
    settle(harness);
    REQUIRE(harness.tree.focused_node() == opener.get());
}

TEST_CASE("dialog reopens after closing without leaking overlays", "[dialog][overlay][lifetime]") {
    OverlayDialogHarness harness;
    (void)harness.dialog->set_content(std::make_shared<widget::Button>("OK"));
    harness.layout();

    for (int round = 0; round < 3; ++round) {
        harness.dialog->open();
        harness.layout();
        REQUIRE(harness.host->overlay_count() == 1);

        harness.dialog->close();
        settle(harness);
        REQUIRE(harness.host->overlay_count() == 0);
        REQUIRE_FALSE(harness.dialog->is_open());
    }
}

TEST_CASE("floating content opened inside a dialog stays above the scrim", "[dialog][overlay][nested]") {
    OverlayDialogHarness harness;
    // 模态面板里放一个 Select：它的弹层必须压在遮罩之上，否则会被埋掉且点不到。
    auto select = widget::Select::create({"A", "B", "C"});
    harness.dialog->set_content(select);
    harness.dialog->open();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);

    const auto field = foundation::NanPoint(
        select->global_bounds().get_left() + 10.0F,
        select->global_bounds().get_top() + 10.0F
    );
    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        field
    ));
    harness.layout();
    REQUIRE(select->is_open());
    REQUIRE(harness.host->overlay_count() == 2);

    auto* surface = harness.host->layer_at(1)->layout_root();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 2);
    auto* popup_dismiss = surface->get_child(1)->as_control();
    REQUIRE(popup_dismiss != nullptr);
    auto* popup = popup_dismiss->get_child(0)->as_control();
    REQUIRE(popup != nullptr);

    // 直接点第二条选项：只有弹层真的压在遮罩之上，这次点击才会被选项接走。
    const auto row_height = popup->global_bounds().get_height() / 3.0F;
    const auto option_point = foundation::NanPoint(
        popup->global_bounds().get_left() + 10.0F,
        popup->global_bounds().get_top() + row_height * 1.5F
    );
    harness.tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        option_point
    ));
    REQUIRE(select->selected_index() == 1);
    REQUIRE_FALSE(select->is_open());
}

TEST_CASE("dialog panel lays out the title inside the padded content area", "[dialog][layout]") {
    OverlayDialogHarness harness;
    harness.dialog->set_title("删除这条记录？");
    harness.dialog->set_content(widget::Label::create(harness.graph, "删除后无法恢复。"));
    harness.dialog->open();
    harness.layout();

    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    REQUIRE(panel->child_count() == 2);

    // 标题是 header 槽位的内容，必须和 content 一样参与测量与布局；只把它挂成子节点而
    // 不登记到槽位，它会停在面板原点，与下方内容重叠。
    auto* first = panel->get_child(0)->as_node2d();
    auto* second = panel->get_child(1)->as_node2d();
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    auto* title = first->global_bounds().get_top() <= second->global_bounds().get_top() ? first
                                                                                        : second;
    auto* content = title == first ? second : first;

    const auto style = harness.dialog->resolved_style();
    const auto panel_bounds = panel->global_bounds();
    const auto title_bounds = title->global_bounds();
    REQUIRE(title_bounds.get_left() - panel_bounds.get_left()
            == Catch::Approx(style.metrics.padding_x));
    REQUIRE(title_bounds.get_top() - panel_bounds.get_top()
            == Catch::Approx(style.metrics.padding_y));
    // 两段文字之间是 recipe 的 gap，不重叠。
    const auto gap = content->global_bounds().get_top()
        - (title_bounds.get_top() + title_bounds.get_height());
    REQUIRE(gap == Catch::Approx(style.metrics.gap));
}

TEST_CASE("a floating dialog leaves no gap in its parent layout", "[dialog][overlay][layout]") {
    // 浮层承载时 Dialog 只是页面里的锚点。它一旦被标记为可见，父级布局就会为这个零高子节点
    // 多算一个 gap，把后面的内容整体推下去。
    auto host = scene::OverlayHost::create();
    auto column = widget::Column::create();
    column->set_gap(12.0F);
    auto above = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 40.0F));
    auto below = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 40.0F));
    auto dialog = widget::Dialog::create();
    column->add(above);
    column->add(dialog);
    column->add(below);
    host->set_content(column);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 400.0F)) >= 1);
    const auto before = below->position().get_y() - (above->position().get_y() + above->height());
    REQUIRE(before == Catch::Approx(12.0F));

    dialog->open();
    column->mark_layout_dirty();
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 400.0F)) >= 1);
    REQUIRE(host->overlay_count() == 1);

    const auto after = below->position().get_y() - (above->position().get_y() + above->height());
    REQUIRE(after == Catch::Approx(before));
    // 锚点保持不可见正是间距不变的机制。
    REQUIRE_FALSE(dialog->is_visible_in_tree());
}

TEST_CASE("a floating dialog exposes the dialog role from its panel", "[dialog][overlay][semantics]") {
    OverlayDialogHarness harness;
    harness.dialog->set_title("删除这条记录？");
    harness.dialog->set_content(std::make_shared<widget::Button>("删除"));
    harness.dialog->open();
    harness.layout();

    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    REQUIRE(harness.tree.update_semantics());
    const auto* node = harness.tree.semantics_tree().find(panel->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::dialog);
    REQUIRE(node->properties.label == "删除这条记录？");
}
