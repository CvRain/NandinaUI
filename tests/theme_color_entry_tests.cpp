// D2 step 4b: theme author color entry — primitive palette access and explicit NanColor overrides.
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/style_document.hpp>
#include <nandina/theme/theme.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/theme/theme_scheme.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace theme = nandina::theme;
namespace foundation = nandina::foundation;

namespace
{
    using theme::ColorShade;
    using theme::ColorToken;
    using theme::ReferenceColorFamily;

    constexpr std::array<ReferenceColorFamily, 7> all_families {
        ReferenceColorFamily::primary,
        ReferenceColorFamily::secondary,
        ReferenceColorFamily::tertiary,
        ReferenceColorFamily::neutral,
        ReferenceColorFamily::success,
        ReferenceColorFamily::warning,
        ReferenceColorFamily::error,
    };

    /// Canonical semantic roles only (aliases share the same storage).
    constexpr std::array<ColorToken, 34> canonical_roles {
        ColorToken::background,
        ColorToken::foreground,
        ColorToken::card,
        ColorToken::card_foreground,
        ColorToken::popover,
        ColorToken::popover_foreground,
        ColorToken::primary,
        ColorToken::primary_foreground,
        ColorToken::secondary,
        ColorToken::secondary_foreground,
        ColorToken::muted,
        ColorToken::muted_foreground,
        ColorToken::accent,
        ColorToken::accent_foreground,
        ColorToken::destructive,
        ColorToken::destructive_foreground,
        ColorToken::border,
        ColorToken::input,
        ColorToken::ring,
        ColorToken::surface,
        ColorToken::surface_foreground,
        ColorToken::surface_variant,
        ColorToken::surface_variant_foreground,
        ColorToken::tertiary,
        ColorToken::tertiary_foreground,
        ColorToken::success,
        ColorToken::success_foreground,
        ColorToken::warning,
        ColorToken::warning_foreground,
        ColorToken::error,
        ColorToken::error_foreground,
        ColorToken::info,
        ColorToken::info_foreground,
        ColorToken::selection,
    };

    constexpr std::array<ColorShade, 11> all_shades {
        ColorShade::shade_50,
        ColorShade::shade_100,
        ColorShade::shade_200,
        ColorShade::shade_300,
        ColorShade::shade_400,
        ColorShade::shade_500,
        ColorShade::shade_600,
        ColorShade::shade_700,
        ColorShade::shade_800,
        ColorShade::shade_900,
        ColorShade::shade_950,
    };

    auto
    require_same_palette(const theme::NanColorScheme& actual, const theme::NanColorScheme& expected)
        -> void {
        for (const auto role: canonical_roles) {
            CAPTURE(static_cast<int>(role));
            CHECK(theme::semantic_color(actual, role) == theme::semantic_color(expected, role));
        }
    }

    auto require_same_reference(
        const theme::NanReferencePalette& actual,
        const theme::NanReferencePalette& expected
    ) -> void {
        for (const auto family: all_families) {
            for (const auto shade: all_shades) {
                CAPTURE(theme::reference_family_name(family));
                CAPTURE(theme::color_shade_name(shade));
                CHECK(
                    theme::reference_color(actual, family, shade)
                    == theme::reference_color(expected, family, shade)
                );
            }
        }
    }
} // namespace

