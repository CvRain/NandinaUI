//
// Theme DesignSystem / shared-fragment tests.
//

#include "foundation/geometry.hpp"
#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/design_system.hpp"
#include "theme/theme.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/slider.hpp"
#include "widget/text_field.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using namespace nandina;

    void require_same_color(
        const foundation::NanColor& actual,
        const foundation::NanColor& expected
    ) {
        const auto actual_oklch = actual.oklch();
        const auto expected_oklch = expected.oklch();
        REQUIRE(actual_oklch.light == Catch::Approx(expected_oklch.light));
        REQUIRE(actual_oklch.chroma == Catch::Approx(expected_oklch.chroma));
        REQUIRE(actual_oklch.hue == Catch::Approx(expected_oklch.hue));
        REQUIRE(actual_oklch.alpha == Catch::Approx(expected_oklch.alpha));
    }

    void require_same_scheme(
        const theme::NanColorScheme& actual,
        const theme::NanColorScheme& expected
    ) {
        require_same_color(actual.background, expected.background);
        require_same_color(actual.on_background, expected.on_background);
        require_same_color(actual.primary, expected.primary);
        require_same_color(actual.on_primary, expected.on_primary);
        require_same_color(actual.secondary, expected.secondary);
        require_same_color(actual.on_secondary, expected.on_secondary);
        require_same_color(actual.tertiary, expected.tertiary);
        require_same_color(actual.on_tertiary, expected.on_tertiary);
        require_same_color(actual.surface, expected.surface);
        require_same_color(actual.on_surface, expected.on_surface);
        require_same_color(actual.surface_variant, expected.surface_variant);
        require_same_color(actual.on_surface_variant, expected.on_surface_variant);
        require_same_color(actual.outline, expected.outline);
        require_same_color(actual.outline_variant, expected.outline_variant);
        require_same_color(actual.success, expected.success);
        require_same_color(actual.on_success, expected.on_success);
        require_same_color(actual.warning, expected.warning);
        require_same_color(actual.on_warning, expected.on_warning);
        require_same_color(actual.error, expected.error);
        require_same_color(actual.on_error, expected.on_error);
        require_same_color(actual.focus_ring, expected.focus_ring);
        require_same_color(actual.selection, expected.selection);
    }

    /** revision 观察探针：统计 on_theme_revision_changed 回调次数。 */
    class RevisionProbe final: public theme::ThemeObserver {
    public:
        std::uint64_t changes = 0;
        theme::ColorAppearance last_appearance = theme::ColorAppearance::light;
        bool last_reduced_motion = false;

        void on_theme_revision_changed(const theme::ThemeManager& manager) override {
            ++changes;
            last_appearance = manager.appearance();
            last_reduced_motion = manager.reduced_motion();
        }
        void on_theme_manager_destroyed(const theme::ThemeManager& /*manager*/) noexcept override {}
    };

    /** 最小绘制录制设备：记录 rect / outline 调用。 */
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
        void draw_rect_outline(const foundation::NanRect& r, float, const foundation::NanColor&) override {
            rects.push_back({.rect = r, .outline = true});
        }
        void draw_rounded_rect(const foundation::NanRect& r, float, const foundation::NanColor&) override {
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

TEST_CASE("design system resolves fragment tokens against the active palette", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    const auto box = theme::resolve(
        system,
        theme::ColorAppearance::light,
        system.components.button.base.container
    );
    // base 容器：surface 透明 + radius.sm（treatment 规则在此之上覆盖）
    REQUIRE(box.fill.oklch().light == Catch::Approx(system.light.surface.oklch().light));
    REQUIRE(box.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(box.radius == Catch::Approx(system.tokens.radius.sm));

    const auto focus = theme::resolve(
        system,
        theme::ColorAppearance::light,
        system.components.button.base.focus
    );
    REQUIRE(focus.width == Catch::Approx(system.tokens.border.focus_ring));
}

TEST_CASE("resolve_button produces recipe-driven values for filled medium normal", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    constexpr auto appearance = theme::ColorAppearance::light;
    const auto resolved = theme::resolve_button(
        system,
        appearance,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    // filled：accent 实心 + 反色文本
    REQUIRE(resolved.container.fill.oklch().light == Catch::Approx(system.light.primary.oklch().light));
    REQUIRE(resolved.container.border_width == Catch::Approx(0.0F));
    REQUIRE(resolved.label.color.oklch().light == Catch::Approx(system.light.on_primary.oklch().light));
    // medium：40 高 / spacing.md 内边距 / label_md 字号
    REQUIRE(resolved.metrics.height == Catch::Approx(40.0F));
    REQUIRE(resolved.metrics.padding_x == Catch::Approx(system.tokens.spacing.md));
    REQUIRE(resolved.label.font_size == Catch::Approx(system.tokens.typography.label_md));
    // 焦点环始终开启（与遗留语义一致）
    REQUIRE(resolved.focus.width == Catch::Approx(system.tokens.border.focus_ring));
}

TEST_CASE("design system button rules overlay the base resolution by selector", "[theme][design-system]") {
    auto system = theme::default_design_system();
    theme::ButtonRecipeRule rule;
    rule.selector.state = theme::ButtonVisualState::hovered;
    rule.container_radius = theme::ThemeScalar::literal(99.0F);
    rule.label_color = theme::ThemeColor::token(theme::ColorToken::error);
    system.components.button.rules.push_back(rule);

    const auto hovered = theme::resolve_button(
        system,
        theme::ColorAppearance::light,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::hovered
    );
    REQUIRE(hovered.container.radius == Catch::Approx(99.0F));
    REQUIRE(hovered.label.color.oklch().light == Catch::Approx(system.light.error.oklch().light));

    const auto normal = theme::resolve_button(
        system,
        theme::ColorAppearance::light,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(normal.container.radius != Catch::Approx(99.0F));
}

TEST_CASE("button state layer is recipe data and can be overridden", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    constexpr auto appearance = theme::ColorAppearance::light;

    // filled 保留 accent 基础填充，并把 on_accent + hover alpha 解析为独立叠加色。
    const auto hovered = theme::resolve_button(
        system,
        appearance,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::hovered
    );
    const auto accent = system.light.primary;
    const auto on_accent = system.light.on_primary;
    REQUIRE(hovered.container.fill.oklch().light == Catch::Approx(accent.oklch().light));
    REQUIRE(hovered.state_layer.hover.oklch().light == Catch::Approx(on_accent.oklch().light));
    REQUIRE(hovered.state_layer.hover.alpha() == Catch::Approx(system.tokens.opacity.hover_overlay));
    REQUIRE(
        theme::button_state_layer_color(hovered, theme::ButtonVisualState::hovered).alpha()
        == Catch::Approx(system.tokens.opacity.hover_overlay)
    );
    // normal / disabled 不选择状态层，基础填充始终保持 treatment 的 accent 实心。
    const auto normal = theme::resolve_button(
        system,
        appearance,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(normal.container.fill.oklch().light == Catch::Approx(accent.oklch().light));
    REQUIRE(theme::button_state_layer_color(normal, theme::ButtonVisualState::normal).alpha() == Catch::Approx(0.0F));

    // 品牌主题可覆盖某个 treatment 的 hover 状态层，无需改解析器。
    auto branded = theme::default_design_system();
    theme::ButtonRecipeRule override_rule;
    override_rule.selector.treatment = theme::ButtonTreatment::filled;
    override_rule.state_layer_hover = theme::ThemeColor::token(theme::ColorToken::error);
    branded.components.button.rules.push_back(override_rule);
    const auto branded_hovered = theme::resolve_button(
        branded,
        appearance,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::hovered
    );
    REQUIRE(branded_hovered.container.fill.oklch().light == Catch::Approx(branded.light.primary.oklch().light));
    REQUIRE(branded_hovered.state_layer.hover.oklch().light == Catch::Approx(branded.light.error.oklch().light));
}

TEST_CASE("resolve_checkbox and resolve_slider produce composed fragments", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    constexpr auto appearance = theme::ColorAppearance::light;

    const auto checkbox = theme::resolve_checkbox(
        system,
        appearance,
        /*checked=*/true,
        theme::CheckboxVisualState::normal
    );
    REQUIRE(
        checkbox.indicator.fill.oklch().light == Catch::Approx(system.light.primary.oklch().light)
    );
    REQUIRE(checkbox.metrics.box_size == Catch::Approx(20.0F));
    REQUIRE(checkbox.metrics.gap == Catch::Approx(system.tokens.spacing.sm));

    const auto slider = theme::resolve_slider(system, appearance, theme::SliderVisualState::normal);
    REQUIRE(
        slider.active_track.box.fill.oklch().light
        == Catch::Approx(system.light.primary.oklch().light)
    );
    REQUIRE(slider.active_track.thickness == Catch::Approx(4.0F));
    REQUIRE(slider.metrics.min_height == Catch::Approx(32.0F));
}

TEST_CASE("ThemeManager apply publishes exactly one revision", "[theme][manager][atomic]") {
    // probe 先于 manager 构造，保证 manager 析构时观察者仍存活。
    RevisionProbe probe;
    theme::ThemeManager manager;
    manager.add_observer(probe);
    const auto before = manager.revision();

    auto system = theme::default_design_system();
    system.light.primary = theme::nan_color(0.51F, 0.10F, 200.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(system)));

    REQUIRE(manager.revision() == before + 1);
    REQUIRE(probe.changes == 1);
    REQUIRE(manager.design_system().light.primary.oklch().light == Catch::Approx(0.51F));
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.51F));
}

TEST_CASE("motion preference resolves system and explicit reduced motion", "[theme][motion]") {
    theme::ThemeManager manager;
    RevisionProbe probe;
    manager.add_observer(probe);

    REQUIRE(manager.motion_preference() == theme::MotionPreference::system);
    REQUIRE_FALSE(manager.reduced_motion());

    manager.set_system_reduced_motion(true);
    REQUIRE(manager.reduced_motion());
    REQUIRE(probe.changes == 1);

    manager.set_motion_preference(theme::MotionPreference::full);
    REQUIRE_FALSE(manager.reduced_motion());
    REQUIRE(probe.changes == 2);

    manager.set_system_reduced_motion(false);
    REQUIRE_FALSE(manager.reduced_motion());
    REQUIRE(probe.changes == 2);

    manager.set_motion_preference(theme::MotionPreference::reduced);
    REQUIRE(manager.reduced_motion());
    REQUIRE(probe.changes == 3);

    manager.set_motion_preference(theme::MotionPreference::system);
    REQUIRE_FALSE(manager.reduced_motion());
    REQUIRE(probe.changes == 4);

    manager.remove_observer(probe);
}

TEST_CASE(
    "system preference snapshot updates appearance and motion atomically",
    "[theme][manager][atomic][platform]"
) {
    theme::ThemeManager manager;
    RevisionProbe probe;
    manager.add_observer(probe);
    const theme::SystemPreferences default_preferences {
        .appearance = theme::ColorAppearance::light,
        .reduced_motion = false,
    };

    REQUIRE(manager.system_preferences() == default_preferences);
    REQUIRE(manager.appearance() == theme::ColorAppearance::light);
    REQUIRE_FALSE(manager.reduced_motion());

    manager.set_system_preferences({
        .appearance = theme::ColorAppearance::dark,
        .reduced_motion = true,
    });

    REQUIRE(manager.appearance() == theme::ColorAppearance::dark);
    REQUIRE(manager.reduced_motion());
    REQUIRE(probe.changes == 1);
    REQUIRE(probe.last_appearance == theme::ColorAppearance::dark);
    REQUIRE(probe.last_reduced_motion);

    manager.set_system_preferences(manager.system_preferences());
    REQUIRE(probe.changes == 1);

    manager.remove_observer(probe);
}

TEST_CASE(
    "explicit preferences mask platform snapshot revisions",
    "[theme][manager][platform]"
) {
    theme::ThemeManager manager;
    manager.set_preference(theme::ThemePreference::light);
    manager.set_motion_preference(theme::MotionPreference::full);
    RevisionProbe probe;
    manager.add_observer(probe);
    const theme::SystemPreferences platform_preferences {
        .appearance = theme::ColorAppearance::dark,
        .reduced_motion = true,
    };

    manager.set_system_preferences(platform_preferences);

    REQUIRE(manager.system_preferences() == platform_preferences);
    REQUIRE(manager.appearance() == theme::ColorAppearance::light);
    REQUIRE_FALSE(manager.reduced_motion());
    REQUIRE(probe.changes == 0);

    manager.set_preference(theme::ThemePreference::system);
    REQUIRE(manager.appearance() == theme::ColorAppearance::dark);
    REQUIRE(probe.changes == 1);

    manager.set_motion_preference(theme::MotionPreference::system);
    REQUIRE(manager.reduced_motion());
    REQUIRE(probe.changes == 2);

    manager.remove_observer(probe);
}

TEST_CASE(
    "equivalent explicit appearance does not publish a revision",
    "[theme][manager][appearance]"
) {
    theme::ThemeManager manager;
    RevisionProbe probe;
    manager.add_observer(probe);

    manager.set_preference(theme::ThemePreference::light);

    REQUIRE(manager.preference() == theme::ThemePreference::light);
    REQUIRE(manager.appearance() == theme::ColorAppearance::light);
    REQUIRE(probe.changes == 0);

    manager.remove_observer(probe);
}

TEST_CASE("default design system defines reusable motion durations", "[theme][motion]") {
    const auto system = theme::default_design_system();

    REQUIRE(system.tokens.motion.short_duration == Catch::Approx(0.12F));
    REQUIRE(system.tokens.motion.medium_duration == Catch::Approx(0.20F));
    REQUIRE(system.tokens.motion.long_duration == Catch::Approx(0.32F));
}

TEST_CASE("one DesignSystem carries light and dark palettes switched by preference", "[theme][manager][appearance]") {
    theme::ThemeManager manager;
    auto system = theme::default_design_system();
    system.light.primary = theme::nan_color(0.82F, 0.08F, 250.0F);
    system.dark.primary = theme::nan_color(0.38F, 0.08F, 250.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(system)));

    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.82F));
    manager.set_preference(theme::ThemePreference::dark);
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.38F));
    manager.set_preference(theme::ThemePreference::light);
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.82F));
}

