//
// Theme / ProgressBar tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/controls.hpp"
#include "widget/progress_bar.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <string>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;

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
        ) override {}
    };
} // namespace

TEST_CASE("progress bar resolves track and fill tokens from the recipe", "[progress-bar][theme]") {
    auto design = theme::default_design_system();
    design.tokens.radius.full = 9999.0F;

    const auto style = theme::resolve_progress_bar(
        design,
        theme::ColorAppearance::light,
        theme::ProgressBarVisualState::normal
    );

    REQUIRE(
        style.track.fill.oklch().light
        == Catch::Approx(design.light.outline_variant.oklch().light)
    );
    REQUIRE(
        style.fill.fill.oklch().light == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(style.track.radius == Catch::Approx(9999.0F));
    REQUIRE(style.fill.radius == Catch::Approx(9999.0F));
    REQUIRE(style.metrics.height == Catch::Approx(8.0F));
    REQUIRE(style.metrics.preferred_width == Catch::Approx(240.0F));
}

TEST_CASE("progress bar resolves light and dark surfaces from the same snapshot", "[progress-bar][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_progress_bar(
        design,
        theme::ColorAppearance::light,
        theme::ProgressBarVisualState::normal
    );
    const auto dark = theme::resolve_progress_bar(
        design,
        theme::ColorAppearance::dark,
        theme::ProgressBarVisualState::normal
    );

    REQUIRE(
        light.track.fill.oklch().light
        == Catch::Approx(design.light.outline_variant.oklch().light)
    );
    REQUIRE(
        dark.track.fill.oklch().light
        == Catch::Approx(design.dark.outline_variant.oklch().light)
    );
    REQUIRE(dark.track.fill.oklch().light < light.track.fill.oklch().light);
}

TEST_CASE("progress bar disabled state scales track and fill alpha", "[progress-bar][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = theme::resolve_progress_bar(
        design,
        theme::ColorAppearance::light,
        theme::ProgressBarVisualState::normal
    );
    const auto disabled = theme::resolve_progress_bar(
        design,
        theme::ColorAppearance::light,
        theme::ProgressBarVisualState::disabled
    );

    const float expected = design.tokens.opacity.disabled;
    REQUIRE(disabled.track.fill.alpha() == Catch::Approx(normal.track.fill.alpha() * expected));
    REQUIRE(disabled.fill.fill.alpha() == Catch::Approx(normal.fill.fill.alpha() * expected));
}

TEST_CASE("progress bar override patches fields and survives a system apply", "[progress-bar][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto bar = widget::ProgressBar::create(0.5F);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(bar);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    const float fill_light = bar->resolved_style().fill.fill.oklch().light;

    bar->set_override(theme::ProgressBarRecipeRule {
        .fill_fill = theme::ThemeColor::token(theme::ColorToken::error),
        .metrics_height = theme::ThemeScalar::literal(12.0F),
    });
    const auto overridden = bar->resolved_style();
    REQUIRE(
        overridden.fill.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );
    REQUIRE(overridden.metrics.height == Catch::Approx(12.0F));

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(
        bar->resolved_style().fill.fill.oklch().light == Catch::Approx(0.60F)
    );
    REQUIRE(bar->resolved_style().fill.fill.oklch().light != Catch::Approx(fill_light));
}

TEST_CASE("progress bar clamps value into the unit interval", "[progress-bar][value]") {
    auto bar = widget::ProgressBar::create(0.25F);
    REQUIRE(bar->value() == Catch::Approx(0.25F));

    bar->set_value(1.5F);
    REQUIRE(bar->value() == Catch::Approx(1.0F));
    bar->set_value(-0.5F);
    REQUIRE(bar->value() == Catch::Approx(0.0F));
    bar->set_value(std::numeric_limits<float>::quiet_NaN());
    REQUIRE(bar->value() == Catch::Approx(0.0F));
}

TEST_CASE("progress bar measures preferred width under loose and tight constraints", "[progress-bar][layout]") {
    auto bar = widget::ProgressBar::create(0.5F);

    // loose 测量：高度 = 配方厚度 8、宽度 = 配方首选 240。
    const auto loose = bar->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(loose.get_width() == Catch::Approx(240.0F));
    REQUIRE(loose.get_height() == Catch::Approx(8.0F));

    // 窄约束：宽度收缩到约束上限，厚度保持 8。
    const auto narrow = bar->measure_layout(scene::LayoutConstraints {
        .max_width = 24.0F,
        .max_height = 48.0F,
    });
    REQUIRE(narrow.get_width() == Catch::Approx(24.0F));
    REQUIRE(narrow.get_height() == Catch::Approx(8.0F));

    // 作为 root 时被窗口 tight 约束拉伸（宽度跟随窗口，厚度不再固定）。
    scene::NanSceneTree tree;
    tree.set_root(bar);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    REQUIRE(bar->width() == Catch::Approx(280.0F));
    REQUIRE(bar->height() == Catch::Approx(48.0F));
}

TEST_CASE("progress bar exposes progress_bar semantics with percentage value", "[progress-bar][semantics]") {
    auto bar = widget::ProgressBar::create(0.42F);
    bar->set_label("Upload");
    scene::NanSceneTree tree;
    tree.set_root(bar);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(bar->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::progress_bar);
    REQUIRE(node->properties.label == "Upload");
    REQUIRE(node->properties.value == "42%");

    bar->set_disabled(true);
    REQUIRE(tree.update_semantics());
    const auto* disabled_node = tree.semantics_tree().find(bar->semantics_id());
    REQUIRE(disabled_node != nullptr);
    REQUIRE(disabled_node->properties.state.disabled);
}

TEST_CASE("progress bar paints track plus a partial fill", "[progress-bar][paint]") {
    auto empty = widget::ProgressBar::create(0.0F);
    scene::NanSceneTree tree;
    tree.set_root(empty);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    RecordingDevice dev;
    tree.draw(dev);
    // 0%：只画轨道。
    REQUIRE(dev.rounded_rects == 1);

    auto half = widget::ProgressBar::create(0.5F);
    scene::NanSceneTree tree2;
    tree2.set_root(half);
    REQUIRE(tree2.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    RecordingDevice dev2;
    tree2.draw(dev2);
    // 50%：轨道 + 填充。
    REQUIRE(dev2.rounded_rects == 2);
}

TEST_CASE("BuildContext progress bar binds a progress signal one-way", "[progress-bar][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto& progress = ui.signal<float>(0.25F);
    auto bar = ui.make<widget::ProgressBar>(progress).build();

    REQUIRE(bar->value() == Catch::Approx(0.25F));
    progress.set(0.75F);
    REQUIRE(bar->value() == Catch::Approx(0.75F));
}
