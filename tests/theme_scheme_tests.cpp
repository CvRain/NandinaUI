#include <nandina/reactive/scope.hpp>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/theme/style_document.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/theme/theme_presentation.hpp>
#include <nandina/theme/theme_scheme.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace theme = nandina::theme;

namespace
{
    auto scheme_id(std::string_view text) -> theme::ThemeSchemeId {
        auto result = theme::ThemeSchemeId::parse(text);
        REQUIRE(result.has_value());
        return *result;
    }

    auto family_id(std::string_view text) -> theme::ThemeFamilyId {
        auto result = theme::ThemeFamilyId::parse(text);
        REQUIRE(result.has_value());
        return *result;
    }

    auto make_scheme(
        const std::string_view id,
        const theme::ColorAppearance appearance,
        std::shared_ptr<const theme::ThemeSchemeContent> content =
            std::make_shared<const theme::ThemeSchemeContent>(theme::ThemeSchemeContent::from(
                theme::default_design_system(),
                theme::ColorAppearance::light
            )),
        std::optional<theme::ThemeFamilyId> family = std::nullopt
    ) -> theme::ThemeScheme {
        return {
            .id = scheme_id(id),
            .display_name = std::string(id),
            .appearance = appearance,
            .family = std::move(family),
            .content = std::move(content),
        };
    }
} // namespace

TEST_CASE("scheme ids are canonical and family ids are strongly typed", "[theme][scheme][id]") {
    static_assert(!std::same_as<theme::ThemeSchemeId, theme::ThemeFamilyId>);
    constexpr std::string_view text = "org.example:catppuccin/macchiato";
    const auto scheme = theme::ThemeSchemeId::parse(text);
    REQUIRE(scheme);
    CHECK(scheme->text() == text);
    CHECK(theme::ThemeFamilyId::parse("org.example:catppuccin"));
}

TEST_CASE("scheme ids reject malformed and noncanonical text", "[theme][scheme][id]") {
    for (const std::string_view value:
         {"",
          "theme",
          ":name",
          "ns:",
          "Org.example:name",
          "org..example:name",
          "org.example:Name",
          "org.example:name/",
          "org.example:name//child",
          "org.example:../name",
          "org.example:name with space",
          "org.example:nämé"})
    {
        INFO(value);
        CHECK_FALSE(theme::ThemeSchemeId::parse(value));
    }
    const std::string too_long(256, 'a');
    CHECK(
        theme::ThemeSchemeId::parse("org.example:" + too_long).error().kind
        == theme::ThemeSchemeIdErrorKind::too_long
    );
    const auto bad_segment = theme::ThemeSchemeId::parse("org.example:good//bad");
    REQUIRE_FALSE(bad_segment);
    CHECK(bad_segment.error().kind == theme::ThemeSchemeIdErrorKind::invalid_name);
    // 空段由相邻分隔符构成；offset 指向空段本体的起始（第二个 '/'，相对 name 偏移 5，
    // 绝对偏移 12 + 5 = 17），而不是下一个段的字符。
    CHECK(bad_segment.error().offset == 17);
}

TEST_CASE(
    "families group schemes without selecting appearance variants",
    "[theme][scheme][catalog]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto family = family_id("org.example:catppuccin");
    REQUIRE(catalog.register_family({.id = family, .display_name = "Catppuccin"}));
    CHECK_FALSE(catalog.register_family({.id = family, .display_name = "Duplicate"}));
    const auto light = make_scheme(
        "org.example:catppuccin/latte",
        theme::ColorAppearance::light,
        std::make_shared<const theme::ThemeSchemeContent>(theme::ThemeSchemeContent::from(
            theme::default_design_system(),
            theme::ColorAppearance::light
        )),
        family
    );
    auto dark_content = theme::default_design_system();
    dark_content.tokens.radius.md += 2;
    REQUIRE(catalog.register_scheme(make_scheme(
        "org.example:catppuccin/mocha",
        theme::ColorAppearance::dark,
        std::make_shared<const theme::ThemeSchemeContent>(
            theme::ThemeSchemeContent::from(dark_content, theme::ColorAppearance::dark)
        ),
        family
    )));
    REQUIRE(catalog.register_scheme(light));
    CHECK_FALSE(catalog.register_scheme(light));
    CHECK(catalog.schemes_in_family(family).size() == 2);
    REQUIRE(catalog.find(light.id));
    CHECK(catalog.find(light.id)->display_name == light.display_name);
}

