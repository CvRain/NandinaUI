// Theme contrast diagnostics: explicit profiles, shared compositor, scenario-scoped reports.
#include <nandina/foundation/composition.hpp>
#include <nandina/foundation/contrast.hpp>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/theme/contrast_diagnostics.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_scheme.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace foundation = nandina::foundation;
namespace theme = nandina::theme;

namespace
{
    auto content_of(const theme::DesignSystem& system, theme::ColorAppearance appearance)
        -> theme::ThemeSchemeContent {
        return theme::ThemeSchemeContent::from(system, appearance);
    }

    auto literal(const foundation::NanColor& value) -> theme::ThemeColor {
        return theme::ThemeColor::literal(value);
    }

    auto rgb(const float red, const float green, const float blue, const float alpha = 1.0F)
        -> foundation::NanColor {
        return foundation::NanColor::from_rgb(red, green, blue, alpha);
    }
} // namespace

TEST_CASE(
    "default contrast profile passes default light and dark palettes",
    "[theme][contrast][diagnostics]"
) {
    const auto profile = theme::default_contrast_profile();
    CHECK(profile.rules.size() == 15);
    for (const auto appearance: {theme::ColorAppearance::light, theme::ColorAppearance::dark}) {
        const auto content = content_of(theme::default_design_system(), appearance);
        const auto diagnostics = theme::evaluate_contrast(content, appearance, profile);
        REQUIRE(diagnostics.size() == profile.rules.size());
        CHECK_FALSE(theme::has_required_contrast_failure(diagnostics));
        CHECK_FALSE(theme::has_insufficient_contrast_context(diagnostics));
        for (const auto& diagnostic: diagnostics) {
            CAPTURE(diagnostic.rule);
            CHECK(diagnostic.status == theme::ContrastStatus::passed);
            CHECK(diagnostic.measured_ratio.has_value());
            CHECK(diagnostic.effective_background.has_value());
            CHECK(diagnostic.effective_foreground.has_value());
        }
    }
}

TEST_CASE(
    "default contrast profile passes built-in families at their declared floors",
    "[theme][contrast][diagnostics]"
) {
    const auto profile = theme::default_contrast_profile();
    for (const auto& family: theme::default_theme_families()) {
        CAPTURE(family.name);
        const auto system = theme::build_family_design_system(family);
        for (const auto appearance: {theme::ColorAppearance::light, theme::ColorAppearance::dark}) {
            const auto content = content_of(system, appearance);
            const auto diagnostics = theme::evaluate_contrast(content, appearance, profile);
            CHECK_FALSE(theme::has_required_contrast_failure(diagnostics));
            CHECK_FALSE(theme::has_insufficient_contrast_context(diagnostics));
        }
    }
}

TEST_CASE(
    "failing required rules block while advisory rules only report",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "faded text",
             .foreground = literal(rgb(0.95F, 0.95F, 0.95F)),
             .background = {literal(rgb(1.0F, 1.0F, 1.0F))},
             .required_ratio = foundation::nan_contrast_aa_text,
             .required = true},
            {.name = "advisory hint",
             .foreground = literal(rgb(0.9F, 0.9F, 0.9F)),
             .background = {literal(rgb(1.0F, 1.0F, 1.0F))},
             .required_ratio = foundation::nan_contrast_aa_text,
             .required = false},
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 2);
    CHECK(diagnostics[0].status == theme::ContrastStatus::failed);
    CHECK(diagnostics[0].required);
    CHECK(diagnostics[0].measured_ratio.value() < diagnostics[0].required_ratio);
    CHECK(diagnostics[0].message.find("below") != std::string::npos);
    CHECK(diagnostics[1].status == theme::ContrastStatus::failed);
    CHECK_FALSE(diagnostics[1].required);
    CHECK(theme::has_required_contrast_failure(diagnostics));
    // The blocking set only contains the required rule.
    CHECK(theme::has_required_contrast_failure({&diagnostics[0], 1}));
    CHECK_FALSE(theme::has_required_contrast_failure({&diagnostics[1], 1}));
    CHECK_FALSE(theme::has_insufficient_contrast_context(diagnostics));
}

