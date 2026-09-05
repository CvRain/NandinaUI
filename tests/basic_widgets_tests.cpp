//
// Theme / Divider / Avatar / Chip tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/avatar.hpp"
#include "widget/chip.hpp"
#include "widget/divider.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rects = 0;
        int rounded_rects = 0;
        int text_calls = 0;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {
            ++rects;
        }
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

TEST_CASE("divider resolves color and thickness", "[divider][theme]") {
    auto design = theme::default_design_system();
    design.tokens.border.thin = 2.0F;

    const auto style = theme::resolve_divider(design, theme::ColorAppearance::light);
    REQUIRE(
        style.color.oklch().light == Catch::Approx(design.light.outline_variant.oklch().light)
    );
    REQUIRE(style.thickness == Catch::Approx(2.0F));
}

TEST_CASE("divider measures and exposes separator semantics", "[divider][layout][semantics]") {
    auto divider = widget::Divider::create();
    scene::NanSceneTree tree;
    tree.set_root(divider);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(divider->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::separator);
}

TEST_CASE("avatar computes initials and exposes name semantics", "[avatar][semantics]") {
    auto avatar = widget::Avatar::create("nandina");
    REQUIRE(avatar->initials() == "N");
    REQUIRE(avatar->resolved_style().metrics.box_size == Catch::Approx(40.0F));

    scene::NanSceneTree tree;
    tree.set_root(avatar);
    REQUIRE(tree.layout_root(foundation::NanSize(100.0F, 100.0F)) >= 1);
    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(avatar->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.label == "nandina");
}

TEST_CASE("chip resolves recipe and reports removable semantics", "[chip][theme][semantics]") {
    auto design = theme::default_design_system();
    const auto style = theme::resolve_chip(design, theme::ColorAppearance::light);
    REQUIRE(
        style.container.fill.oklch().light
        == Catch::Approx(design.light.surface_variant.oklch().light)
    );
    REQUIRE(style.metrics.height == Catch::Approx(28.0F));

    auto chip = widget::Chip::create("Tag", true);
    scene::NanSceneTree tree;
    tree.set_root(chip);
    REQUIRE(tree.layout_root(foundation::NanSize(160.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(chip->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::button);
    REQUIRE(node->properties.label == "Tag");
}

TEST_CASE("chip remove button emits removed event", "[chip][interaction]") {
    auto chip = widget::Chip::create("Dismiss me", true);
    int removed = 0;
    auto subscription = chip->removed().subscribe([&] { ++removed; });
    scene::NanSceneTree tree;
    tree.set_root(chip);
    REQUIRE(tree.layout_root(foundation::NanSize(160.0F, 48.0F)) >= 1);

    // 点击右端移除标记区域（本地坐标，位于 width - padding_x 内侧）。
    const float x = chip->width() - 10.0F;
    const float y = chip->height() * 0.5F;
    scene::MouseButtonEvent press(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint {x, y}
    );
    chip->on_input(press);
    REQUIRE(removed == 1);
}

TEST_CASE("chip is focusable when removable and activates via keyboard", "[chip][focus][keyboard]") {
    auto chip = widget::Chip::create("Tag", true);
    auto plain = widget::Chip::create("Plain", false);
    REQUIRE(chip->is_focusable());
    REQUIRE_FALSE(plain->is_focusable());

    int removed = 0;
    auto subscription = chip->removed().subscribe([&] { ++removed; });

    // Enter / Space 激活移除；Backspace / Delete 删除移除。
    scene::KeyEvent enter(257, scene::KeyEvent::Action::press);
    REQUIRE(chip->on_input(enter));
    REQUIRE(removed == 1);

    scene::KeyEvent space(32, scene::KeyEvent::Action::press);
    REQUIRE(chip->on_input(space));
    REQUIRE(removed == 2);

    scene::KeyEvent backspace(259, scene::KeyEvent::Action::press);
    REQUIRE(chip->on_input(backspace));
    REQUIRE(removed == 3);

    scene::KeyEvent del(261, scene::KeyEvent::Action::press);
    REQUIRE(chip->on_input(del));
    REQUIRE(removed == 4);

    // 不可移除 chip 忽略键盘。
    scene::KeyEvent press(257, scene::KeyEvent::Action::press);
    REQUIRE_FALSE(plain->on_input(press));
}

TEST_CASE("chip reports focused semantics for the focus ring", "[chip][focus][semantics]") {
    auto chip = widget::Chip::create("Tag", true);
    scene::NanSceneTree tree;
    tree.set_root(chip);
    REQUIRE(tree.layout_root(foundation::NanSize(160.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    tree.set_focus(chip.get());
    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(chip->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.state.focusable);
    REQUIRE(node->properties.state.focused);

    tree.set_focus(nullptr);
    REQUIRE(tree.update_semantics());
    node = tree.semantics_tree().find(chip->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE_FALSE(node->properties.state.focused);
}

TEST_CASE("avatar handles CJK initials without corrupting bytes", "[avatar]") {
    auto avatar = widget::Avatar::create("南天竹");
    REQUIRE(avatar->initials() == "南");

    auto ascii = widget::Avatar::create("nandina");
    REQUIRE(ascii->initials() == "N");
}

TEST_CASE("divider dashed and double patterns draw multiple segments", "[divider][paint]") {
    auto dashed = widget::Divider::create();
    dashed->set_pattern(widget::Divider::Pattern::dashed);
    scene::NanSceneTree tree;
    tree.set_root(dashed);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 8.0F)) >= 1);
    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.rects > 1); // 多个虚线小段

    auto doubled = widget::Divider::create();
    doubled->set_pattern(widget::Divider::Pattern::double_line);
    scene::NanSceneTree tree2;
    tree2.set_root(doubled);
    REQUIRE(tree2.layout_root(foundation::NanSize(200.0F, 8.0F)) >= 1);
    RecordingDevice dev2;
    tree2.draw(dev2);
    REQUIRE(dev2.rects == 2); // 双实线
}

TEST_CASE("divider and avatar paint expected primitives", "[divider][avatar][paint]") {
    auto divider = widget::Divider::create();
    scene::NanSceneTree tree;
    tree.set_root(divider);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 8.0F)) >= 1);
    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.rects == 1); // 分隔线

    auto avatar = widget::Avatar::create("A");
    scene::NanSceneTree tree2;
    tree2.set_root(avatar);
    REQUIRE(tree2.layout_root(foundation::NanSize(48.0F, 48.0F)) >= 1);
    RecordingDevice dev2;
    tree2.draw(dev2);
    REQUIRE(dev2.rounded_rects == 1); // 圆形容器
    REQUIRE(dev2.text_calls == 1);
}