TEST_CASE("primitive palette access is typed by family and shade", "[theme][color-entry][author]") {
    auto reference = theme::default_reference_palette();
    const auto expected_fields = std::array {
        std::pair {ReferenceColorFamily::primary, &reference.primary},
        std::pair {ReferenceColorFamily::secondary, &reference.secondary},
        std::pair {ReferenceColorFamily::tertiary, &reference.tertiary},
        std::pair {ReferenceColorFamily::neutral, &reference.neutral},
        std::pair {ReferenceColorFamily::success, &reference.success},
        std::pair {ReferenceColorFamily::warning, &reference.warning},
        std::pair {ReferenceColorFamily::error, &reference.error},
    };
    for (const auto& [family, field]: expected_fields) {
        CHECK(&theme::reference_scale(reference, family) == field);
        CHECK(theme::reference_family_name(family) != "unknown");
    }
    CHECK(theme::reference_family_name(ReferenceColorFamily::primary) == "primary");
    CHECK(theme::reference_family_name(ReferenceColorFamily::neutral) == "neutral");
    CHECK(theme::color_shade_name(ColorShade::shade_50) == "50");
    CHECK(theme::color_shade_name(ColorShade::shade_950) == "950");

    // Reading a shade matches the named scale, and the non-const entry writes through.
    CHECK(
        theme::reference_color(reference, ReferenceColorFamily::primary, ColorShade::shade_500)
        == reference.primary.at(ColorShade::shade_500)
    );
    const auto explicit_color = theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F);
    theme::reference_scale(reference, ReferenceColorFamily::primary).at(ColorShade::shade_600) =
        explicit_color;
    CHECK(
        theme::reference_color(reference, ReferenceColorFamily::primary, ColorShade::shade_600)
        == explicit_color
    );
}

TEST_CASE(
    "reference-derived content records its primitive palette",
    "[theme][color-entry][author]"
) {
    const auto reference = theme::default_reference_palette();
    const auto policy = theme::PaletteVariantPolicy::material_dark_tone();
    const auto content =
        theme::ThemeSchemeContent::from_reference(reference, theme::ColorAppearance::dark, policy);
    REQUIRE(content.reference.has_value());
    require_same_reference(*content.reference, reference);
    require_same_palette(
        content.palette,
        theme::make_color_scheme(reference, theme::ColorAppearance::dark, policy)
    );
    // Adapter exposes the same palette for both appearances; tokens come from the default template.
    CHECK(content.design_system().light.primary == content.palette.primary);
    CHECK(content.design_system().dark.primary == content.palette.primary);
    CHECK(content.tokens.radius.md == theme::default_design_system().tokens.radius.md);

    // Other author entries do not fabricate a primitive source.
    const auto plain = theme::ThemeSchemeContent::from(
        theme::default_design_system(),
        theme::ColorAppearance::light
    );
    CHECK_FALSE(plain.reference.has_value());
    const auto semantic = theme::ThemeSchemeContent::from_semantic_colors(
        theme::SemanticColorSpec {
            .background = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
            .foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .card = theme::NanColor::from_oklch(0.99F, 0.0F, 0.0F),
            .card_foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .popover = theme::NanColor::from_oklch(0.99F, 0.0F, 0.0F),
            .popover_foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .primary = theme::NanColor::from_oklch(0.5F, 0.2F, 30.0F),
            .primary_foreground = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
            .secondary = theme::NanColor::from_oklch(0.9F, 0.0F, 0.0F),
            .secondary_foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .muted = theme::NanColor::from_oklch(0.9F, 0.0F, 0.0F),
            .muted_foreground = theme::NanColor::from_oklch(0.3F, 0.0F, 0.0F),
            .accent = theme::NanColor::from_oklch(0.9F, 0.0F, 0.0F),
            .accent_foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .destructive = theme::NanColor::from_oklch(0.5F, 0.2F, 25.0F),
            .destructive_foreground = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
            .border = theme::NanColor::from_oklch(0.86F, 0.0F, 0.0F),
            .input = theme::NanColor::from_oklch(0.86F, 0.0F, 0.0F),
            .ring = theme::NanColor::from_oklch(0.5F, 0.2F, 30.0F),
            .surface = theme::NanColor::from_oklch(0.95F, 0.0F, 0.0F),
            .surface_foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .surface_variant = theme::NanColor::from_oklch(0.93F, 0.0F, 0.0F),
            .surface_variant_foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .tertiary = theme::NanColor::from_oklch(0.6F, 0.1F, 200.0F),
            .tertiary_foreground = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
            .success = theme::NanColor::from_oklch(0.6F, 0.1F, 150.0F),
            .success_foreground = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
            .warning = theme::NanColor::from_oklch(0.75F, 0.15F, 90.0F),
            .warning_foreground = theme::NanColor::from_oklch(0.2F, 0.0F, 0.0F),
            .error = theme::NanColor::from_oklch(0.5F, 0.2F, 25.0F),
            .error_foreground = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
            .info = theme::NanColor::from_oklch(0.6F, 0.1F, 240.0F),
            .info_foreground = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
            .selection = theme::NanColor::from_oklch(0.9F, 0.05F, 30.0F),
        }
    );
    CHECK_FALSE(semantic.reference.has_value());
    CHECK(semantic.palette.primary == theme::NanColor::from_oklch(0.5F, 0.2F, 30.0F));
    CHECK(semantic.palette.on_primary == semantic.palette.primary_foreground);
}

