//
// Theme / Skeleton tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/skeleton.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

using namespace nandina;

namespace
{
    /// 记录圆角矩形尺寸，用于校验多行占位条与末行比例。
    class RecordingDevice final: public render::IRenderDevice {
    public:
        std::vector<foundation::NanRect> rounded_rects;

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
            const foundation::NanRect& rect,
            float,
            const foundation::NanColor&
        ) override {
            rounded_rects.push_back(rect);
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

TEST_CASE("skeleton resolves muted surface and metrics from the recipe", "[skeleton][theme]") {
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_skeleton(
        design,
        theme::ColorAppearance::light,
        theme::SkeletonVisualState::normal
    );

    REQUIRE(style.surface.fill.oklch().light == Catch::Approx(design.light.muted.oklch().light));
    REQUIRE(style.surface.radius == Catch::Approx(design.tokens.radius.md));
    REQUIRE(style.metrics.height == Catch::Approx(12.0F));
    REQUIRE(style.metrics.line_gap == Catch::Approx(8.0F));
    REQUIRE(style.metrics.last_line_ratio == Catch::Approx(0.6F));
    REQUIRE(style.metrics.preferred_width == Catch::Approx(240.0F));
}

TEST_CASE("skeleton resolves light and dark surfaces from the same snapshot", "[skeleton][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_skeleton(
        design,
        theme::ColorAppearance::light,
        theme::SkeletonVisualState::normal
    );
    const auto dark = theme::resolve_skeleton(
        design,
        theme::ColorAppearance::dark,
        theme::SkeletonVisualState::normal
    );

    REQUIRE(light.surface.fill.oklch().light == Catch::Approx(design.light.muted.oklch().light));
    REQUIRE(dark.surface.fill.oklch().light == Catch::Approx(design.dark.muted.oklch().light));
    REQUIRE(dark.surface.fill.oklch().light < light.surface.fill.oklch().light);
}

TEST_CASE("skeleton defaults to a single text line at the preferred width", "[skeleton][layout]") {
    auto skeleton = widget::Skeleton::create();
    REQUIRE(skeleton->variant() == widget::SkeletonVariant::text);
    REQUIRE(skeleton->lines() == 1);
    REQUIRE(skeleton->visual_state() == theme::SkeletonVisualState::normal);
    REQUIRE(skeleton->label().empty());

    const auto loose = skeleton->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(loose.get_width() == Catch::Approx(240.0F));
    REQUIRE(loose.get_height() == Catch::Approx(12.0F));

    // 有界约束下宽度跟随约束上限，单行高度不变。
    const auto narrow = skeleton->measure_layout(scene::LayoutConstraints {
        .max_width = 80.0F,
        .max_height = 48.0F,
    });
    REQUIRE(narrow.get_width() == Catch::Approx(80.0F));
    REQUIRE(narrow.get_height() == Catch::Approx(12.0F));
}

TEST_CASE("skeleton stacks text lines with the recipe line gap", "[skeleton][layout]") {
    auto skeleton = widget::Skeleton::create();
    skeleton->set_lines(3);
    REQUIRE(skeleton->lines() == 3);

    const auto loose = skeleton->measure_layout(scene::LayoutConstraints::loose());
    // 3 * 12 + 2 * 8 = 52
    REQUIRE(loose.get_height() == Catch::Approx(52.0F));
}

TEST_CASE("skeleton clamps line counts to at least one", "[skeleton][boundary]") {
    auto skeleton = widget::Skeleton::create();

    skeleton->set_lines(0);
    REQUIRE(skeleton->lines() == 1);
    skeleton->set_lines(-5);
    REQUIRE(skeleton->lines() == 1);
    skeleton->set_lines(4);
    REQUIRE(skeleton->lines() == 4);
}

TEST_CASE("skeleton narrows the last text line by the recipe ratio", "[skeleton][paint]") {
    auto skeleton = widget::Skeleton::create();
    skeleton->set_lines(3);
    scene::NanSceneTree tree;
    tree.set_root(skeleton);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 100.0F)) >= 1);

    RecordingDevice device;
    tree.draw(device);
    REQUIRE(device.rounded_rects.size() == 3);
    const auto first_width = device.rounded_rects.front().get_width();
    const auto last_width = device.rounded_rects.back().get_width();
    REQUIRE(first_width == Catch::Approx(200.0F));
    REQUIRE(last_width == Catch::Approx(200.0F * 0.6F));
    REQUIRE(last_width < first_width);
}