TEST_CASE("default light/dark palettes keep on_* contrast and flip neutrals", "[theme][palette]") {
    const auto light = theme::default_light_palette();
    const auto dark = theme::default_dark_palette();

    // 每对 on_*/底色需拉开明度（|ΔL| ≥ 0.25），防止可读性退化。
    const auto check_pairs = [](const theme::NanColorScheme& scheme) {
        const auto diff = [](const foundation::NanColor& a, const foundation::NanColor& b) {
            return std::abs(a.oklch().light - b.oklch().light);
        };
        REQUIRE(diff(scheme.background, scheme.on_background) >= 0.25F);
        REQUIRE(diff(scheme.surface, scheme.on_surface) >= 0.25F);
        REQUIRE(diff(scheme.surface_variant, scheme.on_surface_variant) >= 0.25F);
        REQUIRE(diff(scheme.primary, scheme.on_primary) >= 0.25F);
        REQUIRE(diff(scheme.secondary, scheme.on_secondary) >= 0.25F);
        REQUIRE(diff(scheme.tertiary, scheme.on_tertiary) >= 0.25F);
        REQUIRE(diff(scheme.success, scheme.on_success) >= 0.25F);
        REQUIRE(diff(scheme.warning, scheme.on_warning) >= 0.25F);
        REQUIRE(diff(scheme.error, scheme.on_error) >= 0.25F);
    };
    check_pairs(light);
    check_pairs(dark);

    // 中性色两模式显著翻转：亮=浅底深字，暗=深底浅字。
    REQUIRE(light.background.oklch().light > dark.background.oklch().light + 0.5F);
    REQUIRE(light.on_surface.oklch().light < dark.on_surface.oklch().light - 0.5F);
    // 品牌色两模式同值（对齐 Skeleton brand 语义）。
    REQUIRE(light.primary.oklch().light == Catch::Approx(dark.primary.oklch().light));
}

