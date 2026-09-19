//
// Slider range, input, semantics, theme, and authoring tests.
//

#include <nandina/reactive/scope.hpp>
#include <nandina/render/draw_context.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/slider.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

using namespace nandina;

namespace
{
    /// 记录每个图元的包围盒。Slider 曾把 radius_full(9999) 当作拇指圆半径直接
    /// 传给 draw_circle，画出一个覆盖整窗的圆盘；本设备让这种越界可被断言。
    class BoundsRecordingDevice final: public render::IRenderDevice {
    public:
        std::vector<foundation::NanRect> rects;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect& r, const foundation::NanColor&) override {
            rects.push_back(r);
        }
        void draw_rect_outline(
            const foundation::NanRect& r,
            float,
            const foundation::NanColor&
        ) override {
            rects.push_back(r);
        }
        void draw_rounded_rect(
            const foundation::NanRect& r,
            float,
            const foundation::NanColor&
        ) override {
            rects.push_back(r);
        }
        void draw_line(
            const foundation::NanPoint& a,
            const foundation::NanPoint& b,
            float,
            const foundation::NanColor&
        ) override {
            rects.push_back(foundation::NanRect::from_points(a, b));
        }
        void draw_circle(
            const foundation::NanPoint& center,
            float radius,
            const foundation::NanColor&
        ) override {
            rects.push_back(foundation::NanRect::from_xywh(
                center.get_x() - radius,
                center.get_y() - radius,
                radius * 2.0F,
                radius * 2.0F
            ));
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
    };
} // namespace

TEST_CASE("slider normalizes values to its range and step", "[slider][value]") {
    widget::Slider slider("Zoom", 1.03F, 0.5F, 2.0F, 0.1F);
    REQUIRE(slider.value() == Catch::Approx(1.0F));

    slider.set_value(4.0F);
    REQUIRE(slider.value() == Catch::Approx(2.0F));
    slider.set_value(0.76F);
    REQUIRE(slider.value() == Catch::Approx(0.8F));
    REQUIRE_THROWS_AS(slider.set_range(2.0F, 1.0F), std::invalid_argument);
    REQUIRE_THROWS_AS(slider.set_step(0.0F), std::invalid_argument);
}

TEST_CASE("slider keyboard and semantic actions emit user changes", "[slider][semantics]") {
    auto slider = std::make_shared<widget::Slider>("Interface scale", 1.0F, 0.5F, 1.5F, 0.1F);
    int changes = 0;
    auto subscription = slider->value_changed().subscribe([&](const float) { ++changes; });
    scene::NanSceneTree tree;
    tree.set_root(slider);
    REQUIRE(tree.layout_root(foundation::NanSize(300.0F, 40.0F)) >= 1);
    tree.set_focus(slider.get());
    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press));
    REQUIRE(slider->value() == Catch::Approx(1.1F));

    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(slider->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::slider);
    REQUIRE(node->properties.value == "1.1");
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::set_value));
    REQUIRE(tree.perform_semantics_action(
        slider->semantics_id(),
        {.action = semantics::Action::set_value, .value = "1.34"}
    ));
    REQUIRE(slider->value() == Catch::Approx(1.3F));
    REQUIRE(tree.perform_semantics_action(
        slider->semantics_id(),
        {.action = semantics::Action::decrement}
    ));
    REQUIRE(slider->value() == Catch::Approx(1.2F));
    REQUIRE(changes == 3);
}

TEST_CASE("slider pointer dragging uses capture outside its bounds", "[slider][input]") {
    auto slider = std::make_shared<widget::Slider>("Volume", 0.0F, 0.0F, 100.0F, 1.0F);
    scene::NanSceneTree tree;
    tree.set_root(slider);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 32.0F)) >= 1);

    tree.dispatch_mouse_move(
        scene::MouseMoveEvent(foundation::NanPoint(10.0F, 16.0F), foundation::NanPoint::zero())
    );
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(10.0F, 16.0F)
        )
    );
    REQUIRE(tree.pointer_capture() == slider.get());
    tree.dispatch_mouse_move(
        scene::MouseMoveEvent(
            foundation::NanPoint(260.0F, 16.0F),
            foundation::NanPoint(250.0F, 0.0F)
        )
    );
    REQUIRE(slider->value() == Catch::Approx(100.0F));
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::release,
            foundation::NanPoint(260.0F, 16.0F)
        )
    );
    REQUIRE(tree.pointer_capture() == nullptr);
}

