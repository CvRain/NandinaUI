#include <nandina/reactive/reactive.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>

using namespace nandina::reactive;

TEST_CASE(
    "failed initial effects release dependencies captures and queued work",
    "[reactive][initial-failure]"
) {
    Graph graph;
    Signal<int> value(graph, 0);
    EffectScope scope(graph);
    int observed = -1;
    scope.add([&] { observed = value.get(); });
    int calls = 0;
    std::weak_ptr<int> capture;
    {
        auto owned = std::make_shared<int>(0);
        capture = owned;
        REQUIRE_THROWS_AS(
            scope.add([&, owned] {
                ++calls;
                const auto current = value.get();
                if (current == 0) {
                    value.set(1); // Also queue this effect before its first execution throws.
                    throw std::runtime_error("initial effect rejected");
                }
            }),
            std::runtime_error
        );
    }
    REQUIRE(capture.expired());
    REQUIRE(scope.count() == 1);
    value.set(2);
    graph.flush();
    REQUIRE(observed == 2);
    REQUIRE(calls == 1);
    scope.clear();
    value.set(3);
    graph.flush();
    REQUIRE(observed == 2);
    REQUIRE(calls == 1);
    // Remove make_effect's catch/dispose: captures and subscriptions survive the exception.
}

TEST_CASE(
    "effect initialization rollback also works during another effect flush",
    "[reactive][nested-initial-failure]"
) {
    Graph graph;
    Signal<int> trigger(graph, 0);
    Signal<int> value(graph, 0);
    EffectScope scope(graph);
    int calls = 0;
    int outer = 0;
    scope.add([&] {
        outer = trigger.get();
        if (outer != 1) {
            return;
        }
        REQUIRE_THROWS_AS(
            scope.add([&] {
                ++calls;
                (void)value.get();
                throw std::runtime_error("nested rejection");
            }),
            std::runtime_error
        );
    });
    trigger.set(1);
    REQUIRE(calls == 1);
    REQUIRE(scope.count() == 1);
    REQUIRE_NOTHROW(value.set(1));
    REQUIRE(calls == 1);
    trigger.set(2);
    REQUIRE(outer == 2);
    // Only cleaning up outside a flush leaves the rejected nested subscription active.
}
