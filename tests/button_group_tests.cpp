//
// Theme / ButtonGroup tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/button_group.hpp>
#include <nandina/widget/controls.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>

using namespace nandina;

namespace
{
    constexpr float kTolerance = 0.01F;
} // namespace

TEST_CASE("button group resolves a transparent container and spacing tokens", "[button-group][theme]") {
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_button_group(
        design,
        theme::ColorAppearance::light,
        theme::ButtonGroupVisualState::normal
    );

    REQUIRE(style.container.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(style.container.border.alpha() == Catch::Approx(0.0F));
    REQUIRE(style.container.radius == Catch::Approx(design.tokens.radius.md));
    REQUIRE(style.metrics.gap == Catch::Approx(design.tokens.spacing.sm));
    REQUIRE(style.metrics.padding_x == Catch::Approx(0.0F));
    REQUIRE(style.metrics.min_height == Catch::Approx(0.0F));
}

TEST_CASE("button group measures children on the main axis with one shared gap", "[button-group][layout]") {
    auto group = widget::ButtonGroup::create();
    auto first = widget::Button::create("A");
    auto second = widget::Button::create("BB");
    group->add_button(first);
    group->add_button(second);

    REQUIRE(group->button_count() == 2);
    REQUIRE(group->orientation() == widget::LayoutAxis::horizontal);

    const float first_width =
        first->measure_layout(scene::LayoutConstraints::loose()).get_width();
    const float second_width =
        second->measure_layout(scene::LayoutConstraints::loose()).get_width();
    const float first_height =
        first->measure_layout(scene::LayoutConstraints::loose()).get_height();

    const float gap = theme::default_theme().tokens.spacing.sm;
    const auto measured = group->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(
        measured.get_width()
        == Catch::Approx(first_width + second_width + gap).margin(kTolerance)
    );
    REQUIRE(measured.get_height() == Catch::Approx(first_height).margin(kTolerance));
}

TEST_CASE("button group lays children out on the shared rhythm", "[button-group][layout]") {
    auto group = widget::ButtonGroup::create();
    auto first = widget::Button::create("A");
    auto second = widget::Button::create("BB");
    group->add_button(first);
    group->add_button(second);

    scene::NanSceneTree tree;
    tree.set_root(group);
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 100.0F)) >= 1);

    const float gap = theme::default_theme().tokens.spacing.sm;
    REQUIRE(
        second->position().get_x()
        == Catch::Approx(first->position().get_x() + first->width() + gap).margin(kTolerance)
    );
    REQUIRE(second->position().get_y() == Catch::Approx(first->position().get_y()));
}

TEST_CASE("button group vertical orientation stacks and a gap override wins", "[button-group][layout]") {
    auto group = widget::ButtonGroup::create();
    auto first = widget::Button::create("A");
    auto second = widget::Button::create("BB");
    group->add_button(first);
    group->add_button(second);

    group->set_orientation(widget::LayoutAxis::vertical);
    REQUIRE(group->orientation() == widget::LayoutAxis::vertical);

    group->set_gap(20.0F);
    REQUIRE(group->gap() == Catch::Approx(20.0F));

    scene::NanSceneTree tree;
    tree.set_root(group);
    REQUIRE(tree.layout_root(foundation::NanSize(400.0F, 200.0F)) >= 1);
    REQUIRE(
        second->position().get_y()
        == Catch::Approx(first->position().get_y() + first->height() + 20.0F).margin(kTolerance)
    );
    REQUIRE(second->position().get_x() == Catch::Approx(first->position().get_x()));

    // 清除显式间距后回到配方 gap。
    group->clear_gap();
    REQUIRE(group->gap() == Catch::Approx(theme::default_theme().tokens.spacing.sm));
}

TEST_CASE("button group rejects null children and clear removes them", "[button-group][boundary]") {
    auto group = widget::ButtonGroup::create();
    REQUIRE_THROWS_AS(group->add_button(nullptr), std::runtime_error);

    group->add_button(widget::Button::create("A"));
    group->add_button(widget::Button::create("B"));
    REQUIRE(group->button_count() == 2);

    group->clear();
    REQUIRE(group->button_count() == 0);
    REQUIRE(group->measure_layout(scene::LayoutConstraints::loose()).get_width() == Catch::Approx(0.0F));
}

TEST_CASE("button group override patches fields and survives a system apply", "[button-group][override]") {
    theme::ThemeManager themes;
    auto group = widget::ButtonGroup::create();
    group->add_button(widget::Button::create("A"));
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(group);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 80.0F)) >= 1);

    group->set_override(theme::ButtonGroupRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::error),
        .metrics_gap = theme::ThemeScalar::literal(24.0F),
    });
    REQUIRE(
        group->resolved_style().container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );
    REQUIRE(group->gap() == Catch::Approx(24.0F));

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(group->resolved_style().container.fill.oklch().light == Catch::Approx(0.60F));
    REQUIRE(group->gap() == Catch::Approx(24.0F));
}

TEST_CASE("button group re-resolves after an appearance switch", "[button-group][theme]") {
    theme::ThemeManager themes;
    auto group = widget::ButtonGroup::create();
    group->add_button(widget::Button::create("A"));
    group->set_override(theme::ButtonGroupRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::background),
    });
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(group);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 80.0F)) >= 1);

    const float light = group->resolved_style().container.fill.oklch().light;
    themes.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);
    REQUIRE(
        group->resolved_style().container.fill.oklch().light
        == Catch::Approx(themes.design_system().dark.background.oklch().light)
    );
    REQUIRE(group->resolved_style().container.fill.oklch().light != Catch::Approx(light));
}

TEST_CASE("button group stays a plain container without inventing a role", "[button-group][semantics]") {
    auto group = widget::ButtonGroup::create();
    auto button = widget::Button::create("Run");
    group->add_button(button);

    // 容器不覆写语义：role 保持 none，场景树会把它折叠、直接暴露子按钮。
    REQUIRE(group->resolved_semantics_properties().role == semantics::Role::none);

    scene::NanSceneTree tree;
    tree.set_root(group);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 80.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.semantics_tree().find(group->semantics_id()) == nullptr);
    const auto* node = tree.semantics_tree().find(button->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::button);
    REQUIRE(node->properties.label == "Run");
}

TEST_CASE("button group is buildable through BuildContext", "[button-group][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    auto horizontal = ui.make<widget::ButtonGroup>().build();
    REQUIRE(horizontal != nullptr);
    REQUIRE(horizontal->orientation() == widget::LayoutAxis::horizontal);

    auto vertical = ui.make<widget::ButtonGroup>(widget::LayoutAxis::vertical).build();
    REQUIRE(vertical != nullptr);
    REQUIRE(vertical->orientation() == widget::LayoutAxis::vertical);
}