TEST_CASE(
    "selection validation is atomic and appearance slots are typed by metadata",
    "[theme][scheme][selection]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto light = make_scheme("org.example:light", theme::ColorAppearance::light);
    const auto dark = make_scheme("org.example:dark", theme::ColorAppearance::dark);
    REQUIRE(catalog.register_scheme(light));
    REQUIRE(catalog.register_scheme(dark));
    REQUIRE(catalog.select(
        theme::FixedThemeSelection {theme::FixedThemeSource {light.id}},
        theme::ColorAppearance::dark
    ));
    const auto before_selection = catalog.selection();
    const auto* before_active = catalog.active();
    const auto invalid = catalog.select(
        theme::AppearanceThemeSelection {
            .light = dark.id,
            .dark = light.id,
            .preference = theme::ThemePreference::dark,
        },
        theme::ColorAppearance::light
    );
    CHECK_FALSE(invalid);
    CHECK(invalid.error().kind == theme::ThemeSelectionErrorKind::appearance_mismatch);
    CHECK(catalog.selection() == before_selection);
    CHECK(catalog.active() == before_active);
    CHECK(catalog.active_appearance() == theme::ColorAppearance::light);
}

TEST_CASE(
    "fixed selection ignores system appearance while appearance selection follows its pair",
    "[theme][scheme][selection]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto light = make_scheme("org.example:light", theme::ColorAppearance::light);
    const auto dark = make_scheme("org.example:dark", theme::ColorAppearance::dark);
    REQUIRE(catalog.register_scheme(light));
    REQUIRE(catalog.register_scheme(dark));
    REQUIRE(catalog.select(
        theme::FixedThemeSelection {theme::FixedThemeSource {light.id}},
        theme::ColorAppearance::dark
    ));
    CHECK(catalog.active_appearance() == theme::ColorAppearance::light);
    REQUIRE(catalog.select(
        theme::AppearanceThemeSelection {
            .light = light.id,
            .dark = dark.id,
            .preference = theme::ThemePreference::system,
        },
        theme::ColorAppearance::dark
    ));
    CHECK(catalog.active()->id == dark.id);
    REQUIRE(catalog.select(
        theme::AppearanceThemeSelection {
            .light = light.id,
            .dark = dark.id,
            .preference = theme::ThemePreference::light,
        },
        theme::ColorAppearance::dark
    ));
    CHECK(catalog.active()->id == light.id);
}

TEST_CASE(
    "replacement resolves against a staged directory and commits only on success",
    "[theme][scheme][catalog]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto light = make_scheme("org.example:light", theme::ColorAppearance::light);
    const auto dark = make_scheme("org.example:dark", theme::ColorAppearance::dark);
    REQUIRE(catalog.register_scheme(light));
    REQUIRE(catalog.register_scheme(dark));
    REQUIRE(catalog.select(
        theme::AppearanceThemeSelection {
            .light = light.id,
            .dark = dark.id,
            .preference = theme::ThemePreference::system,
        },
        theme::ColorAppearance::light
    ));
    CHECK_FALSE(
        catalog.replace_scheme(make_scheme("org.example:dark", theme::ColorAppearance::light))
    );
    CHECK(catalog.find(dark.id)->appearance == theme::ColorAppearance::dark);
    auto updated = theme::default_design_system();
    updated.tokens.radius.md += 5;
    REQUIRE(catalog.replace_scheme(make_scheme(
        "org.example:light",
        theme::ColorAppearance::light,
        std::make_shared<const theme::ThemeSchemeContent>(
            theme::ThemeSchemeContent::from(updated, theme::ColorAppearance::light)
        )
    )));
    CHECK(catalog.active_content()->tokens.radius.md == updated.tokens.radius.md);
}

TEST_CASE(
    "removal protects schemes referenced in either appearance slot",
    "[theme][scheme][catalog]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto light = make_scheme("org.example:light", theme::ColorAppearance::light);
    const auto dark = make_scheme("org.example:dark", theme::ColorAppearance::dark);
    auto unused = make_scheme("org.example:unused", theme::ColorAppearance::light);
    REQUIRE(catalog.register_scheme(light));
    REQUIRE(catalog.register_scheme(dark));
    REQUIRE(catalog.register_scheme(unused));
    REQUIRE(catalog.select(
        theme::AppearanceThemeSelection {
            .light = light.id,
            .dark = dark.id,
            .preference = theme::ThemePreference::light,
        },
        theme::ColorAppearance::light
    ));
    CHECK_FALSE(catalog.remove_scheme(dark.id));
    CHECK_FALSE(catalog.remove_scheme(light.id));
    REQUIRE(catalog.remove_scheme(unused.id));
}

