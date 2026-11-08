// D2 step 4d: TOML named color literals (file authoring over foundation names).
#include <nandina/foundation/named_color.hpp>
#include <nandina/theme/style_document.hpp>
#include <nandina/theme/theme_manager.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace theme = nandina::theme;
namespace foundation = nandina::foundation;

namespace
{
    auto parsed_primary(const theme::StyleDocument& document, std::string_view theme_name)
        -> foundation::NanColor {
        return document.themes.at(std::string(theme_name)).palette.primary;
    }

    [[nodiscard]] auto theme_color_literal(const theme::ThemeColor& value)
        -> const foundation::NanColor* {
        return std::get_if<foundation::NanColor>(&value.value());
    }
    [[nodiscard]] auto reference_source(std::string_view first_stop) -> std::string {
        return "[palettes.brand.primary]\n50 = " + std::string(first_stop) + R"toml(
100 = "White"
200 = "White"
300 = "White"
400 = "White"
500 = "SteelBlue"
600 = "SteelBlue"
700 = "SteelBlue"
800 = "Black"
900 = "Black"
950 = "Black"
)toml";
    }
} // namespace

TEST_CASE(
    "file palette and reference scale literals accept named colors",
    "[theme][named-color][file]"
) {
    constexpr std::string_view source = R"toml(
[palettes.brand.primary]
50 = "White"
100 = [0.96, 0.01, 80]
200 = [0.92, 0.02, 80]
300 = [0.86, 0.04, 80]
400 = [0.78, 0.08, 80]
500 = [0.68, 0.12, 80]
600 = [0.58, 0.14, 80]
700 = [0.48, 0.12, 80]
800 = [0.38, 0.10, 80]
900 = [0.28, 0.08, 80]
950 = [0.20, 0.06, 80]

[themes.author]
[themes.author.palette]
primary = "SteelBlue"
primary_foreground = "$palettes.brand.primary.50"
muted = "RebeccaPurple"
)toml";
    const auto document = theme::parse_style_document(source);
    REQUIRE(document);
    CHECK(parsed_primary(*document, "author") == foundation::nan_named_color("SteelBlue"));
    CHECK(
        document->themes.at("author").palette.primary_foreground
        == foundation::nan_named_color("white")
    );
    CHECK(
        document->themes.at("author").palette.muted == foundation::nan_named_color("rebeccapurple")
    );
    // The reference scale accepted a named color for its lightest stop.
    CHECK(
        document->reference_palettes.at("brand").primary.at(theme::ColorShade::shade_50)
        == foundation::nan_named_color("White")
    );

    // Import registers the same values in the canonical scheme.
    theme::ThemeManager manager;
    REQUIRE(document->apply(manager));
    const auto id = *theme::ThemeSchemeId::parse("styles:x617574686f72/light");
    const auto* scheme = manager.catalog().find(id);
    REQUIRE(scheme != nullptr);
    REQUIRE(scheme->content);
    CHECK(
        theme::semantic_color(scheme->content->palette, theme::ColorToken::primary)
        == foundation::nan_named_color("SteelBlue")
    );
}

TEST_CASE("recipe color fields accept named colors", "[theme][named-color][file]") {
    constexpr std::string_view source = R"toml(
[[styles.button]]
background = "SteelBlue"
foreground = "White"
border_color = "$palette.primary"
[[styles.text_field]]
background = "Ivory"
placeholder = "RebeccaPurple"
)toml";
    const auto document = theme::parse_style_document(source);
    REQUIRE(document);
    const auto& rules = document->style->button_rules();
    REQUIRE(rules.size() == 1);
    REQUIRE(rules[0].background.has_value());
    const auto* background = theme_color_literal(*rules[0].background);
    REQUIRE(background != nullptr);
    CHECK(*background == foundation::nan_named_color("SteelBlue"));
    REQUIRE(rules[0].foreground.has_value());
    const auto* foreground = theme_color_literal(*rules[0].foreground);
    REQUIRE(foreground != nullptr);
    CHECK(*foreground == foundation::nan_named_color("White"));
    // `$palette.*` stays a semantic token, not a color literal.
    REQUIRE(rules[0].border_color.has_value());
    CHECK(theme_color_literal(*rules[0].border_color) == nullptr);
    CHECK(std::get_if<theme::ColorToken>(&rules[0].border_color->value()) != nullptr);

    const auto& fields = document->style->text_field_rules();
    REQUIRE(fields.size() == 1);
    REQUIRE(fields[0].background.has_value());
    const auto* field_background = theme_color_literal(*fields[0].background);
    REQUIRE(field_background != nullptr);
    CHECK(*field_background == foundation::nan_named_color("Ivory"));
    REQUIRE(fields[0].placeholder.has_value());
    const auto* field_placeholder = theme_color_literal(*fields[0].placeholder);
    REQUIRE(field_placeholder != nullptr);
    CHECK(*field_placeholder == foundation::nan_named_color("RebeccaPurple"));
}