TEST_CASE(
    "insufficient context is not reported as a measured failure",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "text over unknown host",
             .foreground = literal(rgb(1, 1, 1)),
             .background = {literal(rgb(0, 0, 0, 0.5F))},
             .required_ratio = foundation::nan_contrast_aa_text,
             .required = true},
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].status == theme::ContrastStatus::insufficient_context);
    CHECK_FALSE(diagnostics[0].measured_ratio.has_value());
    CHECK_FALSE(diagnostics[0].effective_background.has_value());
    CHECK_FALSE(diagnostics[0].message.empty());
    CHECK_FALSE(theme::has_required_contrast_failure(diagnostics));
    CHECK(theme::has_insufficient_contrast_context(diagnostics));

    // A rule without a usable threshold is also context-insufficient, not a pass.
    theme::ContrastProfile invalid {
        .rules = {
            {.name = "broken threshold",
             .foreground = literal(rgb(0, 0, 0)),
             .background = {literal(rgb(1, 1, 1))},
             .required_ratio = 0.0F},
        },
    };
    const auto invalid_diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, invalid);
    REQUIRE(invalid_diagnostics.size() == 1);
    CHECK(invalid_diagnostics[0].status == theme::ContrastStatus::insufficient_context);
}

TEST_CASE(
    "layered scenarios compose intermediate backgrounds before measuring",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "text over 50% scrim on dark panel",
             .foreground = literal(rgb(1, 1, 1)),
             .background = {literal(rgb(0, 0, 0, 0.5F)), literal(rgb(0.2F, 0.2F, 0.2F))},
             .required_ratio = foundation::nan_contrast_aa_text},
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].status == theme::ContrastStatus::passed);
    REQUIRE(diagnostics[0].measured_ratio.has_value());
    // Hand-computed: scrim over rgb(0.2) becomes rgb(0.1); white against it is ~17.49:1.
    CHECK(diagnostics[0].measured_ratio.value() == Catch::Approx(17.49F).margin(0.05F));
    REQUIRE(diagnostics[0].effective_background.has_value());
    CHECK(diagnostics[0].effective_background->to<foundation::NanRgb>().red == Catch::Approx(0.1F));
    CHECK(diagnostics[0].background_roles == std::vector<std::string> {"literal", "literal"});
    CHECK(diagnostics[0].foreground_role == "literal");
}

TEST_CASE(
    "diagnostics keep scheme identity, appearance and both color representations",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::dark);
    const auto id = *theme::ThemeSchemeId::parse("org.test:diagnostic");
    theme::ContrastProfile profile {
        .rules = {
            {.name = "card text",
             .state = "normal",
             .foreground = theme::ThemeColor::token(theme::ColorToken::card_foreground),
             .background = {theme::ThemeColor::token(theme::ColorToken::card)},
             .required_ratio = foundation::nan_contrast_aa_text},
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::dark, profile, id);
    REQUIRE(diagnostics.size() == 1);
    const auto& diagnostic = diagnostics[0];
    CHECK(diagnostic.scheme.has_value());
    CHECK(*diagnostic.scheme == id);
    CHECK(diagnostic.appearance == theme::ColorAppearance::dark);
    CHECK(diagnostic.state == "normal");
    CHECK(diagnostic.foreground_role == "card_foreground");
    CHECK(diagnostic.background_roles == std::vector<std::string> {"card"});
    REQUIRE(diagnostic.foreground.has_value());
    CHECK(std::isfinite(diagnostic.foreground->resolved.oklch().light));
    CHECK(std::isfinite(diagnostic.foreground->converted.red));
    const auto described = theme::describe(diagnostic);
    CHECK(described.find("card text") != std::string::npos);
    CHECK(described.find("org.test:diagnostic") != std::string::npos);
    CHECK(described.find("card_foreground") != std::string::npos);

    // Anonymous sources report without a fake registered identity.
    const auto anonymous = theme::evaluate_contrast(content, theme::ColorAppearance::dark, profile);
    CHECK_FALSE(anonymous[0].scheme.has_value());
    CHECK(theme::describe(anonymous[0]).find("anonymous") != std::string::npos);
}

TEST_CASE("tone references require explicit scenario context", "[theme][contrast][diagnostics]") {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "unknown tone",
             .foreground = theme::ThemeColor::on_accent(),
             .background = {theme::ThemeColor::accent()}},
            {.name = "unknown transformed tone",
             .foreground = theme::ThemeColor::with_alpha(
                 theme::on_accent_ref,
                 theme::ThemeScalar::literal(0.5F)
             ),
             .background = {literal(rgb(0, 0, 0))}},
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 2);
    for (const auto& diagnostic: diagnostics) {
        CHECK(diagnostic.status == theme::ContrastStatus::insufficient_context);
        CHECK_FALSE(diagnostic.measured_ratio.has_value());
        CHECK(diagnostic.message.find("tone") != std::string::npos);
        CHECK_FALSE(diagnostic.foreground.has_value());
    }
}

