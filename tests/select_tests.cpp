//
// Theme / Select tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/controls.hpp"
#include "widget/select.hpp"

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
