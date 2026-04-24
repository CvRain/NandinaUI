#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>

import nandina.foundation.nan_constraints;
using namespace std;

namespace {
    constexpr auto kFloatTolerance = 1.0e-6f;
}

// ============================================================
// NanConstraints Tests
// ============================================================

TEST(NandinaConstraints, DefaultConstructCreatesTightZero) {
    const nandina::geometry::NanConstraints c;
    EXPECT_EQ(c.minWidth(), 0.0f);
    EXPECT_EQ(c.maxWidth(), 0.0f);
    EXPECT_EQ(c.minHeight(), 0.0f);
    EXPECT_EQ(c.maxHeight(), 0.0f);
    EXPECT_TRUE(c.isTight());
    EXPECT_TRUE(c.isValid());
}

TEST(NandinaConstraints, ConstructWithFourValues) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    EXPECT_EQ(c.minWidth(), 10.0f);
    EXPECT_EQ(c.maxWidth(), 100.0f);
    EXPECT_EQ(c.minHeight(), 20.0f);
    EXPECT_EQ(c.maxHeight(), 200.0f);
    EXPECT_TRUE(c.isValid());
}

TEST(NandinaConstraints, ConstructFromNanSize) {
    const nandina::geometry::NanSize size{100.0f, 200.0f};
    const nandina::geometry::NanConstraints c{size};
    EXPECT_EQ(c.minWidth(), 100.0f);
    EXPECT_EQ(c.maxWidth(), 100.0f);
    EXPECT_EQ(c.minHeight(), 200.0f);
    EXPECT_EQ(c.maxHeight(), 200.0f);
    EXPECT_TRUE(c.isTight());
}

TEST(NandinaConstraints, ConstructWithWidthHeight) {
    const nandina::geometry::NanConstraints c{300.0f, 400.0f};
    EXPECT_EQ(c.minWidth(), 300.0f);
    EXPECT_EQ(c.maxWidth(), 300.0f);
    EXPECT_EQ(c.minHeight(), 400.0f);
    EXPECT_EQ(c.maxHeight(), 400.0f);
    EXPECT_TRUE(c.isTight());
}

TEST(NandinaConstraints, StaticFactoryTight) {
    const auto c = nandina::geometry::NanConstraints::tight(100.0f, 200.0f);
    EXPECT_TRUE(c.isTight());
    EXPECT_EQ(c.minWidth(), 100.0f);
    EXPECT_EQ(c.maxWidth(), 100.0f);
    EXPECT_EQ(c.minHeight(), 200.0f);
    EXPECT_EQ(c.maxHeight(), 200.0f);
}

TEST(NandinaConstraints, StaticFactoryTightWithSize) {
    const nandina::geometry::NanSize size{150.0f, 250.0f};
    const auto c = nandina::geometry::NanConstraints::tight(size);
    EXPECT_TRUE(c.isTight());
    EXPECT_EQ(c.minWidth(), 150.0f);
    EXPECT_EQ(c.maxWidth(), 150.0f);
    EXPECT_EQ(c.minHeight(), 250.0f);
    EXPECT_EQ(c.maxHeight(), 250.0f);
}

TEST(NandinaConstraints, StaticFactoryLoose) {
    const auto c = nandina::geometry::NanConstraints::loose(100.0f, 200.0f);
    EXPECT_TRUE(c.isLoose());
    EXPECT_FALSE(c.isTight());
    EXPECT_EQ(c.minWidth(), 0.0f);
    EXPECT_EQ(c.maxWidth(), 100.0f);
    EXPECT_EQ(c.minHeight(), 0.0f);
    EXPECT_EQ(c.maxHeight(), 200.0f);
}

TEST(NandinaConstraints, StaticFactoryLooseWithSize) {
    const nandina::geometry::NanSize size{150.0f, 250.0f};
    const auto c = nandina::geometry::NanConstraints::loose(size);
    EXPECT_TRUE(c.isLoose());
    EXPECT_EQ(c.minWidth(), 0.0f);
    EXPECT_EQ(c.maxWidth(), 150.0f);
    EXPECT_EQ(c.minHeight(), 0.0f);
    EXPECT_EQ(c.maxHeight(), 250.0f);
}

