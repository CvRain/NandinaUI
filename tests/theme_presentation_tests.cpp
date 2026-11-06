#include "theme_test_support.hpp"
// D2 first delivery: read-only, scope-bound theme presentation projection.
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/theme/theme_presentation.hpp>
#include <nandina/widget/build_context.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace reactive = nandina::reactive;
namespace theme = nandina::theme;
namespace widget = nandina::widget;

namespace
{
    auto snapshot() -> std::shared_ptr<const theme::DesignSystem> {
        return std::make_shared<const theme::DesignSystem>(theme::default_design_system());
    }

    template<typename T>
    concept HasSetter = requires(T& state, theme::ThemePresentationState next) { state.set(next); };

    template<typename T>
    concept HasEmit = requires(T& state, theme::ThemePresentationState next) { state.emit(next); };

    template<typename T>
    concept HasMutableSignal = requires(T& state) {
        { state.signal() } -> std::same_as<reactive::Signal<theme::ThemePresentationState>&>;
    };

    template<typename T>
    concept HasViewSetter =
        requires(T& view, theme::ThemePresentationState next) { view.set(next); };
} // namespace

TEST_CASE(
    "theme presentation starts from manager state without a synthetic event",
    "[theme][presentation]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);

    CHECK(state.valid());
    CHECK(state.get().appearance == theme::ColorAppearance::light);
    CHECK(state.get().revision == manager.revision());

    int calls = 0;
    auto subscription = state.subscribe([&](theme::ThemePresentationState) { ++calls; });
    CHECK(calls == 0); // 订阅本身不补发一次“初始变化”

    auto& is_dark =
        scope.computed([&state] { return state.get().appearance == theme::ColorAppearance::dark; });
    CHECK_FALSE(is_dark.get());

    manager.set_preference(theme::ThemePreference::dark);
    CHECK(calls == 1);
    CHECK(state.get().appearance == theme::ColorAppearance::dark);
    CHECK(state.get().revision == manager.revision());
    CHECK(is_dark.get());
}

TEST_CASE("theme presentation advances revision on motion-only changes", "[theme][presentation]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    const auto before = state.get();

    int calls = 0;
    auto subscription = state.subscribe([&](theme::ThemePresentationState next) {
        ++calls;
        CHECK(next.revision == manager.revision());
        CHECK(next.appearance == theme::ColorAppearance::light);
    });

    manager.set_motion_preference(theme::MotionPreference::reduced);
    CHECK(calls == 1);
    CHECK(state.revision() > before.revision);
    CHECK(state.appearance() == theme::ColorAppearance::light);
}

TEST_CASE(
    "theme presentation follows apply, named theme and family activation",
    "[theme][presentation]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    std::vector<std::uint64_t> revisions;
    auto subscription = state.subscribe([&](theme::ThemePresentationState next) {
        revisions.push_back(next.revision);
    });

    // apply 换快照：外观不变，呈现修订必须前进。
    auto shared = snapshot();
    manager.apply(shared);
    REQUIRE(revisions.size() == 1);
    CHECK(revisions.back() == manager.revision());
    CHECK(state.appearance() == theme::ColorAppearance::light);

    // 同一指针重复 apply 不制造新 revision（也不该通知）。
    manager.apply(shared);
    CHECK(revisions.size() == 1);

    // 具名主题切换：不同快照指针 → 一次通知。
    REQUIRE(manager.register_theme("light_a", theme::default_theme()));
    REQUIRE(manager.register_theme("dark_b", theme::default_theme()));
    REQUIRE(manager.activate("light_a"));
    REQUIRE(revisions.size() == 2);
    REQUIRE(manager.activate("dark_b"));
    REQUIRE(revisions.size() == 3);

    // 遗留家族：外观翻转走具名快照切换。
    REQUIRE(manager.register_family("legacy", "light_a", "dark_b"));
    REQUIRE(manager.activate_family("legacy"));
    REQUIRE(revisions.size() == 4);
    manager.set_preference(theme::ThemePreference::dark);
    REQUIRE(revisions.size() == 5);
    CHECK(state.appearance() == theme::ColorAppearance::dark);
}

