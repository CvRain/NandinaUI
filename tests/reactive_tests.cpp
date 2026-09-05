//
// Reactive-layer unit tests (Catch2 v3).
// Ported from the v2 Zig reactive test suite, adapted to the C++ API.
//

#include <catch2/catch_test_macros.hpp>

#include "reactive/reactive.hpp"

#include <stdexcept>

using namespace nandina::reactive;

TEST_CASE("Signal basic get/set", "[reactive][signal]") {
    Graph g;
    Signal<int> s{g, 10};
    REQUIRE(s.get() == 10);
    s.set(42);
    REQUIRE(s.get() == 42);
}

TEST_CASE("Signal set with equal value does not bump version", "[reactive][signal]") {
    Graph g;
    Signal<int> s{g, 1};
    const auto v0 = s.source_version();
    s.set(1);  // equal, ignored
    REQUIRE(s.source_version() == v0);
    s.set(2);  // changed, bumps
    REQUIRE(s.source_version() == v0 + 1);
}

TEST_CASE("Signal update mutates in place", "[reactive][signal]") {
    Graph g;
    Signal<int> s{g, 0};
    s.update([](int& v) { v += 5; });
    REQUIRE(s.get() == 5);
}

TEST_CASE("ReadSignal is a read-only view", "[reactive][signal]") {
    Graph g;
    Signal<int> s{g, 7};
    auto ro = s.as_readonly();
    REQUIRE(ro.get() == 7);
    s.set(8);
    REQUIRE(ro.peek() == 8);
}

TEST_CASE("Property applies changes once and exposes read-only observation", "[reactive][property]") {
    int applied = 0;
    int observed = 0;
    Property<int> property {1, [&applied](const int& value) { applied = value; }};
    auto readonly = property.as_readonly();
    auto subscription = readonly.observe([&observed](const int& value) { observed = value; });

    REQUIRE_FALSE(property.set(1));
    REQUIRE(applied == 0);
    REQUIRE(observed == 0);

    REQUIRE(property.set(2));
    REQUIRE(readonly.get() == 2);
    REQUIRE(applied == 2);
    REQUIRE(observed == 2);

    subscription.disconnect();
    REQUIRE(property.set(3));
    REQUIRE(applied == 3);
    REQUIRE(observed == 2);
}

TEST_CASE("Event subscriptions may disconnect while emitting", "[reactive][event]") {
    Event<int> event;
    int first = 0;
    int second = 0;
    Subscription second_subscription;
    auto first_subscription = event.subscribe([&](const int value) {
        first += value;
        second_subscription.disconnect();
    });
    second_subscription = event.subscribe([&](const int value) { second += value; });

    event.emit(2);
    event.emit(3);

    REQUIRE(first == 5);
    REQUIRE(second == 0);
    REQUIRE(event.subscriber_count() == 1);
}

TEST_CASE("Subscription may outlive its event", "[reactive][event]") {
    Subscription subscription;
    {
        Event<> event;
        subscription = event.subscribe([] {});
        REQUIRE(subscription.connected());
    }
    REQUIRE_NOTHROW(subscription.disconnect());
}

TEST_CASE("Property binding accepts signal and computed sources", "[reactive][property][binding]") {
    Graph graph;
    EffectScope scope {graph};
    Signal<int> source {graph, 2};
    auto* doubled = make_computed(graph, [&source] { return source.get() * 2; });
    Property<int> property {0};

    property.bind(scope, *doubled);
    REQUIRE(property.get() == 4);

    source.set(5);
    REQUIRE(property.get() == 10);

    scope.clear();
    source.set(7);
    REQUIRE(property.get() == 10);
}

TEST_CASE("Effect runs once on creation", "[reactive][effect]") {
    Graph g;
    int runs = 0;
    make_effect(g, [&] { ++runs; });
    REQUIRE(runs == 1);
}

TEST_CASE("Effect re-runs when a dependency changes", "[reactive][effect]") {
    Graph g;
    Signal<int> s{g, 0};
    int seen = 0;
    int runs = 0;
    make_effect(g, [&] {
        seen = s.get();
        ++runs;
    });

    REQUIRE(runs == 1);
    REQUIRE(seen == 0);

    s.set(5);
    REQUIRE(runs == 2);
    REQUIRE(seen == 5);

    s.set(5);  // equal, no re-run
    REQUIRE(runs == 2);
}

