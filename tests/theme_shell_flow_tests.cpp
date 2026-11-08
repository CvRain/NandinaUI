// Shell multi-scheme workflow: page-level selection, catalog and effective-theme flows.
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/theme/theme_observation.hpp>
#include <nandina/widget/build_context.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace theme = nandina::theme;
namespace widget = nandina::widget;
namespace reactive = nandina::reactive;

namespace
{
    auto scheme_id(const std::string_view text) -> theme::ThemeSchemeId {
        const auto parsed = theme::ThemeSchemeId::parse(text);
        REQUIRE(parsed);
        return *parsed;
    }

    auto family_id(const std::string_view text) -> theme::ThemeFamilyId {
        const auto parsed = theme::ThemeFamilyId::parse(text);
        REQUIRE(parsed);
        return *parsed;
    }

    auto content(const theme::ColorAppearance appearance, const float radius = 12.0F)
        -> std::shared_ptr<const theme::ThemeSchemeContent> {
        auto system = theme::default_design_system();
        system.tokens.radius.md = radius;
        return std::make_shared<const theme::ThemeSchemeContent>(
            theme::ThemeSchemeContent::from(system, appearance)
        );
    }

    auto scheme(
        const std::string_view id,
        const theme::ColorAppearance appearance,
        const float radius = 12.0F,
        const std::optional<theme::ThemeFamilyId> family = std::nullopt
    ) -> theme::ThemeScheme {
        return {
            .id = scheme_id(id),
            .display_name = std::string(id),
            .appearance = appearance,
            .family = family,
            .content = content(appearance, radius),
        };
    }

    struct ShellProbe {
        explicit ShellProbe(theme::ThemeManager& manager): ui(graph, page_scope, manager) {
            ui.on_theme_selection_change([this](auto) { ++selection_changes; });
            ui.on_theme_catalog_change([this](auto) { ++catalog_changes; });
            ui.on_theme_change([this](auto) { ++theme_changes; });
        }

        reactive::Graph graph;
        reactive::ReactiveScope page_scope {graph};
        widget::BuildContext ui;
        int selection_changes = 0;
        int catalog_changes = 0;
        int theme_changes = 0;
    };
} // namespace

TEST_CASE(
    "shell distinguishes selection-only success from effective changes",
    "[theme][shell][selection]"
) {
    SECTION("system dark to fixed dark keeps the effective snapshot") {
        theme::ThemeManager manager;
        const auto light = scheme("org.shell:light", theme::ColorAppearance::light);
        const auto dark = scheme("org.shell:dark", theme::ColorAppearance::dark);
        REQUIRE(manager.register_scheme(light));
        REQUIRE(manager.register_scheme(dark));
        REQUIRE(manager.select(
            theme::AppearanceThemeSelection {
                light.id,
                dark.id,
                theme::ThemePreference::system,
            }
        ));
        manager.set_system_appearance(theme::ColorAppearance::dark);

        ShellProbe shell(manager);
        const auto revision = manager.revision();
        const auto snapshot = manager.design_system_shared();
        REQUIRE(manager.select(theme::FixedThemeSelection {dark.id}));
        CHECK(shell.selection_changes == 1);
        CHECK(shell.theme_changes == 0);
        CHECK(manager.revision() == revision);
        CHECK(manager.design_system_shared() == snapshot);
        CHECK(manager.appearance() == theme::ColorAppearance::dark);
    }

    SECTION("editing the inactive dark slot is staged until dark becomes active") {
        theme::ThemeManager manager;
        const auto light = scheme("org.shell:light", theme::ColorAppearance::light);
        const auto dark = scheme("org.shell:dark", theme::ColorAppearance::dark, 20.0F);
        const auto dark_2 = scheme("org.shell:dark-2", theme::ColorAppearance::dark, 30.0F);
        REQUIRE(manager.register_scheme(light));
        REQUIRE(manager.register_scheme(dark));
        REQUIRE(manager.register_scheme(dark_2));
        REQUIRE(manager.select(
            theme::AppearanceThemeSelection {
                light.id,
                dark.id,
                theme::ThemePreference::light,
            }
        ));
        ShellProbe shell(manager);
        const auto revision = manager.revision();

        REQUIRE(manager.select(
            theme::AppearanceThemeSelection {
                light.id,
                dark_2.id,
                theme::ThemePreference::light,
            }
        ));
        CHECK(shell.selection_changes == 1);
        CHECK(shell.theme_changes == 0);
        CHECK(manager.revision() == revision);
        CHECK(manager.appearance() == theme::ColorAppearance::light);

        REQUIRE(manager.select(
            theme::AppearanceThemeSelection {
                light.id,
                dark_2.id,
                theme::ThemePreference::dark,
            }
        ));
        CHECK(shell.selection_changes == 2);
        CHECK(shell.theme_changes == 1);
        CHECK(manager.revision() == revision + 1);
        CHECK(manager.appearance() == theme::ColorAppearance::dark);
        CHECK(manager.design_system().tokens.radius.md == 30.0F);
    }

    SECTION("switching between two dark schemes changes identity and content") {
        theme::ThemeManager manager;
        const auto light = scheme("org.shell:light", theme::ColorAppearance::light);
        const auto dark = scheme("org.shell:dark", theme::ColorAppearance::dark, 20.0F);
        const auto dark_2 = scheme("org.shell:dark-2", theme::ColorAppearance::dark, 32.0F);
        REQUIRE(manager.register_scheme(light));
        REQUIRE(manager.register_scheme(dark));
        REQUIRE(manager.register_scheme(dark_2));
        REQUIRE(manager.select(
            theme::AppearanceThemeSelection {
                light.id,
                dark.id,
                theme::ThemePreference::system,
            }
        ));
        manager.set_system_appearance(theme::ColorAppearance::dark);
        ShellProbe shell(manager);
        const auto revision = manager.revision();

        REQUIRE(manager.select(
            theme::AppearanceThemeSelection {
                light.id,
                dark_2.id,
                theme::ThemePreference::system,
            }
        ));
        CHECK(shell.selection_changes == 1);
        CHECK(shell.theme_changes == 1);
        CHECK(manager.revision() == revision + 1);
        REQUIRE(manager.active_scheme_id());
        CHECK(*manager.active_scheme_id() == dark_2.id);
        CHECK(manager.design_system().tokens.radius.md == 32.0F);
    }
}