TEST_CASE(
    "anonymous sources have no scheme identity and require nonnull content",
    "[theme][scheme][selection]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto content =
        std::make_shared<const theme::ThemeSchemeContent>(theme::ThemeSchemeContent::from(
            theme::default_design_system(),
            theme::ColorAppearance::light
        ));
    REQUIRE(catalog.select(
        theme::FixedThemeSelection {theme::FixedThemeSource {
            theme::UnregisteredThemeSource {content, theme::ColorAppearance::dark}
        }},
        theme::ColorAppearance::light
    ));
    CHECK(catalog.active() == nullptr);
    CHECK(catalog.active_content() == content.get());
    CHECK(catalog.active_appearance() == theme::ColorAppearance::dark);
    CHECK_FALSE(catalog.select(
        theme::FixedThemeSelection {theme::FixedThemeSource {
            theme::UnregisteredThemeSource {{}, theme::ColorAppearance::light}
        }},
        theme::ColorAppearance::light
    ));
    CHECK(catalog.active_content() == content.get());
}

TEST_CASE(
    "empty families can be removed but populated families remain grouped",
    "[theme][scheme][catalog]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto family = family_id("org.example:group");
    REQUIRE(catalog.register_family({.id = family, .display_name = "Group"}));
    const auto member = make_scheme(
        "org.example:group/item",
        theme::ColorAppearance::light,
        std::make_shared<const theme::ThemeSchemeContent>(theme::ThemeSchemeContent::from(
            theme::default_design_system(),
            theme::ColorAppearance::light
        )),
        family
    );
    REQUIRE(catalog.register_scheme(member));
    CHECK_FALSE(catalog.remove_family(family));
    REQUIRE(catalog.remove_scheme(member.id));
    REQUIRE(catalog.remove_family(family));
}

TEST_CASE(
    "production scheme manager resolves cross-family pairs and fixed anonymous sources",
    "[theme][scheme][manager]"
) {
    theme::ThemeManager manager;
    const auto a = family_id("org.example:a");
    const auto b = family_id("org.example:b");
    REQUIRE(manager.register_family({a, "A"}));
    REQUIRE(manager.register_family({b, "B"}));
    auto light = make_scheme("org.example:a/light", theme::ColorAppearance::light);
    auto dark = make_scheme("org.example:b/dark", theme::ColorAppearance::dark);
    light.family = a;
    dark.family = b;
    REQUIRE(manager.register_scheme(light));
    REQUIRE(manager.register_scheme(dark));
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light.id, dark.id}));
    const auto before = manager.revision();
    manager.set_system_preferences({theme::ColorAppearance::dark, true});
    REQUIRE(manager.active_scheme_id());
    CHECK(*manager.active_scheme_id() == dark.id);
    CHECK(manager.revision() == before + 1);
    CHECK(manager.reduced_motion());
    CHECK(manager.appearance() == theme::ColorAppearance::dark);
    CHECK(manager.catalog().find(light.id)->family == a);
    CHECK(manager.catalog().find(dark.id)->family == b);

    REQUIRE(manager.select(theme::FixedThemeSelection {dark.id}));
    const auto fixed_revision = manager.revision();
    manager.set_system_appearance(theme::ColorAppearance::light);
    CHECK(manager.revision() == fixed_revision);
    CHECK(manager.appearance() == theme::ColorAppearance::dark);
    REQUIRE(manager.select(
        theme::FixedThemeSelection {
            theme::UnregisteredThemeSource {light.content, theme::ColorAppearance::light}
        }
    ));
    CHECK(manager.active_scheme_id() == nullptr);
    CHECK(
        std::holds_alternative<theme::UnregisteredThemeSource>(
            std::get<theme::FixedThemeSelection>(manager.selection()).source
        )
    );
    const auto anonymous_revision = manager.revision();
    REQUIRE(manager.select(manager.selection()));
    CHECK(manager.revision() == anonymous_revision);
    manager.set_system_appearance(theme::ColorAppearance::dark);
    CHECK(manager.appearance() == theme::ColorAppearance::light);
    CHECK(manager.revision() == anonymous_revision);
}

