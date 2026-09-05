//
// Theme / RadioButton tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/controls.hpp"
#include "widget/layout.hpp"
#include "widget/radio_button.hpp"
#include "widget/radio_group.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <string>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int circles = 0;
        int rounded_rect_outlines = 0;
        int text_calls = 0;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_rounded_rect_outline(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_rect_outlines;
        }
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++circles;
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
} // namespace

TEST_CASE("radio button resolves indicator and dot tokens", "[radio][theme]") {
    auto design = theme::default_design_system();
    design.tokens.radius.full = 9999.0F;
    design.tokens.spacing.sm = 9.0F;

    const auto unchecked = theme::resolve_radio_button(
        design,
        theme::ColorAppearance::light,
        /*checked=*/false,
        theme::RadioButtonVisualState::normal
    );
    const auto checked = theme::resolve_radio_button(
        design,
        theme::ColorAppearance::light,
        /*checked=*/true,
        theme::RadioButtonVisualState::normal
    );

    // 未选中：透明填充 + outline 边框；选中：primary 边框（规则覆盖）。
    REQUIRE(unchecked.indicator.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(
        unchecked.indicator.border.oklch().light
        == Catch::Approx(design.light.outline.oklch().light)
    );
    REQUIRE(
        checked.indicator.border.oklch().light
        == Catch::Approx(design.light.primary.oklch().light)
    );
    // 圆形指示器 + primary 内点。
    REQUIRE(unchecked.indicator.radius == Catch::Approx(9999.0F));
    REQUIRE(
        unchecked.dot.oklch().light == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(unchecked.metrics.box_size == Catch::Approx(20.0F));
    REQUIRE(unchecked.metrics.gap == Catch::Approx(9.0F));
}

TEST_CASE("radio button resolves light and dark surfaces", "[radio][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_radio_button(
        design,
        theme::ColorAppearance::light,
        false,
        theme::RadioButtonVisualState::normal
    );
    const auto dark = theme::resolve_radio_button(
        design,
        theme::ColorAppearance::dark,
        false,
        theme::RadioButtonVisualState::normal
    );

    REQUIRE(
        light.label.color.oklch().light == Catch::Approx(design.light.on_surface.oklch().light)
    );
    REQUIRE(
        dark.label.color.oklch().light == Catch::Approx(design.dark.on_surface.oklch().light)
    );
    // 文本在暗色下翻转为浅色（on_surface 从暗变亮）。
    REQUIRE(dark.label.color.oklch().light > light.label.color.oklch().light);
}

TEST_CASE("radio button disabled state scales colors", "[radio][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = theme::resolve_radio_button(
        design,
        theme::ColorAppearance::light,
        true,
        theme::RadioButtonVisualState::normal
    );
    const auto disabled = theme::resolve_radio_button(
        design,
        theme::ColorAppearance::light,
        true,
        theme::RadioButtonVisualState::disabled
    );

    const float expected = design.tokens.opacity.disabled;
    REQUIRE(disabled.indicator.border.alpha() == Catch::Approx(normal.indicator.border.alpha() * expected));
    REQUIRE(disabled.dot.alpha() == Catch::Approx(normal.dot.alpha() * expected));
}

TEST_CASE("radio button override survives a system apply", "[radio][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto radio = widget::RadioButton::create("Option");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(radio);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);

    radio->set_override(theme::RadioButtonRecipeRule {
        .dot_color = theme::ThemeColor::token(theme::ColorToken::error),
    });
    REQUIRE(
        radio->resolved_style().dot.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );

    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(radio->resolved_style().dot.oklch().light == Catch::Approx(0.60F));
}

TEST_CASE("radio group enforces mutual exclusion", "[radio][group]") {
    auto group = widget::RadioGroup::create();
    auto a = widget::RadioButton::create("A", group);
    auto b = widget::RadioButton::create("B", group);
    auto c = widget::RadioButton::create("C", group);

    int last = -1;
    auto subscription = group->selection_changed().subscribe([&](const int index) { last = index; });

    b->select();
    REQUIRE(b->checked());
    REQUIRE_FALSE(a->checked());
    REQUIRE_FALSE(c->checked());
    REQUIRE(group->selected() == b.get());
    REQUIRE(group->selected_index() == 1);
    REQUIRE(last == 1);

    c->select();
    REQUIRE(c->checked());
    REQUIRE_FALSE(b->checked());
    REQUIRE(group->selected_index() == 2);

    // 重复选择同一项是 no-op。
    c->select();
    REQUIRE(group->selected_index() == 2);
    REQUIRE(c->checked());
}

TEST_CASE("radio group arrow keys move focus and selection", "[radio][keyboard]") {
    auto group = widget::RadioGroup::create();
    auto a = widget::RadioButton::create("A", group);
    auto b = widget::RadioButton::create("B", group);
    auto c = widget::RadioButton::create("C", group);
    auto column = widget::Column::create();
    column->add(a).add(b).add(c);

    scene::NanSceneTree tree;
    tree.set_root(column);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);
    tree.set_focus(b.get());
    REQUIRE(tree.focused_node() == b.get());

    // 右箭头：选中下一个并移动焦点。
    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(c->checked());
    REQUIRE(tree.focused_node() == c.get());

    // 末尾循环到首项。
    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(a->checked());
    REQUIRE(tree.focused_node() == a.get());

    // 左箭头回到末尾。
    tree.dispatch_key(scene::KeyEvent(263, scene::KeyEvent::Action::press));
    REQUIRE(tree.flush_post_layout_actions());
    REQUIRE(c->checked());
    REQUIRE(tree.focused_node() == c.get());
}

TEST_CASE("radio button exposes radio semantics with checked state", "[radio][semantics]") {
    auto radio = widget::RadioButton::create("System");
    scene::NanSceneTree tree;
    tree.set_root(radio);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* before = tree.semantics_tree().find(radio->semantics_id());
    REQUIRE(before != nullptr);
    REQUIRE(before->properties.role == semantics::Role::radio);
    REQUIRE(before->properties.label == "System");
    REQUIRE(before->properties.state.checked == false);

    REQUIRE(tree.perform_semantics_action(
        radio->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(radio->checked());
    REQUIRE(tree.update_semantics());
    const auto* after = tree.semantics_tree().find(radio->semantics_id());
    REQUIRE(after != nullptr);
    REQUIRE(after->properties.state.checked == true);
}

TEST_CASE("radio button paints a dot only when checked", "[radio][paint]") {
    auto unchecked = widget::RadioButton::create("Off");
    scene::NanSceneTree tree;
    tree.set_root(unchecked);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.rounded_rect_outlines == 1);
    REQUIRE(dev.circles == 0);
    REQUIRE(dev.text_calls == 1);

    auto checked = widget::RadioButton::create("On");
    checked->select();
    scene::NanSceneTree tree2;
    tree2.set_root(checked);
    REQUIRE(tree2.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    RecordingDevice dev2;
    tree2.draw(dev2);
    REQUIRE(dev2.rounded_rect_outlines == 1);
    REQUIRE(dev2.circles == 1);
}

TEST_CASE("BuildContext radio button constructs a grouped radio", "[radio][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto group = widget::RadioGroup::create();
    auto radio = ui.make<widget::RadioButton>("Light", group).build();

    REQUIRE(radio->group() == group);
    REQUIRE(radio->label() == "Light");
    REQUIRE_FALSE(radio->checked());

    radio->select();
    REQUIRE(radio->checked());
    REQUIRE(group->selected() == radio.get());
}
