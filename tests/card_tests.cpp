//
// Theme / Card tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/card.hpp"
#include "widget/label.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_fills = 0;
        int rounded_outlines = 0;
        int shadows = 0;
        int shadow_before_fill = 0;

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
            ++rounded_fills;
        }
        void draw_rounded_rect_shadow(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor&
        ) override {
            ++shadows;
            if (rounded_fills == 0 && rounded_outlines == 0) {
                ++shadow_before_fill;
            }
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect&,
            float,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_outlines;
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

TEST_CASE("card style resolves surface tokens from the recipe", "[card][theme]") {
    auto design = theme::default_design_system();
    design.tokens.spacing.md = 15.0F;

    const auto style = theme::resolve_card(design, theme::ColorAppearance::light);

    REQUIRE(
        style.container.fill.oklch().light == Catch::Approx(design.light.surface.oklch().light)
    );
    REQUIRE(
        style.container.border.oklch().light
        == Catch::Approx(design.light.outline_variant.oklch().light)
    );
    REQUIRE(style.container.border_width == Catch::Approx(design.tokens.border.thin));
    REQUIRE(style.container.radius == Catch::Approx(design.tokens.radius.md));
    REQUIRE(style.metrics.padding_x == Catch::Approx(15.0F));
    REQUIRE(style.metrics.padding_y == Catch::Approx(15.0F));
}

TEST_CASE("card resolves light and dark surfaces from the same snapshot", "[card][theme]") {
    const auto design = theme::default_design_system();
    const auto light = theme::resolve_card(design, theme::ColorAppearance::light);
    const auto dark = theme::resolve_card(design, theme::ColorAppearance::dark);

    REQUIRE(
        light.container.fill.oklch().light
        == Catch::Approx(design.light.surface.oklch().light)
    );
    REQUIRE(
        dark.container.fill.oklch().light
        == Catch::Approx(design.dark.surface.oklch().light)
    );
    REQUIRE(dark.container.fill.oklch().light < light.container.fill.oklch().light);
}

TEST_CASE("card override patches fields and survives a system apply", "[card][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto card = widget::Card::create();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(card);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 120.0F)) >= 1);

    const float fill_light = card->resolved_style().container.fill.oklch().light;

    card->set_override(theme::CardRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::primary),
        .metrics_padding_x = theme::ThemeScalar::literal(24.0F),
    });
    const auto overridden = card->resolved_style();
    REQUIRE(
        overridden.container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.primary.oklch().light)
    );
    REQUIRE(overridden.metrics.padding_x == Catch::Approx(24.0F));

    // 系统 apply 后 override 不冻结，仍跟随新快照重解析。
    auto design = theme::default_design_system();
    design.light.primary = theme::nan_color(0.50F, 0.20F, 30.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(card->resolved_style().container.fill.oklch().light == Catch::Approx(0.50F));
    REQUIRE(card->resolved_style().container.fill.oklch().light != Catch::Approx(fill_light));
}

TEST_CASE("card measures child plus padding and places it inside", "[card][layout]") {
    reactive::Graph graph;
    auto card = widget::Card::create();
    auto label = widget::Label::create(graph, "Content");
    scene::NanSceneTree tree;
    tree.set_root(card);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 120.0F)) >= 1);

    // 空卡片固有尺寸 = padding * 2（layout_root 会按 tight 约束拉伸 root，
    // 固有尺寸需用 loose 测量）。
    const auto padding_x = card->resolved_style().metrics.padding_x;
    const auto empty_size = card->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(empty_size.get_width() == Catch::Approx(padding_x * 2.0F));
    REQUIRE(empty_size.get_height() == Catch::Approx(padding_x * 2.0F));

    card->set_child(label);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 120.0F)) >= 1);
    const float child_width = label->measured_text_width();
    const auto sized = card->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(sized.get_width() == Catch::Approx(child_width + padding_x * 2.0F));

    // 子节点位于内边距内（左上角偏移 = padding）。
    REQUIRE(label->position().get_x() == Catch::Approx(padding_x));
    REQUIRE(label->position().get_y() == Catch::Approx(padding_x));
}

TEST_CASE("card paints a rounded surface with outline", "[card][paint]") {
    auto card = widget::Card::create();
    scene::NanSceneTree tree;
    tree.set_root(card);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 120.0F)) >= 1);

    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.rounded_fills == 1);
    REQUIRE(dev.rounded_outlines == 1);
    REQUIRE(dev.shadows == 0); // 默认无阴影
}

TEST_CASE("card draws soft shadow before container when configured", "[card][shadow]") {
    auto card = widget::Card::create();
    card->set_override(theme::CardRecipeRule {
        .shadow_color = theme::ThemeColor::literal(
            foundation::NanColor::from_hex(0x000000, 0.5F)
        ),
        .shadow_offset_y = theme::ThemeScalar::literal(4.0F),
        .shadow_spread = theme::ThemeScalar::literal(8.0F),
    });
    scene::NanSceneTree tree;
    tree.set_root(card);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 120.0F)) >= 1);

    RecordingDevice dev;
    tree.draw(dev);
    REQUIRE(dev.shadows == 1);
    REQUIRE(dev.shadow_before_fill == 1);
    REQUIRE(dev.rounded_fills == 1);
}

TEST_CASE("card keeps child semantics reachable", "[card][semantics]") {
    reactive::Graph graph;
    auto card = widget::Card::create();
    auto label = widget::Label::create(graph, "Inside card");
    card->set_child(label);
    scene::NanSceneTree tree;
    tree.set_root(card);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 120.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(label->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::static_text);
    REQUIRE(node->properties.label == "Inside card");
}
