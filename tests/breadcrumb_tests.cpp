//
// Theme / Breadcrumb tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/breadcrumb.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/key_codes.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

using namespace nandina;

TEST_CASE("breadcrumb resolves link, current and separator tokens", "[breadcrumb][theme]") {
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_breadcrumb(
        design,
        theme::ColorAppearance::light,
        theme::BreadcrumbVisualState::normal
    );

    REQUIRE(style.container.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(
        style.link.color.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(
        style.link_hover.oklch().light == Catch::Approx(design.light.foreground.oklch().light)
    );
    REQUIRE(
        style.current.color.oklch().light
        == Catch::Approx(design.light.foreground.oklch().light)
    );
    REQUIRE(
        style.separator.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(style.link.font_size == Catch::Approx(design.tokens.typography.label_sm));
    // 链接条目始终可聚焦，焦点环默认开启（不能靠主题作者补）。
    REQUIRE(style.link_focus.width == Catch::Approx(design.tokens.border.focus_ring));
    REQUIRE(style.metrics.gap == Catch::Approx(design.tokens.spacing.xs));
}

TEST_CASE("breadcrumb tracks items, clickability and the separator", "[breadcrumb][value]") {
    auto crumb = widget::Breadcrumb::create();
    REQUIRE(crumb->item_count() == 0);
    REQUIRE(crumb->separator() == "/");

    crumb->add_item("Home", [] {});
    crumb->add_item("Library");
    crumb->add_item("Data", [] {});
    REQUIRE(crumb->item_count() == 3);
    REQUIRE(crumb->item_label(0) == "Home");
    REQUIRE(crumb->item_label(2) == "Data");
    REQUIRE(crumb->item_clickable(0));
    REQUIRE_FALSE(crumb->item_clickable(1));
    REQUIRE(crumb->item_clickable(2));
    // 只有带回调的条目才是内部链接子控件。
    REQUIRE(crumb->child_count() == 2);

    crumb->set_separator(">");
    REQUIRE(crumb->separator() == ">");

    crumb->clear();
    REQUIRE(crumb->item_count() == 0);
    REQUIRE(crumb->child_count() == 0);
}

TEST_CASE("breadcrumb measures its trail and lays out link children", "[breadcrumb][layout]") {
    auto crumb = widget::Breadcrumb::create();
    const auto empty = crumb->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(empty.get_width() == Catch::Approx(0.0F));

    crumb->add_item("Home", [] {});
    const float one = crumb->measure_layout(scene::LayoutConstraints::loose()).get_width();
    REQUIRE(one > 0.0F);

    crumb->add_item("Library");
    const float two = crumb->measure_layout(scene::LayoutConstraints::loose()).get_width();
    REQUIRE(two > one);

    crumb->add_item("Data", [] {});
    const auto measured = crumb->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(measured.get_width() > two);
    REQUIRE(measured.get_height() > 0.0F);

    // 更长的分隔符会改变整条宽度。
    crumb->set_separator("-->");
    REQUIRE(
        crumb->measure_layout(scene::LayoutConstraints::loose()).get_width()
        > measured.get_width()
    );

    scene::NanSceneTree tree;
    tree.set_root(crumb);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 48.0F)) >= 1);

    auto* first_link = crumb->get_child(0)->as_control();
    auto* second_link = crumb->get_child(1)->as_control();
    REQUIRE(first_link != nullptr);
    REQUIRE(second_link != nullptr);
    REQUIRE(first_link->position().get_x() >= 0.0F);
    REQUIRE(second_link->position().get_x() > first_link->position().get_x());
}

TEST_CASE("breadcrumb links are Tab reachable and activate with Enter and Space", "[breadcrumb][keyboard]") {
    int clicks = 0;
    auto crumb = widget::Breadcrumb::create();
    crumb->add_item("Home", [&clicks] { ++clicks; });
    crumb->add_item("Current");
    crumb->add_item("Data", [&clicks] { ++clicks; });

    scene::NanSceneTree tree;
    tree.set_root(crumb);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 48.0F)) >= 1);

    // Tab 焦点遍历能找到内部链接（无回调条目不是节点，不参与焦点）。
    tree.focus_next();
    auto* first_link = crumb->get_child(0);
    REQUIRE(tree.focused_node() == first_link->as_node2d());
    tree.focus_next();
    REQUIRE(tree.focused_node() == crumb->get_child(1)->as_node2d());

    tree.set_focus(crumb->get_child(0)->as_control());
    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE(clicks == 1);
    tree.dispatch_key(scene::KeyEvent(widget::keys::space, scene::KeyEvent::Action::press));
    REQUIRE(clicks == 2);
}