TEST_CASE(
    "theme presentation publishes appearance flips inside a full snapshot family",
    "[theme][presentation]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    std::vector<theme::ThemePresentationState> events;
    auto subscription =
        state.subscribe([&](theme::ThemePresentationState next) { events.push_back(next); });

    // 全快照族：快照指针不变，只改 appearance —— §5.5 点名的发布点。
    manager.register_theme_family("snap", theme::default_design_system());
    REQUIRE(manager.activate_family("snap"));
    REQUIRE(events.size() == 1);

    manager.set_preference(theme::ThemePreference::dark);
    REQUIRE(events.size() == 2);
    CHECK(events[1].appearance == theme::ColorAppearance::dark);
    CHECK(state.get().appearance == theme::ColorAppearance::dark);
    CHECK(state.get().revision == manager.revision());
}

TEST_CASE("theme presentation deduplicates equivalent states", "[theme][presentation]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int calls = 0;
    auto subscription = state.subscribe([&](theme::ThemePresentationState) { ++calls; });

    // 初始 preference 为 system：等价的系统快照不制造通知。
    manager.set_system_preferences(
        {.appearance = theme::ColorAppearance::light, .reduced_motion = false}
    );
    CHECK(calls == 0);
    manager.set_system_preferences(
        {.appearance = theme::ColorAppearance::dark, .reduced_motion = false}
    );
    CHECK(calls == 1);
    manager.set_system_preferences(
        {.appearance = theme::ColorAppearance::dark, .reduced_motion = false}
    );
    CHECK(calls == 1);
}

TEST_CASE("theme presentation supports multiple independent subscribers", "[theme][presentation]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int first = 0;
    int second = 0;
    auto a = state.subscribe([&](theme::ThemePresentationState) { ++first; });
    auto b = state.subscribe([&](theme::ThemePresentationState) { ++second; });

    manager.set_preference(theme::ThemePreference::dark);
    CHECK(first == 1);
    CHECK(second == 1);

    // 单个订阅注销不影响另一个。
    a = reactive::Subscription {};
    manager.set_preference(theme::ThemePreference::light);
    CHECK(first == 1);
    CHECK(second == 2);
}

TEST_CASE("theme presentation is scope-owned and cleared with the scope", "[theme][presentation]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& first = scope.own<theme::ThemePresentation>(manager, graph);
    auto& second = scope.own<theme::ThemePresentation>(manager, graph);
    CHECK(&first == &second);

    int calls = 0;
    auto subscription = first.subscribe([&](theme::ThemePresentationState) { ++calls; });
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(calls == 1);

    scope.clear();
    manager.set_preference(theme::ThemePreference::light);
    CHECK(calls == 1); // 投影已注销，后续变化不再回调

    // clear() 后按类型重新创建：新实例重新登记观察者并继续工作。
    auto& fresh = scope.own<theme::ThemePresentation>(manager, graph);
    CHECK(fresh.valid());
    CHECK(fresh.get().appearance == theme::ColorAppearance::light);
    int fresh_calls = 0;
    auto fresh_subscription =
        fresh.subscribe([&](theme::ThemePresentationState) { ++fresh_calls; });
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(fresh_calls == 1);
    CHECK(calls == 1);
}

TEST_CASE("theme presentation survives manager destruction first", "[theme][presentation]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemePresentation* state_ptr = nullptr;
    int calls = 0;
    {
        theme_test::Manager manager;
        auto& state = scope.own<theme::ThemePresentation>(manager, graph);
        state_ptr = &state;
        auto subscription = state.subscribe([&](theme::ThemePresentationState) { ++calls; });
        manager.set_preference(theme::ThemePreference::dark);
        REQUIRE(calls == 1);
    } // manager 先销毁：投影通知失效
    REQUIRE(state_ptr != nullptr);
    CHECK_FALSE(state_ptr->valid());
    CHECK(state_ptr->get().appearance == theme::ColorAppearance::dark); // 保留最后发布载荷
    CHECK(state_ptr->revision() > 0);
    // 失效后订阅仍安全，只是不再有事件。
    int later = 0;
    auto late_subscription = state_ptr->subscribe([&](theme::ThemePresentationState) { ++later; });
    CHECK(later == 0);
}

