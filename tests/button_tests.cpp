//
// Theme / primitives / Button tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/nan_style.hpp"
#include "theme/style_document.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/text_field.hpp"
#include "widget/primitives/pressable.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using namespace nandina;

    class RecordingDevice final: public render::IRenderDevice {
    public:
        struct RectCall {
            foundation::NanRect rect;
            float alpha = 0.0F;
            bool outline = false;
            bool rounded = false;
        };
        struct TextCall {
            std::string text;
            float font_size = 0.0F;
            float alpha = 0.0F;
        };
        struct RippleCall {
            foundation::NanPoint center;
            float circle_radius = 0.0F;
            foundation::NanRect clip_rect;
            float clip_radius = 0.0F;
            float alpha = 0.0F;
        };

        std::vector<RectCall> rects;
        std::vector<TextCall> texts;
        std::vector<RippleCall> ripples;
        std::vector<std::string> operations;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}

        void draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) override {
            rects.push_back({.rect = rect, .alpha = color.alpha(), .outline = false, .rounded = false});
            operations.emplace_back("fill");
        }

        void draw_rect_outline(const foundation::NanRect& rect, float, const foundation::NanColor& color) override {
            rects.push_back({.rect = rect, .alpha = color.alpha(), .outline = true, .rounded = false});
            operations.emplace_back("outline");
        }

        void draw_rounded_rect(const foundation::NanRect& rect, float, const foundation::NanColor& color) override {
            rects.push_back({.rect = rect, .alpha = color.alpha(), .outline = false, .rounded = true});
            operations.emplace_back("fill");
        }

        void draw_line(const foundation::NanPoint&, const foundation::NanPoint&, float, const foundation::NanColor&) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}

        void draw_circle_clipped_rounded_rect(
            const foundation::NanPoint& center,
            float circle_radius,
            const foundation::NanRect& clip_rect,
            float clip_radius,
            const foundation::NanColor& color
        ) override {
            ripples.push_back({center, circle_radius, clip_rect, clip_radius, color.alpha()});
            operations.emplace_back("ripple");
        }

        void draw_text(
            std::string_view text,
            const foundation::NanPoint&,
            float font_size,
            const foundation::NanColor& color
        ) override {
            texts.push_back({.text = std::string(text), .font_size = font_size, .alpha = color.alpha()});
            operations.emplace_back("text");
        }
    };

    class TestPressable final: public widget::primitives::Pressable {
    public:
        explicit TestPressable(foundation::NanSize size): Pressable(size) {}
        int state_changes = 0;

    protected:
        void on_pressable_state_changed() override {
            ++state_changes;
        }
    };

} // namespace

