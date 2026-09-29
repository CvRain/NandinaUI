//
// AlertDialog tests: Dialog constrained to "the user must choose".
//

#include <nandina/reactive/graph.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/alert_dialog.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/dialog.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>

using namespace nandina;

namespace
{
    struct AlertHarness {
        reactive::Graph graph;
        theme::ThemeManager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(640.0F, 400.0F));
        std::shared_ptr<widget::AlertDialog> alert;
        scene::NanSceneTree tree;
        foundation::NanSize viewport {640.0F, 400.0F};

        explicit AlertHarness(
            std::string title = "删除这个文件？",
            std::string description = "删除后无法恢复。"
        ):
            alert(widget::AlertDialog::create(std::move(title), std::move(description))) {
            tree.set_theme_manager(themes);
            body->add_child(alert);
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            host->set_content(body);
            tree.set_root(host);
        }

        void layout() {
            (void)tree.layout_root(viewport);
        }
        void open() {
            layout();
            alert->open();
            layout();
        }
        /// 浮层链路：overlay surface → DismissLayer → FocusScope → DialogPanel。
        [[nodiscard]] auto panel() const -> scene::NanControl* {
            auto* layer = host->layer_at(1);
            auto* root = layer != nullptr ? layer->layout_root() : nullptr;
            if (root == nullptr || root->child_count() == 0) {
                return nullptr;
            }
            auto* dismiss = root->get_child(0) != nullptr
                ? root->get_child(0)->as_control()
                : nullptr;
            auto* scope = dismiss != nullptr && dismiss->child_count() > 0
                ? dismiss->get_child(0)->as_control()
                : nullptr;
            return scope != nullptr && scope->child_count() > 0
                ? scope->get_child(0)->as_control()
                : nullptr;
        }
        void press_escape() {
            tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
        }
        /// 按语义在浮层子树里找按钮：比写死节点层级耐改，也顺带验证按钮语义暴露正确。
        [[nodiscard]] static auto find_button(
            scene::NanControl* node,
            const std::string_view label
        ) -> scene::NanControl* {
            if (node == nullptr) {
                return nullptr;
            }
            const auto props = node->semantics_properties();
            if (props.role == semantics::Role::button && props.label == label) {
                return node;
            }
            for (std::size_t i = 0; i < node->child_count(); ++i) {
                auto* child = node->get_child(i);
                if (auto* found = find_button(child != nullptr ? child->as_control() : nullptr, label);
                    found != nullptr)
                {
                    return found;
                }
            }
            return nullptr;
        }

        void click(scene::NanControl& node) {
            // 按钮的 on_click 是 press + release 且当时 hovered（Pressable::emit_click），
            // 所以先 move 建立悬停，再一次完整的按下 / 抬起。
            const auto point = node.global_bounds().get_center();
            tree.dispatch_mouse_move(scene::MouseMoveEvent(point, foundation::NanPoint {}));
            tree.dispatch_mouse_button(scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                point
            ));
            tree.dispatch_mouse_button(scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::release,
                point
            ));
        }

        void click_outside() {
            tree.dispatch_mouse_button(scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                foundation::NanPoint(2.0F, 2.0F)
            ));
        }
        /// 把淡出动画推完，让 phase 真正走到 closed。
        ///
        /// 必须走 `tree.process()`（逐节点调用 on_process）而不是 `alert->on_process()`：
        /// 内部 Dialog 是独立节点，自己会收到 tick。AlertDialog **刻意不转发**
        /// on_process —— 转发了它就会一帧被推进两次，动画与计时都会跑飞。
        void settle() {
            // 淡出是动画：`process()` 只跑 on_process，**动画要单独 advance**
            // （真实窗口每帧两个都做，顺序也是先推进动画再处理节点）。
            // 浮层撤出还复用既有 deferred mutation，移除发生在下一轮 process 开头，
            // 所以多推几帧。
            for (int frame = 0; frame < 4; ++frame) {
                tree.advance_animations(0.5F);
                tree.process(0.5F);
            }
        }
    };
} // namespace