TEST_CASE(
    "explicit semantic colors override only the requested roles",
    "[theme][color-entry][author]"
) {
    auto palette = theme::default_light_palette();
    const auto original = theme::default_light_palette();
    const auto explicit_primary = theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F);
    REQUIRE(theme::set_semantic_color(palette, ColorToken::primary, explicit_primary));
    CHECK(theme::semantic_color(palette, ColorToken::primary) == explicit_primary);
    CHECK(
        theme::semantic_color(palette, ColorToken::primary_foreground)
        == theme::semantic_color(original, ColorToken::primary_foreground)
    );
    CHECK(theme::semantic_color(palette, ColorToken::background) == original.background);
    CHECK(theme::semantic_color(palette, ColorToken::ring) == original.ring);

    // Aliases write the canonical storage and keep the compatibility field in sync.
    const auto explicit_muted = theme::NanColor::from_oklch(0.55F, 0.05F, 90.0F);
    REQUIRE(theme::set_semantic_color(palette, ColorToken::on_muted, explicit_muted));
    CHECK(theme::semantic_color(palette, ColorToken::muted_foreground) == explicit_muted);
    CHECK(palette.on_muted == explicit_muted);

    // Alias reads resolve to the same canonical value.
    CHECK(
        theme::semantic_color(palette, ColorToken::focus_ring)
        == theme::semantic_color(palette, ColorToken::ring)
    );
    CHECK(
        theme::semantic_color(palette, ColorToken::outline)
        == theme::semantic_color(palette, ColorToken::border)
    );
    CHECK(theme::canonical_color_token(ColorToken::on_background) == ColorToken::foreground);
    CHECK(theme::canonical_color_token(ColorToken::outline_variant) == ColorToken::surface_variant);
    CHECK(theme::canonical_color_token(ColorToken::primary) == ColorToken::primary);
}