TEST_CASE(
    "scheme manager catalog failures preserve published state and unused sources",
    "[theme][scheme][manager]"
) {
    theme::ThemeManager manager;
    auto light = make_scheme("org.example:light", theme::ColorAppearance::light);
    auto dark = make_scheme("org.example:dark", theme::ColorAppearance::dark);
    auto future = make_scheme("org.example:future", theme::ColorAppearance::dark);
    REQUIRE(manager.register_scheme(light));
    REQUIRE(manager.register_scheme(dark));
    REQUIRE(manager.register_scheme(future));
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light.id, dark.id}));
    const auto selection = manager.selection();
    const auto snapshot = manager.design_system_shared();
    const auto revision = manager.revision();
    auto invalid = light;
    invalid.content.reset();
    CHECK_FALSE(manager.replace_scheme(invalid));
    CHECK_FALSE(manager.register_scheme(light));
    CHECK_FALSE(manager.remove_scheme(dark.id));
    CHECK_FALSE(manager.select(theme::AppearanceThemeSelection {light.id, light.id}));
    CHECK_FALSE(
        manager.select(theme::AppearanceThemeSelection {light.id, scheme_id("org.example:missing")})
    );
    CHECK(manager.selection() == selection);
    CHECK(manager.design_system_shared() == snapshot);
    CHECK(manager.revision() == revision);
    auto bad_class = dark;
    bad_class.appearance = theme::ColorAppearance::light;
    const auto incompatible = manager.replace_scheme(bad_class);
    REQUIRE_FALSE(incompatible);
    CHECK(incompatible.error().kind == theme::ThemeCatalogErrorKind::appearance_mismatch);
    CHECK(manager.catalog().find(dark.id)->appearance == theme::ColorAppearance::dark);
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light.id, future.id}));
    CHECK(manager.design_system_shared() == snapshot);
    CHECK(manager.revision() == revision);
    REQUIRE(manager.remove_scheme(dark.id));
    auto changed = std::make_shared<theme::ThemeSchemeContent>(*future.content);
    changed->tokens.radius.md += 7;
    future.content = changed;
    REQUIRE(manager.replace_scheme(future));
    CHECK(manager.revision() == revision);
    manager.set_system_appearance(theme::ColorAppearance::dark);
    CHECK(manager.theme().tokens.radius.md == changed->tokens.radius.md);
    const auto active_revision = manager.revision();
    REQUIRE(manager.replace_scheme(future));
    CHECK(manager.revision() == active_revision);
    auto replacement = std::make_shared<theme::ThemeSchemeContent>(*future.content);
    replacement->tokens.radius.md += 3;
    future.content = replacement;
    REQUIRE(manager.replace_scheme(future));
    CHECK(manager.revision() == active_revision + 1);
    CHECK(manager.theme().tokens.radius.md == replacement->tokens.radius.md);
}

TEST_CASE(
    "all canonical catalog writes reject reentry and publish consistent snapshots",
    "[theme][scheme][manager]"
) {
    nandina::reactive::Graph graph;
    nandina::reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    auto light = make_scheme("org.example:light", theme::ColorAppearance::light);
    auto dark = make_scheme("org.example:dark", theme::ColorAppearance::dark);
    const auto group = family_id("org.example:empty");
    REQUIRE(manager.register_scheme(light));
    REQUIRE(manager.register_scheme(dark));
    REQUIRE(manager.register_family({group, "Empty"}));
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int calls = 0;
    auto sub = state.subscribe([&](auto next) {
        ++calls;
        CHECK(next.appearance == manager.appearance());
        CHECK(next.revision == manager.revision());
        CHECK(manager.catalog().active()->id == *manager.active_scheme_id());
        CHECK(
            manager.design_system().palette(next.appearance).primary
            == manager.theme().palette.primary
        );
        CHECK_THROWS_AS(manager.select(theme::FixedThemeSelection {light.id}), std::logic_error);
        CHECK_THROWS_AS(manager.register_scheme(light), std::logic_error);
        CHECK_THROWS_AS(manager.replace_scheme(dark), std::logic_error);
        CHECK_THROWS_AS(manager.remove_scheme(light.id), std::logic_error);
        CHECK_THROWS_AS(
            manager.register_family({family_id("org.example:more"), "More"}),
            std::logic_error
        );
        CHECK_THROWS_AS(manager.remove_family(group), std::logic_error);
    });
    REQUIRE(manager.select(theme::FixedThemeSelection {dark.id}));
    CHECK(calls == 1);
}