TEST_CASE("alert dialog requires a description and a confirm action", "[alert-dialog][boundary]") {
    // 这两条不是风格约束，是可用性约束（见 alert_dialog.hpp 的文件头）：
    // 没有描述时用户不知道自己在确认什么；没有确认动作时不可关闭的模态会把用户困住。
    REQUIRE_THROWS_AS(
        widget::AlertDialog::create("标题", ""),
        std::invalid_argument
    );

    auto alert = widget::AlertDialog::create("标题", "描述");
    REQUIRE_THROWS_AS(alert->set_description(""), std::invalid_argument);
    REQUIRE_THROWS_AS(alert->set_confirm_text(""), std::invalid_argument);

    // 取消按钮允许留空：表示"只留确认一条出路"。
    REQUIRE_NOTHROW(alert->set_cancel_text(""));
    REQUIRE(alert->cancel_text().empty());
    // 拒绝非法输入后原值保持不变（不是"先改坏再抛"）。
    REQUIRE(alert->description() == "描述");
    REQUIRE(alert->confirm_text() == "确定");
}

TEST_CASE("alert dialog exposes alertdialog semantics", "[alert-dialog][semantics]") {
    AlertHarness harness;
    harness.open();
    REQUIRE(harness.panel() != nullptr);
    const auto props = harness.panel()->semantics_properties();
    REQUIRE(props.role == semantics::Role::alert_dialog);
    REQUIRE(props.label == "删除这个文件？");
    REQUIRE(semantics::role_name(props.role) == "alertdialog");

    // 对照：普通 Dialog 仍是 dialog —— 这条确认 role 变体是被 AlertDialog 打开的，
    // 而不是把 Dialog 的语义整体改掉了。
    auto plain = widget::Dialog::create();
    plain->set_title("普通对话框");
    // 不注入服务：host 是树根，Dialog 沿祖先链就能找到它。
    harness.body->add_child(plain);
    harness.layout();
    plain->open();
    harness.layout();
    // 普通 Dialog 是第二个浮层。
    auto* layer = harness.host->layer_at(1);
    auto* root = layer->layout_root();
    REQUIRE(root->child_count() == 2);
    auto* dismiss = root->get_child(1)->as_control();
    auto* scope = dismiss->get_child(0)->as_control();
    auto* dialog_panel = scope->get_child(0)->as_control();
    REQUIRE(dialog_panel->semantics_properties().role == semantics::Role::dialog);
}

TEST_CASE("alert dialog cannot be dismissed by escape or an outside click", "[alert-dialog][interaction]") {
    AlertHarness harness;
    harness.open();
    REQUIRE(harness.alert->is_open());
    REQUIRE_FALSE(widget::AlertDialog::dismissible());

    harness.press_escape();
    harness.settle();
    REQUIRE(harness.alert->is_open());

    harness.click_outside();
    harness.settle();
    REQUIRE(harness.alert->is_open());

    // 程序化关闭仍然可用（应用自己决定要走哪条路时必须能收掉它）。
    harness.alert->close();
    harness.settle();
    REQUIRE_FALSE(harness.alert->is_open());
}

TEST_CASE("a plain dialog in the same setup does dismiss", "[alert-dialog][interaction]") {
    // 对照实验：同样的浮层、同样的 Escape，普通 Dialog 必须关掉。
    // 没有这条，"不可关闭"可能只是因为按钮/事件根本没接通，而不是语义约束生效。
    auto dialog = widget::Dialog::create();
    dialog->set_title("可关闭");
    int plain_closes = 0;
    dialog->set_on_close([&plain_closes] { ++plain_closes; });
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    auto body = std::make_shared<scene::NanControl>(foundation::NanSize(640.0F, 400.0F));
    body->add_child(dialog);
    body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    host->set_content(body);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(host);
    (void)tree.layout_root(foundation::NanSize(640.0F, 400.0F));
    dialog->open();
    (void)tree.layout_root(foundation::NanSize(640.0F, 400.0F));
    REQUIRE(dialog->is_open());

    tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    for (int frame = 0; frame < 4; ++frame) {
        tree.advance_animations(0.5F);
        tree.process(0.5F);
    }
    REQUIRE_FALSE(dialog->is_open());
    // on_close 只在淡出真正结束时触发 —— 这条同时守住"动画必须被推进"。
    REQUIRE(plain_closes == 1);
}

