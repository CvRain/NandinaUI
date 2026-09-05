//
// Built-in theme family tests.
//

#include "foundation/contrast.hpp"
#include "theme/builtin_themes.hpp"
#include "theme/theme_manager.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace nandina;

TEST_CASE("default theme families include butter fluent and material", "[theme][families]") {
    const auto families = theme::default_theme_families();
    REQUIRE(families.size() == 3);
    REQUIRE(families[0].name == "butter");
    REQUIRE(families[1].name == "fluent");
    REQUIRE(families[2].name == "material");
}

TEST_CASE("butter family compiles to a warm cream design system", "[theme][families]") {
    const auto families = theme::default_theme_families();
    const auto design = theme::build_family_design_system(families[0]);

    // 卡片风格 tokens：圆角 12/16/24、边框 2。
    REQUIRE(design.tokens.radius.sm == Catch::Approx(12.0F));
    REQUIRE(design.tokens.radius.md == Catch::Approx(16.0F));
    REQUIRE(design.tokens.radius.lg == Catch::Approx(24.0F));
    REQUIRE(design.tokens.border.thin == Catch::Approx(2.0F));

    // 亮暗中性翻转：暗底更暗、暗文字更亮。
    REQUIRE(design.dark.background.oklch().light < design.light.background.oklch().light);
    REQUIRE(design.dark.on_background.oklch().light > design.light.on_background.oklch().light);

    // 暗色品牌提升到更亮档位（dark_brand = shade_300）。
    REQUIRE(design.dark.primary.oklch().light > design.light.primary.oklch().light);
}

TEST_CASE(
    "every built-in family keeps AA text contrast in both appearances",
    "[theme][families][contrast]"
) {
    for (const auto& family: theme::default_theme_families()) {
        CAPTURE(family.name);
        const auto design = theme::build_family_design_system(family);

        const auto require_aa = [](const theme::NanColorScheme& scheme) {
            REQUIRE(
                foundation::nan_contrast_ratio(scheme.primary, scheme.on_primary)
                >= foundation::nan_contrast_aa_text
            );
            REQUIRE(
                foundation::nan_contrast_ratio(scheme.surface, scheme.on_surface)
                >= foundation::nan_contrast_aa_text
            );
            REQUIRE(
                foundation::nan_contrast_ratio(scheme.background, scheme.on_background)
                >= foundation::nan_contrast_aa_text
            );
        };
        require_aa(design.light);
        require_aa(design.dark);
    }
}

TEST_CASE(
    "fluent family compiles to a flat blue design system with flipped on-brand",
    "[theme][families]"
) {
    const auto families = theme::default_theme_families();
    const auto design = theme::build_family_design_system(families[1]);

    // Fluent 锐角：圆角 4/8/12。
    REQUIRE(design.tokens.radius.sm == Catch::Approx(4.0F));
    REQUIRE(design.tokens.radius.md == Catch::Approx(8.0F));
    REQUIRE(design.tokens.radius.lg == Catch::Approx(12.0F));

    // 蓝色 accent 亮暗翻转：暗色品牌更亮。
    REQUIRE(design.dark.primary.oklch().light > design.light.primary.oklch().light);
    // on-primary 随外观翻转：亮色 accent 配浅字，暗色 accent 配深字。
    REQUIRE(design.light.on_primary.oklch().light > design.light.primary.oklch().light);
    REQUIRE(design.dark.on_primary.oklch().light < design.dark.primary.oklch().light);
}

