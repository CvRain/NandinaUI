//
// Created by cvrain on 2026/4/26.
//

#include <gtest/gtest.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

import nandina.reactive;

namespace {

// ── Core tracking and invalidation semantics ───────────────────────────────

TEST(ReactiveTest, EffectDeduplicatesReads) {
    nandina::reactive::State<int> value{0};
    int runs = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        (void)value();
        (void)value();
    }};

    EXPECT_EQ(runs, 1);
    value.set(1);
    EXPECT_EQ(runs, 2);
}

TEST(ReactiveTest, BatchDeduplicatesEffectReruns) {
    nandina::reactive::State<int> left{1};
    nandina::reactive::State<int> right{2};
    int runs = 0;
    int snapshot = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        snapshot = left() + right();
    }};

    EXPECT_EQ(runs, 1);
    EXPECT_EQ(snapshot, 3);

    nandina::reactive::batch([&] {
        left.set(10);
        right.set(20);
    });

    EXPECT_EQ(runs, 2);
    EXPECT_EQ(snapshot, 30);
}

TEST(ReactiveTest, NestedBatchFlushesOnceAtOuterBoundary) {
    nandina::reactive::State<int> value{0};
    int runs = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        (void)value();
    }};

    EXPECT_EQ(runs, 1);

    nandina::reactive::batch([&] {
        value.set(1);
        nandina::reactive::batch([&] {
            value.set(2);
            value.set(3);
        });
        EXPECT_EQ(runs, 1);
    });

    EXPECT_EQ(runs, 2);
    EXPECT_EQ(value.get(), 3);
}

TEST(ReactiveTest, BatchFlushesBeforeRethrowingCallbackException) {
    nandina::reactive::State<int> value{0};
    int runs = 0;
    int snapshot = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        snapshot = value();
    }};

    EXPECT_EQ(runs, 1);
    EXPECT_EQ(snapshot, 0);

    EXPECT_THROW(
        nandina::reactive::batch([&] {
            value.set(42);
            throw std::runtime_error{"batch callback failure"};
        }),
        std::runtime_error
    );

    EXPECT_EQ(runs, 2);
    EXPECT_EQ(snapshot, 42);
}

TEST(ReactiveTest, BatchRethrowsInvalidatorExceptionAfterFlush) {
    nandina::reactive::State<int> value{0};
    bool should_throw = true;
    int throwing_runs = 0;
    int stable_runs = 0;

    nandina::reactive::EffectScope scope;
    scope.push([&] {
        ++throwing_runs;
        if (value() == 1 && should_throw) {
            throw std::runtime_error{"batched effect failure"};
        }
    });
    scope.push([&] {
        ++stable_runs;
        (void)value();
    });

    EXPECT_EQ(throwing_runs, 1);
    EXPECT_EQ(stable_runs, 1);

    EXPECT_THROW(
        nandina::reactive::batch([&] {
            value.set(1);
        }),
        std::runtime_error
    );

    EXPECT_EQ(throwing_runs, 2);
    EXPECT_EQ(stable_runs, 2);

    should_throw = false;
    value.set(2);

    EXPECT_EQ(throwing_runs, 3);
    EXPECT_EQ(stable_runs, 3);
}

TEST(ReactiveTest, StateNotifyRestoresObserversAfterException) {
    nandina::reactive::State<int> value{0};
    bool should_throw = true;
    int throwing_runs = 0;
    int stable_runs = 0;

    nandina::reactive::EffectScope scope;
    scope.push([&] {
        ++throwing_runs;
        (void)value();
        if (should_throw && value.get() == 1) {
            throw std::runtime_error{"effect failure"};
        }
    });
    scope.push([&] {
        ++stable_runs;
        (void)value();
    });

    EXPECT_EQ(throwing_runs, 1);
    EXPECT_EQ(stable_runs, 1);

    EXPECT_THROW(value.set(1), std::runtime_error);

    should_throw = false;
    value.set(2);

    EXPECT_EQ(throwing_runs, 3);
    EXPECT_EQ(stable_runs, 2);
}

// ── Derived state semantics ────────────────────────────────────────────────

TEST(ReactiveTest, ComputedRecomputesLazily) {
    nandina::reactive::State<int> left{2};
    nandina::reactive::State<int> right{3};
    int recomputes = 0;

    nandina::reactive::Computed sum{[&] {
        ++recomputes;
        return left() + right();
    }};

    EXPECT_EQ(sum(), 5);
    EXPECT_EQ(sum(), 5);
    EXPECT_EQ(recomputes, 1);

    left.set(10);
    EXPECT_EQ(sum(), 13);
    EXPECT_EQ(recomputes, 2);
}