TEST_CASE("invalid semantic colors are rejected without mutation", "[theme][color-entry][author]") {
    auto palette = theme::default_light_palette();
    const auto original = theme::default_light_palette();

    const auto non_finite =
        theme::NanColor::from_oklch(std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F);
    const auto invalid = theme::set_semantic_color(palette, ColorToken::primary, non_finite);
    REQUIRE_FALSE(invalid);
    CHECK(invalid.error().kind == theme::ThemeColorErrorKind::non_finite_color);
    CHECK(invalid.error().role == ColorToken::primary);
    CHECK_FALSE(theme::describe(invalid.error()).empty());
    require_same_palette(palette, original);

    // NanColor normalizes light/hue/alpha at construction; the entry does not re-clamp and
    // records the normalized value instead of inventing a second range policy.
    const auto normalized = theme::NanColor::from_oklch(0.5F, 0.1F, 30.0F, 1.5F);
    CHECK(normalized.alpha() == 1.0F);
    REQUIRE(theme::set_semantic_color(palette, ColorToken::primary, normalized));
    CHECK(theme::semantic_color(palette, ColorToken::primary) == normalized);
    palette = original;

    // Batch safety: a later invalid entry must not leave earlier entries applied.
    const std::array overrides {
        theme::SemanticColorOverride {
            .role = ColorToken::primary,
            .color = theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F),
        },
        theme::SemanticColorOverride {
            .role = ColorToken::muted,
            .color =
                theme::NanColor::from_oklch(std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F),
        },
    };
    const auto batch = theme::apply_semantic_colors(palette, overrides);
    REQUIRE_FALSE(batch);
    CHECK(batch.error().kind == theme::ThemeColorErrorKind::non_finite_color);
    CHECK(batch.error().role == ColorToken::muted);
    require_same_palette(palette, original);

    // A fully valid batch commits every role at once.
    const std::array valid {
        theme::SemanticColorOverride {
            .role = ColorToken::primary,
            .color = theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F),
        },
        theme::SemanticColorOverride {
            .role = ColorToken::on_muted,
            .color = theme::NanColor::from_oklch(0.55F, 0.05F, 90.0F),
        },
    };
    REQUIRE(theme::apply_semantic_colors(palette, valid));
    CHECK(theme::semantic_color(palette, ColorToken::primary) == valid[0].color);
    CHECK(theme::semantic_color(palette, ColorToken::muted_foreground) == valid[1].color);
}

TEST_CASE(
    "author color entries survive derivation and become effective after registration",
    "[theme][color-entry][author][manager]"
) {
    theme::ThemeManager manager;
    const auto base = theme::ThemeSchemeContent::from(
        theme::default_design_system(),
        theme::ColorAppearance::light
    );
    auto custom = theme::derive_content(base);
    const auto explicit_primary = theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F);
    const std::array overrides {
        theme::SemanticColorOverride {
            .role = ColorToken::primary,
            .color = explicit_primary,
        },
        theme::SemanticColorOverride {
            .role = ColorToken::on_primary,
            .color = theme::NanColor::from_oklch(0.98F, 0.0F, 0.0F),
        },
    };
    REQUIRE(theme::apply_semantic_colors(custom.palette, overrides));
    // The base snapshot is independent of its derivation.
    CHECK(base.palette.primary != explicit_primary);

    const auto id = *theme::ThemeSchemeId::parse("org.test:author-colors");
    const theme::ThemeScheme scheme {
        .id = id,
        .display_name = "Author colors",
        .appearance = theme::ColorAppearance::light,
        .family = std::nullopt,
        .content = std::make_shared<const theme::ThemeSchemeContent>(custom),
    };
    REQUIRE(theme::validate_scheme(scheme, manager.catalog()));
    REQUIRE(manager.register_scheme(scheme));
    REQUIRE(manager.select(theme::FixedThemeSelection {id}));
    CHECK(manager.design_system().light.primary == explicit_primary);
    CHECK(manager.design_system().dark.primary == explicit_primary);
    // Rows that were not overridden still follow the default semantic palette.
    CHECK(manager.design_system().light.background == theme::default_light_palette().background);

    // Component recipes consume the explicit value through the normal resolver.
    const auto resolved = theme::resolve_button(
        manager.design_system(),
        theme::ColorAppearance::light,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    CHECK(resolved.container.fill == explicit_primary);

    // Deriving from a reference-derived content keeps the primitive palette source.
    const auto reference = theme::default_reference_palette();
    const auto from_reference =
        theme::ThemeSchemeContent::from_reference(reference, theme::ColorAppearance::dark);
    const auto derived_reference = theme::derive_content(from_reference);
    REQUIRE(derived_reference.reference.has_value());
    require_same_reference(*derived_reference.reference, reference);
    require_same_palette(derived_reference.palette, from_reference.palette);
}