TEST_CASE("diagnostic role paths preserve transform operands", "[theme][contrast][diagnostics]") {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "transformed text",
             .foreground = theme::ThemeColor::with_alpha(
                 theme::ColorToken::foreground,
                 theme::ThemeScalar::literal(0.5F)
             ),
             .background = {theme::ThemeColor::mix(
                 theme::ColorToken::card,
                 theme::ColorToken::background,
                 theme::ThemeScalar::literal(0.25F)
             )}}
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].foreground_role == "with_alpha(foreground)");
    CHECK(diagnostics[0].background_roles == std::vector<std::string> {"mix(card, background)"});
}

TEST_CASE(
    "threshold decisions and reports preserve adjacent float values",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    const auto threshold = std::nextafter(1.0F, 2.0F);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "adjacent threshold",
             .foreground = literal(rgb(0, 0, 0)),
             .background = {literal(rgb(0, 0, 0))},
             .required_ratio = threshold}
        },
    };
    auto diagnostics = theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 1);
    REQUIRE(diagnostics[0].measured_ratio.has_value());
    CHECK(*diagnostics[0].measured_ratio == 1.0F);
    CHECK(diagnostics[0].status == theme::ContrastStatus::failed);
    const auto report = theme::describe(diagnostics[0]);
    const auto required = report.find("required=");
    REQUIRE(required != std::string::npos);
    CHECK(std::stof(report.substr(required + 9)) > *diagnostics[0].measured_ratio);
    CHECK(std::stof(report.substr(required + 9)) == threshold);
    const auto below = diagnostics[0].message.find("below ");
    REQUIRE(below != std::string::npos);
    CHECK(std::stof(diagnostics[0].message.substr(below + 6)) > *diagnostics[0].measured_ratio);
    CHECK(std::stof(diagnostics[0].message.substr(below + 6)) == threshold);
    profile.rules[0].required_ratio = 1.0F;
    diagnostics = theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    CHECK(diagnostics[0].status == theme::ContrastStatus::passed);
}

TEST_CASE(
    "diagnostic sampling stops at the first opaque background",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "occluded layer",
             .foreground = literal(rgb(0, 0, 0)),
             .background = {
                 literal(rgb(0, 0, 0, 0.5F)),
                 literal(rgb(1, 1, 1)),
                 literal(rgb(std::numeric_limits<float>::quiet_NaN(), 0, 0))
             }}
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].status == theme::ContrastStatus::passed);
    CHECK(diagnostics[0].background.size() == 2);
    CHECK(diagnostics[0].background_roles.size() == 2);
}

TEST_CASE("diagnostic reports identify the CPU color policy", "[theme][contrast][diagnostics]") {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    const auto diagnostics = theme::evaluate_contrast(
        content,
        theme::ColorAppearance::light,
        theme::default_contrast_profile()
    );
    REQUIRE_FALSE(diagnostics.empty());
    const auto report = theme::describe(diagnostics[0]);
    CHECK(report.find("composition=encoded_srgb") != std::string::npos);
    CHECK(report.find("gamut=linear_rgb_clip") != std::string::npos);
    CHECK(report.find("quantization=float_no_u8") != std::string::npos);
}

TEST_CASE("explicit tones resolve the declared palette pair", "[theme][contrast][diagnostics]") {
    for (const auto appearance: {theme::ColorAppearance::light, theme::ColorAppearance::dark}) {
        const auto content = content_of(theme::default_design_system(), appearance);
        const auto& palette = content.palette;
        struct Expected {
            theme::ButtonTone tone;
            foundation::NanColor foreground;
            foundation::NanColor background;
        };
        const std::array cases {
            Expected {theme::ButtonTone::primary, palette.primary_foreground, palette.primary},
            Expected {
                theme::ButtonTone::secondary,
                palette.secondary_foreground,
                palette.secondary
            },
            Expected {
                theme::ButtonTone::neutral,
                palette.surface_variant_foreground,
                palette.surface_variant
            },
            Expected {theme::ButtonTone::danger, palette.error_foreground, palette.error},
        };
        for (const auto& expected: cases) {
            theme::ContrastProfile profile {
                .rules = {
                    {.name = "tone text",
                     .foreground = theme::ThemeColor::on_accent(),
                     .background = {theme::ThemeColor::accent()},
                     .required_ratio = 1.0F,
                     .tone = expected.tone}
                },
            };
            const auto diagnostics = theme::evaluate_contrast(content, appearance, profile);
            REQUIRE(diagnostics.size() == 1);
            const auto& diagnostic = diagnostics[0];
            CHECK(diagnostic.status == theme::ContrastStatus::passed);
            REQUIRE(diagnostic.foreground.has_value());
            CHECK(diagnostic.foreground->resolved == expected.foreground);
            REQUIRE(diagnostic.background.size() == 1);
            CHECK(diagnostic.background[0].resolved == expected.background);
            CHECK(diagnostic.foreground_role == "tone.on_accent");
            CHECK(diagnostic.background_roles == std::vector<std::string> {"tone.accent"});
            CHECK(diagnostic.tone == expected.tone);
            CHECK(theme::describe(diagnostic).find("tone=") != std::string::npos);
        }
    }
}

