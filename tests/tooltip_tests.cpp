//
// Theme / Tooltip tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/controls.hpp"
#include "widget/tooltip.hpp"

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

TEST_CASE("tooltip resolves bubble tokens from the recipe", "[tooltip][theme]") {
    auto design = theme::default_design_system();
    design.tokens.spacing.sm = 9.0F;

    const auto style = theme::resolve_tooltip(design, theme::ColorAppearance::light);

    REQUIRE(
        style.container.fill.oklch().light == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(
        style.label.color.oklch().light == Catch::Approx(design.light.on_primary.oklch().light)
    );
    REQUIRE(style.metrics.padding_x == Catch::Approx(9.0F));
    REQUIRE(style.metrics.min_height == Catch::Approx(24.0F));
}

TEST_CASE("tooltip override survives a system apply", "[tooltip][override]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    auto tooltip = widget::Tooltip::create("Hint");
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(tooltip);
    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 40.0F)) >= 1);

    tooltip->set_override(theme::TooltipRecipeRule {
        .container_fill = theme::ThemeColor::token(theme::ColorToken::error),
    });
    REQUIRE(
        tooltip->resolved_style().container.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );

    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(tooltip->resolved_style().container.fill.oklch().light == Catch::Approx(0.60F));
}

TEST_CASE("tooltip shows after hover delay and hides on leave", "[tooltip][interaction]") {
    auto tooltip = widget::Tooltip::create("Hint");
    tooltip->set_delay(0.3F);

    scene::MouseEnterEvent enter(foundation::NanPoint {});
    tooltip->on_input(enter);
    REQUIRE_FALSE(tooltip->visible());

    tooltip->on_process(0.2F);
    REQUIRE_FALSE(tooltip->visible());

    tooltip->on_process(0.2F);
    REQUIRE(tooltip->visible());

    scene::MouseLeaveEvent leave(foundation::NanPoint {});
    tooltip->on_input(leave);
    REQUIRE_FALSE(tooltip->visible());
}

TEST_CASE("tooltip exposes tooltip semantics", "[tooltip][semantics]") {
    auto tooltip = widget::Tooltip::create("Saves your preferences");
    scene::NanSceneTree tree;
    tree.set_root(tooltip);
    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 40.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(tooltip->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::tooltip);
    REQUIRE(node->properties.label == "Saves your preferences");
}

TEST_CASE("tooltip paints bubble only when visible", "[tooltip][paint]") {
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    scene::NanSceneTree tree;
    tree.set_root(tooltip);
    REQUIRE(tree.layout_root(foundation::NanSize(160.0F, 48.0F)) >= 1);

    RecordingDevice hidden_dev;
    tree.draw(hidden_dev);
    // 未悬停：仅按钮自身（1 填充 + 1 文本）。
    REQUIRE(hidden_dev.rounded_rects == 1);
    REQUIRE(hidden_dev.text_calls == 1);

    tooltip->show();
    RecordingDevice visible_dev;
    tree.draw(visible_dev);
    // 悬停：按钮 + 气泡（各 +1）。
    REQUIRE(visible_dev.rounded_rects == 2);
    REQUIRE(visible_dev.text_calls == 2);
}

TEST_CASE("BuildContext tooltip wraps a trigger control", "[tooltip][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto trigger = ui.make<widget::Button>("Save").build();
    auto tooltip = ui.make<widget::Tooltip>("Saves preferences", trigger).build();

    REQUIRE(tooltip->text() == "Saves preferences");
    REQUIRE_FALSE(tooltip->visible());
}