TEST_CASE("material family compiles to a rounded purple design system", "[theme][families]") {
    const auto families = theme::default_theme_families();
    const auto design = theme::build_family_design_system(families[2]);

    // Material 圆角：圆角 8/12/16。
    REQUIRE(design.tokens.radius.sm == Catch::Approx(8.0F));
    REQUIRE(design.tokens.radius.md == Catch::Approx(12.0F));
    REQUIRE(design.tokens.radius.lg == Catch::Approx(16.0F));

    REQUIRE(design.dark.primary.oklch().light > design.light.primary.oklch().light);
    REQUIRE(design.light.on_primary.oklch().light > design.light.primary.oklch().light);
    REQUIRE(design.dark.on_primary.oklch().light < design.dark.primary.oklch().light);

    // 精确对齐 M3 baseline 语义色（skill 文档 color-system.md）。
    REQUIRE(design.light.primary == foundation::NanColor::from_hex(0x6750A4));
    REQUIRE(design.dark.primary == foundation::NanColor::from_hex(0xD0BCFF));
    REQUIRE(design.dark.on_primary == foundation::NanColor::from_hex(0x381E72));
    REQUIRE(design.light.outline == foundation::NanColor::from_hex(0x79747E));
    REQUIRE(design.dark.outline == foundation::NanColor::from_hex(0x938F99));
    REQUIRE(design.light.on_surface_variant == foundation::NanColor::from_hex(0x49454F));
    REQUIRE(design.light.outline_variant == foundation::NanColor::from_hex(0xCAC4D0));
}

TEST_CASE(
    "fluent and material stay flat while butter keeps its soft card shadow",
    "[theme][families][card]"
) {
    const auto families = theme::default_theme_families();
    const auto fluent = theme::build_family_design_system(families[1]);
    const auto material = theme::build_family_design_system(families[2]);

    REQUIRE(
        theme::resolve_card(fluent, theme::ColorAppearance::light).shadow.spread
        == Catch::Approx(0.0F)
    );
    REQUIRE(
        theme::resolve_card(material, theme::ColorAppearance::light).shadow.spread
        == Catch::Approx(0.0F)
    );
}

TEST_CASE(
    "register_theme_family activates a full snapshot and flips appearance",
    "[theme][families]"
) {
    theme::ThemeManager themes;
    theme::register_default_theme_families(themes);
    REQUIRE(themes.contains_family("butter"));

    REQUIRE(themes.activate_family("butter"));
    REQUIRE(themes.active_family() == "butter");

    const float light_primary = themes.design_system().light.primary.oklch().light;

    themes.set_preference(theme::ThemePreference::dark);
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);
    REQUIRE(themes.active_family() == "butter"); // 族不变，仅翻转外观
    REQUIRE(themes.design_system().dark.primary.oklch().light > light_primary);
}

TEST_CASE(
    "butter family gives cards a soft shadow while default stays flat",
    "[theme][families][card]"
) {
    const auto families = theme::default_theme_families();
    const auto butter = theme::build_family_design_system(families[0]);
    const auto default_system = theme::default_design_system();

    const auto butter_card = theme::resolve_card(butter, theme::ColorAppearance::light);
    const auto flat_card = theme::resolve_card(default_system, theme::ColorAppearance::light);

    REQUIRE(butter_card.shadow.spread > 0.0F);
    REQUIRE(butter_card.shadow.offset_y == Catch::Approx(4.0F));
    REQUIRE(butter_card.shadow.color.alpha() > 0.0F);

    REQUIRE(flat_card.shadow.spread == Catch::Approx(0.0F));
    REQUIRE(flat_card.shadow.color.alpha() == Catch::Approx(0.0F));
}

TEST_CASE("legacy named families coexist with full-snapshot families", "[theme][families]") {
    theme::ThemeManager themes;
    auto light_theme = theme::default_theme();
    auto dark_theme = theme::default_theme();
    dark_theme.palette = theme::default_dark_palette();
    REQUIRE(themes.register_theme("legacy-light", light_theme));
    REQUIRE(themes.register_theme("legacy-dark", dark_theme));
    REQUIRE(themes.register_family("legacy", "legacy-light", "legacy-dark"));

    theme::register_default_theme_families(themes);
    REQUIRE(themes.activate_family("butter"));
    REQUIRE(themes.active_family() == "butter");

    REQUIRE(themes.activate_family("legacy"));
    REQUIRE(themes.active_family() == "legacy");
    themes.set_preference(theme::ThemePreference::dark);
    REQUIRE(themes.appearance() == theme::ColorAppearance::dark);
}