TEST_CASE("skeleton rectangle variant fills constraints or an explicit size", "[skeleton][layout]") {
    auto skeleton = widget::Skeleton::create();
    skeleton->set_variant(widget::SkeletonVariant::rectangle);
    REQUIRE(skeleton->variant() == widget::SkeletonVariant::rectangle);

    // 无界：回退到首选宽度与配方高度。
    const auto loose = skeleton->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(loose.get_width() == Catch::Approx(240.0F));
    REQUIRE(loose.get_height() == Catch::Approx(12.0F));

    // 有界：铺满约束。
    const auto bounded = skeleton->measure_layout(scene::LayoutConstraints {
        .max_width = 120.0F,
        .max_height = 40.0F,
    });
    REQUIRE(bounded.get_width() == Catch::Approx(120.0F));
    REQUIRE(bounded.get_height() == Catch::Approx(40.0F));

    // 显式尺寸优先。
    skeleton->set_width(64.0F);
    skeleton->set_height(24.0F);
    const auto explicit_size = skeleton->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(explicit_size.get_width() == Catch::Approx(64.0F));
    REQUIRE(explicit_size.get_height() == Catch::Approx(24.0F));
}

TEST_CASE("skeleton override patches fields and survives a system apply", "[skeleton][override]") {
    theme::ThemeManager themes;
    auto skeleton = widget::Skeleton::create();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(skeleton);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 48.0F)) >= 1);

    const float fill_light = skeleton->resolved_style().surface.fill.oklch().light;

    skeleton->set_override(theme::SkeletonRecipeRule {
        .surface_fill = theme::ThemeColor::token(theme::ColorToken::error),
        .metrics_height = theme::ThemeScalar::literal(16.0F),
    });
    const auto overridden = skeleton->resolved_style();
    REQUIRE(
        overridden.surface.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );
    REQUIRE(overridden.metrics.height == Catch::Approx(16.0F));

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(skeleton->resolved_style().surface.fill.oklch().light == Catch::Approx(0.60F));
    REQUIRE(skeleton->resolved_style().surface.fill.oklch().light != Catch::Approx(fill_light));
}

TEST_CASE("skeleton re-resolves after a theme switch", "[skeleton][theme]") {
    theme::ThemeManager themes;
    auto skeleton = widget::Skeleton::create();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(skeleton);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 48.0F)) >= 1);

    const auto before = skeleton->resolved_style().surface.fill.oklch().light;

    auto design = theme::default_design_system();
    design.light.muted = theme::nan_color(0.72F, 0.02F, 200.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));

    REQUIRE(skeleton->resolved_style().surface.fill.oklch().light == Catch::Approx(0.72F));
    REQUIRE(skeleton->resolved_style().surface.fill.oklch().light != Catch::Approx(before));
}

TEST_CASE("skeleton exposes an indeterminate progress_bar semantics label", "[skeleton][semantics]") {
    auto skeleton = widget::Skeleton::create();
    skeleton->set_label("Loading articles");
    scene::NanSceneTree tree;
    tree.set_root(skeleton);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(skeleton->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::progress_bar);
    REQUIRE(node->properties.label == "Loading articles");
    REQUIRE(node->properties.value.empty());
}

TEST_CASE("skeleton narrows only the last line of a multi-line block", "[skeleton][layout]") {
    auto skeleton = widget::Skeleton::create();

    // 单行占位应当占满整行，而不是莫名只剩 60%。
    skeleton->set_lines(1);
    {
        const auto ratios = skeleton->line_width_ratios();
        REQUIRE(ratios.size() == 1);
        REQUIRE(ratios[0] == Catch::Approx(1.0F));
    }

    // 多行时只有末行收窄。
    skeleton->set_lines(3);
    {
        const auto ratios = skeleton->line_width_ratios();
        REQUIRE(ratios.size() == 3);
        REQUIRE(ratios[0] == Catch::Approx(1.0F));
        REQUIRE(ratios[1] == Catch::Approx(1.0F));
        REQUIRE(ratios[2] == Catch::Approx(0.6F));
    }
}

TEST_CASE("skeleton is buildable through BuildContext", "[skeleton][authoring]") {
    // ComponentTraits 是模板，不实例化就不会被编译；这里真实走一遍 ui.make<>。
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    auto text = ui.make<widget::Skeleton>().build();
    REQUIRE(text != nullptr);
    REQUIRE(text->variant() == widget::SkeletonVariant::text);
    REQUIRE(text->lines() == 1);

    auto block = ui.make<widget::Skeleton>(widget::SkeletonVariant::rectangle, 3).build();
    REQUIRE(block->variant() == widget::SkeletonVariant::rectangle);
    REQUIRE(block->lines() == 3);
}