TEST_CASE("Effect stops re-running after dispose", "[reactive][effect]") {
    Graph g;
    Signal<int> s{g, 0};
    int runs = 0;
    auto* e = make_effect(g, [&] {
        (void)s.get();
        ++runs;
    });

    REQUIRE(runs == 1);
    e->dispose();
    s.set(1);
    REQUIRE(runs == 1);
}

TEST_CASE("Computed derives lazily and caches", "[reactive][computed]") {
    Graph g;
    Signal<int> n{g, 0};
    int computes = 0;
    auto* c = make_computed(g, [&] {
        ++computes;
        return n.get();
    });

    REQUIRE(computes == 0);  // not read yet, not computed
    REQUIRE(c->get() == 0);
    REQUIRE(computes == 1);
    (void)c->get();  // clean, reuse cache
    REQUIRE(computes == 1);

    n.set(1);  // invalidated but not recomputed
    REQUIRE(computes == 1);
    REQUIRE(c->get() == 1);  // recompute
    REQUIRE(computes == 2);
}

TEST_CASE("Computed chained dependency (computed depends on computed)", "[reactive][computed]") {
    Graph g;
    Signal<int> n{g, 2};
    auto* doubled = make_computed(g, [&] { return n.get() * 2; });
    auto* plus_one = make_computed(g, [&] { return doubled->get() + 1; });

    REQUIRE(plus_one->get() == 5);  // 2*2+1
    n.set(10);
    REQUIRE(plus_one->get() == 21);  // 10*2+1
}

TEST_CASE("Effect depends on computed, re-runs when source changes", "[reactive][computed][effect]") {
    Graph g;
    Signal<int> n{g, 1};
    auto* doubled = make_computed(g, [&] { return n.get() * 2; });

    int seen = 0;
    int runs = 0;
    make_effect(g, [&] {
        seen = doubled->get();
        ++runs;
    });

    REQUIRE(runs == 1);
    REQUIRE(seen == 2);

    n.set(5);
    REQUIRE(runs == 2);
    REQUIRE(seen == 10);
}

TEST_CASE("Diamond dependency: effect runs once, no glitch", "[reactive][computed][diamond]") {
    Graph g;
    // a -> b, a -> c, (b,c) -> effect. Changing a should run the effect once.
    Signal<int> a{g, 1};
    auto* b = make_computed(g, [&] { return a.get() + 1; });
    auto* c = make_computed(g, [&] { return a.get() * 10; });

    int sum = 0;
    int runs = 0;
    make_effect(g, [&] {
        sum = b->get() + c->get();
        ++runs;
    });

    REQUIRE(runs == 1);
    REQUIRE(sum == 12);  // (1+1) + (1*10)

    a.set(2);
    REQUIRE(runs == 2);  // exactly once
    REQUIRE(sum == 23);  // (2+1) + (2*10)
}

TEST_CASE("Dynamic dependency: old dep stops triggering after branch switch", "[reactive][effect][dynamic]") {
    Graph g;
    Signal<bool> cond{g, true};
    Signal<int> a{g, 1};
    Signal<int> b{g, 100};

    int seen = 0;
    int runs = 0;
    make_effect(g, [&] {
        seen = cond.get() ? a.get() : b.get();
        ++runs;
    });

    REQUIRE(seen == 1);

    b.set(200);  // currently depends on a, changing b should not trigger
    REQUIRE(runs == 1);

    cond.set(false);  // switch to b branch
    REQUIRE(runs == 2);
    REQUIRE(seen == 200);

    a.set(5);  // now depends on b, changing a should not trigger
    REQUIRE(runs == 2);

    b.set(300);  // should trigger
    REQUIRE(runs == 3);
    REQUIRE(seen == 300);
}

TEST_CASE("batch merges multiple sets into one effect run", "[reactive][batch]") {
    Graph g;
    Signal<int> a{g, 0};
    Signal<int> b{g, 0};

    int sum = 0;
    int runs = 0;
    make_effect(g, [&] {
        sum = a.get() + b.get();
        ++runs;
    });
    REQUIRE(runs == 1);

    batch(g, [&] {
        a.set(10);
        b.set(20);
        REQUIRE(runs == 1);  // effect not yet run inside batch
    });

    REQUIRE(runs == 2);  // one run after batch exit
    REQUIRE(sum == 30);
}

TEST_CASE("nested batch flushes only at outermost exit", "[reactive][batch]") {
    Graph g;
    Signal<int> s{g, 0};
    int runs = 0;
    make_effect(g, [&] {
        (void)s.get();
        ++runs;
    });
    REQUIRE(runs == 1);

    batch(g, [&] {
        s.set(1);
        batch(g, [&] {
            s.set(2);
        });
        REQUIRE(runs == 1);  // inner batch exit did not flush
    });

    REQUIRE(runs == 2);
}

