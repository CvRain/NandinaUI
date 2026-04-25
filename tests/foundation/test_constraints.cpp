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
    EXPECT_EQ(c.min_width(), 0.0f);
    EXPECT_EQ(c.max_width(), 0.0f);
    EXPECT_EQ(c.min_height(), 0.0f);
    EXPECT_EQ(c.max_height(), 0.0f);
    EXPECT_TRUE(c.is_tight());
    EXPECT_TRUE(c.is_valid());
}

TEST(NandinaConstraints, ConstructWithFourValues) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    EXPECT_EQ(c.min_width(), 10.0f);
    EXPECT_EQ(c.max_width(), 100.0f);
    EXPECT_EQ(c.min_height(), 20.0f);
    EXPECT_EQ(c.max_height(), 200.0f);
    EXPECT_TRUE(c.is_valid());
}

TEST(NandinaConstraints, ConstructFromNanSize) {
    const nandina::geometry::NanSize size{100.0f, 200.0f};
    const nandina::geometry::NanConstraints c{size};
    EXPECT_EQ(c.min_width(), 100.0f);
    EXPECT_EQ(c.max_width(), 100.0f);
    EXPECT_EQ(c.min_height(), 200.0f);
    EXPECT_EQ(c.max_height(), 200.0f);
    EXPECT_TRUE(c.is_tight());
}

TEST(NandinaConstraints, ConstructWithWidthHeight) {
    const nandina::geometry::NanConstraints c{300.0f, 400.0f};
    EXPECT_EQ(c.min_width(), 300.0f);
    EXPECT_EQ(c.max_width(), 300.0f);
    EXPECT_EQ(c.min_height(), 400.0f);
    EXPECT_EQ(c.max_height(), 400.0f);
    EXPECT_TRUE(c.is_tight());
}

TEST(NandinaConstraints, StaticFactoryTight) {
    const auto c = nandina::geometry::NanConstraints::tight(100.0f, 200.0f);
    EXPECT_TRUE(c.is_tight());
    EXPECT_EQ(c.min_width(), 100.0f);
    EXPECT_EQ(c.max_width(), 100.0f);
    EXPECT_EQ(c.min_height(), 200.0f);
    EXPECT_EQ(c.max_height(), 200.0f);
}

TEST(NandinaConstraints, StaticFactoryTightWithSize) {
    const nandina::geometry::NanSize size{150.0f, 250.0f};
    const auto c = nandina::geometry::NanConstraints::tight(size);
    EXPECT_TRUE(c.is_tight());
    EXPECT_EQ(c.min_width(), 150.0f);
    EXPECT_EQ(c.max_width(), 150.0f);
    EXPECT_EQ(c.min_height(), 250.0f);
    EXPECT_EQ(c.max_height(), 250.0f);
}

TEST(NandinaConstraints, StaticFactoryLoose) {
    const auto c = nandina::geometry::NanConstraints::loose(100.0f, 200.0f);
    EXPECT_TRUE(c.is_loose());
    EXPECT_FALSE(c.is_tight());
    EXPECT_EQ(c.min_width(), 0.0f);
    EXPECT_EQ(c.max_width(), 100.0f);
    EXPECT_EQ(c.min_height(), 0.0f);
    EXPECT_EQ(c.max_height(), 200.0f);
}

TEST(NandinaConstraints, StaticFactoryLooseWithSize) {
    const nandina::geometry::NanSize size{150.0f, 250.0f};
    const auto c = nandina::geometry::NanConstraints::loose(size);
    EXPECT_TRUE(c.is_loose());
    EXPECT_EQ(c.min_width(), 0.0f);
    EXPECT_EQ(c.max_width(), 150.0f);
    EXPECT_EQ(c.min_height(), 0.0f);
    EXPECT_EQ(c.max_height(), 250.0f);
}

TEST(NandinaConstraints, StaticFactoryExpand) {
    const auto c = nandina::geometry::NanConstraints::expand();
    EXPECT_FALSE(c.is_tight());
    EXPECT_TRUE(c.is_loose());  // expand {0, INF, 0, INF} has min==0 so is_loose is true
    EXPECT_TRUE(c.has_unbounded_width());
    EXPECT_TRUE(c.has_unbounded_height());
    EXPECT_FALSE(c.is_bounded());
    EXPECT_EQ(c.min_width(), 0.0f);
    EXPECT_EQ(c.min_height(), 0.0f);
    EXPECT_EQ(c.max_width(), std::numeric_limits<float>::infinity());
    EXPECT_EQ(c.max_height(), std::numeric_limits<float>::infinity());
}