TEST_CASE("theme presentation exposes no write or emit entry", "[theme][presentation]") {
    static_assert(!HasSetter<theme::ThemePresentation>);
    static_assert(!HasEmit<theme::ThemePresentation>);
    static_assert(!HasMutableSignal<theme::ThemePresentation>);
    static_assert(std::is_same_v<
                  decltype(std::declval<const theme::ThemePresentation&>().readonly()),
                  reactive::ReadSignal<theme::ThemePresentationState>>);
    // 只读视图不能写：ReadSignal 只暴露 get/peek。
    static_assert(!HasViewSetter<reactive::ReadSignal<theme::ThemePresentationState>>);
    CHECK(true);
}

TEST_CASE("build context shares one presentation per page scope", "[theme][presentation]") {
    reactive::Graph graph;
    reactive::ReactiveScope page_scope(graph);
    theme_test::Manager manager;
    widget::BuildContext ui(graph, page_scope, manager);

    auto& first = ui.theme_state();
    auto& second = ui.theme_state();
    CHECK(&first == &second);

    reactive::ReactiveScope region_scope(graph);
    auto region = ui.with_scope(region_scope);
    auto& derived = region.theme_state();
    CHECK(&derived == &first); // 派生区域共享页面作用域投影

    manager.set_preference(theme::ThemePreference::dark);
    CHECK(first.get().appearance == theme::ColorAppearance::dark);
    CHECK(first.valid());
}

TEST_CASE(
    "named same-snapshot appearance changes publish the projection",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    REQUIRE(manager.register_theme("named", theme::default_theme()));
    REQUIRE(manager.activate("named"));
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    const auto previous = state.get();
    int calls = 0;
    auto sub = state.subscribe([&](theme::ThemePresentationState) { ++calls; });
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(state.appearance() == theme::ColorAppearance::dark);
    CHECK(state.revision() > previous.revision);
    CHECK(calls == 1);

    manager.set_preference(theme::ThemePreference::system);
    CHECK(state.appearance() == theme::ColorAppearance::light);
    CHECK(calls == 2);
    manager.set_system_appearance(theme::ColorAppearance::dark);
    CHECK(state.appearance() == theme::ColorAppearance::dark);
    CHECK(state.revision() == manager.revision());
    CHECK(calls == 3);
}

TEST_CASE(
    "active family replacement publishes the replacement content",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    REQUIRE(manager.register_theme("first", theme::default_theme()));
    auto changed = theme::default_theme();
    changed.tokens.radius.md += 7;
    REQUIRE(manager.register_theme("second", changed));
    REQUIRE(manager.register_family("legacy", "first", "first"));
    REQUIRE(manager.activate_family("legacy"));
    auto before = state.revision();
    CHECK_FALSE(manager.register_family("legacy", "second", "second"));
    CHECK(manager.theme().tokens.radius.md == changed.tokens.radius.md);
    CHECK(state.revision() > before);

    manager.register_theme_family("full", theme::default_design_system());
    REQUIRE(manager.activate_family("full"));
    auto system = theme::default_design_system();
    system.tokens.radius.md += 11;
    before = state.revision();
    manager.register_theme_family("full", system);
    CHECK(manager.theme().tokens.radius.md == system.tokens.radius.md);
    CHECK(state.revision() > before);
}

TEST_CASE(
    "presentation notification rejects reentrant writes before mutation",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    auto sub = state.subscribe([&](theme::ThemePresentationState) {
        manager.set_preference(theme::ThemePreference::light);
    });
    CHECK_THROWS_AS(manager.set_preference(theme::ThemePreference::dark), std::logic_error);
    CHECK(manager.appearance() == theme::ColorAppearance::dark);
    CHECK(state.appearance() == theme::ColorAppearance::dark);
}

