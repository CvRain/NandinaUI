//
// Toggle interaction, semantics, theme, and authoring tests.
//

#include <nandina/reactive/scope.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/checkbox.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/key_codes.hpp>
#include <nandina/widget/toggle.hpp>
#include <nandina/widget/toggle_group.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

using namespace nandina;

namespace
{
    [[nodiscard]] auto resolve_light(
        const theme::DesignSystem& design,
        const bool checked,
        const theme::ToggleVisualState state = theme::ToggleVisualState::normal
    ) -> theme::ResolvedToggleStyle {
        return theme::resolve_toggle(
            design,
            theme::ColorAppearance::light,
            theme::ButtonTone::primary,
            theme::ButtonTreatment::ghost,
            checked,
            state
        );
    }
} // namespace

TEST_CASE("toggle checked and unchecked styles are clearly different", "[toggle][theme]") {
    auto design = theme::default_design_system();
    design.tokens.spacing.lg = 21.0F;

    const auto off = resolve_light(design, /*checked=*/false);
    const auto on = resolve_light(design, /*checked=*/true);

    // 尺寸来自配方度量（ghost 未选中不覆盖 padding）。
    REQUIRE(off.metrics.height == Catch::Approx(36.0F));
    REQUIRE(off.metrics.padding_x == Catch::Approx(21.0F));

    // 未选中 ghost：透明容器 + muted_foreground 文本。
    REQUIRE(off.container.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(
        off.label.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );

    // 选中：tone 强调色实底 + on_primary 文本 —— 区分来自容器填充/边框，不只是文字色。
    REQUIRE(on.container.fill.alpha() == Catch::Approx(1.0F));
    REQUIRE(on.container.fill.oklch().light == Catch::Approx(design.light.primary.oklch().light));
    REQUIRE(on.container.border.oklch().light == Catch::Approx(design.light.primary.oklch().light));
    REQUIRE(
        on.label.color.oklch().light
        == Catch::Approx(design.light.primary_foreground.oklch().light)
    );
    REQUIRE(on.container.fill.oklch().light != Catch::Approx(off.container.fill.oklch().light));
    REQUIRE(on.label.color.oklch().light != Catch::Approx(off.label.color.oklch().light));
}

TEST_CASE("toggle hover and pressed feedback are distinct", "[toggle][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = resolve_light(design, false, theme::ToggleVisualState::normal);
    const auto hovered = resolve_light(design, false, theme::ToggleVisualState::hovered);
    const auto pressed = resolve_light(design, false, theme::ToggleVisualState::pressed);

    REQUIRE(normal.state_layer.hover.alpha() == Catch::Approx(0.0F));
    REQUIRE(normal.state_layer.pressed.alpha() == Catch::Approx(0.0F));
    REQUIRE(
        hovered.state_layer.hover.alpha()
        == Catch::Approx(design.tokens.opacity.hover_overlay)
    );
    REQUIRE(
        pressed.state_layer.pressed.alpha()
        == Catch::Approx(design.tokens.opacity.pressed_overlay)
    );
    // pressed 必须比 hover 更深一档（契约状态表）。
    REQUIRE(
        theme::toggle_state_layer_color(pressed, theme::ToggleVisualState::pressed).alpha()
        > theme::toggle_state_layer_color(hovered, theme::ToggleVisualState::hovered).alpha()
    );
    REQUIRE(
        theme::toggle_state_layer_color(normal, theme::ToggleVisualState::normal).alpha()
        == Catch::Approx(0.0F)
    );
}

TEST_CASE("toggle disabled state scales colors and drops the focus ring", "[toggle][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = resolve_light(design, true, theme::ToggleVisualState::normal);
    const auto disabled = resolve_light(design, true, theme::ToggleVisualState::disabled);

    const float factor = design.tokens.opacity.disabled;
    REQUIRE(
        disabled.container.fill.alpha()
        == Catch::Approx(normal.container.fill.alpha() * factor)
    );
    REQUIRE(disabled.label.color.alpha() == Catch::Approx(normal.label.color.alpha() * factor));
    REQUIRE(disabled.focus.color.alpha() == Catch::Approx(0.0F));
}

TEST_CASE("toggle resolves dark appearance from the dark palette", "[toggle][theme]") {
    const auto design = theme::default_design_system();
    const auto light = resolve_light(design, false);
    const auto dark = theme::resolve_toggle(
        design,
        theme::ColorAppearance::dark,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::ghost,
        false,
        theme::ToggleVisualState::normal
    );

    REQUIRE(
        dark.label.color.oklch().light
        == Catch::Approx(design.dark.muted_foreground.oklch().light)
    );
    REQUIRE(dark.label.color.oklch().light > light.label.color.oklch().light);
}

TEST_CASE("toggle measures like a button and starts unchecked", "[toggle][layout]") {
    auto toggle = widget::Toggle::create("Bold");
    // 构造时按配方度量落尺寸（未挂树 = 纯 on_measure 结果）。
    REQUIRE(toggle->height() == Catch::Approx(36.0F));
    REQUIRE(toggle->width() > 0.0F);

    scene::NanSceneTree tree;
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);

    REQUIRE_FALSE(toggle->checked());
    REQUIRE(toggle->text() == "Bold");
    REQUIRE(toggle->tone() == theme::ButtonTone::primary);
    REQUIRE(toggle->treatment() == theme::ButtonTreatment::ghost);
    REQUIRE(toggle->width() > 0.0F);
    REQUIRE(toggle->width() <= 240.0F);
}