TEST_CASE("button resolver keeps tone and treatment orthogonal", "[theme][button]") {
    const auto system = theme::default_design_system();
    constexpr auto appearance = theme::ColorAppearance::light;

    const auto filled = theme::resolve_button(
        system,
        appearance,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(filled.container.fill.alpha() == Catch::Approx(1.0F));
    REQUIRE(filled.container.border_width == Catch::Approx(0.0F));

    const auto outlined = theme::resolve_button(
        system,
        appearance,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::outlined,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(outlined.container.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(outlined.container.border_width == Catch::Approx(system.tokens.border.thin));

    const auto danger = theme::button_accent(system.light, theme::ButtonTone::danger);
    REQUIRE(danger.first.alpha() == Catch::Approx(1.0F));
    REQUIRE(danger.second.alpha() == Catch::Approx(1.0F));
}

TEST_CASE("button recipe resolves and overrides ripple feedback", "[theme][button][ripple]") {
    const auto system = theme::default_design_system();
    auto resolved = theme::resolve_button(
        system,
        theme::ColorAppearance::light,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(resolved.ripple.color.alpha() == Catch::Approx(0.20F));
    REQUIRE(resolved.ripple.duration == Catch::Approx(system.tokens.motion.medium_duration));

    theme::ButtonRecipeRule rule;
    rule.ripple_color = theme::ThemeColor::literal(
        theme::nan_color(0.60F, 0.10F, 120.0F, 0.35F)
    );
    rule.ripple_duration = theme::ThemeScalar::literal(0.75F);
    theme::apply_rule(
        system,
        theme::ColorAppearance::light,
        resolved,
        rule,
        theme::ButtonTone::primary
    );
    REQUIRE(resolved.ripple.color.alpha() == Catch::Approx(0.35F));
    REQUIRE(resolved.ripple.duration == Catch::Approx(0.75F));
}

TEST_CASE("NanStyle rules bridge into the design system and resolve", "[theme][style]") {
    auto style = std::make_shared<theme::NanStyle>();
    theme::ButtonStyleRule rule;
    rule.selector.treatment = theme::ButtonTreatment::ghost;
    rule.background = theme::ThemeColor::token(theme::ColorToken::primary);
    rule.radius = theme::ThemeScalar::literal(13.0F);
    rule.padding_x = theme::ThemeScalar::token(theme::ScalarToken::spacing_xl);
    style->add_button_rule(std::move(rule));

    theme::ThemeManager manager;
    manager.set_style(style);

    auto first = theme::default_theme();
    first.palette.primary = theme::nan_color(0.42F, 0.1F, 120.0F);
    first.tokens.spacing.xl = 31.0F;
    manager.set_theme(first);
    const auto first_result = theme::resolve_button(
        manager.design_system(),
        manager.appearance(),
        theme::ButtonTone::primary,
        theme::ButtonTreatment::ghost,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );

    auto second = first;
    second.palette.primary = theme::nan_color(0.78F, 0.1F, 120.0F);
    second.tokens.spacing.xl = 37.0F;
    manager.set_theme(second);
    const auto second_result = theme::resolve_button(
        manager.design_system(),
        manager.appearance(),
        theme::ButtonTone::primary,
        theme::ButtonTreatment::ghost,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );

    REQUIRE(first_result.container.fill.oklch().light == Catch::Approx(0.42F));
    REQUIRE(second_result.container.fill.oklch().light == Catch::Approx(0.78F));
    REQUIRE(first_result.metrics.padding_x == Catch::Approx(31.0F));
    REQUIRE(second_result.metrics.padding_x == Catch::Approx(37.0F));
    REQUIRE(first_result.container.radius == Catch::Approx(13.0F));
    REQUIRE(second_result.container.radius == Catch::Approx(13.0F));
}

TEST_CASE("ThemeManager switches attached widget trees by revision", "[theme][manager]") {
    theme::ThemeManager manager;
    auto light = theme::default_theme();
    light.palette.primary = theme::nan_color(0.72F, 0.12F, 120.0F);
    auto dark = theme::default_theme();
    dark.palette.primary = theme::nan_color(0.38F, 0.12F, 120.0F);
    REQUIRE(manager.register_theme("light", light));
    REQUIRE(manager.register_theme("dark", dark));

    auto style = std::make_shared<theme::NanStyle>();
    theme::ButtonStyleRule rule;
    rule.selector.treatment = theme::ButtonTreatment::ghost;
    rule.background = theme::ThemeColor::token(theme::ColorToken::primary);
    style->add_button_rule(std::move(rule));
    manager.set_style(style);
    REQUIRE(manager.activate("light"));

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Switch");
    button->set_treatment(theme::ButtonTreatment::ghost);
    tree.set_root(button);

    const auto before = manager.revision();
    (void)tree.layout_root(foundation::NanSize(320.0F, 120.0F));
    REQUIRE_FALSE(button->layout_dirty());
    REQUIRE(button->resolved_style().container.fill.oklch().light == Catch::Approx(0.72F));
    REQUIRE(manager.activate("dark"));
    REQUIRE(manager.revision() == before + 1);
    REQUIRE(button->layout_dirty());
    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.38F));
    REQUIRE(button->resolved_style().container.fill.oklch().light == Catch::Approx(0.38F));
}

TEST_CASE("ThemeManager resolves family variants from appearance preference", "[theme][manager]") {
    theme::ThemeManager manager;
    auto light = theme::default_theme();
    light.palette.primary = theme::nan_color(0.82F, 0.08F, 250.0F);
    auto dark = theme::default_theme();
    dark.palette.primary = theme::nan_color(0.42F, 0.08F, 250.0F);

    REQUIRE(manager.register_theme("ocean-light", light));
    REQUIRE(manager.register_theme("ocean-dark", dark));
    REQUIRE(manager.register_family("ocean", "ocean-light", "ocean-dark"));
    REQUIRE(manager.activate_family("ocean"));
    REQUIRE(manager.active_family() == "ocean");
    REQUIRE(manager.active_name() == "ocean-light");

    const auto light_revision = manager.revision();
    manager.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(manager.active_name() == "ocean-dark");
    REQUIRE(manager.revision() == light_revision + 1);

    const auto dark_revision = manager.revision();
    manager.set_preference(theme::ThemePreference::dark);
    REQUIRE(manager.revision() == dark_revision);
    manager.set_system_appearance(theme::ColorAppearance::light);
    REQUIRE(manager.active_name() == "ocean-dark");
    REQUIRE(manager.revision() == dark_revision);

    manager.set_preference(theme::ThemePreference::light);
    REQUIRE(manager.active_name() == "ocean-light");
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.82F));
}

