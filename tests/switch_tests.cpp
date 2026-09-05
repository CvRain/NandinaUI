//
// Theme / Switch tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/build_context.hpp"
#include "widget/controls.hpp"
#include "widget/switch.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        struct RectCall {
            foundation::NanRect rect;
            bool outline = false;
        };

        std::vector<RectCall> rects;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect& r, const foundation::NanColor&) override {
            rects.push_back({.rect = r, .outline = false});
        }
        void draw_rect_outline(
            const foundation::NanRect& r,
            float,
            const foundation::NanColor&
        ) override {
            rects.push_back({.rect = r, .outline = true});
        }
        void draw_rounded_rect(
            const foundation::NanRect& r,
            float,
            const foundation::NanColor&
        ) override {
            rects.push_back({.rect = r, .outline = false});
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

TEST_CASE("switch style resolves semantic theme tokens", "[switch][theme]") {
    auto design = theme::default_design_system();
    design.light.primary = theme::nan_color(0.62F, 0.16F, 250.0F);
    design.tokens.spacing.sm = 11.0F;

    const auto checked = theme::resolve_switch(
        design,
        theme::ColorAppearance::light,
        /*checked=*/true,
        theme::SwitchVisualState::normal
    );
    const auto unchecked = theme::resolve_switch(
        design,
        theme::ColorAppearance::light,
        /*checked=*/false,
        theme::SwitchVisualState::normal
    );

    // 勾选：primary 轨道 + surface 拇指（亮色下近白）。
    REQUIRE(checked.track.fill.oklch().light == Catch::Approx(0.62F));
    REQUIRE(checked.thumb.fill.oklch().light == Catch::Approx(design.light.surface.oklch().light));
    // 未勾选：outline_variant 轨道 + surface 拇指。
    REQUIRE(
        unchecked.track.fill.oklch().light
        == Catch::Approx(design.light.outline_variant.oklch().light)
    );
    REQUIRE(unchecked.thumb.fill.oklch().light == Catch::Approx(design.light.surface.oklch().light));
    REQUIRE(unchecked.metrics.gap == Catch::Approx(11.0F));
    // pill 全圆角轨道。
    REQUIRE(checked.track.radius == Catch::Approx(design.tokens.radius.full));
}

TEST_CASE("switch activation toggles value and semantic state", "[switch][semantics]") {
    auto switch_control = std::make_shared<widget::Switch>("Enable sync");
    bool observed = false;
    auto subscription =
        switch_control->checked_changed().subscribe([&](const bool value) { observed = value; });
    scene::NanSceneTree tree;
    tree.set_root(switch_control);
    REQUIRE(tree.layout_root(foundation::NanSize(280.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* before = tree.semantics_tree().find(switch_control->semantics_id());
    REQUIRE(before != nullptr);
    REQUIRE(before->properties.role == semantics::Role::switch_control);
    REQUIRE(before->properties.state.checked == false);
    REQUIRE(tree.perform_semantics_action(
        switch_control->semantics_id(),
        {.action = semantics::Action::activate}
    ));

    REQUIRE(switch_control->checked());
    REQUIRE(observed);
    REQUIRE(tree.update_semantics());
    const auto* after = tree.semantics_tree().find(switch_control->semantics_id());
    REQUIRE(after != nullptr);
    REQUIRE(after->properties.state.checked == true);
}

TEST_CASE("switch supports keyboard activation and disabled state", "[switch][input]") {
    auto switch_control = std::make_shared<widget::Switch>("Keyboard option");
    scene::NanSceneTree tree;
    tree.set_root(switch_control);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    tree.set_focus(switch_control.get());
    tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press));
    REQUIRE(switch_control->checked());

    switch_control->set_disabled(true);
    tree.dispatch_key(scene::KeyEvent(32, scene::KeyEvent::Action::press));
    REQUIRE(switch_control->checked());
    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(switch_control->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.state.disabled);
    REQUIRE(node->properties.actions == semantics::Action::none);
}

TEST_CASE("BuildContext switch synchronizes a boolean signal", "[switch][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    auto& enabled = ui.signal<bool>(false);
    auto switch_control = ui.make<widget::Switch>(enabled, "Enable sync").build();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(switch_control);
    REQUIRE(tree.layout_root(foundation::NanSize(240.0F, 48.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.perform_semantics_action(
        switch_control->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(enabled.peek());

    enabled.set(false);
    REQUIRE_FALSE(switch_control->checked());
}

TEST_CASE("switch builder forwards checked and change modifiers", "[switch][authoring]") {
    int changes = 0;
    bool observed = true;
    auto switch_control = widget::authoring::make<widget::Switch>("Builder option")
                              .checked(true)
                              .on_change([&](const bool value) {
                                  ++changes;
                                  observed = value;
                              })
                              .build();

    REQUIRE(switch_control->checked());
    switch_control->toggle();
    REQUIRE_FALSE(switch_control->checked());
    REQUIRE(changes == 1);
    REQUIRE_FALSE(observed);
}

TEST_CASE("switch draws track and thumb with checked-dependent position", "[switch][painter]") {
    const float track_height = 24.0F;
    const float track_width = 40.0F;
    const float thumb_size = 16.0F;
    const float thumb_padding = (track_height - thumb_size) * 0.5F;

    const auto draw = [&](const bool checked) {
        RecordingDevice dev;
        scene::NanSceneTree tree;
        auto switch_control = std::make_shared<widget::Switch>("Toggle", checked);
        tree.set_root(switch_control);
        REQUIRE(tree.layout_root(foundation::NanSize(160.0F, 48.0F)) >= 1);
        tree.draw(dev);
        return dev;
    };

    // 未勾选：拇指贴轨道左内壁。
    const auto unchecked_dev = draw(false);
    const auto track_top = (48.0F - track_height) * 0.5F;
    const auto track = foundation::NanRect::from_xywh(0.0F, track_top, track_width, track_height);
    const auto thumb_left_unchecked = foundation::NanRect::from_xywh(
        thumb_padding,
        track_top + thumb_padding,
        thumb_size,
        thumb_size
    );
    bool track_found = false;
    bool thumb_left_found = false;
    for (const auto& call: unchecked_dev.rects) {
        if (!call.outline && call.rect == track) {
            track_found = true;
        }
        if (!call.outline && call.rect == thumb_left_unchecked) {
            thumb_left_found = true;
        }
    }
    REQUIRE(track_found);
    REQUIRE(thumb_left_found);

    // 勾选：拇指贴轨道右内壁。
    const auto checked_dev = draw(true);
    const auto thumb_left_checked = foundation::NanRect::from_xywh(
        track_width - thumb_padding - thumb_size,
        track_top + thumb_padding,
        thumb_size,
        thumb_size
    );
    bool thumb_right_found = false;
    for (const auto& call: checked_dev.rects) {
        if (!call.outline && call.rect == thumb_left_checked) {
            thumb_right_found = true;
        }
    }
    REQUIRE(thumb_right_found);
}
