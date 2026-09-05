//
// Dialog (modal overlay) tests.
//

#include "widget/dialog.hpp"

#include "foundation/geometry.hpp"
#include "reactive/graph.hpp"
#include "reactive/scope.hpp"
#include "render/render_device.hpp"
#include "scene/input_event.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "theme/theme_manager.hpp"
#include "widget/build_context.hpp"
#include "widget/builtin_component_traits.hpp"
#include "widget/button.hpp"
#include "widget/layout.hpp"

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
    REQUIRE_FALSE(harness.dialog->contains_point(foundation::NanPoint(5.0F, 5.0F)));

    harness.dialog->open();
    REQUIRE(harness.dialog->is_open());
    REQUIRE(harness.dialog->z_index_hint() == 1);
    REQUIRE(harness.dialog->is_focusable());
    REQUIRE(harness.dialog->contains_point(foundation::NanPoint(5.0F, 5.0F)));

    const auto props = harness.dialog->resolved_semantics_properties();
    REQUIRE(props.role == semantics::Role::dialog);
    REQUIRE(props.label == "Discard changes?");

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
    REQUIRE(dialog->child_count() == 1);

    dialog->open();
    REQUIRE(dialog->is_open());
}