TEST_CASE(
    "throwing presentation callbacks do not block other subscribers",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope first_scope(graph);
    reactive::ReactiveScope second_scope(graph);
    theme_test::Manager manager;
    auto& first = first_scope.own<theme::ThemePresentation>(manager, graph);
    auto& second = second_scope.own<theme::ThemePresentation>(manager, graph);
    auto bad = first.subscribe([](theme::ThemePresentationState) {
        throw std::runtime_error("subscriber failure");
    });
    int after_bad = 0;
    auto good = first.subscribe([&](theme::ThemePresentationState) { ++after_bad; });
    int other_scope = 0;
    auto other = second.subscribe([&](theme::ThemePresentationState) { ++other_scope; });
    CHECK_THROWS_AS(manager.set_preference(theme::ThemePreference::dark), std::runtime_error);
    CHECK(after_bad == 1);
    CHECK(other_scope == 1);
    CHECK(second.appearance() == theme::ColorAppearance::dark);
    bad.disconnect();
    REQUIRE_NOTHROW(manager.set_preference(theme::ThemePreference::light));
    CHECK(after_bad == 2);
    CHECK(other_scope == 2);
}

TEST_CASE(
    "projection effects observe all projections and scene updates together",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope first_scope(graph);
    reactive::ReactiveScope second_scope(graph);
    theme_test::Manager manager;
    auto& first = first_scope.own<theme::ThemePresentation>(manager, graph);
    auto& second = second_scope.own<theme::ThemePresentation>(manager, graph);
    class Probe final: public nandina::scene::NanControl {
    public:
        theme::ColorAppearance seen = theme::ColorAppearance::light;
        void on_theme_changed(const theme::ThemeManager& value) override {
            seen = value.appearance();
        }
    };
    auto probe = std::make_shared<Probe>();
    nandina::scene::NanSceneTree tree;
    tree.set_root(probe);
    tree.set_theme_manager(manager); // Registered after the projections intentionally.
    auto view = second.readonly();
    int runs = 0;
    first_scope.effect([&] {
        ++runs;
        CHECK(first.get() == view.get());
        CHECK(first.appearance() == probe->seen);
    });
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(runs == 2);
}

TEST_CASE(
    "presentation can clear its scope during subscriber delivery",
    "[theme][presentation][lifetime][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int later = 0;
    auto clear = state.subscribe([&](theme::ThemePresentationState) { scope.clear(); });
    auto other = state.subscribe([&](theme::ThemePresentationState) { ++later; });
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(later == 0);
    REQUIRE_NOTHROW(manager.set_preference(theme::ThemePreference::light));
}

TEST_CASE(
    "presentation survives its scope being cleared by an effect",
    "[theme][presentation][lifetime][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope page_scope(graph);
    reactive::ReactiveScope consumer_scope(graph);
    theme_test::Manager manager;
    auto& state = page_scope.own<theme::ThemePresentation>(manager, graph);
    int callbacks = 0;
    auto sub = state.subscribe([&](theme::ThemePresentationState) { ++callbacks; });
    consumer_scope.effect([&] {
        if (state.appearance() == theme::ColorAppearance::dark) {
            page_scope.clear();
        }
    });
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(callbacks == 0);
    REQUIRE_NOTHROW(manager.set_preference(theme::ThemePreference::light));
}

TEST_CASE(
    "presentation supports manager destruction during delivery",
    "[theme][presentation][lifetime][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope first_scope(graph);
    reactive::ReactiveScope second_scope(graph);
    auto manager = std::make_unique<theme_test::Manager>();
    auto& first = first_scope.own<theme::ThemePresentation>(*manager, graph);
    auto& second = second_scope.own<theme::ThemePresentation>(*manager, graph);
    int later = 0;
    auto destroy = first.subscribe([&](theme::ThemePresentationState) { manager.reset(); });
    auto other = second.subscribe([&](theme::ThemePresentationState) { ++later; });
    auto* source = manager.get();
    source->set_system_preferences(
        {.appearance = theme::ColorAppearance::dark, .reduced_motion = true}
    );
    CHECK(manager == nullptr);
    CHECK_FALSE(first.valid());
    CHECK_FALSE(second.valid());
    CHECK(first.get() == second.get());
    CHECK(first.appearance() == theme::ColorAppearance::dark);
    CHECK(later == 0);
}

