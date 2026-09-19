//
// Theme / Spinner tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/spinner.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;
        int lines = 0;
        int circles = 0;
        foundation::NanColor last_line_color;

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
        void draw_rounded_rect_outline(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_rects;
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor& color
        ) override {
            ++lines;
            last_line_color = color;
        }
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
        ) override {}
    };
} // namespace

TEST_CASE("spinner resolves its indicator token and metrics from the recipe", "[spinner][theme]") {
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_spinner(
        design,
        theme::ColorAppearance::light,
        theme::SpinnerVisualState::normal
    );

    REQUIRE(
        style.indicator.oklch().light == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(style.metrics.diameter == Catch::Approx(16.0F));
    REQUIRE(style.metrics.thickness == Catch::Approx(2.0F));
    REQUIRE(style.metrics.arc_radians == Catch::Approx(4.712389F));
    REQUIRE(style.metrics.rotation_speed == Catch::Approx(4.0F));
}

TEST_CASE("spinner disabled state scales indicator alpha", "[spinner][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = theme::resolve_spinner(
        design,
        theme::ColorAppearance::light,
        theme::SpinnerVisualState::normal
    );
    const auto disabled = theme::resolve_spinner(
        design,
        theme::ColorAppearance::light,
        theme::SpinnerVisualState::disabled
    );

    REQUIRE(disabled.indicator.alpha() == Catch::Approx(normal.indicator.alpha() * design.tokens.opacity.disabled));
}

TEST_CASE("spinner measures a diameter square", "[spinner][layout]") {
    auto spinner = widget::Spinner::create();
    REQUIRE(spinner->resolved_style().metrics.diameter == Catch::Approx(16.0F));

    const auto loose = spinner->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(loose.get_width() == Catch::Approx(16.0F));
    REQUIRE(loose.get_height() == Catch::Approx(16.0F));

    const auto narrow = spinner->measure_layout(scene::LayoutConstraints {
        .max_width = 8.0F,
        .max_height = 8.0F,
    });
    REQUIRE(narrow.get_width() == Catch::Approx(8.0F));
    REQUIRE(narrow.get_height() == Catch::Approx(8.0F));
}

TEST_CASE("spinner advances rotation until disabled", "[spinner][animation]") {
    auto spinner = widget::Spinner::create();
    REQUIRE(spinner->rotation() == Catch::Approx(0.0F));

    spinner->on_process(0.5F);
    // 0.5 s * 4 rad/s.
    REQUIRE(spinner->rotation() == Catch::Approx(2.0F));
    spinner->on_process(0.25F);
    REQUIRE(spinner->rotation() == Catch::Approx(3.0F));

    spinner->set_disabled(true);
    REQUIRE(spinner->visual_state() == theme::SpinnerVisualState::disabled);
    spinner->on_process(1.0F);
    REQUIRE(spinner->rotation() == Catch::Approx(3.0F));
}

TEST_CASE("spinner exposes progress_bar semantics with its label", "[spinner][semantics]") {
    auto spinner = widget::Spinner::create();
    spinner->set_label("Loading");
    scene::NanSceneTree tree;
    tree.set_root(spinner);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(spinner->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::progress_bar);
    REQUIRE(node->properties.label == "Loading");
    REQUIRE_FALSE(node->properties.state.disabled);

    spinner->set_disabled(true);
    REQUIRE(tree.update_semantics());
    const auto* disabled_node = tree.semantics_tree().find(spinner->semantics_id());
    REQUIRE(disabled_node != nullptr);
    REQUIRE(disabled_node->properties.state.disabled);
}

TEST_CASE("spinner override patches fields and survives a system apply", "[spinner][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto spinner = widget::Spinner::create();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(spinner);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    const float indicator_light = spinner->resolved_style().indicator.oklch().light;

    spinner->set_override(theme::SpinnerRecipeRule {
        .indicator = theme::ThemeColor::token(theme::ColorToken::error),
        .metrics_diameter = theme::ThemeScalar::literal(24.0F),
    });
    const auto overridden = spinner->resolved_style();
    REQUIRE(
        overridden.indicator.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );
    REQUIRE(overridden.metrics.diameter == Catch::Approx(24.0F));

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(spinner->resolved_style().indicator.oklch().light == Catch::Approx(0.60F));
    REQUIRE(spinner->resolved_style().indicator.oklch().light != Catch::Approx(indicator_light));
}

TEST_CASE("spinner re-resolves its indicator on a theme switch", "[spinner][theme]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto spinner = widget::Spinner::create();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(spinner);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    REQUIRE(
        spinner->resolved_style().indicator.oklch().light
        == Catch::Approx(themes.design_system().light.primary.oklch().light)
    );

    auto design = theme::default_design_system();
    design.light.primary = theme::nan_color(0.42F, 0.12F, 150.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(
        spinner->resolved_style().indicator.oklch().light == Catch::Approx(0.42F)
    );
}

TEST_CASE("spinner paints an arc through the scene tree", "[spinner][paint]") {
    auto spinner = widget::Spinner::create();
    scene::NanSceneTree tree;
    tree.set_root(spinner);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    RecordingDevice dev;
    tree.draw(dev);
    // 默认弧长 4.712 rad < 2π：走分段弧线路径，不画整环。
    REQUIRE(dev.lines >= 2);
    REQUIRE(dev.rounded_rects == 0);
    REQUIRE(dev.last_line_color.alpha() == Catch::Approx(1.0F));
}

TEST_CASE("BuildContext spinner carries an optional label", "[spinner][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    auto spinner = ui.make<widget::Spinner>("Busy").build();
    REQUIRE(spinner->label() == "Busy");

    auto unlabeled = ui.make<widget::Spinner>().build();
    REQUIRE(unlabeled->label().empty());
}
