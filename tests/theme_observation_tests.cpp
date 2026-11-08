// D2 step 4: selection/catalog observation channels and explicit theme change events.
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/style_document.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/theme/theme_observation.hpp>
#include <nandina/theme/theme_presentation.hpp>
#include <nandina/theme/theme_scheme.hpp>
#include <nandina/widget/build_context.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace reactive = nandina::reactive;
namespace theme = nandina::theme;
namespace widget = nandina::widget;

namespace
{
    auto scheme_id(std::string_view text) -> theme::ThemeSchemeId {
        auto result = theme::ThemeSchemeId::parse(text);
        REQUIRE(result.has_value());
        return *result;
    }

    auto content(const float radius = 12.0F) -> std::shared_ptr<const theme::ThemeSchemeContent> {
        auto system = theme::default_design_system();
        system.tokens.radius.md = radius;
        return std::make_shared<const theme::ThemeSchemeContent>(
            theme::ThemeSchemeContent::from(system, theme::ColorAppearance::light)
        );
    }

    auto make_scheme(
        const std::string_view id,
        const theme::ColorAppearance appearance,
        std::shared_ptr<const theme::ThemeSchemeContent> body = content(),
        const std::optional<theme::ThemeFamilyId> family = std::nullopt
    ) -> theme::ThemeScheme {
        return theme::ThemeScheme {
            .id = scheme_id(id),
            .display_name = std::string(id),
            .appearance = appearance,
            .family = family,
            .content = std::move(body),
        };
    }

    auto register_pair(theme::ThemeManager& manager)
        -> std::pair<theme::ThemeSchemeId, theme::ThemeSchemeId> {
        auto light = make_scheme("org.test:light", theme::ColorAppearance::light);
        auto dark = make_scheme("org.test:dark", theme::ColorAppearance::dark);
        REQUIRE(manager.register_scheme(light));
        REQUIRE(manager.register_scheme(dark));
        return {light.id, dark.id};
    }

    template<typename Channel>
    auto
    channel(reactive::ReactiveScope& scope, theme::ThemeManager& manager, reactive::Graph& graph)
        -> Channel& {
        if constexpr (std::same_as<Channel, theme::ThemeChangeHub>) {
            return scope.own<Channel>(manager);
        }
        else {
            return scope.own<Channel>(manager, graph);
        }
    }

    template<typename Channel>
    void change_channel(theme::ThemeManager& manager, const int index) {
        if constexpr (std::same_as<Channel, theme::ThemeCatalogPresentation>) {
            const auto result = manager.register_scheme(make_scheme(
                "org.test:channel-" + std::to_string(index),
                theme::ColorAppearance::light
            ));
            if (!result) {
                throw std::runtime_error(theme::describe(result.error()));
            }
        }
        else {
            auto next = std::get<theme::AppearanceThemeSelection>(manager.selection());
            next.preference =
                index % 2 ? theme::ThemePreference::dark : theme::ThemePreference::light;
            const auto result = manager.select(next);
            if (!result) {
                throw std::runtime_error(theme::describe(result.error()));
            }
        }
    }

    template<typename Channel>
    void scope_clear_delivery() {
        reactive::Graph graph;
        reactive::ReactiveScope scope(graph);
        theme::ThemeManager manager;
        auto& current = channel<Channel>(scope, manager, graph);
        int calls = 0;
        int later = 0;
        auto clear = current.subscribe([&](auto) {
            ++calls;
            scope.clear();
        });
        auto after_clear = current.subscribe([&](auto) { ++later; });
        change_channel<Channel>(manager, 1);
        CHECK(calls == 1);
        CHECK(later == 0);
        change_channel<Channel>(manager, 2);
        CHECK(calls == 1);
        CHECK(later == 0);
    }

    template<typename Channel>
    void manager_destroy_delivery() {
        reactive::Graph graph;
        reactive::ReactiveScope scope(graph);
        auto manager = std::make_unique<theme::ThemeManager>();
        auto& current = channel<Channel>(scope, *manager, graph);
        int calls = 0;
        int later = 0;
        auto destroy = current.subscribe([&](auto) {
            ++calls;
            manager.reset();
        });
        auto after_destroy = current.subscribe([&](auto) { ++later; });
        change_channel<Channel>(*manager, 1);
        CHECK(manager == nullptr);
        CHECK_FALSE(current.valid());
        CHECK(calls == 1);
        CHECK(later == 0);
    }