TEST_CASE("default reference palette provides seven ordered eleven-stop scales", "[theme][palette][reference]") {
    const auto reference = theme::default_reference_palette();
    const std::array<const theme::NanColorScale*, 7> scales {
        &reference.primary,
        &reference.secondary,
        &reference.tertiary,
        &reference.neutral,
        &reference.success,
        &reference.warning,
        &reference.error,
    };

    for (const auto* scale : scales) {
        STATIC_REQUIRE(theme::NanColorScale::stop_count == 11);
        for (std::size_t index = 1; index < scale->stops.size(); ++index) {
            CAPTURE(index);
            REQUIRE(scale->stops[index - 1].oklch().light > scale->stops[index].oklch().light);
        }
    }
}

TEST_CASE("semantic palette generator maps reference tones by appearance", "[theme][palette][generator]") {
    const auto reference = theme::default_reference_palette();
    const auto light = theme::make_color_scheme(reference, theme::ColorAppearance::light);
    const auto dark = theme::make_color_scheme(reference, theme::ColorAppearance::dark);

    require_same_color(light.background, reference.neutral.at(theme::ColorShade::shade_50));
    require_same_color(light.surface, reference.neutral.at(theme::ColorShade::shade_100));
    require_same_color(light.surface_variant, reference.neutral.at(theme::ColorShade::shade_200));
    require_same_color(light.outline, reference.neutral.at(theme::ColorShade::shade_500));
    require_same_color(light.on_surface_variant, reference.neutral.at(theme::ColorShade::shade_700));

    require_same_color(dark.background, reference.neutral.at(theme::ColorShade::shade_950));
    require_same_color(dark.surface, reference.neutral.at(theme::ColorShade::shade_900));
    require_same_color(dark.surface_variant, reference.neutral.at(theme::ColorShade::shade_800));
    require_same_color(dark.outline, reference.neutral.at(theme::ColorShade::shade_600));
    require_same_color(dark.on_surface_variant, reference.neutral.at(theme::ColorShade::shade_400));

    require_same_color(light.primary, reference.primary.at(theme::ColorShade::shade_500));
    require_same_color(dark.primary, reference.primary.at(theme::ColorShade::shade_500));
    require_same_color(light.on_primary, reference.primary.at(theme::ColorShade::shade_950));
    require_same_color(light.success, reference.success.at(theme::ColorShade::shade_500));
    require_same_color(light.warning, reference.warning.at(theme::ColorShade::shade_500));
    require_same_color(light.error, reference.error.at(theme::ColorShade::shade_500));
    require_same_color(light.focus_ring, light.primary);
    require_same_color(light.selection, light.primary.with_alpha(0.32F));
}

