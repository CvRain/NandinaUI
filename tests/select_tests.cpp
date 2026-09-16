//
// Theme / Select tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/select.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;
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
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_rects;
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}
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

TEST_CASE("select resolves field, popup, and option tokens", "[select][theme]") {
    auto design = theme::default_design_system();
    const auto style = theme::resolve_select(
        design,
        theme::ColorAppearance::light,
        theme::SelectVisualState::normal
    );

    REQUIRE(
        style.container.fill.oklch().light
        == Catch::Approx(design.light.surface_variant.oklch().light)
    );
    REQUIRE(
        style.popup.fill.oklch().light == Catch::Approx(design.light.surface.oklch().light)
    );
    REQUIRE(
        style.value.color.oklch().light == Catch::Approx(design.light.on_surface.oklch().light)
    );
    REQUIRE(
        style.option_selected.color.oklch().light
        == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(style.metrics.height == Catch::Approx(40.0F));
    REQUIRE(style.metrics.preferred_width == Catch::Approx(160.0F));
}

TEST_CASE("select disabled state scales field colors", "[select][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = theme::resolve_select(
        design,
        theme::ColorAppearance::light,
        theme::SelectVisualState::normal
    );
    const auto disabled = theme::resolve_select(
        design,
        theme::ColorAppearance::light,
        theme::SelectVisualState::disabled
    );

    const float expected = design.tokens.opacity.disabled;
    REQUIRE(disabled.container.fill.alpha() == Catch::Approx(normal.container.fill.alpha() * expected));
    REQUIRE(disabled.value.color.alpha() == Catch::Approx(normal.value.color.alpha() * expected));
}

TEST_CASE("select selects by index, clamps, and closes on select", "[select][value]") {
    auto select = widget::Select::create({"A", "B", "C"});
    REQUIRE(select->selected_index() == 0);
    REQUIRE(select->selected_label() == "A");

    select->open();
    REQUIRE(select->is_open());
    select->select(2);
    REQUIRE(select->selected_index() == 2);
    REQUIRE(select->selected_label() == "C");
    REQUIRE_FALSE(select->is_open());

    select->set_selected_index(99);
    REQUIRE(select->selected_index() == 2);
    select->set_selected_index(-1);
    REQUIRE(select->selected_index() == 0);
}

TEST_CASE("select keyboard opens, navigates, selects, and escapes", "[select][keyboard]") {
    auto select = widget::Select::create({"A", "B", "C"});
    scene::NanSceneTree tree;
    tree.set_root(select);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 48.0F)) >= 1);
    tree.set_focus(select.get());

    tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press)); // space 打开
    REQUIRE(select->is_open());

    tree.dispatch_key(scene::KeyEvent(264, scene::KeyEvent::Action::press)); // down
    REQUIRE(select->selected_index() == 1);

    tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press)); // enter 选中
    REQUIRE(select->selected_index() == 1);
    REQUIRE_FALSE(select->is_open());

    tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press)); // 再打开
    REQUIRE(select->is_open());
    tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press)); // escape
    REQUIRE_FALSE(select->is_open());
}

TEST_CASE("select exposes combobox semantics", "[select][semantics]") {
    auto select = widget::Select::create({"General", "Appearance"});
    scene::NanSceneTree tree;
    tree.set_root(select);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(select->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::combobox);
    REQUIRE(node->properties.label == "General");

    select->set_selected_index(1);
    REQUIRE(tree.update_semantics());
    const auto* updated = tree.semantics_tree().find(select->semantics_id());
    REQUIRE(updated->properties.label == "Appearance");
}

TEST_CASE("select paints popup only when open", "[select][paint]") {
    auto select = widget::Select::create({"A", "B"});
    scene::NanSceneTree tree;
    tree.set_root(select);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 200.0F)) >= 1);

    RecordingDevice closed_dev;
    tree.draw(closed_dev);
    // 关闭：字段 + 箭头（无 popup）。字段填充 1 + 值文本 1。
    REQUIRE(closed_dev.rounded_rects == 1);
    REQUIRE(closed_dev.text_calls == 1);

    select->open();
    RecordingDevice open_dev;
    tree.draw(open_dev);
    // 打开：字段 + popup（2 填充）+ 值文本 + 2 选项文本。
    REQUIRE(open_dev.rounded_rects == 2);
    REQUIRE(open_dev.text_calls == 3);
}

TEST_CASE("select extends hit area to the popup when open", "[select][hit-test]") {
    auto select = widget::Select::create({"A", "B"});
    // 触发器高度 40、弹窗区 44..108（gap 4 + 两行 32）。
    const auto below_trigger = foundation::NanPoint(10.0F, 50.0F);
    REQUIRE_FALSE(select->contains_point(below_trigger)); // 关闭：只命中触发器

    select->open();
    REQUIRE(select->contains_point(below_trigger)); // 打开：弹窗区域可命中
}

TEST_CASE("BuildContext select synchronizes a selected-index signal", "[select][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto& selected = ui.signal<int>(0);
    auto select = ui.make<widget::Select>(selected, std::vector<std::string> {"A", "B", "C"}).build();

    REQUIRE(select->selected_index() == 0);
    select->select(2);
    REQUIRE(selected.peek() == 2);

    selected.set(1);
    REQUIRE(select->selected_index() == 1);
}