TEST_CASE("reference color scales expose stable shade indices", "[theme][palette]") {
    theme::NanColorScale scale;
    scale.at(theme::ColorShade::shade_50) = theme::nan_color(0.95F, 0.02F, 240.0F);
    scale.at(theme::ColorShade::shade_500) = theme::nan_color(0.62F, 0.12F, 240.0F);
    scale.at(theme::ColorShade::shade_950) = theme::nan_color(0.18F, 0.04F, 240.0F);

    REQUIRE(scale.at(theme::ColorShade::shade_50).oklch().light == Catch::Approx(0.95F));
    REQUIRE(scale.at(theme::ColorShade::shade_500).oklch().light == Catch::Approx(0.62F));
    REQUIRE(scale.at(theme::ColorShade::shade_950).oklch().light == Catch::Approx(0.18F));
}

TEST_CASE("TextField rules compose focused and invalid states", "[theme][text-field]") {
    theme::ThemeManager manager;
    auto style = std::make_shared<theme::NanStyle>();
    theme::TextFieldStyleRule focused;
    focused.state = theme::TextFieldVisualState::focused;
    focused.height = theme::ThemeScalar::literal(46.0F);
    style->add_text_field_rule(std::move(focused));
    theme::TextFieldStyleRule invalid;
    invalid.state = theme::TextFieldVisualState::invalid;
    invalid.border_color = theme::ThemeColor::token(theme::ColorToken::error);
    style->add_text_field_rule(std::move(invalid));
    manager.set_style(style);

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto field = std::make_shared<widget::TextField>("value");
    tree.set_root(field);
    tree.set_focus(field.get());
    field->set_invalid(true);

    REQUIRE(theme::has_text_field_state(
        field->visual_state(), theme::TextFieldVisualState::focused
    ));
    REQUIRE(theme::has_text_field_state(
        field->visual_state(), theme::TextFieldVisualState::invalid
    ));
    REQUIRE(field->resolved_style().metrics.height == Catch::Approx(46.0F));
    REQUIRE(
        field->resolved_style().container.border.oklch().light
        == Catch::Approx(manager.theme().palette.error.oklch().light)
    );
    REQUIRE(field->resolved_style().focus.width > 0.0F);
}

TEST_CASE("SceneTree releases a destroyed ThemeManager", "[theme][manager][lifetime]") {
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Lifetime");
    tree.set_root(button);
    {
        theme::ThemeManager manager;
        tree.set_theme_manager(manager);
        REQUIRE(tree.theme_manager() == &manager);
    }
    REQUIRE(tree.theme_manager() == nullptr);
}