TEST_CASE("breadcrumb links activate on pointer click", "[breadcrumb][pointer]") {
    int clicks = 0;
    auto crumb = widget::Breadcrumb::create();
    crumb->add_item("Home", [&clicks] { ++clicks; });
    crumb->add_item("Current");

    scene::NanSceneTree tree;
    tree.set_root(crumb);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 48.0F)) >= 1);

    auto* link = crumb->get_child(0)->as_control();
    REQUIRE(link != nullptr);
    const auto center = foundation::NanPoint(
        link->position().get_x() + link->width() * 0.5F,
        link->position().get_y() + link->height() * 0.5F
    );
    tree.dispatch_mouse_move(scene::MouseMoveEvent(center, foundation::NanPoint(0.0F, 0.0F)));
    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        center
    ));
    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        center
    ));
    REQUIRE(clicks == 1);
}

TEST_CASE("breadcrumb override patches fields and survives an appearance switch", "[breadcrumb][override]") {
    theme::ThemeManager themes;
    auto crumb = widget::Breadcrumb::create();
    crumb->add_item("Home", [] {});
    crumb->add_item("Current");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(crumb);
    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 48.0F)) >= 1);

    crumb->set_override(theme::BreadcrumbRecipeRule {
        .current_color = theme::ThemeColor::token(theme::ColorToken::foreground),
        .separator_color = theme::ThemeColor::token(theme::ColorToken::error),
        .metrics_gap = theme::ThemeScalar::literal(12.0F),
    });
    const auto overridden = crumb->resolved_style();
    REQUIRE(overridden.metrics.gap == Catch::Approx(12.0F));
    REQUIRE(
        overridden.separator.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );

    const float light = overridden.current.color.oklch().light;
    themes.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(
        crumb->resolved_style().current.color.oklch().light
        == Catch::Approx(themes.design_system().dark.foreground.oklch().light)
    );
    REQUIRE(crumb->resolved_style().current.color.oklch().light != Catch::Approx(light));
    REQUIRE(crumb->resolved_style().metrics.gap == Catch::Approx(12.0F));
}

TEST_CASE("breadcrumb exposes the trail as generic and links as buttons", "[breadcrumb][semantics]") {
    auto crumb = widget::Breadcrumb::create();
    crumb->add_item("Home", [] {});
    crumb->add_item("Library");
    crumb->add_item("Data", [] {});

    scene::NanSceneTree tree;
    tree.set_root(crumb);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* container = tree.semantics_tree().find(crumb->semantics_id());
    REQUIRE(container != nullptr);
    // 平台语义树没有 nav / breadcrumb role：容器用 generic，label 是整条路径。
    REQUIRE(container->properties.role == semantics::Role::generic);
    REQUIRE(container->properties.label == "Home / Library / Data");

    auto* first_link = crumb->get_child(0)->as_control();
    const auto* link_node = tree.semantics_tree().find(first_link->semantics_id());
    REQUIRE(link_node != nullptr);
    REQUIRE(link_node->properties.role == semantics::Role::button);
    REQUIRE(link_node->properties.label == "Home");

    auto* second_link = crumb->get_child(1)->as_control();
    const auto* second_node = tree.semantics_tree().find(second_link->semantics_id());
    REQUIRE(second_node != nullptr);
    REQUIRE(second_node->properties.label == "Data");
}

TEST_CASE("breadcrumb is buildable through BuildContext", "[breadcrumb][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    auto crumb = ui.make<widget::Breadcrumb>().build();
    REQUIRE(crumb != nullptr);
    crumb->add_item("Home", [] {});
    crumb->add_item("Current");
    REQUIRE(crumb->item_count() == 2);
}
