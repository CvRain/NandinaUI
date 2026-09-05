//
// Theme / Tabs tests.
//

#include "animation/animation_host.hpp"
#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/controls.hpp"
#include "widget/tabs.hpp"

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

TEST_CASE("tabs resolves label and indicator tokens", "[tabs][theme]") {
    auto design = theme::default_design_system();
    design.tokens.spacing.lg = 20.0F;

    const auto style = theme::resolve_tabs(
        design,
        theme::ColorAppearance::light,
        theme::TabsVisualState::normal
    );

    REQUIRE(
        style.label.color.oklch().light
        == Catch::Approx(design.light.on_surface_variant.oklch().light)
    );
    REQUIRE(
        style.label_selected.color.oklch().light
        == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(
        style.indicator.oklch().light == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(style.indicator_thickness == Catch::Approx(2.0F));
    REQUIRE(style.metrics.gap == Catch::Approx(20.0F));
    REQUIRE(style.metrics.min_height == Catch::Approx(40.0F));
}

TEST_CASE("tabs disabled state scales label and indicator alpha", "[tabs][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = theme::resolve_tabs(
        design,
        theme::ColorAppearance::light,
        theme::TabsVisualState::normal
    );
    const auto disabled = theme::resolve_tabs(
        design,
        theme::ColorAppearance::light,
        theme::TabsVisualState::disabled
    );

    const float expected = design.tokens.opacity.disabled;
    REQUIRE(disabled.label.color.alpha() == Catch::Approx(normal.label.color.alpha() * expected));
    REQUIRE(
        disabled.label_selected.color.alpha()
        == Catch::Approx(normal.label_selected.color.alpha() * expected)
    );
    REQUIRE(disabled.indicator.alpha() == Catch::Approx(normal.indicator.alpha() * expected));
}

TEST_CASE("tabs override survives a system apply", "[tabs][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto tabs = widget::Tabs::create({"A", "B"});
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(tabs);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    tabs->set_override(theme::TabsRecipeRule {
        .label_selected_color = theme::ThemeColor::token(theme::ColorToken::error),
    });
    REQUIRE(
        tabs->resolved_style().label_selected.color.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );

    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(tabs->resolved_style().label_selected.color.oklch().light == Catch::Approx(0.60F));
}

TEST_CASE("tabs selects by index and clamps out-of-range", "[tabs][value]") {
    auto tabs = widget::Tabs::create({"A", "B", "C"});
    REQUIRE(tabs->selected_index() == 0);
    REQUIRE(tabs->selected_label() == "A");

    tabs->select(2);
    REQUIRE(tabs->selected_index() == 2);
    REQUIRE(tabs->selected_label() == "C");

    tabs->set_selected_index(99);
    REQUIRE(tabs->selected_index() == 2);
    tabs->set_selected_index(-1);
    REQUIRE(tabs->selected_index() == 0);
}

TEST_CASE("tabs indicator animation advances via the animation host", "[tabs][animation]") {
    // 不同长度的标签使指示条 x 与宽度都发生变化，从而同时注册两条轨道。
    auto tabs = widget::Tabs::create({"A", "BB", "CCC"});
    scene::NanSceneTree tree;
    tree.set_root(tabs);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    tabs->select(1); // 触发下划线切换动画（注册到 AnimationHost）
    REQUIRE(tabs->selected_index() == 1);
    REQUIRE(tree.animation_host().active_count() == 2); // indicator_x + indicator_width

    {
        auto phase = tree.enter_phase(scene::FramePhase::animation);
        tree.advance_animations(0.5F); // 越过 medium_duration，动画结束
    }
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(tabs->selected_index() == 1);
}

TEST_CASE("tabs arrow keys navigate with wrap-around", "[tabs][keyboard]") {
    auto tabs = widget::Tabs::create({"A", "B", "C"});
    scene::NanSceneTree tree;
    tree.set_root(tabs);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    tree.set_focus(tabs.get());

    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press)); // right
    REQUIRE(tabs->selected_index() == 1);

    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press)); // right (wrap)
    REQUIRE(tabs->selected_index() == 2);

    tree.dispatch_key(scene::KeyEvent(262, scene::KeyEvent::Action::press)); // right (wrap)
    REQUIRE(tabs->selected_index() == 0);

    tree.dispatch_key(scene::KeyEvent(263, scene::KeyEvent::Action::press)); // left
    REQUIRE(tabs->selected_index() == 2);
}

TEST_CASE("tabs exposes tab semantics with selected label", "[tabs][semantics]") {
    auto tabs = widget::Tabs::create({"General", "Appearance"});
    scene::NanSceneTree tree;
    tree.set_root(tabs);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(tabs->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::tab);
    REQUIRE(node->properties.label == "General");
    REQUIRE(node->properties.state.selected == true);

    tabs->select(1);
    REQUIRE(tree.update_semantics());
    const auto* updated = tree.semantics_tree().find(tabs->semantics_id());
    REQUIRE(updated != nullptr);
    REQUIRE(updated->properties.label == "Appearance");
}

TEST_CASE("tabs paints labels plus one indicator", "[tabs][paint]") {
    auto tabs = widget::Tabs::create({"A", "B", "C"});
    scene::NanSceneTree tree;
    tree.set_root(tabs);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.text_calls == 3);
    REQUIRE(dev.rounded_rects == 1); // 选中下划线
}

TEST_CASE("tabs draws container, pill, and indicator when configured", "[tabs][paint]") {
    auto tabs = widget::Tabs::create({"A", "B"});
    tabs->set_override(theme::TabsRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::surface_variant),
        .container_radius = theme::ThemeScalar::literal(8.0F),
        .selected_background_fill = theme::ThemeColor::token(theme::ColorToken::surface),
        .selected_background_radius = theme::ThemeScalar::literal(6.0F),
        .metrics_padding_x = theme::ThemeScalar::literal(4.0F),
    });
    scene::NanSceneTree tree;
    tree.set_root(tabs);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.text_calls == 2);
    REQUIRE(dev.rounded_rects == 3); // 容器 + 选中 pill + 下划线
}

TEST_CASE("tabs default recipe keeps container and pill transparent", "[tabs][theme]") {
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_tabs(
        design,
        theme::ColorAppearance::light,
        theme::TabsVisualState::normal
    );
    REQUIRE(style.container.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(style.selected_background.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(style.indicator.alpha() > 0.0F); // 默认下划线风格
}

TEST_CASE("BuildContext tabs synchronizes a selected-index signal", "[tabs][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto& selected = ui.signal<int>(0);
    auto tabs = ui.make<widget::Tabs>(selected, std::vector<std::string> {"A", "B", "C"}).build();

    REQUIRE(tabs->selected_index() == 0);
    tabs->select(2);
    REQUIRE(selected.peek() == 2);

    selected.set(1);
    REQUIRE(tabs->selected_index() == 1);
}