TEST_CASE("toggle toggles with space and enter", "[toggle][input]") {
    auto toggle = std::make_shared<widget::Toggle>("Keyboard option");
    scene::NanSceneTree tree;
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    tree.set_focus(toggle.get());
    REQUIRE(tree.focused_node() == toggle.get());

    tree.dispatch_key(scene::KeyEvent(widget::keys::space, scene::KeyEvent::Action::press));
    REQUIRE(toggle->checked());

    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(toggle->checked());
}

TEST_CASE("toggle activates on pointer release inside its bounds", "[toggle][input]") {
    auto toggle = std::make_shared<widget::Toggle>("Mute");
    scene::NanSceneTree tree;
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);
    const auto center = toggle->global_bounds().get_center();

    tree.dispatch_mouse_move(scene::MouseMoveEvent(center, foundation::NanPoint {}));
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            center
        )
    );
    // 按下不切换，抬起（仍在控件内）才切换。
    REQUIRE_FALSE(toggle->checked());
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::release,
            center
        )
    );
    REQUIRE(toggle->checked());

    // 在控件外释放不计为点击。
    const auto outside =
        foundation::NanPoint(center.get_x() + 400.0F, center.get_y());
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            center
        )
    );
    tree.dispatch_mouse_move(scene::MouseMoveEvent(outside, foundation::NanPoint {}));
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::release,
            outside
        )
    );
    REQUIRE(toggle->checked());
}