TEST(ReactiveTest, ComputedRetriesAfterException) {
    nandina::reactive::State<int> value{0};
    bool should_throw = true;
    int recomputes = 0;

    nandina::reactive::Computed doubled{[&] {
        ++recomputes;
        if (value() == 1 && should_throw) {
            throw std::runtime_error{"computed failure"};
        }
        return value() * 2;
    }};

    EXPECT_EQ(doubled(), 0);
    value.set(1);

    EXPECT_THROW((void)doubled(), std::runtime_error);

    should_throw = false;
    EXPECT_EQ(doubled(), 2);
    EXPECT_EQ(recomputes, 3);
}

TEST(ReactiveTest, ReadStateIsCopyableAndTracks) {
    nandina::reactive::State<int> value{5};
    nandina::reactive::ReadState<int> read_value = value.as_read_only();
    auto read_value_copy = read_value;
    int runs = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        EXPECT_EQ(read_value_copy(), value());
    }};

    EXPECT_EQ(runs, 1);
    value.set(9);
    EXPECT_EQ(runs, 2);
}

// ── Read-only input model semantics ────────────────────────────────────────

TEST(ReactiveTest, PropSupportsStaticAndReadOnlySources) {
    nandina::reactive::Prop<std::string> static_prop{std::string{"static"}};
    bool static_called = false;
    auto static_conn = static_prop.on_change([&](const std::string&) {
        static_called = true;
    });

    EXPECT_EQ(static_prop.get(), "static");
    EXPECT_FALSE(static_prop.is_reactive());
    EXPECT_FALSE(static_conn.connected());
    EXPECT_FALSE(static_called);

    nandina::reactive::State<std::string> name{"alpha"};
    auto read_name = name.as_read_only();
    nandina::reactive::Prop<std::string> reactive_prop{read_name};
    int hits = 0;

    // Note: copying a Prop does NOT copy the underlying reactive subscription.
    // Only the first Prop connected to ReadState gets updates.
    auto reactive_conn = reactive_prop.on_change([&](const std::string& value) {
        ++hits;
        EXPECT_EQ(value, "beta");
    });

    EXPECT_TRUE(reactive_prop.is_reactive());
    EXPECT_EQ(reactive_prop.get(), "alpha");
    name.set("beta");
    EXPECT_EQ(hits, 1);
    EXPECT_TRUE(reactive_conn.connected());
}

TEST(ReactiveTest, PropDisconnectStopsNotifications) {
    nandina::reactive::State<std::string> text{"alpha"};
    nandina::reactive::Prop<std::string> prop{text.as_read_only()};
    int hits = 0;

    auto conn = prop.on_change([&](const std::string&) {
        ++hits;
    });

    text.set("beta");
    EXPECT_EQ(hits, 1);

    conn.disconnect();
    text.set("gamma");
    EXPECT_EQ(hits, 1);
}

// ── Direct subscription semantics ──────────────────────────────────────────

TEST(ReactiveTest, EventSignalContinuesCleanupAndRethrows) {
    nandina::reactive::EventSignal<int> signal;
    std::vector<int> visited;

    auto persistent = signal.connect([&](int value) {
        visited.push_back(value);
    });

    signal.connect_once([&](int) {
        visited.push_back(99);
        throw std::runtime_error{"boom"};
    });

    auto trailing = signal.connect([&](int value) {
        visited.push_back(value * 10);
    });

    EXPECT_THROW(signal.emit(7), std::runtime_error);
    EXPECT_EQ(visited, (std::vector<int>{7, 99, 70}));
    visited.clear();

    signal.emit(8);
    EXPECT_EQ(visited, (std::vector<int>{8, 80}));
    EXPECT_TRUE(persistent.connected());
    EXPECT_TRUE(trailing.connected());
}

// ── Observable collection semantics ────────────────────────────────────────

TEST(ReactiveTest, StateListTracksEffects) {
    nandina::reactive::StateList<std::string> list;
    int runs = 0;
    std::string snapshot;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        snapshot.clear();
        for (const auto& item : list.get()) {
            snapshot += item;
        }
    }};

    EXPECT_EQ(runs, 1);
    EXPECT_TRUE(snapshot.empty());

    list.push_back("A");
    EXPECT_EQ(runs, 2);
    EXPECT_EQ(snapshot, "A");

    list.push_back("B");
    EXPECT_EQ(runs, 3);
    EXPECT_EQ(snapshot, "AB");

    list.replace(0, "Z");
    EXPECT_EQ(runs, 4);
    EXPECT_EQ(snapshot, "ZB");
}

TEST(ReactiveTest, StateListEmitsChangeNotifications) {
    nandina::reactive::StateList<std::string> list;
    std::vector<nandina::reactive::ListChangeKind> changes;
    int watch_hits = 0;

    auto change_conn = list.on_change([&](const std::vector<std::string>&) {
        ++watch_hits;
    });

    list.push_back("A");
    list.replace(0, "B");
    list.pop_back();

    EXPECT_EQ(watch_hits, 3);
}

} // namespace