TEST_CASE(
    "ID length boundary and catalog copies retain independent active identities",
    "[theme][scheme][id]"
) {
    const std::string longest = "ns:" + std::string(252, 'a');
    const auto parsed = theme::ThemeSchemeId::parse(longest);
    REQUIRE(parsed);
    CHECK(parsed->text() == longest);
    const auto round_trip = theme::ThemeSchemeId::parse(std::string(parsed->text()));
    REQUIRE(round_trip);
    CHECK(*round_trip == *parsed);
    CHECK_FALSE(theme::ThemeSchemeId::parse(longest + "a"));
    theme::ThemeSchemeCatalog copy;
    auto entry = make_scheme("org.example:copy", theme::ColorAppearance::dark);
    {
        theme::ThemeSchemeCatalog source;
        REQUIRE(source.register_scheme(entry));
        REQUIRE(
            source.select(theme::FixedThemeSelection {entry.id}, theme::ColorAppearance::light)
        );
        copy = source;
        CHECK(copy.active() != source.active());
    }
    REQUIRE(copy.active());
    CHECK(copy.active()->id == entry.id);
    CHECK(copy.active_content() == entry.content.get());
}

TEST_CASE(
    "style alias conversion failure is reported without a partial catalog commit",
    "[theme][scheme][import][review]"
) {
    theme::ThemeManager manager;
    theme::StyleDocument document;
    document.active_theme = "a";
    document.themes.emplace("a", theme::default_theme());
    document.themes.emplace(std::string(128, 'z'), theme::default_theme());
    const auto revision = manager.revision();
    const auto count = manager.catalog().schemes().size();
    const auto selected = manager.selection();
    const auto snapshot = manager.design_system_shared();
    std::expected<void, std::string> result;
    REQUIRE_NOTHROW(result = document.apply(manager));
    CHECK_FALSE(result);
    CHECK(manager.catalog().schemes().size() == count);
    CHECK(manager.selection() == selected);
    CHECK(manager.design_system_shared() == snapshot);
    CHECK(manager.revision() == revision);
}

TEST_CASE(
    "style import notifies once after all content and selection are ready",
    "[theme][scheme][import][review]"
) {
    nandina::reactive::Graph graph;
    nandina::reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    theme::StyleDocument document;
    document.themes.emplace("light", theme::default_theme());
    document.themes.emplace("dark", theme::default_theme());
    document.themes.at("light").tokens.radius.md = 19;
    document.themes.at("dark").tokens.radius.md = 31;
    document.theme_families.emplace("pair", theme::StyleThemePair {"light", "dark"});
    document.active_family = "pair";
    document.preference = theme::ThemePreference::light;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int events = 0;
    auto sub = state.subscribe([&](auto) {
        ++events;
        CHECK(manager.theme().tokens.radius.md == 19);
        CHECK(std::holds_alternative<theme::AppearanceThemeSelection>(manager.selection()));
        CHECK(
            std::get<theme::AppearanceThemeSelection>(manager.selection()).preference
            == theme::ThemePreference::light
        );
        REQUIRE(manager.catalog().active());
        CHECK(manager.catalog().active()->family == family_id("styles:x70616972"));
    });
    REQUIRE(document.apply(manager));
    CHECK(events == 1);
    events = 0;
    REQUIRE(document.apply(manager));
    CHECK(events == 1);
}

TEST_CASE(
    "inactive file families are validated and represented without shared-theme reassignment",
    "[theme][scheme][import][review]"
) {
    theme::ThemeManager manager;
    theme::StyleDocument document;
    document.themes.emplace("light", theme::default_theme());
    document.themes.emplace("dark", theme::default_theme());
    document.theme_families.emplace("a", theme::StyleThemePair {"light", "dark"});
    document.theme_families.emplace("b", theme::StyleThemePair {"light", "dark"});
    document.active_family = "a";
    REQUIRE(document.apply(manager));
    const auto a = family_id("styles:x61");
    const auto b = family_id("styles:x62");
    CHECK(manager.catalog().schemes_in_family(a).size() == 2);
    CHECK(manager.catalog().schemes_in_family(b).size() == 2);
    document.active_family = "b";
    REQUIRE(document.apply(manager));
    CHECK(manager.catalog().schemes_in_family(a).size() == 2);
    CHECK(manager.catalog().schemes_in_family(b).size() == 2);
    const auto count = manager.catalog().schemes().size();
    const auto revision = manager.revision();
    document.theme_families.emplace("invalid", theme::StyleThemePair {"missing", "dark"});
    CHECK_FALSE(document.apply(manager));
    CHECK(manager.catalog().schemes().size() == count);
    CHECK(manager.revision() == revision);
}