TEST_CASE(
    "references keep their semantics and never fall back to names",
    "[theme][named-color][file]"
) {
    // `$palettes.*` resolves to the exact reference-scale value.
    const auto palette_doc = theme::parse_style_document(R"toml(
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
[themes.author.palette]
primary = "$palettes.brand.primary.500"
)toml");
    REQUIRE(palette_doc);
    CHECK(
        parsed_primary(*palette_doc, "author")
        == palette_doc->reference_palettes.at("brand").primary.at(theme::ColorShade::shade_500)
    );

    // Unknown palette, malformed reference and unknown named color are all errors.
    const auto unknown_palette = theme::parse_style_document(R"toml(
[themes.author.palette]
primary = "$palettes.missing.primary.500"
)toml");
    REQUIRE_FALSE(unknown_palette);
    CHECK(unknown_palette.error().find("unknown palette") != std::string::npos);

    const auto malformed = theme::parse_style_document(R"toml(
[themes.author.palette]
primary = "$palettes.brand"
)toml");
    REQUIRE_FALSE(malformed);
    CHECK(malformed.error().find("palette, scale, and shade") != std::string::npos);

    const auto unknown_name = theme::parse_style_document(R"toml(
[themes.author.palette]
primary = "NotAColor"
)toml");
    REQUIRE_FALSE(unknown_name);
    CHECK(unknown_name.error().find("unknown named color") != std::string::npos);
}

TEST_CASE(
    "named color errors carry the field path and failed imports leave state untouched",
    "[theme][named-color][file]"
) {
    const auto bad_role = theme::parse_style_document(R"toml(
[themes.author.palette]
primary = "NotAColor"
)toml");
    REQUIRE_FALSE(bad_role);
    CHECK(bad_role.error().find("themes.author.palette.primary") != std::string::npos);

    const auto bad_stop = theme::parse_style_document(R"toml(
[palettes.brand.primary]
50 = "NotAColor"
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
)toml");
    REQUIRE_FALSE(bad_stop);
    CHECK(bad_stop.error().find("palettes.brand.primary.50") != std::string::npos);

    const auto bad_reference = theme::parse_style_document(R"toml(
[themes.author.palette]
primary = "$palettes.missing.primary.500"
)toml");
    REQUIRE_FALSE(bad_reference);
    CHECK(bad_reference.error().find("themes.author.palette.primary") != std::string::npos);
}

TEST_CASE(
    "canonical palette names still win over aliases when both use named colors",
    "[theme][named-color][file]"
) {
    const auto canonical = theme::parse_style_document(R"toml(
[themes.author.palette]
foreground = "White"
on_background = "Black"
)toml");
    REQUIRE(canonical);
    CHECK(
        canonical->themes.at("author").palette.foreground == foundation::nan_named_color("White")
    );

    const auto alias_only = theme::parse_style_document(R"toml(
[themes.author.palette]
on_background = "Black"
)toml");
    REQUIRE(alias_only);
    CHECK(
        alias_only->themes.at("author").palette.foreground == foundation::nan_named_color("Black")
    );
    CHECK(
        alias_only->themes.at("author").palette.on_background
        == foundation::nan_named_color("Black")
    );
}

