//
// Theme / Tooltip tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/scroll_view.hpp>
#include <nandina/widget/tooltip.hpp>
#include <nandina/scene/overlay_host.hpp>

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
        struct TextCall {
            std::string text;
            float size = 0.0F;
        };

        int rounded_rects = 0;
        int text_calls = 0;
        std::vector<TextCall> texts;

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
            std::string_view text,
            const foundation::NanPoint&,
            float size,
            const foundation::NanColor&
        ) override {
            ++text_calls;
            texts.push_back({.text = std::string(text), .size = size});
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

TEST_CASE("mounted tooltip presents its bubble through the overlay host", "[tooltip][overlay]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    host->set_content(tooltip);
    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    REQUIRE(host->overlay_count() == 1);
    tooltip->hide();
    REQUIRE(host->overlay_count() == 0);
}

namespace
{
    /// The single presented portal bubble, or null when none is mounted.
    auto overlay_bubble(scene::OverlayHost& host) -> scene::NanControl* {
        auto* layer = host.layer_at(1);
        auto* surface = layer != nullptr ? layer->layout_root() : nullptr;
        if (surface == nullptr || surface->child_count() == 0) {
            return nullptr;
        }
        auto* child = surface->get_child(0);
        return child != nullptr ? child->as_control() : nullptr;
    }
} // namespace

TEST_CASE("tooltip bubble escapes a clipping container", "[tooltip][overlay][clip]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    auto clip = std::make_shared<scene::NanControl>(foundation::NanSize(48.0F, 24.0F));
    clip->set_overflow(scene::ControlOverflow::clip);
    clip->add_child(tooltip);
    host->set_content(clip);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    auto* bubble = overlay_bubble(*host);
    REQUIRE(bubble != nullptr);
    // Hosting above the content layer means no content clip can reach the bubble.
    REQUIRE_FALSE(clip->is_ancestor_of(*bubble));
    REQUIRE(host->layer_at(1)->is_ancestor_of(*bubble));
}

TEST_CASE("tooltip bubble stays inside the viewport near an edge", "[tooltip][overlay][placement]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    tooltip->set_placement(widget::Tooltip::Placement::top);
    host->set_content(tooltip);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    // The trigger fills the viewport, so a `top` bubble must flip and shift back in.
    tooltip->show();
    auto* bubble = overlay_bubble(*host);
    REQUIRE(bubble != nullptr);
    const auto bounds = bubble->global_bounds();
    REQUIRE(bounds.get_left() >= 0.0F);
    REQUIRE(bounds.get_top() >= 0.0F);
    REQUIRE(bounds.get_right() <= 240.0F);
    REQUIRE(bounds.get_bottom() <= 120.0F);
}

TEST_CASE("tooltip bubble does not take pointer hits", "[tooltip][overlay][input]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    host->set_content(tooltip);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    auto* bubble = overlay_bubble(*host);
    REQUIRE(bubble != nullptr);
    const auto center = bubble->global_bounds().get_center();
    REQUIRE(bubble->global_bounds().contains_point(center));
    // Presentational only: the point falls through to the trigger underneath.
    auto* hit = tree.hit_test(center);
    REQUIRE(hit != bubble);
    REQUIRE(hit != nullptr);
}

TEST_CASE("replacing the trigger keeps a single portal", "[tooltip][overlay][lifetime]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    host->set_content(tooltip);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    REQUIRE(host->overlay_count() == 1);

    auto replacement = widget::Button::create("Discard");
    tooltip->set_trigger(replacement);
    REQUIRE(host->overlay_count() == 1);

    tooltip->set_text("Discards changes");
    REQUIRE(host->overlay_count() == 1);
}

TEST_CASE("unmounting the tooltip releases its portal", "[tooltip][overlay][lifetime]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    auto holder = std::make_shared<scene::NanControl>(foundation::NanSize(80.0F, 32.0F));
    holder->add_child(tooltip);
    host->set_content(holder);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    REQUIRE(host->overlay_count() == 1);

    // The bubble lives in the overlay layer, so unmounting must close it explicitly.
    (void)holder->remove_child(*tooltip);
    REQUIRE(host->overlay_count() == 0);
}

TEST_CASE(
    "BuildContext tooltip portals through the window overlay service",
    "[tooltip][overlay][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<scene::NanControl>(foundation::NanSize(120.0F, 40.0F)));
    // Lay the portal out in its own tree so the injected service, not the ancestor
    // lookup, is what makes this tooltip present a bubble.
    scene::NanSceneTree host_tree;
    host_tree.set_root(host);
    REQUIRE(host_tree.layout_root(foundation::NanSize(120.0F, 40.0F)) >= 1);

    widget::BuildContext ui {graph, scope, themes, nullptr, host.get()};

    auto trigger = ui.make<widget::Button>("Save").build();
    auto tooltip = ui.make<widget::Tooltip>("Saves preferences", trigger).build();

    // A plain control root: the ancestor fallback finds no OverlayHost, so only the
    // injected BuildContext service can present the bubble.
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(120.0F, 40.0F));
    root->add_child(tooltip);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(120.0F, 40.0F)) >= 1);

    tooltip->show();
    REQUIRE(host->overlay_count() == 1);
    tooltip->hide();
    REQUIRE(host->overlay_count() == 0);
}