TEST_CASE(
    "canonical selection failures and callback exceptions have separate commit boundaries",
    "[theme][scheme][manager]"
) {
    nandina::reactive::Graph graph;
    nandina::reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    auto dark = make_scheme("org.example:solo-dark", theme::ColorAppearance::dark);
    REQUIRE(manager.register_scheme(dark));
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int calls = 0;
    auto throwing = state.subscribe([](auto) { throw std::runtime_error("after commit"); });
    auto following = state.subscribe([&](auto next) {
        ++calls;
        CHECK(next == state.get());
        CHECK(*manager.active_scheme_id() == dark.id);
    });
    CHECK_THROWS_AS(manager.select(theme::FixedThemeSelection {dark.id}), std::runtime_error);
    CHECK(calls == 1);
    CHECK(manager.appearance() == theme::ColorAppearance::dark);
    CHECK(state.revision() == manager.revision());
    throwing.disconnect();
    following.disconnect();
    auto alternate = dark;
    alternate.id = scheme_id("org.example:another-id");
    REQUIRE(manager.register_scheme(alternate));
    const auto before = manager.revision();
    REQUIRE(manager.select(theme::FixedThemeSelection {alternate.id}));
    CHECK(manager.revision() == before + 1); // Identity changes even with shared content.
    CHECK(state.revision() == manager.revision());
}

TEST_CASE(
    "manager can be destroyed during a scheme replacement notification",
    "[theme][scheme][lifetime]"
) {
    nandina::reactive::Graph graph;
    nandina::reactive::ReactiveScope scope(graph);
    auto manager = std::make_unique<theme::ThemeManager>();
    auto entry = make_scheme("org.example:replace", theme::ColorAppearance::dark);
    REQUIRE(manager->register_scheme(entry));
    REQUIRE(manager->select(theme::FixedThemeSelection {entry.id}));
    auto& state = scope.own<theme::ThemePresentation>(*manager, graph);
    auto sub = state.subscribe([&](auto) { manager.reset(); });
    entry.content = std::make_shared<const theme::ThemeSchemeContent>(*entry.content);
    REQUIRE_NOTHROW((void)manager->replace_scheme(entry));
    CHECK(manager == nullptr);
    CHECK_FALSE(state.valid());
    CHECK(state.appearance() == theme::ColorAppearance::dark);
}

TEST_CASE(
    "file imports reject reentry and null style before changing the catalog",
    "[theme][scheme][import]"
) {
    nandina::reactive::Graph graph;
    nandina::reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    theme::StyleDocument document;
    document.themes.emplace("unused", theme::default_theme());
    const auto before = manager.catalog().schemes().size();
    document.style.reset();
    CHECK_FALSE(document.apply(manager));
    CHECK(manager.catalog().schemes().size() == before);
    document.style = std::make_shared<theme::NanStyle>();
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    auto sub =
        state.subscribe([&](auto) { CHECK_THROWS_AS(document.apply(manager), std::logic_error); });
    auto selection = std::get<theme::AppearanceThemeSelection>(manager.selection());
    selection.preference = theme::ThemePreference::dark;
    REQUIRE(manager.select(selection));
    CHECK(manager.catalog().schemes().size() == before);
}

TEST_CASE(
    "derived schemes deep-copy content and inherit classification",
    "[theme][scheme][author]"
) {
    auto base = make_scheme("org.test:base", theme::ColorAppearance::dark);
    auto base_content = std::make_shared<theme::ThemeSchemeContent>(*base.content);
    base_content->tokens.radius.md = 17.0F;
    base.content = base_content;
    const auto derived = theme::derive_scheme(base, scheme_id("org.test:derived"), "Derived");
    CHECK(derived.appearance == base.appearance);
    CHECK(derived.display_name == "Derived");
    CHECK(derived.content != base.content);
    CHECK(derived.content->tokens.radius.md == 17.0F);

    // Mutating the derived snapshot does not pollute the base scheme.
    auto derived_content = std::make_shared<theme::ThemeSchemeContent>(*derived.content);
    derived_content->tokens.radius.md = 99.0F;
    CHECK(base.content->tokens.radius.md == 17.0F);
    CHECK(derived.content->tokens.radius.md == 17.0F);

    // Replacing the base content later does not rewrite the already derived snapshot.
    auto replaced = std::make_shared<theme::ThemeSchemeContent>(*base.content);
    replaced->tokens.radius.md = 3.0F;
    base.content = replaced;
    CHECK(derived.content->tokens.radius.md == 17.0F);
}