    template<typename Channel>
    void reentry_and_disconnect() {
        reactive::Graph graph;
        reactive::ReactiveScope scope(graph);
        theme::ThemeManager manager;
        auto& current = channel<Channel>(scope, manager, graph);
        int calls = 0;
        int skipped = 0;
        int survivor = 0;
        auto failure = current.subscribe([&](auto) { (void)manager.select(manager.selection()); });
        reactive::Subscription later;
        auto disconnect = current.subscribe([&](auto) {
            ++calls;
            later.disconnect();
        });
        later = current.subscribe([&](auto) { ++skipped; });
        auto independent = current.subscribe([&](auto) { ++survivor; });
        CHECK_THROWS_AS(change_channel<Channel>(manager, 1), std::logic_error);
        CHECK(calls == 1);
        CHECK(skipped == 0);
        CHECK(survivor == 1);
        failure.disconnect();
        change_channel<Channel>(manager, 2);
        CHECK(calls == 2);
        CHECK(skipped == 0);
        CHECK(survivor == 2);
    }

    template<typename Channel, typename Payload>
    concept CanWriteState = requires(Channel& current, Payload value) { current.set(value); };

    template<typename Channel, typename Payload>
    concept CanPublishEvent = requires(Channel& current, Payload value) { current.emit(value); };

    static_assert(!CanWriteState<theme::ThemeSelectionPresentation, theme::ThemeSelectionState>);
    static_assert(!CanWriteState<theme::ThemeCatalogPresentation, theme::ThemeCatalogState>);
    static_assert(
        !CanWriteState<reactive::ReadSignal<theme::ThemeSelectionState>, theme::ThemeSelectionState>
    );
    static_assert(!CanPublishEvent<theme::ThemeSelectionPresentation, theme::ThemeSelectionState>);
    static_assert(!CanPublishEvent<theme::ThemeCatalogPresentation, theme::ThemeCatalogState>);
    static_assert(!CanPublishEvent<theme::ThemeChangeHub, theme::ThemeChange>);
} // namespace

TEST_CASE(
    "selection projection follows slot and preference edits without presenting a change",
    "[theme][observation][selection]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    const auto [light, dark] = register_pair(manager);
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light, dark}));

    auto& selection = scope.own<theme::ThemeSelectionPresentation>(manager, graph);
    auto& presentation = scope.own<theme::ThemePresentation>(manager, graph);
    int selection_events = 0;
    int presentation_events = 0;
    auto selection_sub = selection.subscribe([&](theme::ThemeSelectionState next) {
        ++selection_events;
        CHECK(std::holds_alternative<theme::AppearanceThemeSelection>(next));
    });
    auto presentation_sub = presentation.subscribe([&](auto) { ++presentation_events; });
    CHECK(selection.get() == manager.selection());
    CHECK(selection_events == 0); // A subscription does not fabricate an initial change.
    CHECK(presentation_events == 0);

    // Editing the dark slot in light mode changes selection only (D2 §5.1 table).
    auto dark_2 = make_scheme("org.test:dark-2", theme::ColorAppearance::dark);
    REQUIRE(manager.register_scheme(dark_2));
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light, dark_2.id}));
    CHECK(selection_events == 1);
    CHECK(presentation_events == 0);
    CHECK(std::get<theme::AppearanceThemeSelection>(selection.get()).dark == dark_2.id);

    // system -> explicit light keeps the effective appearance; selection still changes.
    REQUIRE(manager.select(
        theme::AppearanceThemeSelection {light, dark_2.id, theme::ThemePreference::light}
    ));
    CHECK(selection_events == 2);
    CHECK(presentation_events == 0);
    CHECK(
        std::get<theme::AppearanceThemeSelection>(selection.get()).preference
        == theme::ThemePreference::light
    );

    // The effective appearance is still light: back to system while the host is light.
    manager.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(manager.select(
        theme::AppearanceThemeSelection {light, dark_2.id, theme::ThemePreference::system}
    ));
    CHECK(selection_events == 3);
    CHECK(presentation_events == 1);
    CHECK(presentation.appearance() == theme::ColorAppearance::dark);
    CHECK(selection.get() == manager.selection());
}