TEST_CASE("styles.toml compiles into the same theme runtime objects", "[theme][toml]") {
    constexpr std::string_view source = R"toml(
active_theme = "dark"

[themes.dark.palette]
primary = [0.31, 0.12, 250.0, 1.0]

[themes.dark.tokens.spacing]
xl = 33.0

[[styles.button]]
treatment = "ghost"
background = "$palette.primary"
radius = 12.0
padding_x = "$tokens.spacing.xl"

[[styles.text_field]]
state = "focused"
border_color = "$palette.primary"
height = 44.0

[[fonts.family]]
name = "families/application-ui"
default = true
fallbacks = ["families/fallback-ui"]
faces = [{ resource = "fonts/application-ui/medium", weight = 500, slant = "normal" }]

[[fonts.family]]
name = "families/fallback-ui"
faces = [{ resource = "fonts/fallback-ui/regular", weight = 400 }]
)toml";

    auto document = theme::parse_style_document(source);
    REQUIRE(document.has_value());
    REQUIRE(document->themes.size() == 1);
    REQUIRE(document->font_families.size() == 2);
    REQUIRE(document->style->button_rules().size() == 1);
    REQUIRE(document->style->text_field_rules().size() == 1);

    theme::ThemeManager manager;
    text::FontFamilyRegistry fonts;
    const auto applied = document->apply(manager, &fonts);
    REQUIRE(applied.has_value());
    REQUIRE(manager.active_name() == "dark");

    const auto resolved = theme::resolve_button(
        manager.design_system(),
        manager.appearance(),
        theme::ButtonTone::primary,
        theme::ButtonTreatment::ghost,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(resolved.container.fill.oklch().light == Catch::Approx(0.31F));
    REQUIRE(resolved.metrics.padding_x == Catch::Approx(33.0F));
    REQUIRE(resolved.container.radius == Catch::Approx(12.0F));

    const auto field_style = theme::resolve_text_field(
        manager.design_system(), manager.appearance(), theme::TextFieldVisualState::focused
    );
    REQUIRE(field_style.container.border.oklch().light == Catch::Approx(0.31F));
    REQUIRE(field_style.metrics.height == Catch::Approx(44.0F));

    const auto invalid = theme::parse_style_document(R"toml(
[[styles.button]]
background = "$palette.missing"
)toml");
    REQUIRE_FALSE(invalid.has_value());
}

TEST_CASE("styles.toml resolves reference palettes and appearance-aware families", "[theme][toml]") {
    constexpr std::string_view source = R"toml(
active_family = "ocean"
appearance = "dark"

[palettes.ocean.primary]
"50" = [0.98, 0.01, 250.0]
"100" = [0.93, 0.02, 250.0]
"200" = [0.87, 0.04, 250.0]
"300" = [0.80, 0.06, 250.0]
"400" = [0.72, 0.09, 250.0]
"500" = [0.64, 0.12, 250.0]
"600" = [0.56, 0.12, 250.0]
"700" = [0.48, 0.11, 250.0]
"800" = [0.39, 0.09, 250.0]
"900" = [0.30, 0.07, 250.0]
"950" = [0.21, 0.05, 250.0]

[themes.ocean-light.palette]
primary = "$palettes.ocean.primary.500"

[themes.ocean-dark.palette]
primary = "$palettes.ocean.primary.300"

[theme_families.ocean]
light = "ocean-light"
dark = "ocean-dark"
)toml";

    const auto document = theme::parse_style_document(source);
    REQUIRE(document.has_value());
    REQUIRE(document->reference_palettes.size() == 1);
    REQUIRE(document->theme_families.size() == 1);

    theme::ThemeManager manager;
    REQUIRE(document->apply(manager).has_value());
    REQUIRE(manager.active_family() == "ocean");
    REQUIRE(manager.preference() == theme::ThemePreference::dark);
    REQUIRE(manager.active_name() == "ocean-dark");
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.80F));

    const auto missing_stop = theme::parse_style_document(R"toml(
[palettes.incomplete.primary]
"50" = [0.9, 0.1, 250.0]
)toml");
    REQUIRE_FALSE(missing_stop.has_value());
}