TEST(NandinaConstraints, StaticFactoryExpand) {
    const auto c = nandina::geometry::NanConstraints::expand();
    EXPECT_FALSE(c.isTight());
    EXPECT_FALSE(c.isLoose());
    EXPECT_TRUE(c.hasUnboundedWidth());
    EXPECT_TRUE(c.hasUnboundedHeight());
    EXPECT_FALSE(c.isBounded());
    EXPECT_EQ(c.minWidth(), 0.0f);
    EXPECT_EQ(c.minHeight(), 0.0f);
    EXPECT_EQ(c.maxWidth(), std::numeric_limits<float>::infinity());
    EXPECT_EQ(c.maxHeight(), std::numeric_limits<float>::infinity());
}

TEST(NandinaConstraints, QueryIsTight) {
    const nandina::geometry::NanConstraints tight{50.0f, 50.0f, 75.0f, 75.0f};
    EXPECT_TRUE(tight.isTight());
    EXPECT_TRUE(tight.isTightWidth());
    EXPECT_TRUE(tight.isTightHeight());

    const nandina::geometry::NanConstraints notTight{0.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_FALSE(notTight.isTight());
    EXPECT_FALSE(notTight.isTightWidth());
    EXPECT_FALSE(notTight.isTightHeight());

    const nandina::geometry::NanConstraints tightW{50.0f, 50.0f, 0.0f, 200.0f};
    EXPECT_FALSE(tightW.isTight());
    EXPECT_TRUE(tightW.isTightWidth());
    EXPECT_FALSE(tightW.isTightHeight());
}

TEST(NandinaConstraints, QueryIsLoose) {
    const auto loose = nandina::geometry::NanConstraints::loose(100.0f, 200.0f);
    EXPECT_TRUE(loose.isLoose());

    const nandina::geometry::NanConstraints notLoose{10.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_FALSE(notLoose.isLoose());

    const nandina::geometry::NanConstraints tight{50.0f, 50.0f, 75.0f, 75.0f};
    EXPECT_FALSE(tight.isLoose());
}

TEST(NandinaConstraints, QueryIsBoundedAndUnbounded) {
    const nandina::geometry::NanConstraints bounded{0.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_TRUE(bounded.isBounded());
    EXPECT_FALSE(bounded.hasUnboundedWidth());
    EXPECT_FALSE(bounded.hasUnboundedHeight());

    const nandina::geometry::NanConstraints unboundedW{0.0f, std::numeric_limits<float>::infinity(), 0.0f, 200.0f};
    EXPECT_TRUE(unboundedW.hasUnboundedWidth());
    EXPECT_FALSE(unboundedW.hasUnboundedHeight());
    EXPECT_FALSE(unboundedW.isBounded());
}

TEST(NandinaConstraints, QueryIsValid) {
    const nandina::geometry::NanConstraints valid{0.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_TRUE(valid.isValid());

    const nandina::geometry::NanConstraints invalid{100.0f, 50.0f, 0.0f, 200.0f};
    EXPECT_FALSE(invalid.isValid());
}

TEST(NandinaConstraints, Tighten) {
    const nandina::geometry::NanConstraints c{0.0f, 200.0f, 0.0f, 300.0f};
    const nandina::geometry::NanSize target{100.0f, 150.0f};
    const auto tightened = c.tighten(target);

    EXPECT_TRUE(tightened.isTight());
    EXPECT_EQ(tightened.minWidth(), 100.0f);
    EXPECT_EQ(tightened.maxWidth(), 100.0f);
    EXPECT_EQ(tightened.minHeight(), 150.0f);
    EXPECT_EQ(tightened.maxHeight(), 150.0f);
}

TEST(NandinaConstraints, TightenClamped) {
    const nandina::geometry::NanConstraints c{50.0f, 100.0f, 50.0f, 200.0f};
    // target outside range -> clamped to bounds
    const nandina::geometry::NanSize target{25.0f, 300.0f};
    const auto tightened = c.tighten(target);

    EXPECT_EQ(tightened.minWidth(), 50.0f);
    EXPECT_EQ(tightened.maxWidth(), 50.0f);
    EXPECT_EQ(tightened.minHeight(), 200.0f);
    EXPECT_EQ(tightened.maxHeight(), 200.0f);
}

TEST(NandinaConstraints, Loosen) {
    const nandina::geometry::NanConstraints c{50.0f, 100.0f, 75.0f, 200.0f};
    const auto loosened = c.loosen();

    EXPECT_TRUE(loosened.isLoose());
    EXPECT_EQ(loosened.minWidth(), 0.0f);
    EXPECT_EQ(loosened.maxWidth(), 100.0f);
    EXPECT_EQ(loosened.minHeight(), 0.0f);
    EXPECT_EQ(loosened.maxHeight(), 200.0f);
}

TEST(NandinaConstraints, Intersect) {
    const nandina::geometry::NanConstraints a{0.0f, 100.0f, 0.0f, 200.0f};
    const nandina::geometry::NanConstraints b{50.0f, 150.0f, 25.0f, 175.0f};
    const auto intersection = a.intersect(b);

    EXPECT_EQ(intersection.minWidth(), 50.0f);
    EXPECT_EQ(intersection.maxWidth(), 100.0f);
    EXPECT_EQ(intersection.minHeight(), 25.0f);
    EXPECT_EQ(intersection.maxHeight(), 175.0f);
}

TEST(NandinaConstraints, IntersectInvalid) {
    const nandina::geometry::NanConstraints a{0.0f, 50.0f, 0.0f, 50.0f};
    const nandina::geometry::NanConstraints b{100.0f, 200.0f, 100.0f, 200.0f};
    const auto intersection = a.intersect(b);

    // min > max both directions -> invalid
    EXPECT_FALSE(intersection.isValid());
    EXPECT_GT(intersection.minWidth(), intersection.maxWidth());
    EXPECT_GT(intersection.minHeight(), intersection.maxHeight());
}

TEST(NandinaConstraints, ConstrainSize) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};

    // Within range
    const nandina::geometry::NanSize within{50.0f, 100.0f};
    const auto constrained = c.constrain(within);
    EXPECT_EQ(constrained.width(), 50.0f);
    EXPECT_EQ(constrained.height(), 100.0f);

    // Below range
    const nandina::geometry::NanSize below{5.0f, 10.0f};
    const auto constrainedBelow = c.constrain(below);
    EXPECT_EQ(constrainedBelow.width(), 10.0f);
    EXPECT_EQ(constrainedBelow.height(), 20.0f);

    // Above range
    const nandina::geometry::NanSize above{200.0f, 300.0f};
    const auto constrainedAbove = c.constrain(above);
    EXPECT_EQ(constrainedAbove.width(), 100.0f);
    EXPECT_EQ(constrainedAbove.height(), 200.0f);
}

TEST(NandinaConstraints, ConstrainWidthHeight) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};

    EXPECT_EQ(c.constrainWidth(50.0f), 50.0f);
    EXPECT_EQ(c.constrainWidth(5.0f), 10.0f);   // below min
    EXPECT_EQ(c.constrainWidth(200.0f), 100.0f); // above max

    EXPECT_EQ(c.constrainHeight(100.0f), 100.0f);
    EXPECT_EQ(c.constrainHeight(5.0f), 20.0f);   // below min
    EXPECT_EQ(c.constrainHeight(300.0f), 200.0f); // above max
}