TEST_CASE(
    "catalog projection follows directory edits that do not affect the presented theme",
    "[theme][observation][catalog]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(manager, graph);
    auto& presentation = scope.own<theme::ThemePresentation>(manager, graph);
    int catalog_events = 0;
    int presentation_events = 0;
    std::vector<std::size_t> scheme_counts;
    auto catalog_sub = catalog.subscribe([&](theme::ThemeCatalogState next) {
        ++catalog_events;
        scheme_counts.push_back(next.schemes.size());
        CHECK(next.version == manager.catalog().version());
    });
    auto presentation_sub = presentation.subscribe([&](auto) { ++presentation_events; });

    const auto initial = catalog.get();
    CHECK(initial.version == manager.catalog().version());
    CHECK(initial.schemes.size() == manager.catalog().schemes().size());
    CHECK(!initial.families.empty());
    CHECK(catalog_events == 0);

    // Registering an unused scheme only changes the directory.
    auto unused = make_scheme("org.test:unused", theme::ColorAppearance::light);
    REQUIRE(manager.register_scheme(unused));
    CHECK(catalog_events == 1);
    CHECK(presentation_events == 0);
    CHECK(scheme_counts.back() == initial.schemes.size() + 1);

    // Replacing an unused scheme also only changes the directory.
    auto replaced = make_scheme("org.test:unused", theme::ColorAppearance::light, content(20.0F));
    REQUIRE(manager.replace_scheme(replaced));
    CHECK(catalog_events == 2);
    CHECK(presentation_events == 0);

    // Family registration/removal is a directory change too.
    const auto family = *theme::ThemeFamilyId::parse("org.test:family");
    REQUIRE(manager.register_family({family, "Family"}));
    CHECK(catalog_events == 3);
    REQUIRE(manager.remove_family(family));
    CHECK(catalog_events == 4);
    CHECK(presentation_events == 0);

    // Selection-only changes do not touch the directory.
    const auto [light, dark] = register_pair(manager);
    const int after_registration = catalog_events;
    CHECK(after_registration == 6); // Two more unused schemes were registered.
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light, dark}));
    CHECK(catalog_events == after_registration);
    CHECK(presentation_events == 1); // Selection made the appearance effective for the first time.
}

TEST_CASE(
    "change hub reports effective identity and content changes only",
    "[theme][observation][change]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    const auto [light, dark] = register_pair(manager);
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light, dark}));
    auto& changes = scope.own<theme::ThemeChangeHub>(manager);
    std::vector<theme::ThemeChange> events;
    auto sub = changes.subscribe([&](theme::ThemeChange change) { events.push_back(change); });
    CHECK(events.empty());

    // Effective appearance flip: before/after carry the identity and appearance.
    manager.set_system_preferences({theme::ColorAppearance::dark, false});
    REQUIRE(events.size() == 1);
    CHECK(*events[0].before.scheme == light);
    CHECK(*events[0].after.scheme == dark);
    CHECK(events[0].before.appearance == theme::ColorAppearance::light);
    CHECK(events[0].after.appearance == theme::ColorAppearance::dark);
    CHECK(events[0].before.system != events[0].after.system);

    // Motion-only publication does not change the effective theme.
    manager.set_motion_preference(theme::MotionPreference::reduced);
    CHECK(events.size() == 1);

    // Non-effective selection edit: the host is dark, so changing only the light slot
    // changes selection without changing the effective theme.
    auto light_2 = make_scheme("org.test:light-2", theme::ColorAppearance::light);
    REQUIRE(manager.register_scheme(light_2));
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light_2.id, dark}));
    CHECK(events.size() == 1);

    // Registered identity switch with content change fires.
    auto fixed = make_scheme("org.test:fixed", theme::ColorAppearance::light, content(33.0F));
    REQUIRE(manager.register_scheme(fixed));
    REQUIRE(manager.select(theme::FixedThemeSelection {fixed.id}));
    REQUIRE(events.size() == 2);
    CHECK(*events[1].after.scheme == fixed.id);
    CHECK(events[1].after.appearance == theme::ColorAppearance::light);

    // Content replacement of the active scheme keeps identity but fires.
    auto replacement = fixed;
    replacement.content = content(44.0F);
    REQUIRE(manager.replace_scheme(replacement));
    REQUIRE(events.size() == 3);
    CHECK(events[2].before.scheme == events[2].after.scheme);
    CHECK(events[2].before.system != events[2].after.system);
}