TEST_CASE(
    "shell catalog changes stay independent from effective theme publication",
    "[theme][shell][catalog]"
) {
    theme::ThemeManager manager;
    auto unused = scheme("org.shell:unused", theme::ColorAppearance::light);
    REQUIRE(manager.register_scheme(unused));
    ShellProbe shell(manager);
    const auto revision = manager.revision();
    const auto snapshot = manager.design_system_shared();

    unused.display_name = "Renamed";
    unused.content = content(theme::ColorAppearance::light, 26.0F);
    REQUIRE(manager.replace_scheme(unused));
    CHECK(shell.catalog_changes == 1);
    CHECK(shell.selection_changes == 0);
    CHECK(shell.theme_changes == 0);

    REQUIRE(manager.remove_scheme(unused.id));
    CHECK(shell.catalog_changes == 2);
    CHECK(shell.selection_changes == 0);
    CHECK(shell.theme_changes == 0);

    const auto family = family_id("org.shell:empty");
    REQUIRE(manager.register_family({family, "Empty"}));
    REQUIRE(manager.remove_family(family));
    CHECK(shell.catalog_changes == 4);
    CHECK(shell.selection_changes == 0);
    CHECK(shell.theme_changes == 0);
    CHECK(manager.revision() == revision);
    CHECK(manager.design_system_shared() == snapshot);
}

TEST_CASE(
    "active slot replacement publishes while inactive replacement is staged",
    "[theme][shell][replacement]"
) {
    theme::ThemeManager manager;
    auto light = scheme("org.shell:light", theme::ColorAppearance::light, 10.0F);
    auto dark = scheme("org.shell:dark", theme::ColorAppearance::dark, 20.0F);
    REQUIRE(manager.register_scheme(light));
    REQUIRE(manager.register_scheme(dark));
    REQUIRE(manager.select(theme::FixedThemeSelection {light.id}));
    ShellProbe shell(manager);

    const auto revision = manager.revision();
    const auto snapshot = manager.design_system_shared();
    dark.content = content(theme::ColorAppearance::dark, 24.0F);
    REQUIRE(manager.replace_scheme(dark));
    CHECK(shell.catalog_changes == 1);
    CHECK(shell.theme_changes == 0);
    CHECK(manager.revision() == revision);
    CHECK(manager.design_system_shared() == snapshot);

    light.content = content(theme::ColorAppearance::light, 18.0F);
    REQUIRE(manager.replace_scheme(light));
    CHECK(shell.catalog_changes == 2);
    CHECK(shell.theme_changes == 1);
    CHECK(manager.revision() == revision + 1);
    CHECK(manager.design_system().tokens.radius.md == 18.0F);
}