TEST_CASE("Button instance theme and StyleContext keep their cascade priority", "[theme][cascade]") {
    theme::ThemeManager manager;
    auto application_theme = theme::default_theme();
    application_theme.palette.primary = theme::nan_color(0.35F, 0.1F, 40.0F);
    manager.set_theme(application_theme);

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Fixed");
    auto instance_theme = theme::default_theme();
    instance_theme.palette.primary = theme::nan_color(0.81F, 0.1F, 40.0F);
    button->set_theme(instance_theme);

    theme::StyleContext context;
    context.font_size = theme::StyleValue<float>::explicit_value(29.0F);
    context.text_color = theme::StyleValue<foundation::NanColor>::explicit_value(
        theme::nan_color(0.66F, 0.08F, 180.0F, 0.4F)
    );
    button->set_style_context(context);
    tree.set_root(button);

    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.81F));
    REQUIRE(button->resolved_style().container.fill.oklch().light == Catch::Approx(0.81F));
    REQUIRE(button->text_node().font_size() == Catch::Approx(29.0F));
    REQUIRE(button->text_node().color().oklch().light == Catch::Approx(0.66F));
    REQUIRE(button->text_node().color().alpha() == Catch::Approx(0.4F));

    auto replacement = application_theme;
    replacement.palette.primary = theme::nan_color(0.22F, 0.1F, 40.0F);
    manager.set_theme(replacement);
    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.81F));
}

TEST_CASE("pressable tracks hover press and emits click", "[widget][primitive][pressable]") {
    TestPressable pressable {foundation::NanSize(100.0F, 40.0F)};
    int clicks = 0;
    pressable.set_on_click([&] { ++clicks; });

    scene::MouseEnterEvent enter {foundation::NanPoint(2.0F, 2.0F)};
    REQUIRE_FALSE(pressable.on_input(enter));
    REQUIRE(pressable.hovered());

    scene::MouseButtonEvent down {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(2.0F, 2.0F)
    };
    REQUIRE(pressable.on_input(down));
    REQUIRE(pressable.pressed());

    scene::MouseButtonEvent up {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(2.0F, 2.0F)
    };
    REQUIRE(pressable.on_input(up));
    REQUIRE_FALSE(pressable.pressed());
    REQUIRE(clicks == 1);
    REQUIRE(pressable.state_changes >= 2);
}

TEST_CASE("button draws background and text and reacts to press state", "[widget][button]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Open");
    button->set_position(foundation::NanPoint(10.0F, 20.0F));
    int clicks = 0;
    button->set_on_click([&] { ++clicks; });
    tree.set_root(button);

    tree.draw(dev);
    REQUIRE(dev.rects.size() == 1);
    REQUIRE(dev.rects[0].rounded);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts[0].text == "Open");

    tree.dispatch_mouse_move(scene::MouseMoveEvent {foundation::NanPoint(20.0F, 30.0F), foundation::NanPoint::zero()});
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(20.0F, 30.0F)
    });
    REQUIRE(button->pressed());
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(20.0F, 30.0F)
    });
    REQUIRE(clicks == 1);
}

TEST_CASE("button paints state feedback as an overlay between base and content", "[widget][button][state-layer]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Overlay");
    tree.set_root(button);

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(20.0F, 20.0F), foundation::NanPoint::zero()
    });
    REQUIRE(button->hovered());
    tree.draw(dev);

    const auto style = button->resolved_style();
    REQUIRE(dev.rects.size() == 2);
    REQUIRE(dev.rects[0].alpha == Catch::Approx(style.container.fill.alpha()));
    REQUIRE(dev.rects[1].alpha == Catch::Approx(style.state_layer.hover.alpha()));
    REQUIRE(dev.operations == std::vector<std::string> {"fill", "fill", "text"});
}