TEST(NandinaConstraints, ConstrainPoint) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanPoint point{-5.0f, 300.0f};
    const auto constrained = c.constrain(point);

    EXPECT_EQ(constrained.x(), 10.0f);   // clamped to minWidth
    EXPECT_EQ(constrained.y(), 200.0f);  // clamped to maxHeight
}

TEST(NandinaConstraints, MinSize) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const auto minSize = c.minSize();
    EXPECT_EQ(minSize.width(), 10.0f);
    EXPECT_EQ(minSize.height(), 20.0f);
}

TEST(NandinaConstraints, MaxSize) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const auto maxSize = c.maxSize();
    EXPECT_EQ(maxSize.width(), 100.0f);
    EXPECT_EQ(maxSize.height(), 200.0f);
}

TEST(NandinaConstraints, Middle) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const auto mid = c.middle();
    EXPECT_EQ(mid.width(), 55.0f);   // (10 + 100) * 0.5
    EXPECT_EQ(mid.height(), 110.0f); // (20 + 200) * 0.5
}

TEST(NandinaConstraints, MiddleWithTight) {
    const nandina::geometry::NanConstraints c{100.0f, 100.0f, 200.0f, 200.0f};
    const auto mid = c.middle();
    EXPECT_EQ(mid.width(), 100.0f);   // tight -> min
    EXPECT_EQ(mid.height(), 200.0f);  // tight -> min
}

TEST(NandinaConstraints, MiddleWithUnbounded) {
    const auto c = nandina::geometry::NanConstraints::expand();
    const auto mid = c.middle();
    EXPECT_EQ(mid.width(), 0.0f);   // unbounded -> min
    EXPECT_EQ(mid.height(), 0.0f);  // unbounded -> min
}