TEST_CASE("deferred effect scope keeps values synchronous and coalesces one wave", "[reactive][scheduler]") {
    Graph graph;
    Signal<int> first {graph, 0};
    Signal<int> second {graph, 0};
    int seen = 0;
    int runs = 0;
    make_effect(graph, [&] {
        seen = first.get() + second.get();
        ++runs;
    });

    auto deferred = graph.defer_effects();
    first.set(10);
    second.set(20);

    REQUIRE(first.peek() == 10);
    REQUIRE(second.peek() == 20);
    REQUIRE(runs == 1);
    REQUIRE(graph.has_pending_effects());

    deferred.commit();
    REQUIRE(runs == 2);
    REQUIRE(seen == 30);
    REQUIRE_FALSE(graph.has_pending_effects());
}

TEST_CASE("property bindings update in the deferred reactive wave", "[reactive][scheduler][property]") {
    Graph graph;
    EffectScope scope {graph};
    Signal<int> source {graph, 1};
    Property<int> property {0};
    property.bind(scope, source);
    REQUIRE(property.get() == 1);

    auto deferred = graph.defer_effects();
    source.set(2);
    source.set(3);
    graph.flush();

    REQUIRE(source.peek() == 3);
    REQUIRE(property.get() == 1);
    deferred.commit();
    REQUIRE(property.get() == 3);
}

TEST_CASE("deferred effect scopes and batches flush only at the outer tick", "[reactive][scheduler][batch]") {
    Graph graph;
    Signal<int> value {graph, 0};
    int runs = 0;
    make_effect(graph, [&] {
        (void)value.get();
        ++runs;
    });

    auto outer = graph.defer_effects();
    value.set(1);
    {
        auto inner = graph.defer_effects();
        batch(graph, [&] { value.set(2); });
        inner.commit();
    }
    REQUIRE(runs == 1);

    outer.commit();
    REQUIRE(runs == 2);
    REQUIRE(value.peek() == 2);
}

TEST_CASE("abandoned deferred effect scope restores automatic flushing", "[reactive][scheduler]") {
    Graph graph;
    Signal<int> value {graph, 0};
    int seen = 0;
    int runs = 0;
    make_effect(graph, [&] {
        seen = value.get();
        ++runs;
    });

    {
        auto deferred = graph.defer_effects();
        value.set(1);
    }
    REQUIRE(runs == 1);
    REQUIRE(graph.has_pending_effects());

    value.set(2);
    REQUIRE(runs == 2);
    REQUIRE(seen == 2);
    REQUIRE_FALSE(graph.has_pending_effects());
}

TEST_CASE("deferred effect commit restores scheduling after failure", "[reactive][scheduler]") {
    Graph graph;
    Signal<int> failing {graph, 0};
    Signal<int> healthy {graph, 0};
    int healthy_runs = 0;
    make_effect(graph, [&] {
        if (failing.get() == 1) {
            throw std::runtime_error("expected failure");
        }
    });
    make_effect(graph, [&] {
        (void)healthy.get();
        ++healthy_runs;
    });

    auto deferred = graph.defer_effects();
    failing.set(1);
    REQUIRE_THROWS_AS(deferred.commit(), std::runtime_error);
    REQUIRE_FALSE(graph.has_pending_effects());

    healthy.set(1);
    REQUIRE(healthy_runs == 2);
}

TEST_CASE("self-invalidating effects advance once per deferred wave", "[reactive][scheduler]") {
    Graph graph;
    Signal<int> value {graph, 0};
    int runs = 0;
    make_effect(graph, [&] {
        const auto current = value.get();
        ++runs;
        if (current < 2) {
            value.set(current + 1);
        }
    });

    REQUIRE(runs == 1);
    REQUIRE(value.peek() == 1);

    auto first_tick = graph.defer_effects();
    first_tick.commit();
    REQUIRE(runs == 2);
    REQUIRE(value.peek() == 2);
    REQUIRE(graph.has_pending_effects());

    auto second_tick = graph.defer_effects();
    second_tick.commit();
    REQUIRE(runs == 3);
    REQUIRE_FALSE(graph.has_pending_effects());
}