TEST_CASE("recipe color errors identify the rule and array index", "[theme][named-color][file]") {
    const auto button = theme::parse_style_document(
        R"toml(
[[styles.button]]
background = "SteelBlue"
[[styles.button]]
foreground = "NotAColor"
)toml",
        "author.toml"
    );
    REQUIRE_FALSE(button);
    CHECK(button.error().find("author.toml") != std::string::npos);
    CHECK(button.error().find("styles.button[1].foreground") != std::string::npos);

    const auto field = theme::parse_style_document(R"toml(
[[styles.text_field]]
background = "Ivory"
[[styles.text_field]]
placeholder = "$palette.unknown"
)toml");
    REQUIRE_FALSE(field);
    CHECK(field.error().find("styles.text_field[1].placeholder") != std::string::npos);
    CHECK(field.error().find("unknown semantic color token") != std::string::npos);
}

TEST_CASE(
    "semantic validation errors preserve the full palette input path",
    "[theme][named-color][file]"
) {
    for (const auto role: {"primary", "on_primary"}) {
        const auto source = std::string("[themes.author.palette]\n") + role + " = [nan, 0.1, 30]\n";
        const auto document = theme::parse_style_document(source);
        REQUIRE_FALSE(document);
        CHECK(
            document.error().find(std::string("themes.author.palette.") + role) != std::string::npos
        );
        CHECK(document.error().find("non-finite") != std::string::npos);
    }
    const auto reference = theme::parse_style_document(R"toml(
[palettes.brand.primary]
50 = [nan, 0.0, 0.0]
100 = "White"
200 = "White"
300 = "White"
400 = "White"
500 = "SteelBlue"
600 = "SteelBlue"
700 = "SteelBlue"
800 = "Black"
900 = "Black"
950 = "Black"
[themes.author.palette]
primary = "$palettes.brand.primary.50"
)toml");
    REQUIRE_FALSE(reference);
    CHECK(reference.error().find("themes.author.palette.primary") != std::string::npos);
    CHECK(reference.error().find("non-finite") != std::string::npos);
}

TEST_CASE("all named colors use the same file literal path", "[theme][named-color][file]") {
    for (std::size_t index = 0; index < 148; ++index) {
        const auto named = static_cast<foundation::NanNamedColor>(index);
        const auto name = foundation::nan_named_color_name(named);
        CAPTURE(name);
        std::string mixed(name);
        for (std::size_t position = 0; position < mixed.size(); position += 2) {
            mixed[position] = static_cast<char>(mixed[position] - 'a' + 'A');
        }
        const auto literal = "\"" + mixed + "\"";
        const auto document = theme::parse_style_document(
            reference_source(literal) + "[themes.author.palette]\nprimary = " + literal
            + "\nprimary_foreground = \"$palettes.brand.primary.50\"\n"
            + "[[styles.button]]\nbackground = " + literal + "\n"
            + "[[styles.text_field]]\nplaceholder = " + literal + "\n"
        );
        REQUIRE(document);
        const auto expected = foundation::nan_named_color(named);
        CHECK(document->themes.at("author").palette.primary == expected);
        CHECK(document->themes.at("author").palette.primary_foreground == expected);
        CHECK(
            document->reference_palettes.at("brand").primary.at(theme::ColorShade::shade_50)
            == expected
        );
        REQUIRE(document->style->button_rules().size() == 1);
        REQUIRE(document->style->button_rules()[0].background);
        CHECK(
            std::get<foundation::NanColor>(document->style->button_rules()[0].background->value())
            == expected
        );
        REQUIRE(document->style->text_field_rules().size() == 1);
        REQUIRE(document->style->text_field_rules()[0].placeholder);
        CHECK(
            std::get<foundation::NanColor>(
                document->style->text_field_rules()[0].placeholder->value()
            )
            == expected
        );
    }
}