TEST_CASE("alert dialog actions fire their callbacks and close", "[alert-dialog][interaction]") {
    AlertHarness harness;
    harness.open();

    int confirms = 0;
    int cancels = 0;
    int closes = 0;
    harness.alert->set_on_confirm([&confirms] { ++confirms; });
    harness.alert->set_on_cancel([&cancels] { ++cancels; });
    harness.alert->set_on_close([&closes] { ++closes; });

    REQUIRE(harness.panel() != nullptr);
    REQUIRE(harness.alert->is_open());

    // 点确认：走的是真实的按钮命中路径，不是直接调用回调。
    auto* confirm = AlertHarness::find_button(harness.panel(), "确定");
    REQUIRE(confirm != nullptr);
    harness.click(*confirm);
    harness.settle();
    REQUIRE(confirms == 1);
    REQUIRE(cancels == 0);
    REQUIRE_FALSE(harness.alert->is_open());
    REQUIRE(closes == 1);

    // 再点一次取消（重新打开）：两条路各自只触发自己的回调。
    harness.alert->open();
    harness.layout();
    auto* cancel = AlertHarness::find_button(harness.panel(), "取消");
    REQUIRE(cancel != nullptr);
    harness.click(*cancel);
    harness.settle();
    REQUIRE(cancels == 1);
    REQUIRE(confirms == 1);
    REQUIRE(closes == 2);
}

TEST_CASE("alert dialog applies the confirm tone", "[alert-dialog][interaction]") {
    AlertHarness harness;
    harness.alert->set_confirm_tone(theme::ButtonTone::danger);
    REQUIRE(harness.alert->confirm_tone() == theme::ButtonTone::danger);
    harness.open();
    REQUIRE(harness.alert->is_open());
}

TEST_CASE("alert dialog mounts a portal while open and releases it on exit", "[alert-dialog][overlay]") {
    AlertHarness harness;
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);

    harness.alert->open();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);

    harness.alert->close();
    harness.settle();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);

    // 重新打开仍然可用；离开场景树必须收掉浮层。
    harness.alert->open();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);
    harness.body->remove_and_delete(*harness.alert);
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 0);
    REQUIRE_FALSE(harness.alert->is_open());
}

TEST_CASE("alert dialog reuses the Dialog recipe and honors overrides", "[alert-dialog][theme]") {
    const auto design = theme::default_design_system();
    const auto dialog = theme::resolve_dialog(design, theme::ColorAppearance::light);
    // 说明文本是审查后补上的字段：AlertDialog 的必需项必须有主题落点，
    // 否则它只能靠组件里写死颜色，主题作者想调也调不了。
    REQUIRE(dialog.description.color.alpha() > 0.0F);
    REQUIRE(dialog.description.font_size > 0.0F);

    AlertHarness harness;
    harness.layout();
    const auto initial = harness.alert->resolved_style();

    // AlertDialog 没有自己的配方，实例覆盖直接落在 DialogRecipeRule 上。
    harness.alert->set_override(
        theme::DialogRecipeRule {
            .title_color = theme::ThemeColor::token(theme::ColorToken::error),
        }
    );
    const auto overridden = harness.alert->resolved_style();
    REQUIRE_FALSE(overridden.title.color == initial.title.color);
    REQUIRE(overridden.panel.radius == Catch::Approx(initial.panel.radius));

    // 跟随系统切换后覆盖保留。
    harness.alert->on_theme_changed(harness.themes);
    REQUIRE_FALSE(harness.alert->resolved_style().title.color == initial.title.color);
}

TEST_CASE("alert dialog updates its title and description", "[alert-dialog][boundary]") {
    AlertHarness harness;
    harness.alert->set_title("新的标题");
    harness.alert->set_description("新的说明");
    REQUIRE(harness.alert->title() == "新的标题");
    REQUIRE(harness.alert->description() == "新的说明");
    harness.open();
    REQUIRE(harness.panel() != nullptr);
    REQUIRE(harness.panel()->semantics_properties().label == "新的标题");
}

TEST_CASE("alert dialog is reachable through BuildContext", "[alert-dialog][traits]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};

    auto alert = ui.make<widget::AlertDialog>(
                      "退出登录？",
                      "未保存的修改会丢失。"
    )
                     .build();
    REQUIRE(alert != nullptr);
    REQUIRE(alert->title() == "退出登录？");
    REQUIRE(alert->description() == "未保存的修改会丢失。");
    alert->open();
    REQUIRE(alert->is_open());
    alert->close();
}