TEST_CASE(
    "independent shell pages follow external state and detach with their scopes",
    "[theme][shell][observation]"
) {
    theme::ThemeManager manager;
    ShellProbe first(manager);
    ShellProbe second(manager);
    const auto light = scheme("org.shell:light", theme::ColorAppearance::light);
    const auto dark = scheme("org.shell:dark", theme::ColorAppearance::dark);

    REQUIRE(manager.register_scheme(light));
    CHECK(first.catalog_changes == 1);
    CHECK(second.catalog_changes == 1);
    REQUIRE(manager.register_scheme(dark));
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light.id, dark.id}));
    CHECK(first.selection_changes == 1);
    CHECK(second.selection_changes == 1);

    first.page_scope.clear();
    REQUIRE(manager.register_scheme(scheme("org.shell:dark-2", theme::ColorAppearance::dark)));
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light.id, dark.id}));
    CHECK(first.catalog_changes == 2);
    CHECK(first.selection_changes == 1);
    CHECK(second.catalog_changes == 3);
    CHECK(second.selection_changes == 1);
}

TEST_CASE(
    "shell failures expose typed subjects and preserve committed state",
    "[theme][shell][failure]"
) {
    theme::ThemeManager manager;
    const auto family = family_id("org.shell:family");
    REQUIRE(manager.register_family({family, "Family"}));
    auto light = scheme("org.shell:family/light", theme::ColorAppearance::light, 10.0F, family);
    auto dark = scheme("org.shell:family/dark", theme::ColorAppearance::dark, 20.0F, family);
    REQUIRE(manager.register_scheme(light));
    REQUIRE(manager.register_scheme(dark));
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light.id, dark.id}));
    ShellProbe shell(manager);

    const auto revision = manager.revision();
    const auto version = manager.catalog().version();
    const auto selection = manager.selection();
    const auto schemes = manager.catalog().schemes();
    const auto snapshot = manager.design_system_shared();

    const auto missing =
        manager.select(theme::FixedThemeSelection {scheme_id("org.shell:missing")});
    REQUIRE_FALSE(missing);
    CHECK(missing.error().kind == theme::ThemeSelectionErrorKind::unknown_scheme);

    const auto wrong_slot = manager.select(theme::AppearanceThemeSelection {light.id, light.id});
    REQUIRE_FALSE(wrong_slot);
    CHECK(wrong_slot.error().kind == theme::ThemeSelectionErrorKind::appearance_mismatch);

    const auto protected_scheme = manager.remove_scheme(light.id);
    REQUIRE_FALSE(protected_scheme);
    CHECK(protected_scheme.error().kind == theme::ThemeCatalogErrorKind::scheme_in_use);
    CHECK(protected_scheme.error().scheme == light.id);

    const auto protected_family = manager.remove_family(family);
    REQUIRE_FALSE(protected_family);
    CHECK(protected_family.error().kind == theme::ThemeCatalogErrorKind::family_in_use);
    CHECK(protected_family.error().family == family);
    CHECK(theme::describe(protected_family.error()) != theme::describe(protected_scheme.error()));

    auto unknown_family = scheme("org.shell:unknown-family", theme::ColorAppearance::light);
    unknown_family.family = family_id("org.shell:not-registered");
    const auto unknown = manager.register_scheme(unknown_family);
    REQUIRE_FALSE(unknown);
    CHECK(unknown.error().kind == theme::ThemeCatalogErrorKind::unknown_family);
    CHECK(unknown.error().scheme == unknown_family.id);
    CHECK(unknown.error().family == unknown_family.family);

    CHECK(shell.selection_changes == 0);
    CHECK(shell.catalog_changes == 0);
    CHECK(shell.theme_changes == 0);
    CHECK(manager.revision() == revision);
    CHECK(manager.catalog().version() == version);
    CHECK(manager.selection() == selection);
    CHECK(manager.catalog().schemes() == schemes);
    CHECK(manager.design_system_shared() == snapshot);
}