TEST_CASE(
    "button ripple expands from the pointer and stays below content",
    "[widget][button][ripple]"
) {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Ripple");
    button->set_position(foundation::NanPoint(10.0F, 20.0F));
    tree.set_root(button);

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(24.0F, 32.0F), foundation::NanPoint::zero()
    });
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(24.0F, 32.0F)
    });
    REQUIRE(button->ripple_active());
    REQUIRE(button->ripple_progress() == Catch::Approx(0.0F));

    const auto style = button->resolved_style();
    tree.process(style.ripple.duration * 0.5F);
    REQUIRE(button->ripple_progress() == Catch::Approx(0.5F));
    tree.draw(dev);

    REQUIRE(dev.ripples.size() == 1);
    REQUIRE(dev.ripples[0].center.get_x() == Catch::Approx(24.0F));
    REQUIRE(dev.ripples[0].center.get_y() == Catch::Approx(32.0F));
    REQUIRE(dev.ripples[0].clip_rect.get_left() == Catch::Approx(10.0F));
    REQUIRE(dev.ripples[0].clip_rect.get_top() == Catch::Approx(20.0F));
    const float far_x = std::max(
        std::abs(dev.ripples[0].center.get_x() - dev.ripples[0].clip_rect.get_left()),
        std::abs(dev.ripples[0].clip_rect.get_right() - dev.ripples[0].center.get_x())
    );
    const float far_y = std::max(
        std::abs(dev.ripples[0].center.get_y() - dev.ripples[0].clip_rect.get_top()),
        std::abs(dev.ripples[0].clip_rect.get_bottom() - dev.ripples[0].center.get_y())
    );
    REQUIRE(
        dev.ripples[0].circle_radius == Catch::Approx(std::hypot(far_x, far_y) * 0.875F)
    );
    REQUIRE(dev.ripples[0].alpha == Catch::Approx(style.ripple.color.alpha() * 0.5F));
    REQUIRE(dev.ripples[0].clip_radius == Catch::Approx(style.container.radius));
    REQUIRE(
        dev.operations
        == std::vector<std::string> {"fill", "fill", "ripple", "text", "outline"}
    );

    tree.process(style.ripple.duration);
    REQUIRE_FALSE(button->ripple_active());
    REQUIRE(button->ripple_progress() == Catch::Approx(1.0F));
}

TEST_CASE(
    "button cancels ripple for reduced motion and disabled state",
    "[widget][button][ripple]"
) {
    theme::ThemeManager manager;
    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Motion");
    tree.set_root(button);

    const auto press = [&tree] {
        tree.dispatch_mouse_move(scene::MouseMoveEvent {
            foundation::NanPoint(12.0F, 12.0F), foundation::NanPoint::zero()
        });
        tree.dispatch_mouse_button(scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(12.0F, 12.0F)
        });
    };

    press();
    REQUIRE(button->ripple_active());
    manager.set_motion_preference(theme::MotionPreference::reduced);
    REQUIRE_FALSE(button->ripple_active());

    press();
    REQUIRE_FALSE(button->ripple_active());
    manager.set_motion_preference(theme::MotionPreference::full);
    scene::MouseButtonEvent release {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(12.0F, 12.0F)
    };
    tree.dispatch_mouse_button(release);
    press();
    REQUIRE(button->ripple_active());
    button->set_disabled(true);
    REQUIRE_FALSE(button->ripple_active());
}

TEST_CASE("button keeps outline and focus ring above the state overlay", "[widget][button][state-layer]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Outlined");
    button->set_treatment(theme::ButtonTreatment::outlined);
    tree.set_root(button);
    tree.set_focus(button.get());
    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(20.0F, 20.0F), foundation::NanPoint::zero()
    });

    tree.draw(dev);

    // 透明 base 不提交；状态层 → 控件描边 → 文本 → 焦点环。
    REQUIRE(
        dev.operations
        == std::vector<std::string> {"fill", "outline", "text", "outline"}
    );
    REQUIRE(dev.rects[0].alpha == Catch::Approx(button->resolved_style().state_layer.hover.alpha()));
    REQUIRE(dev.rects[1].outline);
    REQUIRE(dev.rects[2].outline);
}

TEST_CASE("button forwards text state through its text primitive", "[widget][button][text]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("A very long button label");
    button->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 72.0F, button->height()));
    tree.set_root(button);

    tree.draw(dev);

    REQUIRE(
        button->text_node().font_size() == Catch::Approx(button->resolved_style().label.font_size)
    );
    REQUIRE(button->text_node().layout_result().overflowed);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts[0].text.ends_with("..."));
}