TEST_CASE("BuildContext slider synchronizes a float signal", "[slider][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto& scale = ui.signal<float>(1.0F);
    auto slider = ui.make<widget::Slider>(scale, "Scale", 0.5F, 2.0F, 0.1F).build();
    scene::NanSceneTree tree;
    tree.set_root(slider);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 32.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.perform_semantics_action(
        slider->semantics_id(),
        {.action = semantics::Action::increment}
    ));
    REQUIRE(scale.peek() == Catch::Approx(1.1F));
    scale.set(1.5F);
    REQUIRE(slider->value() == Catch::Approx(1.5F));
}

TEST_CASE("slider style resolves semantic theme colors", "[slider][theme]") {
    auto design = theme::default_design_system();
    design.light.primary = theme::nan_color(0.72F, 0.12F, 190.0F);
    const auto style = theme::resolve_slider(
        design,
        theme::ColorAppearance::light,
        theme::SliderVisualState::dragging
    );
    REQUIRE(style.active_track.box.fill.oklch().light == Catch::Approx(0.72F));
    REQUIRE(style.thumb.box.radius == Catch::Approx(11.0F));
}

TEST_CASE("slider value label opt-in increases measured height and tracks value", "[slider][label]") {
    auto slider = std::make_shared<widget::Slider>("Zoom", 0.5F, 0.0F, 1.0F, 0.1F);
    REQUIRE_FALSE(slider->show_value_label());
    REQUIRE(slider->value_label_text() == "0.5");

    const auto base = slider->measure_layout(scene::LayoutConstraints::loose());
    slider->set_show_value_label(true);
    REQUIRE(slider->show_value_label());
    const auto with_label = slider->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(with_label.get_height() > base.get_height());

    // 开启后不额外占高（重复调用幂等），值标签随 value 更新。
    const auto again = slider->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(again.get_height() == Catch::Approx(with_label.get_height()));
    slider->set_value(0.9F);
    REQUIRE(slider->value_label_text() == "0.9");
}

TEST_CASE("slider thumb paints inside the slider bounds", "[slider][paint][theme]") {
    // 回归：默认主题的 thumb.box.radius 曾被解析为 radius_full(9999)，而
    // Slider::on_draw 把它当像素半径传给 draw_circle，画出一个 19998x19998 的
    // 不透明圆盘。圆盘在内容区最后绘制，会盖掉先绘制的外壳（playground 里
    // inputs / loading 两页外壳整块消失）。这里断言拇指不会越出控件。
    auto slider = std::make_shared<widget::Slider>("Scale", 0.5F, 0.0F, 1.0F, 0.01F);
    scene::NanSceneTree tree;
    tree.set_root(slider);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 32.0F)) >= 1);

    BoundsRecordingDevice device;
    render::DrawContext context {device};
    tree.render(context);

    REQUIRE_FALSE(device.rects.empty());
    const auto bounds = slider->global_bounds().expanded(1.0F);
    for (const auto& rect: device.rects) {
        INFO(
            "primitive rect = ("
            << rect.get_left() << ", " << rect.get_top() << " " << rect.get_width() << "x"
            << rect.get_height() << ")"
        );
        REQUIRE(bounds.contains_rect(rect));
    }
}

TEST_CASE("slider normal thumb radius is a concrete pixel radius", "[slider][theme]") {
    // radius_full 是"胶囊"圆角 token（9999），不能当像素半径用；正常态应当与
    // dragging 11 / hovered 10 同一量级。
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_slider(
        design,
        theme::ColorAppearance::light,
        theme::SliderVisualState::normal
    );
    REQUIRE(style.thumb.box.radius > 0.0F);
    REQUIRE(style.thumb.box.radius < 32.0F);
}