TEST_CASE("self-invalidating effect is deferred to the next flush", "[reactive][effect][scheduler]") {
    Graph g;
    Signal<int> value {g, 0};
    int runs = 0;
    make_effect(g, [&] {
        const int current = value.get();
        ++runs;
        if (current < 2) {
            value.set(current + 1);
        }
    });

    REQUIRE(runs == 1);
    REQUIRE(value.peek() == 1);
    REQUIRE(g.has_pending_effects());

    g.flush();
    REQUIRE(runs == 2);
    REQUIRE(value.peek() == 2);
    REQUIRE(g.has_pending_effects());

    g.flush();
    REQUIRE(runs == 3);
    REQUIRE_FALSE(g.has_pending_effects());
}

TEST_CASE("effect failure restores graph scheduling state", "[reactive][effect][scheduler]") {
    Graph g;
    Signal<int> failing {g, 0};
    Signal<int> healthy {g, 0};
    int healthy_runs = 0;

    make_effect(g, [&] {
        if (failing.get() == 1) {
            throw std::runtime_error("expected failure");
        }
    });
    make_effect(g, [&] {
        (void)healthy.get();
        ++healthy_runs;
    });

    REQUIRE_THROWS_AS(failing.set(1), std::runtime_error);
    REQUIRE_FALSE(g.has_pending_effects());

    healthy.set(1);
    REQUIRE(healthy_runs == 2);
}

TEST_CASE("effect failure restores dependency tracking context", "[reactive][effect][scheduler]") {
    Graph graph;
    Signal<int> failing {graph, 0};
    Signal<int> unrelated {graph, 0};
    int runs = 0;

    make_effect(graph, [&] {
        ++runs;
        if (failing.get() == 1) {
            throw std::runtime_error("expected failure");
        }
    });

    REQUIRE_THROWS_AS(failing.set(1), std::runtime_error);
    (void)unrelated.get();
    unrelated.set(1);
    REQUIRE(runs == 2);
}

TEST_CASE("EffectScope clears all effects together", "[reactive][effect][scope]") {
    Graph g;
    Signal<int> s{g, 0};
    int runs_a = 0;
    int runs_b = 0;

    EffectScope scope{g};
    scope.add([&] { (void)s.get(); ++runs_a; });
    scope.add([&] { (void)s.get(); ++runs_b; });
    REQUIRE(scope.count() == 2);

    s.set(1);
    REQUIRE(runs_a == 2);
    REQUIRE(runs_b == 2);

    scope.clear();
    s.set(2);
    REQUIRE(runs_a == 2);  // no re-run after clear
    REQUIRE(runs_b == 2);
}

TEST_CASE("ReactiveScope owns signals computed values and effects", "[reactive][scope]") {
    Graph g;
    int seen = 0;
    int runs = 0;
    Signal<int>* count = nullptr;

    {
        ReactiveScope scope {g};
        count = &scope.signal<int>(1);
        auto& doubled = scope.computed([&] { return count->get() * 2; });
        scope.effect([&] {
            seen = doubled.get();
            ++runs;
        });

        REQUIRE(scope.signal_count() == 1);
        REQUIRE(scope.computed_count() == 1);
        REQUIRE(scope.effect_count() == 1);
        REQUIRE(seen == 2);
        REQUIRE(runs == 1);

        count->set(3);
        REQUIRE(seen == 6);
        REQUIRE(runs == 2);

        scope.clear();
        REQUIRE(scope.signal_count() == 0);
        REQUIRE(scope.computed_count() == 0);
        REQUIRE(scope.effect_count() == 0);
    }
}

TEST_CASE("ReactiveScope disconnects owned event subscriptions", "[reactive][scope][event]") {
    Graph graph;
    Event<int> event;
    int observed = 0;

    ReactiveScope scope {graph};
    scope.connect(event, [&](const int value) { observed += value; });
    REQUIRE(scope.subscription_count() == 1);
    REQUIRE(event.subscriber_count() == 1);

    event.emit(3);
    REQUIRE(observed == 3);

    scope.clear();
    REQUIRE(scope.subscription_count() == 0);
    REQUIRE(event.subscriber_count() == 0);
    event.emit(4);
    REQUIRE(observed == 3);
}

TEST_CASE("Undisposed effect is released by Graph destruction", "[reactive][lifetime]") {
    // No leak / crash: effect and computed outlive their handles but the Graph
    // owns them and tears down cleanly. (Signal must outlive the Graph.)
    Graph g;
    Signal<int> s{g, 0};
    make_effect(g, [&] { (void)s.get(); });
    (void)make_computed(g, [&] { return s.get() + 1; });
    s.set(1);
    // Graph destructor runs here — must not crash.
}