TEST_CASE("palette variant policy can lift dark brand tones without changing light", "[theme][palette][policy]") {
    const auto reference = theme::default_reference_palette();
    const auto policy = theme::PaletteVariantPolicy::material_dark_tone();
    const auto light = theme::make_color_scheme(reference, theme::ColorAppearance::light, policy);
    const auto dark = theme::make_color_scheme(reference, theme::ColorAppearance::dark, policy);

    require_same_color(light.primary, reference.primary.at(theme::ColorShade::shade_500));
    require_same_color(light.secondary, reference.secondary.at(theme::ColorShade::shade_500));
    require_same_color(light.tertiary, reference.tertiary.at(theme::ColorShade::shade_500));
    require_same_color(dark.primary, reference.primary.at(theme::ColorShade::shade_400));
    require_same_color(dark.secondary, reference.secondary.at(theme::ColorShade::shade_400));
    require_same_color(dark.tertiary, reference.tertiary.at(theme::ColorShade::shade_400));
    require_same_color(dark.focus_ring, dark.primary);
}

TEST_CASE("generated defaults preserve legacy light scheme construction", "[theme][palette][compat]") {
    const theme::NanColorScheme constructed;
    const auto light = theme::default_light_palette();
    const auto dark = theme::default_dark_palette();
    const auto reference = theme::default_reference_palette();

    require_same_scheme(constructed, light);
    require_same_scheme(
        light,
        theme::make_color_scheme(reference, theme::ColorAppearance::light)
    );
    require_same_scheme(
        dark,
        theme::make_color_scheme(reference, theme::ColorAppearance::dark)
    );

    // Phase 7 的默认品牌色与亮/暗中性色保持不变，避免升级后 example 视觉漂移。
    REQUIRE(light.primary.oklch().light == Catch::Approx(0.6803F));
    REQUIRE(light.background.oklch().light == Catch::Approx(1.0F));
    REQUIRE(dark.background.oklch().light == Catch::Approx(0.1776F));
    REQUIRE(dark.surface.oklch().light == Catch::Approx(0.2520F));
}