TEST_CASE(
    "background tone context is required only before the opaque endpoint",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "missing background tone",
             .foreground = literal(rgb(0, 0, 0)),
             .background = {theme::ThemeColor::accent()}},
            {.name = "missing mix rhs tone",
             .foreground = literal(rgb(0, 0, 0)),
             .background = {theme::ThemeColor::mix(
                 theme::ColorToken::background,
                 theme::accent_ref,
                 theme::ThemeScalar::literal(0.5F)
             )}},
            {.name = "hidden tone",
             .foreground = literal(rgb(0, 0, 0)),
             .background = {literal(rgb(1, 1, 1)), theme::ThemeColor::accent()}},
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 3);
    for (const auto index: {0, 1}) {
        CHECK(diagnostics[index].status == theme::ContrastStatus::insufficient_context);
        CHECK_FALSE(diagnostics[index].measured_ratio.has_value());
        CHECK(diagnostics[index].background.empty());
        CHECK(diagnostics[index].message.find("tone") != std::string::npos);
    }
    CHECK(
        diagnostics[1].background_roles == std::vector<std::string> {"mix(background, tone.accent)"}
    );
    CHECK(diagnostics[2].status == theme::ContrastStatus::passed);
    CHECK(diagnostics[2].background_roles == std::vector<std::string> {"literal"});
}

TEST_CASE(
    "unsupported composition and invalid thresholds remain unmeasured",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "invalid input",
             .foreground = literal(rgb(0, 0, 0)),
             .background = {literal(rgb(1, 1, 1))}}
        },
    };
    const foundation::CompositeConfig unknown {
        .space = static_cast<foundation::CompositeSpace>(17)
    };
    const auto unsupported = theme::evaluate_contrast(
        content,
        theme::ColorAppearance::light,
        profile,
        std::nullopt,
        unknown
    );
    REQUIRE(unsupported.size() == 1);
    CHECK(unsupported[0].config.space == unknown.space);
    CHECK(unsupported[0].status == theme::ContrastStatus::insufficient_context);
    CHECK_FALSE(unsupported[0].measured_ratio.has_value());
    CHECK(theme::describe(unsupported[0]).find("composition=unsupported") != std::string::npos);
    for (const auto threshold:
         {0.0F,
          -1.0F,
          std::numeric_limits<float>::quiet_NaN(),
          std::numeric_limits<float>::infinity()})
    {
        profile.rules[0].required_ratio = threshold;
        const auto diagnostics =
            theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
        REQUIRE(diagnostics.size() == 1);
        CHECK(diagnostics[0].status == theme::ContrastStatus::insufficient_context);
        CHECK_FALSE(diagnostics[0].measured_ratio.has_value());
        CHECK_FALSE(theme::has_required_contrast_failure(diagnostics));
    }
}

TEST_CASE(
    "diagnostics preserve normalized OKLCH separately from clipped sRGB",
    "[theme][contrast][diagnostics]"
) {
    const auto content = content_of(theme::default_design_system(), theme::ColorAppearance::light);
    const auto vivid = foundation::NanColor::from_oklch(0.7F, 0.5F, 40.0F, 0.5F);
    theme::ContrastProfile profile {
        .rules = {
            {.name = "out of gamut",
             .foreground = literal(vivid),
             .background = {literal(rgb(1, 1, 1))}}
        },
    };
    const auto diagnostics =
        theme::evaluate_contrast(content, theme::ColorAppearance::light, profile);
    REQUIRE(diagnostics.size() == 1);
    const auto& diagnostic = diagnostics[0];
    REQUIRE(diagnostic.foreground.has_value());
    CHECK(diagnostic.foreground->resolved == vivid);
    CHECK(diagnostic.foreground->converted.red >= 0.0F);
    CHECK(diagnostic.foreground->converted.red <= 1.0F);
    CHECK(diagnostic.foreground->converted.green >= 0.0F);
    CHECK(diagnostic.foreground->converted.green <= 1.0F);
    CHECK(diagnostic.foreground->converted.blue >= 0.0F);
    CHECK(diagnostic.foreground->converted.blue <= 1.0F);
    CHECK(diagnostic.foreground->converted.alpha == 0.5F);
    CHECK_FALSE(foundation::NanColor::from(diagnostic.foreground->converted) == vivid);
    CHECK(diagnostic.measured_ratio.has_value());
}