TEST_CASE(
    "observation channels deliver committed state and isolate subscriber failures",
    "[theme][observation][delivery]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    const auto [light, dark] = register_pair(manager);
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light, dark}));
    auto& selection = scope.own<theme::ThemeSelectionPresentation>(manager, graph);
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(manager, graph);
    auto& changes = scope.own<theme::ThemeChangeHub>(manager);
    int later = 0;
    int later_changes = 0;
    auto catalog_bad = catalog.subscribe([](auto) { throw std::runtime_error("catalog failure"); });
    auto catalog_good = catalog.subscribe([&](theme::ThemeCatalogState next) {
        ++later;
        // Committed state is visible to every channel inside the callback.
        CHECK(next.version == manager.catalog().version());
        CHECK(selection.get() == manager.selection());
    });
    auto change_sub = changes.subscribe([&](const theme::ThemeChange& change) {
        ++later_changes;
        CHECK(change.before.scheme == light);
        CHECK(change.after.scheme == dark);
    });

    // A throwing subscriber does not block later subscribers; the first failure rethrows
    // after the publication completes.
    auto unused = make_scheme("org.test:unused", theme::ColorAppearance::light);
    CHECK_THROWS_AS(manager.register_scheme(unused), std::runtime_error);
    CHECK(later == 1);
    CHECK(later_changes == 0);

    catalog_bad.disconnect();
    auto unused_2 = make_scheme("org.test:unused-2", theme::ColorAppearance::light);
    REQUIRE(manager.register_scheme(unused_2));
    CHECK(later == 2);

    // Effective change reaches the change hub and the presentation projection.
    manager.set_system_preferences({theme::ColorAppearance::dark, false});
    CHECK(later_changes == 1);
    CHECK(selection.get() == manager.selection());
    CHECK(later == 2);
}

TEST_CASE(
    "observation channels survive manager and scope destruction",
    "[theme][observation][lifetime]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    auto manager = std::make_unique<theme::ThemeManager>();
    auto& selection = scope.own<theme::ThemeSelectionPresentation>(*manager, graph);
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(*manager, graph);
    auto& changes = scope.own<theme::ThemeChangeHub>(*manager);
    int events = 0;
    auto selection_sub = selection.subscribe([&](auto) { ++events; });
    auto catalog_sub = catalog.subscribe([&](auto) { ++events; });
    auto change_sub = changes.subscribe([&](auto) { ++events; });
    manager.reset();
    CHECK_FALSE(selection.valid());
    CHECK_FALSE(catalog.valid());
    CHECK_FALSE(changes.valid());
    CHECK(events == 0);
    REQUIRE_NOTHROW(selection.get());
    REQUIRE_NOTHROW(catalog.get());
    scope.clear();
    CHECK(true);
}

TEST_CASE(
    "build context exposes one scope-owned instance per observation channel",
    "[theme][observation][build-context]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope page_scope(graph);
    theme::ThemeManager manager;
    widget::BuildContext ui(graph, page_scope, manager);
    CHECK(&ui.theme_selection() == &ui.theme_selection());
    CHECK(&ui.theme_catalog() == &ui.theme_catalog());
    CHECK(&ui.theme_changes() == &ui.theme_changes());

    reactive::ReactiveScope region_scope(graph);
    auto region = ui.with_scope(region_scope);
    CHECK(&region.theme_selection() == &ui.theme_selection());
    CHECK(&region.theme_catalog() == &ui.theme_catalog());
    CHECK(&region.theme_changes() == &ui.theme_changes());

    int changes = 0;
    int selection_changes = 0;
    int catalog_changes = 0;
    ui.on_theme_change([&](const theme::ThemeChange&) { ++changes; });
    region.on_theme_selection_change([&](auto) { ++selection_changes; });
    region.on_theme_catalog_change([&](auto) { ++catalog_changes; });
    auto fixed = make_scheme("org.test:ui", theme::ColorAppearance::light);
    REQUIRE(manager.register_scheme(fixed));
    CHECK(catalog_changes == 1);
    CHECK(selection_changes == 0);
    CHECK(changes == 0);
    REQUIRE(manager.select(theme::FixedThemeSelection {fixed.id}));
    CHECK(catalog_changes == 1);
    CHECK(selection_changes == 1);
    CHECK(changes == 1);
    page_scope.clear();
    manager.set_system_appearance(theme::ColorAppearance::dark);
    const auto after_clear = make_scheme("org.test:after-clear", theme::ColorAppearance::light);
    REQUIRE(manager.register_scheme(after_clear));
    REQUIRE(manager.select(theme::FixedThemeSelection {after_clear.id}));
    CHECK(changes == 1); // Scope-owned subscription was released with the page scope.
    CHECK(selection_changes == 1);
    CHECK(catalog_changes == 1);
}