TEST_CASE("attached widget follows an atomic DesignSystem apply", "[theme][manager][widget]") {
    theme::ThemeManager manager;
    auto system = theme::default_design_system();
    system.light.primary = theme::nan_color(0.61F, 0.12F, 200.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(system)));

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Apply");
    tree.set_root(button);

    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.61F));
    REQUIRE(button->resolved_style().container.fill.oklch().light == Catch::Approx(0.61F));
}

TEST_CASE("legacy named themes and families still route through the design system", "[theme][manager][compat]") {
    theme::ThemeManager manager;
    auto light = theme::default_theme();
    light.palette.primary = theme::nan_color(0.72F, 0.12F, 120.0F);
    auto dark = theme::default_theme();
    dark.palette.primary = theme::nan_color(0.38F, 0.12F, 120.0F);
    REQUIRE(manager.register_theme("brand-light", light));
    REQUIRE(manager.register_theme("brand-dark", dark));
    REQUIRE(manager.register_family("brand", "brand-light", "brand-dark"));
    REQUIRE(manager.activate_family("brand"));

    REQUIRE(manager.active_name() == "brand-light");
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.72F));
    REQUIRE(
        manager.design_system().palette(theme::ColorAppearance::light).primary.oklch().light
        == Catch::Approx(0.72F)
    );

    manager.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(manager.active_name() == "brand-dark");
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.38F));
    REQUIRE(
        manager.design_system().palette(theme::ColorAppearance::dark).primary.oklch().light
        == Catch::Approx(0.38F)
    );
}

