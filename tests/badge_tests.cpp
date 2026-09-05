//
// Theme / Badge tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/badge.hpp"

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

TEST_CASE("badge style resolves pill tokens from the recipe", "[badge][theme]") {
    auto design = theme::default_design_system();
    design.tokens.spacing.sm = 9.0F;

    const auto style = theme::resolve_badge(design, theme::ColorAppearance::light);

    REQUIRE(style.container.fill.oklch().light == Catch::Approx(
        design.light.surface_variant.oklch().light
    ));
    REQUIRE(
        style.label.color.oklch().light
        == Catch::Approx(design.light.on_surface_variant.oklch().light)
    );
    REQUIRE(style.label.font_size == Catch::Approx(design.tokens.typography.label_sm));
    REQUIRE(style.metrics.height == Catch::Approx(22.0F));
    REQUIRE(style.metrics.padding_x == Catch::Approx(9.0F));
    // pill 全圆角容器。
    REQUIRE(style.container.radius == Catch::Approx(design.tokens.radius.full));
}

TEST_CASE("badge resolves light and dark surfaces from the same snapshot", "[badge][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_badge(design, theme::ColorAppearance::light);
    const auto dark = theme::resolve_badge(design, theme::ColorAppearance::dark);

    REQUIRE(
        light.container.fill.oklch().light
        == Catch::Approx(design.light.surface_variant.oklch().light)
    );
    REQUIRE(
        dark.container.fill.oklch().light
        == Catch::Approx(design.dark.surface_variant.oklch().light)
    );
    REQUIRE(dark.container.fill.oklch().light < light.container.fill.oklch().light);
}

TEST_CASE("badge override patches fields and survives a system apply", "[badge][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto badge = widget::Badge::create("Beta");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(badge);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    const float fill_light = badge->resolved_style().container.fill.oklch().light;

    badge->set_override(theme::BadgeRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::primary),
        .label_color = theme::ThemeColor::token(theme::ColorToken::on_primary),
    });
    const auto overridden = badge->resolved_style();
    REQUIRE(
        overridden.container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.primary.oklch().light)
    );
    REQUIRE(
        overridden.label.color.oklch().light
        == Catch::Approx(themes.design_system().light.on_primary.oklch().light)
    );

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.primary = theme::nan_color(0.50F, 0.20F, 30.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(
        badge->resolved_style().container.fill.oklch().light
        == Catch::Approx(0.50F)
    );
    REQUIRE(badge->resolved_style().container.fill.oklch().light != Catch::Approx(fill_light));
}

TEST_CASE("badge measures text plus padding under tight constraints", "[badge][layout]") {
    auto badge = widget::Badge::create("New");
    scene::NanSceneTree tree;
    tree.set_root(badge);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    const float full_width = badge->width();
    REQUIRE(full_width > 0.0F);

    // 窄约束下文字裁剪，控件不超过约束。
    REQUIRE(tree.layout_root(foundation::NanSize(24.0F, 48.0F)) >= 1);
    REQUIRE(badge->width() <= 24.0F);

    // 固有高度：loose 测量 = 配方度量高度 22（layout_root 会按 tight 约束拉伸 root）。
    const auto loose_size = badge->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(loose_size.get_height() == Catch::Approx(22.0F));
}

TEST_CASE("badge exposes static text semantics", "[badge][semantics]") {
    auto badge = widget::Badge::create("Beta");
    scene::NanSceneTree tree;
    tree.set_root(badge);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(badge->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::static_text);
    REQUIRE(node->properties.label == "Beta");

    badge->set_text("RC1");
    REQUIRE(tree.update_semantics());
    const auto* updated = tree.semantics_tree().find(badge->semantics_id());
    REQUIRE(updated != nullptr);
    REQUIRE(updated->properties.label == "RC1");
}

TEST_CASE("badge paints a rounded pill container plus text", "[badge][paint]") {
    auto badge = widget::Badge::create("New");
    scene::NanSceneTree tree;
    tree.set_root(badge);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);

    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.rounded_rects == 1);
    REQUIRE(dev.text_calls == 1);
}