TEST_CASE(
    "equivalent catalog replacement does not publish a directory change",
    "[theme][observation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    auto unused = make_scheme("org.test:equivalent", theme::ColorAppearance::dark);
    REQUIRE(manager.register_scheme(unused));
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(manager, graph);
    int events = 0;
    int effects = 0;
    auto sub = catalog.subscribe([&](auto) { ++events; });
    scope.effect([&] {
        (void)catalog.get();
        ++effects;
    });
    const auto version = manager.catalog().version();
    const auto revision = manager.revision();
    const auto snapshot = manager.design_system_shared();
    REQUIRE(manager.replace_scheme(unused));
    CHECK(events == 0);
    CHECK(effects == 1);
    CHECK(manager.catalog().version() == version);
    CHECK(manager.revision() == revision);
    CHECK(manager.design_system_shared() == snapshot);

    unused.display_name = "Renamed";
    REQUIRE(manager.replace_scheme(unused));
    CHECK(events == 1);
    CHECK(effects == 2);
    CHECK(manager.catalog().version() == version + 1);
    CHECK(manager.revision() == revision);
    REQUIRE(manager.replace_scheme(unused));
    CHECK(events == 1);
    CHECK(effects == 2);
}

TEST_CASE(
    "each observation channel tolerates scope clear during delivery",
    "[theme][observation][lifetime][review]"
) {
    SECTION("selection") {
        scope_clear_delivery<theme::ThemeSelectionPresentation>();
    }
    SECTION("catalog") {
        scope_clear_delivery<theme::ThemeCatalogPresentation>();
    }
    SECTION("change event") {
        scope_clear_delivery<theme::ThemeChangeHub>();
    }
}

TEST_CASE(
    "each observation channel tolerates manager destruction during delivery",
    "[theme][observation][lifetime][review]"
) {
    SECTION("selection") {
        manager_destroy_delivery<theme::ThemeSelectionPresentation>();
    }
    SECTION("catalog") {
        manager_destroy_delivery<theme::ThemeCatalogPresentation>();
    }
    SECTION("change event") {
        manager_destroy_delivery<theme::ThemeChangeHub>();
    }
}

TEST_CASE(
    "each observation channel rejects reentry and isolates disconnections and failures",
    "[theme][observation][delivery][review]"
) {
    SECTION("selection") {
        reentry_and_disconnect<theme::ThemeSelectionPresentation>();
    }
    SECTION("catalog") {
        reentry_and_disconnect<theme::ThemeCatalogPresentation>();
    }
    SECTION("change event") {
        reentry_and_disconnect<theme::ThemeChangeHub>();
    }
}

TEST_CASE(
    "staged selection and directory updates do not invalidate the scene",
    "[theme][observation][staging][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    class Root final: public nandina::scene::NanControl {
    public:
        int notifications = 0;
        void on_theme_changed(const theme::ThemeManager&) override {
            ++notifications;
        }
    };
    auto root = std::make_shared<Root>();
    nandina::scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    tree.set_root(root);
    const auto initial = root->notifications;
    const auto revision = manager.revision();
    auto& selection = scope.own<theme::ThemeSelectionPresentation>(manager, graph);
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(manager, graph);
    int selection_events = 0;
    int catalog_events = 0;
    auto sel_sub = selection.subscribe([&](auto) { ++selection_events; });
    auto cat_sub = catalog.subscribe([&](auto) { ++catalog_events; });
    auto pair = std::get<theme::AppearanceThemeSelection>(manager.selection());
    pair.preference = theme::ThemePreference::light;
    REQUIRE(manager.select(pair));
    const auto spare = make_scheme("org.test:spare", theme::ColorAppearance::dark);
    REQUIRE(manager.register_scheme(spare));
    pair.dark = spare.id;
    REQUIRE(manager.select(pair));
    REQUIRE(
        manager.register_family({*theme::ThemeFamilyId::parse("org.test:empty-family"), "Empty"})
    );
    CHECK(selection_events == 2);
    CHECK(catalog_events == 2);
    CHECK(root->notifications == initial);
    CHECK(manager.revision() == revision);
    pair.preference = theme::ThemePreference::dark;
    REQUIRE(manager.select(pair));
    CHECK(root->notifications == initial + 1);
    CHECK(selection_events == 3);
    CHECK(catalog_events == 2);
}