TEST_CASE("Button set_override patches fields and survives a system apply", "[theme][override]") {
    theme::ThemeManager manager;
    auto system = theme::default_design_system();
    system.light.primary = theme::nan_color(0.60F, 0.10F, 200.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(system)));

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Override");
    theme::ButtonRecipeRule rule;
    rule.container_radius = theme::ThemeScalar::literal(99.0F);
    rule.label_color = theme::ThemeColor::token(theme::ColorToken::error);
    button->set_override(std::move(rule));
    tree.set_root(button);

    // 覆盖字段生效，未覆盖字段跟随当前系统。
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(99.0F));
    REQUIRE(
        button->resolved_style().label.color.oklch().light
        == Catch::Approx(manager.theme().palette.error.oklch().light)
    );
    REQUIRE(button->resolved_style().container.fill.oklch().light == Catch::Approx(0.60F));

    // 系统切换：覆盖保留且跟随新快照重解析，未覆盖字段跟随新系统（不冻结）。
    auto next = theme::default_design_system();
    next.light.primary = theme::nan_color(0.30F, 0.10F, 200.0F);
    next.light.error = theme::nan_color(0.44F, 0.10F, 20.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(next)));

    REQUIRE(button->resolved_style().container.radius == Catch::Approx(99.0F));
    REQUIRE(button->resolved_style().label.color.oklch().light == Catch::Approx(0.44F));
    REQUIRE(button->resolved_style().container.fill.oklch().light == Catch::Approx(0.30F));
}