TEST_CASE("throwing effects do not block presentation delivery", "[theme][presentation][review]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int events = 0;
    auto sub = state.subscribe([&](theme::ThemePresentationState next) {
        CHECK(next == state.get());
        ++events;
    });
    scope.effect([&] {
        if (state.appearance() == theme::ColorAppearance::dark) {
            throw std::runtime_error("effect failure");
        }
    });
    CHECK_THROWS_AS(manager.set_preference(theme::ThemePreference::dark), std::runtime_error);
    CHECK(events == 1);
    CHECK(state.revision() == manager.revision());
    REQUIRE_NOTHROW(manager.set_preference(theme::ThemePreference::light));
    CHECK(events == 2);
}

TEST_CASE(
    "revision delivery survives removing and replacing observers at the same address",
    "[theme][presentation][review]"
) {
    theme_test::Manager manager;
    class Observer final: public theme::ThemeObserver {
    public:
        int calls = 0;
        std::function<void()> callback;
        void on_theme_revision_changed(const theme::ThemeManager&) override {
            ++calls;
            if (callback) {
                callback();
            }
        }
        void on_theme_manager_destroyed(const theme::ThemeManager&) noexcept override {}
    } first;
    std::optional<Observer> second(std::in_place);
    manager.add_observer(first);
    manager.add_observer(*second);
    first.callback = [&] {
        manager.remove_observer(*second);
        second.emplace(); // Deliberately reuse the same storage with a new registration.
        manager.add_observer(*second);
    };
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(first.calls == 1);
    CHECK(second->calls == 0);
    first.callback = {};
    manager.set_preference(theme::ThemePreference::light);
    CHECK(first.calls == 2);
    CHECK(second->calls == 1);
    manager.remove_observer(first);
    manager.remove_observer(*second);
}

TEST_CASE(
    "manager destruction skips observers removed by another destruction callback",
    "[theme][presentation][lifetime][review]"
) {
    auto manager = std::make_unique<theme_test::Manager>();
    class Observer final: public theme::ThemeObserver {
    public:
        std::function<void()> destroyed;
        void on_theme_revision_changed(const theme::ThemeManager&) override {}
        void on_theme_manager_destroyed(const theme::ThemeManager&) noexcept override {
            destroyed();
        }
    } first, second;
    int later = 0;
    auto* source = manager.get();
    first.destroyed = [&] { source->remove_observer(second); };
    second.destroyed = [&] { ++later; };
    manager->add_observer(first);
    manager->add_observer(second);
    manager.reset();
    CHECK(later == 0);
}

TEST_CASE(
    "theme publication keeps window-deferred effects in the reactive phase",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int runs = 0;
    scope.effect([&] {
        (void)state.get();
        ++runs;
    });
    auto frame = graph.defer_effects();
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(state.appearance() == theme::ColorAppearance::dark);
    CHECK(runs == 1);
    frame.commit();
    CHECK(runs == 2);
}

TEST_CASE(
    "readonly theme views track updates and survive manager destruction",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    auto manager = std::make_unique<theme_test::Manager>();
    auto& state = scope.own<theme::ThemePresentation>(*manager, graph);
    const auto view = state.readonly();
    auto& dark =
        scope.computed([view] { return view.get().appearance == theme::ColorAppearance::dark; });
    CHECK_FALSE(dark.get());
    manager->set_preference(theme::ThemePreference::dark);
    CHECK(dark.get());
    manager.reset();
    CHECK(view.get().appearance == theme::ColorAppearance::dark);
    CHECK_FALSE(state.valid());
}