TEST_CASE("mounted select presents its popup through the overlay host", "[select][overlay]") {
    auto host = scene::OverlayHost::create();
    auto select = widget::Select::create({"A", "B", "C"});
    host->set_content(select);
    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    select->open();
    REQUIRE(host->overlay_count() == 1);
    auto* surface = host->layer_at(1)->layout_root();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 1);
    auto* dismiss = surface->get_child(0)->as_control();
    REQUIRE(dismiss != nullptr);
    REQUIRE(dismiss->child_count() == 1);
    auto* popup = dismiss->get_child(0)->as_control();
    REQUIRE(popup != nullptr);
    const auto second_option = foundation::NanPoint(
        popup->global_bounds().get_left() + 10.0F,
        popup->global_bounds().get_top() + 48.0F
    );
    REQUIRE(tree.hit_test(second_option) == popup);

    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        second_option,
    });
    REQUIRE(select->selected_index() == 1);
    REQUIRE_FALSE(select->is_open());
    REQUIRE(host->overlay_count() == 0);
}

TEST_CASE("mounted select dismisses when clicking outside its popup", "[select][overlay]") {
    auto host = scene::OverlayHost::create();
    auto select = widget::Select::create({"A", "B"});
    host->set_content(select);
    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    select->open();
    REQUIRE(host->overlay_count() == 1);
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(220.0F, 140.0F),
    });
    REQUIRE_FALSE(select->is_open());
    REQUIRE(host->overlay_count() == 0);
}

TEST_CASE("BuildContext injects the window overlay service into select", "[select][overlay]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<scene::NanControl>());
    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 160.0F)) >= 1);

    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};
    auto select = ui.make<widget::Select>(std::vector<std::string> {"A", "B"}).build();
    select->open();

    REQUIRE(select->is_open());
    REQUIRE(host->overlay_count() == 1);
    select->close();
    REQUIRE(host->overlay_count() == 0);
}

TEST_CASE("moving a select reuses its popup instead of rebuilding it", "[select][overlay]") {
    auto host = scene::OverlayHost::create();
    auto select = widget::Select::create({"A", "B", "C"});
    // Keep the select at its natural size inside a container: a sole content root
    // would be stretched to the whole viewport and mask the anchor movement.
    auto content = std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 240.0F));
    select->set_position(foundation::NanPoint(20.0F, 20.0F));
    content->add_child(select);
    content->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    host->set_content(content);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 240.0F)) >= 1);

    select->open();
    auto* dismiss = host->layer_at(1)->layout_root()->get_child(0)->as_control();
    REQUIRE(dismiss != nullptr);
    auto* before = dismiss->get_child(0)->as_control();
    REQUIRE(before != nullptr);
    const auto before_top = before->global_bounds().get_top();

    // The anchor moves (scroll / re-layout); the same popup should follow it rather
    // than being torn down and rebuilt every frame.
    select->set_position(foundation::NanPoint(20.0F, 60.0F));
    select->on_process(0.016F);

    auto* after = dismiss->get_child(0)->as_control();
    REQUIRE(after == before);
    REQUIRE(after->global_bounds().get_top() > before_top);
}

TEST_CASE("releasing over the field keeps the popup open", "[select][overlay]") {
    auto host = scene::OverlayHost::create();
    auto select = widget::Select::create({"A", "B", "C"});
    // A sole content root would be stretched over the viewport, so keep the field at
    // its natural size: the release must land on the field, not on the popup.
    auto content = std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 240.0F));
    select->set_position(foundation::NanPoint(20.0F, 20.0F));
    content->add_child(select);
    content->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
    host->set_content(content);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 240.0F)) >= 1);

    const auto field = foundation::NanPoint(
        select->global_bounds().get_left() + 10.0F,
        select->global_bounds().get_top() + 10.0F
    );
    REQUIRE(select->global_bounds().contains_point(field));

    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        field,
    });
    REQUIRE(select->is_open());
    REQUIRE(host->overlay_count() == 1);
    REQUIRE(tree.focused_node() == select.get());

    // Real frames lay out between the press and the release, which is what makes the
    // overlay surface hit-testable under the pointer by the time the button comes up.
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 240.0F)) >= 1);

    // The button comes up over the field the user just clicked. The pointer is now over
    // the overlay surface, but that must not move focus off the field and tear the
    // popup down before the user can pick an option.
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        field,
    });
    REQUIRE(select->is_open());
    REQUIRE(host->overlay_count() == 1);
    REQUIRE(tree.focused_node() == select.get());
}

TEST_CASE("selecting an option while the field holds focus", "[select][overlay]") {
    auto host = scene::OverlayHost::create();
    auto select = widget::Select::create({"A", "B", "C"});
    host->set_content(select);
    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 200.0F)) >= 1);

    // A real interaction focuses the field first (the click on it) and then clicks an
    // option, so the focus change and the option click land in the same dispatch.
    tree.set_focus(select.get());
    select->open();
    REQUIRE(host->overlay_count() == 1);

    auto* dismiss = host->layer_at(1)->layout_root()->get_child(0)->as_control();
    REQUIRE(dismiss != nullptr);
    auto* popup = dismiss->get_child(0)->as_control();
    REQUIRE(popup != nullptr);
    const auto option = foundation::NanPoint(
        popup->global_bounds().get_left() + 10.0F,
        popup->global_bounds().get_top() + 48.0F
    );
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        option,
    });

    REQUIRE(select->selected_index() == 1);
    REQUIRE_FALSE(select->is_open());
    REQUIRE(host->overlay_count() == 0);
    // The field keeps focus, the way it did while the popup was drawn inside it.
    REQUIRE(tree.focused_node() == select.get());
}