TEST_CASE(
    "custom components can apply arbitrary literals while semantic tokens stay default",
    "[theme][color-entry][author][recipe]"
) {
    auto system = theme::default_design_system();
    const auto brand = theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F);
    theme::ButtonRecipeRule rule {
        .selector = {},
        .container_fill = theme::ThemeColor::literal(brand),
        .label_color = theme::ThemeColor::literal(theme::NanColor::from_oklch(0.99F, 0.0F, 0.0F)),
    };
    auto resolved = theme::resolve_button(
        system,
        theme::ColorAppearance::light,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    theme::apply_rule(
        system,
        theme::ColorAppearance::light,
        resolved,
        rule,
        theme::ButtonTone::primary
    );
    CHECK(resolved.container.fill == brand);

    // The design-system layer is untouched: other tones still follow semantic tokens.
    const auto neutral = theme::resolve_button(
        system,
        theme::ColorAppearance::light,
        theme::ButtonTone::neutral,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    CHECK(
        neutral.container.fill == theme::semantic_color(system.light, ColorToken::surface_variant)
    );
}

TEST_CASE(
    "file import and C++ author entry produce equal semantic palettes",
    "[theme][color-entry][author][file]"
) {
    constexpr std::string_view source = R"toml(
[palettes.brand.primary]
50 = [0.97, 0.02, 80]
100 = [0.94, 0.04, 80]
200 = [0.88, 0.08, 80]
300 = [0.80, 0.12, 80]
400 = [0.72, 0.15, 80]
500 = [0.64, 0.16, 80]
600 = [0.56, 0.14, 80]
700 = [0.46, 0.12, 80]
800 = [0.36, 0.10, 80]
900 = [0.26, 0.08, 80]
950 = [0.18, 0.05, 80]

[themes.author]
[themes.author.palette]
primary = [0.42, 0.17, 285]
primary_foreground = "$palettes.brand.primary.500"
muted = [0.93, 0.01, 285]
)toml";
    const auto document = theme::parse_style_document(source);
    REQUIRE(document);
    const auto& file_theme = document->themes.at("author");

    // C++ author path: default palette plus the same explicit overrides.
    auto expected = theme::default_light_palette();
    REQUIRE(
        theme::set_semantic_color(
            expected,
            ColorToken::primary,
            theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F)
        )
    );
    REQUIRE(
        theme::set_semantic_color(
            expected,
            ColorToken::primary_foreground,
            theme::reference_color(
                document->reference_palettes.at("brand"),
                ReferenceColorFamily::primary,
                ColorShade::shade_500
            )
        )
    );
    REQUIRE(
        theme::set_semantic_color(
            expected,
            ColorToken::muted,
            theme::NanColor::from_oklch(0.93F, 0.01F, 285.0F)
        )
    );
    require_same_palette(file_theme.palette, expected);

    // Import registers the same palette as a canonical scheme.
    theme::ThemeManager manager;
    REQUIRE(document->apply(manager));
    const auto scheme_id = *theme::ThemeSchemeId::parse("styles:x617574686f72/light");
    const auto* registered = manager.catalog().find(scheme_id);
    REQUIRE(registered != nullptr);
    REQUIRE(registered->content);
    require_same_palette(registered->content->palette, expected);
    CHECK(
        theme::semantic_color(registered->content->palette, ColorToken::primary)
        == theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F)
    );
}

