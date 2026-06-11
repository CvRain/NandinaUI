//
// test_app_var.cpp — Var<T> 和 ComputedVar<T> 的测试用例
//

#include <gtest/gtest.h>

import nandina.app.authoring;  // re-exports nandina.app.var, nandina.app.computed_var, nandina.reactive

namespace {

// ── Var<T> 基础行为 ──────────────────────────────────────────────────────────

TEST(VarTest, InitialValueIsAccessible) {
    nandina::app::Var<int> v{42};
    EXPECT_EQ(v(), 42);
    EXPECT_EQ(v.get(), 42);
}

TEST(VarTest, SetNotifiesEffect) {
    nandina::app::Var<int> v{0};
    int snapshot = -1;
    int runs = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        snapshot = v();
    }};

    EXPECT_EQ(runs, 1);
    EXPECT_EQ(snapshot, 0);

    v.set(99);
    EXPECT_EQ(runs, 2);
    EXPECT_EQ(snapshot, 99);
}

TEST(VarTest, AssignmentOperatorNotifiesEffect) {
    nandina::app::Var<int> v{1};
    int snapshot = -1;

    nandina::reactive::Effect effect{[&] {
        snapshot = v();
    }};

    EXPECT_EQ(snapshot, 1);
    v = 7;
    EXPECT_EQ(snapshot, 7);
}

// ── Var<T>::update() 原地修改 ─────────────────────────────────────────────────

TEST(VarTest, UpdateInPlaceIncrement) {
    nandina::app::Var<int> v{10};
    int snapshot = 0;
    int runs = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        snapshot = v();
    }};

    EXPECT_EQ(runs, 1);
    EXPECT_EQ(snapshot, 10);

    v.update([](int& x){ x++; });

    EXPECT_EQ(runs, 2);
    EXPECT_EQ(snapshot, 11);
    EXPECT_EQ(v(), 11);
}

TEST(VarTest, UpdateInPlaceDecrement) {
    nandina::app::Var<int> v{5};

    v.update([](int& x){ x--; });

    EXPECT_EQ(v(), 4);
}

TEST(VarTest, UpdateInPlaceWithComplexExpression) {
    nandina::app::Var<int> v{3};

    v.update([](int& x){ x = x * x + 1; });

    EXPECT_EQ(v(), 10);
}

// ── Var<T>::update() 值语义 ───────────────────────────────────────────────────

TEST(VarTest, UpdateValueSemanticReturnsNewValue) {
    nandina::app::Var<int> v{3};
    int snapshot = 0;
    int runs = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        snapshot = v();
    }};

    EXPECT_EQ(runs, 1);

    v.update([](const int& x){ return x * 2; });

    EXPECT_EQ(runs, 2);
    EXPECT_EQ(snapshot, 6);
    EXPECT_EQ(v(), 6);
}

TEST(VarTest, UpdateValueSemanticReset) {
    nandina::app::Var<int> v{42};

    // 值语义：忽略旧值，返回常量
    v.update([](int){ return 1; });

    EXPECT_EQ(v(), 1);
}

// ── Var<T>::watch() ───────────────────────────────────────────────────────────

TEST(VarTest, WatchCallbackFiresOnChange) {
    nandina::app::Var<int> v{0};
    int hit = 0;
    int received = -1;

    v.watch([&](const int& val) {
        ++hit;
        received = val;
    });

    v.set(5);
    EXPECT_EQ(hit, 1);
    EXPECT_EQ(received, 5);

    v.set(10);
    EXPECT_EQ(hit, 2);
    EXPECT_EQ(received, 10);
}

// ── ComputedVar<T> 基础行为 ───────────────────────────────────────────────────

TEST(ComputedVarTest, InitialValueIsComputed) {
    nandina::app::Var<int> count{3};
    nandina::app::ComputedVar<int> doubled{[&]{ return count() * 2; }};

    EXPECT_EQ(doubled(), 6);
    EXPECT_EQ(doubled.get(), 6);
}

TEST(ComputedVarTest, UpdatesWhenSourceChanges) {
    nandina::app::Var<int> count{1};
    nandina::app::ComputedVar<int> doubled{[&]{ return count() * 2; }};

    EXPECT_EQ(doubled(), 2);

    count.set(5);
    EXPECT_EQ(doubled(), 10);

    count.set(10);
    EXPECT_EQ(doubled(), 20);
}

TEST(ComputedVarTest, PropagatesIntoEffect) {
    nandina::app::Var<int> count{2};
    nandina::app::ComputedVar<int> doubled{[&]{ return count() * 2; }};

    int snapshot = 0;
    int runs = 0;

    nandina::reactive::Effect effect{[&] {
        ++runs;
        snapshot = doubled();
    }};

    EXPECT_EQ(runs, 1);
    EXPECT_EQ(snapshot, 4);

    count.set(7);

    EXPECT_EQ(runs, 2);
    EXPECT_EQ(snapshot, 14);
}

TEST(ComputedVarTest, ChainedDerivedSignals) {
    // count → doubled → quadrupled
    nandina::app::Var<int> count{1};
    nandina::app::ComputedVar<int> doubled{[&]{ return count() * 2; }};
    nandina::app::ComputedVar<int> quadrupled{[&]{ return doubled() * 2; }};

    EXPECT_EQ(quadrupled(), 4);

    count.set(3);
    EXPECT_EQ(doubled(), 6);
    EXPECT_EQ(quadrupled(), 12);
}

TEST(ComputedVarTest, UpdateInPlaceTriggersPropagation) {
    nandina::app::Var<int> count{1};
    nandina::app::ComputedVar<int> doubled{[&]{ return count() * 2; }};

    int snapshot = 0;
    nandina::reactive::Effect effect{[&] {
        snapshot = doubled();
    }};

    EXPECT_EQ(snapshot, 2);

    count.update([](int& v){ v += 4; });

    EXPECT_EQ(count(), 5);
    EXPECT_EQ(snapshot, 10);
}

} // namespace