TEST(NandinaConstraints, Queryis_tight) {
    const nandina::geometry::NanConstraints tight{50.0f, 50.0f, 75.0f, 75.0f};
    EXPECT_TRUE(tight.is_tight());
    EXPECT_TRUE(tight.is_tight_width());
    EXPECT_TRUE(tight.is_tight_height());

    const nandina::geometry::NanConstraints notTight{0.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_FALSE(notTight.is_tight());
    EXPECT_FALSE(notTight.is_tight_width());
    EXPECT_FALSE(notTight.is_tight_height());

    const nandina::geometry::NanConstraints tightW{50.0f, 50.0f, 0.0f, 200.0f};
    EXPECT_FALSE(tightW.is_tight());
    EXPECT_TRUE(tightW.is_tight_width());
    EXPECT_FALSE(tightW.is_tight_height());
}

TEST(NandinaConstraints, QueryIsLoose) {
    const auto loose = nandina::geometry::NanConstraints::loose(100.0f, 200.0f);
    EXPECT_TRUE(loose.is_loose());

    const nandina::geometry::NanConstraints notLoose{10.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_FALSE(notLoose.is_loose());

    const nandina::geometry::NanConstraints tight{50.0f, 50.0f, 75.0f, 75.0f};
    EXPECT_FALSE(tight.is_loose());
}

TEST(NandinaConstraints, QueryIsBoundedAndUnbounded) {
    const nandina::geometry::NanConstraints bounded{0.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_TRUE(bounded.is_bounded());
    EXPECT_FALSE(bounded.has_unbounded_width());
    EXPECT_FALSE(bounded.has_unbounded_height());

    const nandina::geometry::NanConstraints unboundedW{0.0f, std::numeric_limits<float>::infinity(), 0.0f, 200.0f};
    EXPECT_TRUE(unboundedW.has_unbounded_width());
    EXPECT_FALSE(unboundedW.has_unbounded_height());
    EXPECT_FALSE(unboundedW.is_bounded());
}

TEST(NandinaConstraints, Queryis_valid) {
    const nandina::geometry::NanConstraints valid{0.0f, 100.0f, 0.0f, 200.0f};
    EXPECT_TRUE(valid.is_valid());

    const nandina::geometry::NanConstraints invalid{100.0f, 50.0f, 0.0f, 200.0f};
    EXPECT_FALSE(invalid.is_valid());
}

TEST(NandinaConstraints, Tighten) {
    const nandina::geometry::NanConstraints c{0.0f, 200.0f, 0.0f, 300.0f};
    const nandina::geometry::NanSize target{100.0f, 150.0f};
    const auto tightened = c.tighten(target);

    EXPECT_TRUE(tightened.is_tight());
    EXPECT_EQ(tightened.min_width(), 100.0f);
    EXPECT_EQ(tightened.max_width(), 100.0f);
    EXPECT_EQ(tightened.min_height(), 150.0f);
    EXPECT_EQ(tightened.max_height(), 150.0f);
}

TEST(NandinaConstraints, TightenClamped) {
    const nandina::geometry::NanConstraints c{50.0f, 100.0f, 50.0f, 200.0f};
    // target outside range: tighten applies std::max(min, value) to min and std::min(max, value) to max
    // value 25: min_width=max(50,25)=50, max_width=min(100,25)=25  -> invalid (min>max)
    // value 300: min_height=max(50,300)=300, max_height=min(200,300)=200 -> invalid (min>max)
    const nandina::geometry::NanSize target{25.0f, 300.0f};
    const auto tightened = c.tighten(target);

    EXPECT_EQ(tightened.min_width(), 50.0f);
    EXPECT_EQ(tightened.max_width(), 25.0f);
    EXPECT_EQ(tightened.min_height(), 300.0f);
    EXPECT_EQ(tightened.max_height(), 200.0f);
    EXPECT_FALSE(tightened.is_valid());
}

TEST(NandinaConstraints, Loosen) {
    const nandina::geometry::NanConstraints c{50.0f, 100.0f, 75.0f, 200.0f};
    const auto loosened = c.loosen();

    EXPECT_TRUE(loosened.is_loose());
    EXPECT_EQ(loosened.min_width(), 0.0f);
    EXPECT_EQ(loosened.max_width(), 100.0f);
    EXPECT_EQ(loosened.min_height(), 0.0f);
    EXPECT_EQ(loosened.max_height(), 200.0f);
}

TEST(NandinaConstraints, Intersect) {
    const nandina::geometry::NanConstraints a{0.0f, 100.0f, 0.0f, 200.0f};
    const nandina::geometry::NanConstraints b{50.0f, 150.0f, 25.0f, 175.0f};
    const auto intersection = a.intersect(b);

    EXPECT_EQ(intersection.min_width(), 50.0f);
    EXPECT_EQ(intersection.max_width(), 100.0f);
    EXPECT_EQ(intersection.min_height(), 25.0f);
    EXPECT_EQ(intersection.max_height(), 175.0f);
}

TEST(NandinaConstraints, IntersectInvalid) {
    const nandina::geometry::NanConstraints a{0.0f, 50.0f, 0.0f, 50.0f};
    const nandina::geometry::NanConstraints b{100.0f, 200.0f, 100.0f, 200.0f};
    const auto intersection = a.intersect(b);

    // min > max both directions -> invalid
    EXPECT_FALSE(intersection.is_valid());
    EXPECT_GT(intersection.min_width(), intersection.max_width());
    EXPECT_GT(intersection.min_height(), intersection.max_height());
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

    EXPECT_EQ(c.constrain_width(50.0f), 50.0f);
    EXPECT_EQ(c.constrain_width(5.0f), 10.0f);   // below min
    EXPECT_EQ(c.constrain_width(200.0f), 100.0f); // above max

    EXPECT_EQ(c.constrain_height(100.0f), 100.0f);
    EXPECT_EQ(c.constrain_height(5.0f), 20.0f);   // below min
    EXPECT_EQ(c.constrain_height(300.0f), 200.0f); // above max
}

TEST(NandinaConstraints, ConstrainPoint) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanPoint point{-5.0f, 300.0f};
    const auto constrained = c.constrain(point);

    EXPECT_EQ(constrained.x(), 10.0f);   // clamped to min_width
    EXPECT_EQ(constrained.y(), 200.0f);  // clamped to max_height
}

TEST(NandinaConstraints, MinSize) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const auto min_size = c.min_size();
    EXPECT_EQ(min_size.width(), 10.0f);
    EXPECT_EQ(min_size.height(), 20.0f);
}

TEST(NandinaConstraints, MaxSize) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const auto max_size = c.max_size();
    EXPECT_EQ(max_size.width(), 100.0f);
    EXPECT_EQ(max_size.height(), 200.0f);
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
    c.set_min_width(10.0f);
    c.set_max_width(100.0f);
    c.set_min_height(20.0f);
    c.set_max_height(200.0f);

    EXPECT_EQ(c.min_width(), 10.0f);
    EXPECT_EQ(c.max_width(), 100.0f);
    EXPECT_EQ(c.min_height(), 20.0f);
    EXPECT_EQ(c.max_height(), 200.0f);
}