TEST_CASE(
    "every supported recipe color field preserves literal and token identity",
    "[theme][named-color][file]"
) {
    using ButtonField = std::optional<theme::ThemeColor> theme::ButtonStyleRule::*;
    using TextField = std::optional<theme::ThemeColor> theme::TextFieldStyleRule::*;
    const std::array<std::pair<std::string_view, ButtonField>, 4> button_fields {{
        {"background", &theme::ButtonStyleRule::background},
        {"foreground", &theme::ButtonStyleRule::foreground},
        {"border_color", &theme::ButtonStyleRule::border_color},
        {"focus_ring_color", &theme::ButtonStyleRule::focus_ring_color},
    }};
    const std::array<std::pair<std::string_view, TextField>, 6> text_fields {{
        {"background", &theme::TextFieldStyleRule::background},
        {"foreground", &theme::TextFieldStyleRule::foreground},
        {"placeholder", &theme::TextFieldStyleRule::placeholder},
        {"border_color", &theme::TextFieldStyleRule::border_color},
        {"focus_ring_color", &theme::TextFieldStyleRule::focus_ring_color},
        {"selection", &theme::TextFieldStyleRule::selection},
    }};
    // A generic reader returns the exact member under test, so all ten consumer slots are covered.
    const auto check = [&](const auto& fields, std::string_view kind, auto read_rule) {
        for (const auto& [key, member]: fields) {
            CAPTURE(kind, key);
            for (const std::string_view input:
                 {"\"SteelBlue\"", "[0.5, 0.1, 30, 0.25]", "\"$palette.primary\""})
            {
                const auto document = theme::parse_style_document(
                    "[[styles." + std::string(kind) + "]]\n" + std::string(key) + " = "
                    + std::string(input) + "\n"
                );
                REQUIRE(document);
                const auto& value = read_rule(*document).*member;
                REQUIRE(value);
                if (input == "\"$palette.primary\"") {
                    const auto* token = std::get_if<theme::ColorToken>(&value->value());
                    REQUIRE(token);
                    CHECK(*token == theme::ColorToken::primary);
                    theme::NanTheme view = theme::default_theme();
                    CHECK(theme::resolve_theme_color(view, *value) == view.palette.primary);
                    const auto replacement =
                        foundation::nan_named_color(foundation::NanNamedColor::red);
                    REQUIRE(
                        theme::set_semantic_color(
                            view.palette,
                            theme::ColorToken::primary,
                            replacement
                        )
                    );
                    CHECK(theme::resolve_theme_color(view, *value) == replacement);
                }
                else {
                    const auto* color = theme_color_literal(*value);
                    REQUIRE(color);
                    const auto expected = input.front() == '['
                        ? foundation::NanColor::from_oklch(0.5F, 0.1F, 30.0F, 0.25F)
                        : foundation::nan_named_color(foundation::NanNamedColor::steelblue);
                    CHECK(*color == expected);
                }
            }
        }
    };
    check(button_fields, "button", [](const theme::StyleDocument& doc) -> const auto& {
        return doc.style->button_rules()[0];
    });
    check(text_fields, "text_field", [](const theme::StyleDocument& doc) -> const auto& {
        return doc.style->text_field_rules()[0];
    });
}

TEST_CASE(
    "file named color inputs keep foundation rejection boundaries",
    "[theme][named-color][file]"
) {
    const std::array<std::string_view, 19> invalid {{
        "\"\"",
        "\" SteelBlue\"",
        "\"SteelBlue \"",
        "\"steel blue\"",
        "\"steel\\tblue\"",
        "\"steelblue\\u0000suffix\"",
        "\"steelbl\\u00FCe\"",
        "\"transparent\"",
        "\"CURRENTCOLOR\"",
        "\"#4682b4\"",
        "\"rgb(70, 130, 180)\"",
        "\"notacolor\"",
        "\"$\"",
        "\"$palette\"",
        "\"$palettes\"",
        "42",
        "true",
        "[]",
        "{}",
    }};
    for (const auto input: invalid) {
        CAPTURE(input);
        const std::array sources {
            std::pair {
                std::string("[themes.author.palette]\nprimary = ") + std::string(input),
                std::string_view("themes.author.palette.primary")
            },
            std::pair {reference_source(input), std::string_view("palettes.brand.primary.50")},
            std::pair {
                std::string("[[styles.button]]\nbackground = ") + std::string(input),
                std::string_view("styles.button[0].background")
            },
            std::pair {
                std::string("[[styles.text_field]]\nselection = ") + std::string(input),
                std::string_view("styles.text_field[0].selection")
            },
        };
        for (const auto& [source, path]: sources) {
            const auto document = theme::parse_style_document(source);
            REQUIRE_FALSE(document);
            CHECK(document.error().find(path) != std::string::npos);
        }
    }
}