TEST_CASE("button font size supports logical and percentage values", "[widget][button][text]") {
    // 逻辑字号：显式值优先于主题与继承上下文。
    widget::Button logical("Logical");
    logical.set_font_size(20.0F);
    (void)logical.measure_layout(scene::LayoutConstraints {
        .max_width = 400.0F,
        .max_height = 80.0F,
    });
    REQUIRE(logical.text_node().font_size() == Catch::Approx(20.0F));
    REQUIRE(logical.font_size() == Catch::Approx(20.0F));

    // 百分比字号：相对按钮自身最终高度解析，因此跟随窗口一起缩放。
    widget::Button proportional("Proportional");
    proportional.set_width(scene::percent(50.0F)).set_height(scene::percent(50.0F));
    proportional.set_font_size(scene::percent(45.0F));
    const auto size = proportional.measure_layout(scene::LayoutConstraints {
        .max_width = 400.0F,
        .max_height = 200.0F,
    });
    proportional.layout_to(
        foundation::NanRect::from_origin_size(foundation::NanPoint::zero(), size)
    );
    REQUIRE(size.get_height() == Catch::Approx(100.0F));
    REQUIRE(proportional.text_node().font_size() == Catch::Approx(45.0F));

    // 显式值覆盖百分比；清除后回退主题配方。
    proportional.set_font_size(30.0F);
    (void)proportional.measure_layout(scene::LayoutConstraints {
        .max_width = 400.0F,
        .max_height = 200.0F,
    });
    REQUIRE(proportional.text_node().font_size() == Catch::Approx(30.0F));

    proportional.clear_font_size();
    (void)proportional.measure_layout(scene::LayoutConstraints {
        .max_width = 400.0F,
        .max_height = 200.0F,
    });
    REQUIRE(
        proportional.text_node().font_size()
        == Catch::Approx(proportional.resolved_style().label.font_size)
    );
}

TEST_CASE("button visual instances outrank inherited text context", "[widget][button][style]") {
    widget::Button button("Instance");
    theme::StyleContext context;
    context.font_size = theme::StyleValue<float>::explicit_value(31.0F);
    context.text_color = theme::StyleValue<foundation::NanColor>::explicit_value(
        foundation::NanColor::from_oklch(0.35F, 0.08F, 240.0F)
    );
    button.set_style_context(context);

    const auto instance_color = foundation::NanColor::from_oklch(0.78F, 0.12F, 30.0F);
    widget::property::write(button, widget::visual::label.color, instance_color);
    widget::property::write(button, widget::visual::label.font_size, 19.0F);
    (void)button.measure_layout(scene::LayoutConstraints::loose());

    REQUIRE(button.text_node().color().approx_equals(instance_color));
    REQUIRE(button.text_node().font_size() == Catch::Approx(19.0F));

    button.clear_font_size();
    (void)button.measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(button.text_node().font_size() == Catch::Approx(31.0F));

    button.set_font_size(18.0F);
    (void)button.measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(button.text_node().font_size() == Catch::Approx(18.0F));
}

TEST_CASE("button applies font-only StyleContext changes", "[widget][button][style]") {
    widget::Button button("Font");
    auto context = theme::StyleContext {};
    auto first = text::FontRequest {};
    first.family = resource::ResourceKey::parse("families/first");
    context.font = theme::StyleValue<text::FontRequest>::explicit_value(first);
    button.set_style_context(context);
    REQUIRE(button.text_node().font() == first);

    auto second = first;
    second.family = resource::ResourceKey::parse("families/second");
    context.font = theme::StyleValue<text::FontRequest>::explicit_value(second);
    button.set_style_context(context);
    REQUIRE(button.text_node().font() == second);
}

TEST_CASE("button overrides refresh detached metrics immediately", "[widget][button][style]") {
    widget::Button button("Metrics");
    const float before = button.width();
    theme::ButtonRecipeRule rule;
    rule.metrics_padding_x = theme::ThemeScalar::literal(48.0F);
    button.set_override(std::move(rule));

    REQUIRE(button.resolved_style().metrics.padding_x == Catch::Approx(48.0F));
    REQUIRE(button.width() > before);
}