TEST_CASE(
    "structural validation reports scheme setup errors before registration",
    "[theme][scheme][author]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto family = family_id("org.test:family");
    REQUIRE(catalog.register_family({.id = family, .display_name = "Family"}));

    auto valid = make_scheme("org.test:valid", theme::ColorAppearance::light);
    valid.family = family;
    auto validated = theme::validate_scheme(valid, catalog);
    REQUIRE(validated);
    REQUIRE(catalog.register_scheme(valid));

    // Duplicate identity.
    auto duplicate = theme::validate_scheme(valid, catalog);
    REQUIRE_FALSE(duplicate);
    CHECK(duplicate.error().kind == theme::ThemeSchemeErrorKind::duplicate_id);

    // Empty display name.
    auto unnamed = make_scheme("org.test:unnamed", theme::ColorAppearance::light);
    unnamed.display_name.clear();
    auto unnamed_result = theme::validate_scheme(unnamed, catalog);
    REQUIRE_FALSE(unnamed_result);
    CHECK(unnamed_result.error().kind == theme::ThemeSchemeErrorKind::empty_display_name);

    // Null content.
    auto empty = make_scheme("org.test:empty", theme::ColorAppearance::light, nullptr);
    auto empty_result = theme::validate_scheme(empty, catalog);
    REQUIRE_FALSE(empty_result);
    CHECK(empty_result.error().kind == theme::ThemeSchemeErrorKind::null_content);

    // Unknown family.
    auto orphan = make_scheme("org.test:orphan", theme::ColorAppearance::light);
    orphan.family = family_id("org.test:missing");
    auto orphan_result = theme::validate_scheme(orphan, catalog);
    REQUIRE_FALSE(orphan_result);
    CHECK(orphan_result.error().kind == theme::ThemeSchemeErrorKind::unknown_family);

    // Validation diagnostics stay descriptive.
    CHECK(!theme::describe(empty_result.error()).empty());
}

TEST_CASE(
    "catalog versions advance for entries but not for selection",
    "[theme][scheme][catalog][version]"
) {
    theme::ThemeSchemeCatalog catalog;
    const auto initial = catalog.version();
    auto light = make_scheme("org.test:light", theme::ColorAppearance::light);
    REQUIRE(catalog.register_scheme(light));
    CHECK(catalog.version() > initial);
    const auto after_registration = catalog.version();
    REQUIRE(catalog.select(theme::FixedThemeSelection {light.id}, theme::ColorAppearance::light));
    CHECK(catalog.version() == after_registration); // Selection is not a directory mutation.
    auto replaced = make_scheme("org.test:light", theme::ColorAppearance::light);
    replaced.content =
        std::make_shared<const theme::ThemeSchemeContent>(theme::ThemeSchemeContent::from(
            theme::default_design_system(),
            theme::ColorAppearance::light
        ));
    REQUIRE(catalog.replace_scheme(replaced));
    CHECK(catalog.version() > after_registration);
    const auto after_replace = catalog.version();
    // The replacement is still selected, so removal stays rejected; the version does not move.
    CHECK_FALSE(catalog.remove_scheme(light.id));
    CHECK(catalog.version() == after_replace);
}