TEST(NandinaConstraints, ToString) {
    const nandina::geometry::NanConstraints c{10.0f, 100.0f, 20.0f, 200.0f};
    const auto str = c.to_string();
    EXPECT_NE(str.find("minW=10"), std::string::npos);
    EXPECT_NE(str.find("maxW=100"), std::string::npos);
    EXPECT_NE(str.find("minH=20"), std::string::npos);
    EXPECT_NE(str.find("maxH=200"), std::string::npos);

    // Unbounded constraints show INF
    const auto expand = nandina::geometry::NanConstraints::expand();
    const auto expandStr = expand.to_string();
    EXPECT_NE(expandStr.find("INF"), std::string::npos);
}

TEST(NandinaConstraints, TightTightenIdempotent) {
    // Tightening an already tight constraint should yield the same tight constraint
    const nandina::geometry::NanConstraints tight{100.0f, 100.0f, 200.0f, 200.0f};
    const nandina::geometry::NanSize size{100.0f, 200.0f};
    const auto tightened = tight.tighten(size);

    EXPECT_TRUE(tightened.is_tight());
    EXPECT_EQ(tightened.min_width(), 100.0f);
    EXPECT_EQ(tightened.max_width(), 100.0f);
    EXPECT_EQ(tightened.min_height(), 200.0f);
    EXPECT_EQ(tightened.max_height(), 200.0f);
}

TEST(NandinaConstraints, LoosenIdempotent) {
    // Loosening an already loose constraint should yield the same loose constraint
    const auto loose = nandina::geometry::NanConstraints::loose(100.0f, 200.0f);
    const auto loosened = loose.loosen();

    EXPECT_TRUE(loosened.is_loose());
    EXPECT_EQ(loosened.min_width(), 0.0f);
    EXPECT_EQ(loosened.max_width(), 100.0f);
}

TEST(NandinaConstraints, ConstrainUnboundedWidth) {
    const nandina::geometry::NanConstraints c{0.0f, std::numeric_limits<float>::infinity(), 0.0f, 200.0f};
    EXPECT_TRUE(c.has_unbounded_width());

    // Any width should pass through
    const nandina::geometry::NanSize large{10000.0f, 100.0f};
    const auto constrained = c.constrain(large);
    EXPECT_EQ(constrained.width(), 10000.0f);
    EXPECT_EQ(constrained.height(), 100.0f);
}

TEST(NandinaConstraints, ConstrainUnboundedHeight) {
    const nandina::geometry::NanConstraints c{0.0f, 100.0f, 0.0f, std::numeric_limits<float>::infinity()};
    EXPECT_TRUE(c.has_unbounded_height());

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
    EXPECT_EQ(intersection.min_width(), 50.0f);
    EXPECT_EQ(intersection.max_width(), 100.0f);
    EXPECT_EQ(intersection.min_height(), 25.0f);
    EXPECT_EQ(intersection.max_height(), 200.0f);
    EXPECT_TRUE(intersection.is_valid());
}

TEST(NandinaConstraints, Hash) {
    const nandina::geometry::NanConstraints a{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanConstraints b{10.0f, 100.0f, 20.0f, 200.0f};
    const nandina::geometry::NanConstraintsHash hash;

    EXPECT_EQ(hash(a), hash(b));
}