TEST_CASE("tooltip bubble follows a moving trigger", "[tooltip][overlay][placement]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    host->set_content(tooltip);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    auto* bubble = overlay_bubble(*host);
    REQUIRE(bubble != nullptr);
    const auto before = bubble->global_bounds();

    // A trigger that moves (scroll / re-layout) must drag the bubble with it even
    // though the bubble is not re-shaped every frame.
    trigger->set_position(foundation::NanPoint(0.0F, 48.0F));
    tooltip->on_process(0.016F);

    const auto after = bubble->global_bounds();
    REQUIRE_FALSE(before == after);
    REQUIRE(after.get_top() > before.get_top());
}

TEST_CASE("portal tooltip bubble renders the resolved label style", "[tooltip][overlay][paint]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    // A distinctive size so a bubble that forgot the resolved TextStyle is caught.
    tooltip->set_override(theme::TooltipRecipeRule {
        .label_font_size = theme::ThemeScalar::literal(13.0F),
    });
    host->set_content(tooltip);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    RecordingDevice device;
    tree.draw(device);

    // Content paints first; the portal bubble is the last text draw.
    REQUIRE(device.text_calls == 2);
    REQUIRE(device.texts.back().text == "Saves preferences");
    REQUIRE(device.texts.back().size == Catch::Approx(13.0F));
}

namespace
{
    /// Mounts a tooltip so it keeps the trigger's natural size at `position`,
    /// instead of stretching to the whole content layer.
    struct PositionedTooltipHarness {
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<widget::Button> trigger = widget::Button::create("Save");
        std::shared_ptr<widget::Tooltip> tooltip =
            widget::Tooltip::create("Saves preferences", trigger);
        std::shared_ptr<scene::NanControl> content;
        scene::NanSceneTree tree;
        foundation::NanSize viewport;

        explicit PositionedTooltipHarness(
            foundation::NanPoint position,
            foundation::NanSize viewport_size = foundation::NanSize(240.0F, 240.0F)
        ):
            content(std::make_shared<scene::NanControl>(viewport_size)),
            viewport(viewport_size) {
            tooltip->set_position(position);
            content->add_child(tooltip);
            // A second child stops NanControl::on_layout from stretching the only
            // child to the whole container, which would hide placement behaviour.
            content->add_child(
                std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F))
            );
            host->set_content(content);
            tree.set_root(host);
            (void)tree.layout_root(viewport);
        }
    };
} // namespace

TEST_CASE("tooltip inside a clipped scrolling viewport still portals", "[tooltip][overlay][clip]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    auto scroll = widget::ScrollView::create(widget::ScrollAxis::vertical);
    scroll->set_child(tooltip);
    host->set_content(scroll);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    auto* bubble = overlay_bubble(*host);
    REQUIRE(bubble != nullptr);
    // ScrollView clips its own child, but the bubble is hosted above the content.
    REQUIRE_FALSE(scroll->is_ancestor_of(*bubble));
    REQUIRE(host->layer_at(1)->is_ancestor_of(*bubble));
}

TEST_CASE("tooltip places its bubble above or below the trigger", "[tooltip][overlay][placement]") {
    PositionedTooltipHarness harness {foundation::NanPoint(0.0F, 100.0F)};
    const auto trigger_bounds = harness.trigger->global_bounds();
    REQUIRE(trigger_bounds.is_valid());
    // Room above and below so neither placement has to flip.
    REQUIRE(trigger_bounds.get_top() > 0.0F);
    REQUIRE(trigger_bounds.get_bottom() < harness.viewport.get_height());

    harness.tooltip->set_placement(widget::Tooltip::Placement::bottom);
    harness.tooltip->show();
    auto* bubble = overlay_bubble(*harness.host);
    REQUIRE(bubble != nullptr);
    REQUIRE(bubble->global_bounds().get_top() >= trigger_bounds.get_bottom());

    // Switching placement while visible re-anchors without leaking a portal.
    harness.tooltip->set_placement(widget::Tooltip::Placement::top);
    REQUIRE(harness.host->overlay_count() == 1);
    REQUIRE(bubble->global_bounds().get_bottom() <= trigger_bounds.get_top());
}

TEST_CASE("tooltip leaves the rest of the page interactive", "[tooltip][overlay][input]") {
    auto host = scene::OverlayHost::create();
    auto trigger = widget::Button::create("Save");
    auto tooltip = widget::Tooltip::create("Saves preferences", trigger);
    tooltip->set_position(foundation::NanPoint(0.0F, 0.0F));
    auto other = widget::Button::create("Other");
    other->set_position(foundation::NanPoint(0.0F, 80.0F));
    auto content = std::make_shared<scene::NanControl>(foundation::NanSize(240.0F, 120.0F));
    content->add_child(tooltip);
    content->add_child(other);
    host->set_content(content);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 120.0F)) >= 1);

    tooltip->show();
    REQUIRE(host->overlay_count() == 1);
    // Only a tooltip is presented: the overlay layer stays pass-through and the
    // control below still receives the hit.
    REQUIRE(host->layer_at(1)->input_mode() == scene::LayerInputMode::pass);
    auto* hit = tree.hit_test(other->global_bounds().get_center());
    REQUIRE(hit != nullptr);
    REQUIRE_FALSE(host->layer_at(1)->is_ancestor_of(*hit));
}