TEST_CASE("Slider set_override patches thumb radius", "[theme][override]") {
    auto slider = std::make_shared<widget::Slider>("Scale", 0.5F, 0.0F, 1.0F, 0.05F);
    theme::SliderRecipeRule rule;
    rule.thumb_radius = theme::ThemeScalar::literal(12.0F);
    slider->set_override(std::move(rule));
    REQUIRE(slider->resolved_style().thumb.box.radius == Catch::Approx(12.0F));
}

TEST_CASE("detached widget resolves against its fallback design system", "[theme][override]") {
    auto button = std::make_shared<widget::Button>("Detached");
    const auto fallback = theme::default_theme();
    REQUIRE(
        button->theme_ref().palette.primary.oklch().light
        == Catch::Approx(fallback.palette.primary.oklch().light)
    );
    const auto style = button->resolved_style();
    REQUIRE(
        style.container.fill.oklch().light
        == Catch::Approx(fallback.palette.primary.oklch().light)
    );
    REQUIRE(style.metrics.height == Catch::Approx(40.0F)); // medium 尺寸
}

TEST_CASE("focused button draws a normalized focus ring", "[theme][painter]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Focus");
    button->layout_to(foundation::NanRect::from_xywh(10.0F, 10.0F, 120.0F, 40.0F));
    tree.set_root(button);
    tree.set_focus(button.get());
    tree.draw(dev);

    // 唯一 outline 调用即焦点环：控件边界向外扩 (focus.width + 默认 gap 1)。
    const auto ring = foundation::NanRect::from_xywh(10.0F, 10.0F, 120.0F, 40.0F)
                          .expanded(2.0F + 1.0F);
    bool found = false;
    for (const auto& call: dev.rects) {
        if (call.outline && call.rect == ring) {
            found = true;
        }
    }
    REQUIRE(found);

    // 取消聚焦后不再绘制焦点环。
    tree.set_focus(nullptr);
    dev.rects.clear();
    tree.draw(dev);
    for (const auto& call: dev.rects) {
        REQUIRE_FALSE(call.outline);
    }
}

TEST_CASE("text field draws rounded fill and outline through BoxPainter", "[theme][painter]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto field = std::make_shared<widget::TextField>("value");
    tree.set_root(field);
    tree.draw(dev);

    bool fill = false;
    bool outline = false;
    for (const auto& call: dev.rects) {
        if (call.outline) {
            outline = true;
        }
        else {
            fill = true;
        }
    }
    REQUIRE(fill);
    REQUIRE(outline);
}

TEST_CASE("disabled state scales slider and checkbox colors by disabled opacity", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    constexpr auto appearance = theme::ColorAppearance::light;
    const auto slider =
        theme::resolve_slider(system, appearance, theme::SliderVisualState::disabled);
    REQUIRE(slider.active_track.box.fill.alpha() == Catch::Approx(system.tokens.opacity.disabled));

    const auto checkbox =
        theme::resolve_checkbox(system, appearance, /*checked=*/true, theme::CheckboxVisualState::disabled);
    REQUIRE(checkbox.indicator.fill.alpha() == Catch::Approx(system.tokens.opacity.disabled));
}