TEST_CASE("disabled toggle is not focusable and emits nothing", "[toggle][input]") {
    auto toggle = std::make_shared<widget::Toggle>("Bold");
    scene::NanSceneTree tree;
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);

    int changes = 0;
    auto subscription = toggle->checked_changed().subscribe([&](bool) { ++changes; });

    tree.set_focus(toggle.get());
    REQUIRE(tree.focused_node() == toggle.get());

    toggle->set_disabled(true);
    // Checkbox / Pressable 同款：禁用时立刻清空焦点。
    REQUIRE(tree.focused_node() == nullptr);
    tree.dispatch_key(scene::KeyEvent(widget::keys::space, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(toggle->checked());
    REQUIRE(changes == 0);

    // 不能再获得焦点。
    tree.set_focus(toggle.get());
    REQUIRE(tree.focused_node() == nullptr);

    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(toggle->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.state.disabled);
    REQUIRE_FALSE(node->properties.state.focusable);
    REQUIRE(node->properties.actions == semantics::Action::none);
}

TEST_CASE("toggle fires change notifications once per change", "[toggle][events]") {
    auto toggle = std::make_shared<widget::Toggle>("Italic");
    int callbacks = 0;
    bool last = false;
    std::vector<bool> observed;
    toggle->set_on_change([&](const bool value) {
        ++callbacks;
        last = value;
    });
    auto subscription =
        toggle->checked_changed().subscribe([&](const bool value) { observed.push_back(value); });

    scene::NanSceneTree tree;
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    tree.set_focus(toggle.get());

    tree.dispatch_key(scene::KeyEvent(widget::keys::space, scene::KeyEvent::Action::press));
    REQUIRE(toggle->checked());
    REQUIRE(callbacks == 1);
    REQUIRE(observed.size() == 1);
    REQUIRE(last);
    REQUIRE(observed.front());

    // 程序化 setter 幂等且不冒充用户输入。
    toggle->set_checked(true);
    REQUIRE(callbacks == 1);
    REQUIRE(observed.size() == 1);

    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE_FALSE(toggle->checked());
    REQUIRE(callbacks == 2);
    REQUIRE(observed.size() == 2);
    REQUIRE_FALSE(last);
    REQUIRE_FALSE(observed.back());
}

TEST_CASE("toggle exposes checkbox semantics with checked state", "[toggle][semantics]") {
    auto toggle = std::make_shared<widget::Toggle>("Bold");
    scene::NanSceneTree tree;
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* before = tree.semantics_tree().find(toggle->semantics_id());
    REQUIRE(before != nullptr);
    // Role 枚举没有 toggle：两态按钮按 checkbox 暴露 checked + activate。
    REQUIRE(before->properties.role == semantics::Role::checkbox);
    REQUIRE(before->properties.label == "Bold");
    REQUIRE(before->properties.state.checked == false);
    REQUIRE(before->properties.state.focusable);
    REQUIRE(semantics::supports(before->properties.actions, semantics::Action::activate));
    REQUIRE(semantics::supports(before->properties.actions, semantics::Action::focus));

    REQUIRE(tree.perform_semantics_action(
        toggle->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(toggle->checked());
    REQUIRE(tree.update_semantics());
    const auto* after = tree.semantics_tree().find(toggle->semantics_id());
    REQUIRE(after != nullptr);
    REQUIRE(after->properties.state.checked == true);
}

TEST_CASE("toggle override survives a system apply", "[toggle][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto toggle = widget::Toggle::create("Bold");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);

    toggle->set_override(theme::ToggleRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::error),
    });
    REQUIRE(
        toggle->resolved_style().container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );

    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(toggle->resolved_style().container.fill.oklch().light == Catch::Approx(0.60F));
}

TEST_CASE("toggle widget re-resolves when the appearance switches", "[toggle][theme]") {
    theme::ThemeManager themes;
    auto toggle = widget::Toggle::create("Bold");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(toggle);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);

    const auto light = toggle->resolved_style();
    themes.set_system_appearance(theme::ColorAppearance::dark);
    const auto dark = toggle->resolved_style();

    REQUIRE(
        dark.label.color.oklch().light
        == Catch::Approx(themes.design_system().dark.muted_foreground.oklch().light)
    );
    REQUIRE(dark.label.color.oklch().light != Catch::Approx(light.label.color.oklch().light));
}

TEST_CASE("BuildContext toggle instantiates ComponentTraits and a signal binding", "[toggle][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    auto toggle = ui.make<widget::Toggle>("Bold").build();
    REQUIRE(toggle->text() == "Bold");
    REQUIRE_FALSE(toggle->checked());

    auto& pressed = ui.signal<bool>(false);
    auto synced = ui.make<widget::Toggle>(pressed, "Italic").build();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(synced);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.perform_semantics_action(
        synced->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(pressed.peek());

    pressed.set(false);
    REQUIRE_FALSE(synced->checked());
}

TEST_CASE("toggle builder forwards checked tone and treatment", "[toggle][authoring]") {
    auto toggle = widget::authoring::make<widget::Toggle>("Bold")
                      .checked(true)
                      .tone(theme::ButtonTone::danger)
                      .treatment(theme::ButtonTreatment::outlined)
                      .build();

    REQUIRE(toggle->checked());
    REQUIRE(toggle->tone() == theme::ButtonTone::danger);
    REQUIRE(toggle->treatment() == theme::ButtonTreatment::outlined);
    REQUIRE(
        toggle->resolved_style().container.fill.oklch().light
        == Catch::Approx(theme::default_design_system().light.error.oklch().light)
    );
}