TEST_CASE(
    "file palette overrides reject the same non-finite colors as C++",
    "[theme][color-entry][author][file]"
) {
    const std::array inputs {
        std::pair {
            std::string_view {"[nan, 0.1, 30]"},
            theme::NanColor::from_oklch(std::numeric_limits<float>::quiet_NaN(), 0.1F, 30.0F)
        },
        std::pair {
            std::string_view {"[0.5, inf, 30]"},
            theme::NanColor::from_oklch(0.5F, std::numeric_limits<float>::infinity(), 30.0F)
        },
        std::pair {
            std::string_view {"[0.5, 0.1, inf]"},
            theme::NanColor::from_oklch(0.5F, 0.1F, std::numeric_limits<float>::infinity())
        },
        std::pair {
            std::string_view {"[0.5, 0.1, 30, nan]"},
            theme::NanColor::from_oklch(0.5F, 0.1F, 30.0F, std::numeric_limits<float>::quiet_NaN())
        },
    };
    for (const auto& [text, value]: inputs) {
        CAPTURE(text);
        auto palette = theme::default_light_palette();
        REQUIRE_FALSE(theme::set_semantic_color(palette, ColorToken::primary, value));
        const auto source =
            std::string {"[themes.author.palette]\nprimary = "} + std::string {text} + "\n";
        const auto document = theme::parse_style_document(source);
        CHECK_FALSE(document.has_value());
        if (!document) {
            CHECK(document.error().find("primary") != std::string::npos);
        }
    }

    const auto reference = theme::parse_style_document(R"toml(
[palettes.brand.primary]
50 = [nan, 0.0, 0.0]
100 = [0.9, 0.0, 0.0]
200 = [0.8, 0.0, 0.0]
300 = [0.7, 0.0, 0.0]
400 = [0.6, 0.0, 0.0]
500 = [0.5, 0.0, 0.0]
600 = [0.4, 0.0, 0.0]
700 = [0.3, 0.0, 0.0]
800 = [0.2, 0.0, 0.0]
900 = [0.1, 0.0, 0.0]
950 = [0.0, 0.0, 0.0]
[themes.author.palette]
primary = "$palettes.brand.primary.50"
)toml");
    CHECK_FALSE(reference.has_value());
}

TEST_CASE(
    "typed primitive access matches every field and serialized name",
    "[theme][color-entry][author]"
) {
    const auto palette = theme::default_reference_palette();
    const std::array fields {
        &palette.primary,
        &palette.secondary,
        &palette.tertiary,
        &palette.neutral,
        &palette.success,
        &palette.warning,
        &palette.error
    };
    const std::array<std::string_view, 7>
        family_names {"primary", "secondary", "tertiary", "neutral", "success", "warning", "error"};
    const std::array<std::string_view, 11>
        shade_names {"50", "100", "200", "300", "400", "500", "600", "700", "800", "900", "950"};
    for (std::size_t family = 0; family < all_families.size(); ++family) {
        CHECK(&theme::reference_scale(palette, all_families[family]) == fields[family]);
        CHECK(theme::reference_family_name(all_families[family]) == family_names[family]);
        for (std::size_t shade = 0; shade < all_shades.size(); ++shade) {
            CHECK(
                &theme::reference_color(palette, all_families[family], all_shades[shade])
                == &fields[family]->stops[shade]
            );
        }
    }
    for (std::size_t shade = 0; shade < all_shades.size(); ++shade) {
        CHECK(theme::color_shade_name(all_shades[shade]) == shade_names[shade]);
    }
}

TEST_CASE(
    "every canonical role reaches the existing component color resolver",
    "[theme][color-entry][author]"
) {
    const auto system = theme::default_design_system();
    const auto replacement = theme::NanColor::from_oklch(0.543F, 0.123F, 201.0F, 0.7F);
    for (const auto appearance: {theme::ColorAppearance::light, theme::ColorAppearance::dark}) {
        for (const auto role: canonical_roles) {
            CAPTURE(theme::color_token_name(role));
            auto palette = system.palette(appearance);
            CHECK(theme::canonical_color_token(role) == role);
            REQUIRE(theme::set_semantic_color(palette, role, replacement));
            const theme::NanTheme view {system.tokens, palette};
            CHECK(theme::resolve_theme_color(view, theme::ThemeColor::token(role)) == replacement);
            CHECK(theme::semantic_color(palette, role) == replacement);
        }
    }
}