TEST_CASE(
    "built-in families are published as canonical catalog schemes",
    "[theme][scheme][builtin]"
) {
    theme::ThemeManager manager;
    theme::register_default_theme_families(manager);
    const auto scheme_count = manager.catalog().schemes().size();
    CHECK(scheme_count == 8); // nandina:default plus three families x two appearances.

    for (const std::string_view name: {"butter", "fluent", "material"}) {
        const auto prefix = "nandina:" + std::string(name);
        const auto family = family_id(prefix);
        CHECK(manager.catalog().contains_family(family));
        const auto light = scheme_id(prefix + "/light");
        const auto dark = scheme_id(prefix + "/dark");
        const auto* light_scheme = manager.catalog().find(light);
        REQUIRE(light_scheme != nullptr);
        CHECK(light_scheme->appearance == theme::ColorAppearance::light);
        CHECK(light_scheme->family == family);
        CHECK(light_scheme->content != nullptr);
        const auto* dark_scheme = manager.catalog().find(dark);
        REQUIRE(dark_scheme != nullptr);
        CHECK(dark_scheme->appearance == theme::ColorAppearance::dark);
        CHECK(dark_scheme->family == family);
        // Each appearance owns an independent content snapshot.
        CHECK(light_scheme->content != dark_scheme->content);
        CHECK(manager.catalog().schemes_in_family(family).size() == 2);

        // Canonical selection works end to end through the new scheme API.
        REQUIRE(manager.select(
            theme::AppearanceThemeSelection {
                light,
                dark,
                theme::ThemePreference::dark,
            }
        ));
        REQUIRE(manager.active_scheme_id() != nullptr);
        CHECK(*manager.active_scheme_id() == dark);
        CHECK(manager.appearance() == theme::ColorAppearance::dark);
        CHECK(manager.design_system().dark.primary.oklch().light > 0.0F);

        // A fixed selection of the light scheme ignores the host appearance.
        REQUIRE(manager.select(theme::FixedThemeSelection {light}));
        manager.set_system_appearance(theme::ColorAppearance::dark);
        CHECK(*manager.active_scheme_id() == light);
        CHECK(manager.appearance() == theme::ColorAppearance::light);
        manager.set_system_appearance(theme::ColorAppearance::light);
    }

    // Re-registration is idempotent: existing entries are skipped, not duplicated.
    theme::register_default_theme_families(manager);
    CHECK(manager.catalog().schemes().size() == scheme_count);
    CHECK(manager.catalog().families().size() == 4); // default plus three built-in families.
}

TEST_CASE(
    "author derivation isolates nested recipes and inherited family metadata",
    "[theme][scheme][author][review]"
) {
    theme::ThemeManager manager;
    const auto family = family_id("org.test:author");
    REQUIRE(manager.register_family({family, "Author"}));
    auto original = std::make_shared<theme::ThemeSchemeContent>(theme::ThemeSchemeContent::from(
        theme::default_design_system(),
        theme::ColorAppearance::dark
    ));
    original->components.button.rules.clear();
    theme::ButtonRecipeRule first;
    first.container_radius = theme::ThemeScalar::literal(13);
    original->components.button.rules.push_back(first);
    auto base = make_scheme("org.test:author/base", theme::ColorAppearance::dark, original, family);
    auto derived = theme::derive_scheme(base, scheme_id("org.test:author/derived"), "Derived");
    REQUIRE(derived.content);
    CHECK(derived.family == family);
    CHECK(derived.appearance == theme::ColorAppearance::dark);
    auto separate = theme::derive_content(*original);
    separate.components.button.rules[0].container_radius = theme::ThemeScalar::literal(27);
    separate.components.button.rules.push_back(first);
    CHECK(original->components.button.rules.size() == 1);
    const auto radius = [](const theme::ThemeSchemeContent& content) {
        return theme::resolve_button(
                   content.design_system(),
                   theme::ColorAppearance::dark,
                   theme::ButtonTone::primary,
                   theme::ButtonTreatment::filled,
                   theme::ButtonSize::medium,
                   theme::ButtonVisualState::normal
        )
            .container.radius;
    };
    CHECK(radius(*derived.content) == 13);
    original->components.button.rules[0].container_radius = theme::ThemeScalar::literal(39);
    CHECK(radius(*derived.content) == 13);
    CHECK(radius(*original) == 39);
    REQUIRE(theme::validate_scheme(derived, manager.catalog()));
    REQUIRE(manager.register_scheme(base));
    REQUIRE(manager.register_scheme(derived));
    REQUIRE(manager.select(theme::FixedThemeSelection {derived.id}));
    const auto revision = manager.revision();
    auto updated = std::make_shared<theme::ThemeSchemeContent>(*original);
    updated->components.button.rules[0].container_radius = theme::ThemeScalar::literal(51);
    base.content = updated;
    REQUIRE(manager.replace_scheme(base));
    CHECK(manager.revision() == revision);
    CHECK(radius(*manager.catalog().find(derived.id)->content) == 13);
    CHECK(radius(*manager.catalog().find(base.id)->content) == 51);
}

TEST_CASE(
    "author structural validation rejects an invalid ID without changing the directory",
    "[theme][scheme][author][review]"
) {
    theme::ThemeManager manager;
    const auto version = manager.catalog().version();
    auto invalid = make_scheme("org.test:valid-first", theme::ColorAppearance::light);
    invalid.id = {};
    const auto result = theme::validate_scheme(invalid, manager.catalog());
    REQUIRE_FALSE(result);
    CHECK(result.error().kind == theme::ThemeSchemeErrorKind::invalid_id);
    CHECK_FALSE(theme::describe(result.error()).empty());
    CHECK(manager.catalog().version() == version);
}