TEST_CASE(
    "selection readonly and catalog computed values track staged-only publications",
    "[theme][observation][reactive][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    auto& selection = scope.own<theme::ThemeSelectionPresentation>(manager, graph);
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(manager, graph);
    const auto read = selection.readonly();
    auto& preference = scope.computed([read] {
        return std::get<theme::AppearanceThemeSelection>(read.get()).preference;
    });
    auto& count = scope.computed([&catalog] { return catalog.get().schemes.size(); });
    const auto initial_count = count.get();
    CHECK(preference.get() == theme::ThemePreference::system);
    auto next = std::get<theme::AppearanceThemeSelection>(manager.selection());
    next.preference = theme::ThemePreference::light;
    REQUIRE(manager.select(next));
    CHECK(preference.get() == theme::ThemePreference::light);
    REQUIRE(manager.register_scheme(make_scheme("org.test:lazy", theme::ColorAppearance::dark)));
    CHECK(count.get() == initial_count + 1);
    CHECK(preference.get() == theme::ThemePreference::light);
}

TEST_CASE(
    "multi-channel effects and events see one complete publication",
    "[theme][observation][staging][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    const auto [light, dark] = register_pair(manager);
    REQUIRE(manager.select(theme::AppearanceThemeSelection {light, dark}));
    auto& selection = scope.own<theme::ThemeSelectionPresentation>(manager, graph);
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(manager, graph);
    auto& presentation = scope.own<theme::ThemePresentation>(manager, graph);
    auto& changes = scope.own<theme::ThemeChangeHub>(manager);
    int effect_runs = 0;
    scope.effect([&] {
        ++effect_runs;
        CHECK(selection.get() == manager.selection());
        CHECK(catalog.get().version == manager.catalog().version());
        CHECK(presentation.appearance() == manager.appearance());
        CHECK(presentation.revision() == manager.revision());
    });
    int events = 0;
    auto sub = changes.subscribe([&](const auto& change) {
        ++events;
        CHECK(change.after.system == manager.design_system_shared());
        CHECK(selection.get() == manager.selection());
        CHECK(catalog.get().version == manager.catalog().version());
        CHECK(effect_runs == 2);
    });
    auto next = std::get<theme::AppearanceThemeSelection>(manager.selection());
    next.preference = theme::ThemePreference::dark;
    REQUIRE(manager.select(next));
    CHECK(events == 1);
    CHECK(effect_runs == 2);

    // Frame-level deferral still owns the time effects execute.
    auto frame = graph.defer_effects();
    REQUIRE(
        manager.register_scheme(make_scheme("org.test:deferred", theme::ColorAppearance::dark))
    );
    CHECK(effect_runs == 2);
    CHECK(catalog.get().version == manager.catalog().version());
    frame.commit();
    CHECK(effect_runs == 3);
}

TEST_CASE(
    "change endpoints own snapshots through style anonymous and motion-only changes",
    "[theme][observation][change][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    auto manager = std::make_unique<theme::ThemeManager>();
    auto& changes = scope.own<theme::ThemeChangeHub>(*manager);
    const auto original_id = *manager->active_scheme_id();
    std::vector<theme::ThemeChange> events;
    auto sub = changes.subscribe([&](const auto& event) { events.push_back(event); });
    const auto anonymous = content(31);
    REQUIRE(manager->select(
        theme::FixedThemeSelection {
            theme::UnregisteredThemeSource {anonymous, theme::ColorAppearance::dark}
        }
    ));
    REQUIRE(events.size() == 1);
    CHECK(events[0].before.scheme == original_id);
    CHECK_FALSE(events[0].after.scheme);
    CHECK(events[0].after.system->tokens.radius.md == 31);
    auto style = std::make_shared<theme::NanStyle>();
    theme::ButtonStyleRule rule;
    rule.radius = theme::ThemeScalar::literal(42);
    style->add_button_rule(rule);
    manager->set_style(style);
    REQUIRE(events.size() == 2);
    CHECK(events[1].before == events[0].after);
    CHECK_FALSE(events[1].after.scheme);
    CHECK(events[1].before.system != events[1].after.system);
    CHECK(
        theme::resolve_button(
            *events[1].after.system,
            theme::ColorAppearance::dark,
            theme::ButtonTone::primary,
            theme::ButtonTreatment::filled,
            theme::ButtonSize::medium,
            theme::ButtonVisualState::normal
        )
            .container.radius
        == 42
    );
    manager->set_system_preferences({theme::ColorAppearance::light, true});
    CHECK(events.size() == 2); // Fixed appearance plus motion-only revision.
    manager.reset();
    CHECK_FALSE(changes.valid());
    CHECK(events[1].before.system->tokens.radius.md == 31);
    CHECK(events[0].before.system->tokens.radius.md != 31);
}