TEST_CASE(
    "every legacy alias writes and consumes its canonical role",
    "[theme][color-entry][author]"
) {
    const std::array aliases {
        std::pair {ColorToken::on_background, ColorToken::foreground},
        std::pair {ColorToken::on_primary, ColorToken::primary_foreground},
        std::pair {ColorToken::on_secondary, ColorToken::secondary_foreground},
        std::pair {ColorToken::on_tertiary, ColorToken::tertiary_foreground},
        std::pair {ColorToken::on_surface, ColorToken::surface_foreground},
        std::pair {ColorToken::on_surface_variant, ColorToken::surface_variant_foreground},
        std::pair {ColorToken::on_muted, ColorToken::muted_foreground},
        std::pair {ColorToken::outline, ColorToken::border},
        std::pair {ColorToken::outline_variant, ColorToken::surface_variant},
        std::pair {ColorToken::on_success, ColorToken::success_foreground},
        std::pair {ColorToken::on_warning, ColorToken::warning_foreground},
        std::pair {ColorToken::on_error, ColorToken::error_foreground},
        std::pair {ColorToken::on_info, ColorToken::info_foreground},
        std::pair {ColorToken::focus_ring, ColorToken::ring},
    };
    const auto first = theme::NanColor::from_oklch(0.45F, 0.12F, 230.0F);
    const auto second = theme::NanColor::from_oklch(0.65F, 0.08F, 180.0F);
    for (const auto& [alias, canonical]: aliases) {
        CAPTURE(theme::color_token_name(alias));
        auto palette = theme::default_light_palette();
        CHECK(theme::canonical_color_token(alias) == canonical);
        REQUIRE(theme::set_semantic_color(palette, alias, first));
        CHECK(theme::semantic_color(palette, canonical) == first);
        CHECK(
            theme::resolve_theme_color(
                theme::NanTheme {{}, palette},
                theme::ThemeColor::token(alias)
            )
            == first
        );
        REQUIRE(theme::set_semantic_color(palette, canonical, second));
        CHECK(
            theme::resolve_theme_color(
                theme::NanTheme {{}, palette},
                theme::ThemeColor::token(alias)
            )
            == second
        );
        CHECK(theme::semantic_color(palette, alias) == second);

        // TOML chooses the canonical spelling regardless of source-key ordering.
        for (const auto canonical_first: {true, false}) {
            const auto canonical_line =
                std::string(theme::color_token_name(canonical)) + " = [0.65, 0.08, 180]\n";
            const auto alias_line =
                std::string(theme::color_token_name(alias)) + " = [0.45, 0.12, 230]\n";
            const auto source = std::string {"[themes.author.palette]\n"}
                + (canonical_first ? canonical_line + alias_line : alias_line + canonical_line);
            const auto document = theme::parse_style_document(source);
            REQUIRE(document);
            const auto& imported = document->themes.at("author").palette;
            CHECK(theme::semantic_color(imported, canonical) == second);
            CHECK(
                theme::resolve_theme_color(
                    theme::NanTheme {{}, imported},
                    theme::ThemeColor::token(alias)
                )
                == second
            );
        }
    }
}

TEST_CASE(
    "batch duplicates are ordered and failures preserve compatibility fields",
    "[theme][color-entry][author]"
) {
    const auto first = theme::NanColor::from_oklch(0.45F, 0.12F, 230.0F);
    const auto last = theme::NanColor::from_oklch(0.65F, 0.08F, 180.0F);
    for (const auto alias_first: {true, false}) {
        auto palette = theme::default_light_palette();
        const std::array overrides {
            theme::SemanticColorOverride {
                alias_first ? ColorToken::on_primary : ColorToken::primary_foreground,
                first
            },
            theme::SemanticColorOverride {
                alias_first ? ColorToken::primary_foreground : ColorToken::on_primary,
                last
            },
        };
        REQUIRE(theme::apply_semantic_colors(palette, overrides));
        CHECK(palette.primary_foreground == last);
        CHECK(palette.on_primary == last);
    }
    auto palette = theme::default_light_palette();
    REQUIRE(theme::apply_semantic_colors(palette, {}));
    require_same_palette(palette, theme::default_light_palette());

    // Failure may not even resynchronize a pre-existing compatibility-field difference.
    palette.on_primary = first;
    palette.focus_ring = last;
    const auto before = palette;
    const std::array invalid {
        theme::SemanticColorOverride {ColorToken::on_primary, last},
        theme::SemanticColorOverride {
            ColorToken::focus_ring,
            theme::NanColor::from_oklch(0.5F, 0.1F, 30.0F, std::numeric_limits<float>::quiet_NaN())
        },
    };
    const auto result = theme::apply_semantic_colors(palette, invalid);
    REQUIRE_FALSE(result);
    CHECK(result.error().role == ColorToken::focus_ring);
    CHECK(palette.on_primary == before.on_primary);
    CHECK(palette.focus_ring == before.focus_ring);
    require_same_palette(palette, before);
}