TEST(NandinaConstraints, EqualityOperators) {
    const nandina::geometry::NanConstraints a{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanConstraints b{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 300.0f};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(NandinaConstraints, SetMethods) {
    nandina::geometry::NanConstraints c;
    c.setMinWidth(10.0f);
    c.setMaxWidth(100.0f);
    c.setMinHeight(20.0f);
    c.setMaxHeight(200.0f);

    EXPECT_EQ(c.minWidth(), 10.0f);
    EXPECT_EQ(c.maxWidth(), 100.0f);
    EXPECT_EQ(c.minHeight(), 20.0f);
    EXPECT_EQ(c.maxHeight(), 200.0f);
}

TEST(NandinaConstraints, ToString) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const auto str = c.toString();
    EXPECT_NE(str.find("minW=10"), std::string::npos);
    EXPECT_NE(str.find("maxW=100"), std::string::npos);
    EXPECT_NE(str.find("minH=20"), std::string::npos);
    EXPECT_NE(str.find("maxH=200"), std::string::npos);

    // Unbounded constraints show INF
    const auto expand = nandina::geometry::NanConstraints::expand();
    const auto expandStr = expand.toString();
    EXPECT_NE(expandStr.find("INF"), std::string::npos);
}

TEST(NandinaConstraints, TightTightenIdempotent) {
    // Tightening an already tight constraint should yield the same tight constraint
    const nandina::geometry::NanConstraints tight{100.0f, 100.0f, 200.0f, 200.0f};
    const nandina::geometry::NanSize size{100.0f, 200.0f};
    const auto tightened = tight.tighten(size);

    EXPECT_TRUE(tightened.isTight());
    EXPECT_EQ(tightened.minWidth(), 100.0f);
    EXPECT_EQ(tightened.maxWidth(), 100.0f);
    EXPECT_EQ(tightened.minHeight(), 200.0f);
    EXPECT_EQ(tightened.maxHeight(), 200.0f);
}

TEST(NandinaConstraints, LoosenIdempotent) {
    // Loosening an already loose constraint should yield the same loose constraint
    const auto loose = nandina::geometry::NanConstraints::loose(100.0f, 200.0f);
    const auto loosened = loose.loosen();

    EXPECT_TRUE(loosened.isLoose());
    EXPECT_EQ(loosened.minWidth(), 0.0f);
    EXPECT_EQ(loosened.maxWidth(), 100.0f);
}

TEST(NandinaConstraints, ConstrainUnboundedWidth) {
    const nandina::geometry::NanConstraints c{0.0f, std::numeric_limits<float>::infinity(), 0.0f, 200.0f};
    EXPECT_TRUE(c.hasUnboundedWidth());

    // Any width should pass through
    const nandina::geometry::NanSize large{10000.0f, 100.0f};
    const auto constrained = c.constrain(large);
    EXPECT_EQ(constrained.width(), 10000.0f);
    EXPECT_EQ(constrained.height(), 100.0f);
}

TEST(NandinaConstraints, ConstrainUnboundedHeight) {
    const nandina::geometry::NanConstraints c{0.0f, 100.0f, 0.0f, std::numeric_limits<float>::infinity()};
    EXPECT_TRUE(c.hasUnboundedHeight());

    const nandina::geometry::NanSize large{50.0f, 5000.0f};
    const auto constrained = c.constrain(large);
    EXPECT_EQ(constrained.width(), 50.0f);
    EXPECT_EQ(constrained.height(), 5000.0f);
}

TEST(NandinaConstraints, ConstrainTight) {
    const nandina::geometry::NanConstraints c{100.0f, 100.0f, 200.0f, 200.0f};

    const nandina::geometry::NanSize different{50.0f, 300.0f};
    const auto constrained = c.constrain(different);
    EXPECT_EQ(constrained.width(), 100.0f);  // forced to tight width
    EXPECT_EQ(constrained.height(), 200.0f); // forced to tight height
}

TEST(NandinaConstraints, IntersectWithUnbounded) {
    const auto expand = nandina::geometry::NanConstraints::expand();
    const nandina::geometry::NanConstraints bounded{50.0f, 100.0f, 25.0f, 200.0f};

    const auto intersection = expand.intersect(bounded);
    EXPECT_EQ(intersection.minWidth(), 50.0f);
    EXPECT_EQ(intersection.maxWidth(), 100.0f);
    EXPECT_EQ(intersection.minHeight(), 25.0f);
    EXPECT_EQ(intersection.maxHeight(), 200.0f);
    EXPECT_TRUE(intersection.isValid());
}

TEST(NandinaConstraints, Hash) {
    const nandina::geometry::NanConstraints a{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanConstraints b{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanConstraintsHash hash;

    EXPECT_EQ(hash(a), hash(b));
}