TEST_CASE(
    "all theme mutation entries reject writes during notification",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    REQUIRE(manager.register_theme("named", theme::default_theme()));
    REQUIRE(manager.register_family("legacy", "named", "named"));
    manager.register_theme_family("full", theme::default_design_system());
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int callbacks = 0;
    auto sub = state.subscribe([&](theme::ThemePresentationState) {
        ++callbacks;
        CHECK_THROWS_AS(manager.apply(snapshot()), std::logic_error);
        CHECK_THROWS_AS(manager.register_theme("new", theme::default_theme()), std::logic_error);
        CHECK_THROWS_AS(manager.activate("named"), std::logic_error);
        CHECK_THROWS_AS(manager.set_theme(theme::default_theme()), std::logic_error);
        CHECK_THROWS_AS(manager.register_family("new", "named", "named"), std::logic_error);
        CHECK_THROWS_AS(manager.activate_family("full"), std::logic_error);
        CHECK_THROWS_AS(
            manager.register_theme_family("new", theme::default_design_system()),
            std::logic_error
        );
        CHECK_THROWS_AS(manager.set_preference(theme::ThemePreference::light), std::logic_error);
        CHECK_THROWS_AS(manager.set_system_preferences({}), std::logic_error);
        CHECK_THROWS_AS(
            manager.set_system_appearance(theme::ColorAppearance::dark),
            std::logic_error
        );
        CHECK_THROWS_AS(
            manager.set_motion_preference(theme::MotionPreference::reduced),
            std::logic_error
        );
        CHECK_THROWS_AS(manager.set_system_reduced_motion(true), std::logic_error);
        CHECK_THROWS_AS(manager.set_style(theme::default_style()), std::logic_error);
    });
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(callbacks == 1);
    CHECK_FALSE(manager.contains("new"));
    CHECK_FALSE(manager.contains_family("new"));
    CHECK(manager.appearance() == theme::ColorAppearance::dark);
    CHECK(manager.system_preferences() == theme::SystemPreferences {});
    CHECK_FALSE(manager.reduced_motion());
}

TEST_CASE(
    "internal observer failure does not block other observers or projections",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    class Observer final: public theme::ThemeObserver {
    public:
        bool fail = false;
        int calls = 0;
        void on_theme_revision_changed(const theme::ThemeManager&) override {
            ++calls;
            if (fail) {
                throw std::runtime_error("internal failure");
            }
        }
        void on_theme_manager_destroyed(const theme::ThemeManager&) noexcept override {}
    } bad, good;
    manager.add_observer(bad);
    manager.add_observer(good);
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    int events = 0;
    auto sub = state.subscribe([&](theme::ThemePresentationState) { ++events; });
    bad.fail = true;
    CHECK_THROWS_AS(manager.set_preference(theme::ThemePreference::dark), std::runtime_error);
    CHECK(good.calls == 1);
    CHECK(events == 1);
    CHECK(state.appearance() == theme::ColorAppearance::dark);
    manager.remove_observer(bad);
    manager.remove_observer(good);
}

TEST_CASE(
    "equivalent family changes do not publish presentation revisions",
    "[theme][presentation][review]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager manager;
    REQUIRE(manager.register_theme("same", theme::default_theme()));
    auto dark = theme::default_theme();
    dark.tokens.radius.md += 3;
    REQUIRE(manager.register_theme("other-dark", dark));
    REQUIRE(manager.register_family("a", "same", "same"));
    REQUIRE(manager.register_family("b", "same", "same"));
    REQUIRE(manager.activate_family("a"));
    auto& state = scope.own<theme::ThemePresentation>(manager, graph);
    const auto before = state.get();
    int events = 0;
    auto sub = state.subscribe([&](theme::ThemePresentationState) { ++events; });

    CHECK_FALSE(manager.register_family("a", "same", "same"));
    CHECK(state.get() == before);
    REQUIRE(manager.activate_family("b"));
    CHECK(state.get() == before);
    // A future dark slot can change while light remains active.
    CHECK_FALSE(manager.register_family("b", "same", "other-dark"));
    CHECK(state.get() == before);
    CHECK(events == 0);
    manager.set_preference(theme::ThemePreference::dark);
    CHECK(events == 1);
    CHECK(manager.theme().tokens.radius.md == dark.tokens.radius.md);
}