TEST_CASE("button interaction state invalidates layout paint and semantics", "[widget][button]") {
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("State");
    tree.set_root(button);
    (void)tree.layout_root(foundation::NanSize(240.0F, 80.0F));
    button->clear_dirty(
        scene::DirtyFlags::paint | scene::DirtyFlags::layout
        | scene::DirtyFlags::semantics
    );

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(4.0F, 4.0F), foundation::NanPoint::zero()
    });

    REQUIRE(button->is_dirty(scene::DirtyFlags::paint));
    REQUIRE(button->is_dirty(scene::DirtyFlags::layout));
    REQUIRE(button->is_dirty(scene::DirtyFlags::semantics));
}

TEST_CASE("text rejects invalid font sizes through both public paths", "[widget][text]") {
    widget::primitives::Text text("Invalid");
    REQUIRE_THROWS_AS(text.set_font_size(0.0F), std::invalid_argument);
    REQUIRE_THROWS_AS(
        widget::property::write(text, widget::visual::label.font_size, -1.0F),
        std::invalid_argument
    );
}

TEST_CASE("button text overflow controls its internal text primitive", "[widget][button][text]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("A very long button label");
    button->set_text_overflow(widget::primitives::TextOverflow::clip);
    button->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 72.0F, button->height()));
    tree.set_root(button);

    tree.draw(dev);

    REQUIRE(button->text_overflow() == widget::primitives::TextOverflow::clip);
    REQUIRE(button->text_node().overflow() == widget::primitives::TextOverflow::clip);
    REQUIRE(button->text_node().layout_result().overflowed);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE_FALSE(dev.texts[0].text.ends_with("..."));
}

TEST_CASE("semantic actions activate and focus buttons", "[widget][button][semantics]") {
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Save");
    int activations = 0;
    button->set_on_click([&] { ++activations; });
    tree.set_root(button);

    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(button->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::button);
    REQUIRE(node->properties.label == "Save");
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::activate));
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::focus));

    REQUIRE(tree.perform_semantics_action(
        button->semantics_id(), {.action = semantics::Action::activate}
    ));
    REQUIRE(activations == 1);
    REQUIRE(tree.perform_semantics_action(
        button->semantics_id(), {.action = semantics::Action::focus}
    ));
    REQUIRE(tree.focused_node() == button.get());

    REQUIRE(tree.update_semantics());
    node = tree.semantics_tree().find(button->semantics_id());
    REQUIRE(node->properties.state.focused);

    button->set_disabled(true);
    REQUIRE(tree.update_semantics());
    node = tree.semantics_tree().find(button->semantics_id());
    REQUIRE(node->properties.state.disabled);
    REQUIRE(node->properties.actions == semantics::Action::none);
    REQUIRE_FALSE(tree.perform_semantics_action(
        button->semantics_id(), {.action = semantics::Action::activate}
    ));
}

TEST_CASE("text field semantics expose value and editable state", "[widget][text-field][semantics]") {
    scene::NanSceneTree tree;
    auto field = std::make_shared<widget::TextField>("draft", "Task title");
    tree.set_root(field);

    REQUIRE(tree.update_semantics());
    const auto* node = tree.semantics_tree().find(field->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::text_field);
    REQUIRE(node->properties.label == "Task title");
    REQUIRE(node->properties.value == "draft");
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::set_value));

    REQUIRE(tree.perform_semantics_action(
        field->semantics_id(),
        {.action = semantics::Action::set_value, .value = "review"}
    ));
    REQUIRE(field->value() == "review");

    field->set_invalid(true);
    field->set_read_only(true);
    REQUIRE(tree.update_semantics());
    node = tree.semantics_tree().find(field->semantics_id());
    REQUIRE(node->properties.value == "review");
    REQUIRE(node->properties.state.invalid);
    REQUIRE(node->properties.state.read_only);
    REQUIRE_FALSE(semantics::supports(node->properties.actions, semantics::Action::set_value));
}