TEST_CASE(
    "primitive provenance is copied and never regenerates semantic overrides",
    "[theme][color-entry][author]"
) {
    auto reference = theme::default_reference_palette();
    const auto original = reference.primary.at(ColorShade::shade_500);
    const auto base =
        theme::ThemeSchemeContent::from_reference(reference, theme::ColorAppearance::light);
    const auto explicit_primary = theme::NanColor::from_oklch(0.42F, 0.17F, 285.0F);
    reference.primary.at(ColorShade::shade_500) = explicit_primary;
    REQUIRE(base.reference);
    CHECK(base.reference->primary.at(ColorShade::shade_500) == original);
    auto derived = theme::derive_content(base);
    REQUIRE(derived.reference);
    REQUIRE(theme::set_semantic_color(derived.palette, ColorToken::primary, explicit_primary));
    derived.reference->primary.at(ColorShade::shade_500) =
        theme::NanColor::from_oklch(0.9F, 0.1F, 80.0F);
    CHECK(base.reference->primary.at(ColorShade::shade_500) == original);
    CHECK(base.palette.primary != explicit_primary);
    CHECK(derived.design_system().light.primary == explicit_primary);
    CHECK(derived.design_system().dark.primary == explicit_primary);
}

TEST_CASE(
    "every serialized primitive family and shade matches C++ addressing",
    "[theme][color-entry][author][file]"
) {
    const std::array<std::string_view, 7>
        family_names {"primary", "secondary", "tertiary", "neutral", "success", "warning", "error"};
    const std::array<std::string_view, 11>
        shade_names {"50", "100", "200", "300", "400", "500", "600", "700", "800", "900", "950"};
    std::string source;
    for (std::size_t family = 0; family < family_names.size(); ++family) {
        source += "[palettes.matrix." + std::string(family_names[family]) + "]\n";
        for (std::size_t shade = 0; shade < shade_names.size(); ++shade) {
            const auto light = 10 + family * 10 + shade;
            source += std::string(shade_names[shade]) + " = [0." + std::to_string(light)
                + ", 0.1, 30, 0.5]\n";
        }
    }
    for (std::size_t family = 0; family < family_names.size(); ++family) {
        for (std::size_t shade = 0; shade < shade_names.size(); ++shade) {
            const auto name = "case_" + std::to_string(family * shade_names.size() + shade);
            source += "[themes." + name + ".palette]\nprimary = \"$palettes.matrix."
                + std::string(family_names[family]) + "." + std::string(shade_names[shade])
                + "\"\n";
        }
    }
    const auto document = theme::parse_style_document(source);
    REQUIRE(document);
    const auto& reference = document->reference_palettes.at("matrix");
    for (std::size_t family = 0; family < family_names.size(); ++family) {
        for (std::size_t shade = 0; shade < shade_names.size(); ++shade) {
            const auto name = "case_" + std::to_string(family * shade_names.size() + shade);
            CAPTURE(name);
            const auto expected = theme::NanColor::from_oklch(
                static_cast<float>(10 + family * 10 + shade) / 100.0F,
                0.1F,
                30.0F,
                0.5F
            );
            CHECK(
                theme::reference_color(reference, all_families[family], all_shades[shade])
                == expected
            );
            CHECK(document->themes.at(name).palette.primary == expected);
        }
    }
}