TEST_CASE(
    "build-context subscriptions are released on page clear for later effective changes",
    "[theme][observation][build-context][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope page(graph);
    reactive::ReactiveScope region_scope(graph);
    theme::ThemeManager manager;
    widget::BuildContext ui(graph, page, manager);
    auto region = ui.with_scope(region_scope);
    int first = 0;
    int second = 0;
    ui.on_theme_change([&](const auto&) { ++first; });
    region.on_theme_change([&](const auto&) { ++second; });
    CHECK(page.subscription_count() == 2);
    CHECK(region_scope.subscription_count() == 0);
    auto next = std::get<theme::AppearanceThemeSelection>(manager.selection());
    next.preference = theme::ThemePreference::dark;
    REQUIRE(manager.select(next));
    CHECK(first == 1);
    CHECK(second == 1);
    region_scope.clear();
    next.preference = theme::ThemePreference::light;
    REQUIRE(manager.select(next));
    CHECK(first == 2);
    CHECK(second == 2);
    page.clear();
    next.preference = theme::ThemePreference::dark;
    REQUIRE(manager.select(next));
    CHECK(first == 2);
    CHECK(second == 2);
    CHECK(page.subscription_count() == 0);
}

TEST_CASE(
    "file import publishes selection directory and theme as one observable state",
    "[theme][observation][import][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager manager;
    auto& selection = scope.own<theme::ThemeSelectionPresentation>(manager, graph);
    auto& catalog = scope.own<theme::ThemeCatalogPresentation>(manager, graph);
    auto& presentation = scope.own<theme::ThemePresentation>(manager, graph);
    auto& changes = scope.own<theme::ThemeChangeHub>(manager);
    int runs = 0;
    scope.effect([&] {
        ++runs;
        CHECK(selection.get() == manager.selection());
        CHECK(catalog.get().version == manager.catalog().version());
        CHECK(catalog.get().schemes.size() == manager.catalog().schemes().size());
        CHECK(presentation.appearance() == manager.appearance());
        CHECK(presentation.revision() == manager.revision());
    });
    int selection_calls = 0;
    int catalog_calls = 0;
    int change_calls = 0;
    const auto assert_ready = [&] {
        CHECK(runs == 2);
        CHECK(selection.get() == manager.selection());
        CHECK(catalog.get().version == manager.catalog().version());
        CHECK(presentation.appearance() == theme::ColorAppearance::dark);
        CHECK(manager.theme().tokens.radius.md == 37);
    };
    auto a = selection.subscribe([&](auto) {
        ++selection_calls;
        assert_ready();
    });
    auto b = catalog.subscribe([&](auto) {
        ++catalog_calls;
        assert_ready();
    });
    auto c = changes.subscribe([&](const auto& change) {
        ++change_calls;
        assert_ready();
        CHECK(change.after.system == manager.design_system_shared());
    });
    theme::StyleDocument doc;
    doc.themes.emplace("light", theme::default_theme());
    doc.themes.emplace("dark", theme::default_theme());
    doc.themes.at("dark").tokens.radius.md = 37;
    doc.theme_families.emplace("pair", theme::StyleThemePair {"light", "dark"});
    doc.active_family = "pair";
    doc.preference = theme::ThemePreference::dark;
    REQUIRE(doc.apply(manager));
    CHECK(runs == 2);
    CHECK(selection_calls == 1);
    CHECK(catalog_calls == 1);
    CHECK(change_calls == 1);
}