TEST_CASE(
    "file color reference support stays specific to each location",
    "[theme][named-color][file]"
) {
    const auto with_scale = reference_source("\"White\"");
    const std::array<std::pair<std::string, std::string_view>, 7> inputs {{
        {"[themes.author.palette]\nprimary = \"$palette.primary\"",
         "themes.author.palette.primary"},
        {"[[styles.button]]\nbackground = \"$palettes.brand.primary.50\"",
         "styles.button[0].background"},
        {"[[styles.text_field]]\nforeground = \"$palettes.brand.primary.50\"",
         "styles.text_field[0].foreground"},
        {"[themes.author.palette]\nprimary = \"$palettes.brand.unknown.50\"",
         "themes.author.palette.primary"},
        {"[themes.author.palette]\nprimary = \"$palettes.brand.primary.501\"",
         "themes.author.palette.primary"},
        {"[themes.author.palette]\nprimary = \"$palettes.brand.primary.50.extra\"",
         "themes.author.palette.primary"},
        {"[[styles.button]]\nbackground = \"$palette.primary.extra\"",
         "styles.button[0].background"},
    }};
    for (const auto& [source, path]: inputs) {
        const auto document = theme::parse_style_document(with_scale + source);
        REQUIRE_FALSE(document);
        CHECK(document.error().find(path) != std::string::npos);
        CHECK(document.error().find("unknown") != std::string::npos);
    }
    for (const auto reference: {"\"$palette.primary\"", "\"$palettes.brand.primary.50\""}) {
        const auto document = theme::parse_style_document(reference_source(reference));
        REQUIRE_FALSE(document);
        CHECK(document.error().find("palettes.brand.primary.50") != std::string::npos);
        CHECK(document.error().find("references are not accepted") != std::string::npos);
    }
}

TEST_CASE(
    "named color imports preserve live state on parse and apply failures",
    "[theme][named-color][file]"
) {
    theme::ThemeManager manager;
    const auto import = [&](std::string_view source) -> std::expected<void, std::string> {
        auto document = theme::parse_style_document(source);
        if (!document)
            return std::unexpected(document.error());
        return document->apply(manager);
    };
    REQUIRE(import(R"toml(
active_theme = "author"
[themes.author.palette]
primary = "SteelBlue"
[[styles.button]]
foreground = "White"
)toml"));
    const auto revision = manager.revision();
    const auto version = manager.catalog().version();
    const auto schemes = manager.catalog().schemes();
    const auto selection = manager.selection();
    const auto* style = &manager.style();
    const auto* view = &manager.design_system();
    const std::array<std::string_view, 3> invalid {{
        R"toml([themes.new.palette]
primary = "SteelBlue"
[[styles.button]]
foreground = "NotAColor")toml",
        R"toml(active_theme = "missing"
[themes.new.palette]
primary = "SteelBlue"
[[styles.button]]
foreground = "Red")toml",
        R"toml(active_family = "broken"
[themes.new.palette]
primary = "SteelBlue"
[theme_families.broken]
light = "new"
dark = "missing"
[[styles.text_field]]
foreground = "Red")toml",
    }};
    for (const auto source: invalid) {
        const auto result = import(source);
        REQUIRE_FALSE(result);
        CHECK(manager.revision() == revision);
        CHECK(manager.catalog().version() == version);
        CHECK(manager.catalog().schemes() == schemes);
        CHECK(manager.selection() == selection);
        CHECK(&manager.style() == style);
        CHECK(&manager.design_system() == view);
        CHECK(
            manager.theme().palette.primary
            == foundation::nan_named_color(foundation::NanNamedColor::steelblue)
        );
    }
}
