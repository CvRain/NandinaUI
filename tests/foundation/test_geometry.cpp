#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>

import nandina.foundation.nan_rect;
using namespace std;

namespace {
    constexpr auto kFloatTolerance = 1.0e-6f;
}

// ============================================================
// NanPoint Tests
// ============================================================

TEST(NandinaPoint, DefaultConstructCreatesZeroPoint) {
    const nandina::geometry::NanPoint pt;
    EXPECT_EQ(pt.x(), 0.0f);
    EXPECT_EQ(pt.y(), 0.0f);
    EXPECT_TRUE(pt.isZero());
}

TEST(NandinaPoint, ConstructWithCoordinates) {
    const nandina::geometry::NanPoint pt{3.0f, 4.0f};
    EXPECT_EQ(pt.x(), 3.0f);
    EXPECT_EQ(pt.y(), 4.0f);
}

TEST(NandinaPoint, SetCoordinates) {
    nandina::geometry::NanPoint pt;
    pt.set(5.0f, 6.0f);
    EXPECT_EQ(pt.x(), 5.0f);
    EXPECT_EQ(pt.y(), 6.0f);
    pt.setX(7.0f);
    pt.setY(8.0f);
    EXPECT_EQ(pt.x(), 7.0f);
    EXPECT_EQ(pt.y(), 8.0f);
}

TEST(NandinaPoint, ArithmeticOperators) {
    const nandina::geometry::NanPoint a{1.0f, 2.0f};
    const nandina::geometry::NanPoint b{3.0f, 4.0f};

    // Addition
    const auto sum = a + b;
    EXPECT_EQ(sum.x(), 4.0f);
    EXPECT_EQ(sum.y(), 6.0f);

    // Subtraction
    const auto diff = a - b;
    EXPECT_EQ(diff.x(), -2.0f);
    EXPECT_EQ(diff.y(), -2.0f);

    // Scalar multiplication
    const auto scaled = a * 3.0f;
    EXPECT_EQ(scaled.x(), 3.0f);
    EXPECT_EQ(scaled.y(), 6.0f);

    // Scalar division
    const auto divided = a / 2.0f;
    EXPECT_EQ(divided.x(), 0.5f);
    EXPECT_EQ(divided.y(), 1.0f);

    // Negation
    const auto neg = -a;
    EXPECT_EQ(neg.x(), -1.0f);
    EXPECT_EQ(neg.y(), -2.0f);
}

TEST(NandinaPoint, CompoundAssignmentOperators) {
    nandina::geometry::NanPoint pt{1.0f, 2.0f};

    pt += nandina::geometry::NanPoint{3.0f, 4.0f};
    EXPECT_EQ(pt.x(), 4.0f);
    EXPECT_EQ(pt.y(), 6.0f);

    pt -= nandina::geometry::NanPoint{1.0f, 2.0f};
    EXPECT_EQ(pt.x(), 3.0f);
    EXPECT_EQ(pt.y(), 4.0f);

    pt *= 2.0f;
    EXPECT_EQ(pt.x(), 6.0f);
    EXPECT_EQ(pt.y(), 8.0f);

    pt /= 2.0f;
    EXPECT_EQ(pt.x(), 3.0f);
    EXPECT_EQ(pt.y(), 4.0f);
}

TEST(NandinaPoint, DivisionByZeroThrows) {
    const nandina::geometry::NanPoint pt{1.0f, 2.0f};
    EXPECT_THROW([[maybe_unused]] auto result = pt / 0.0f, std::domain_error);
}

TEST(NandinaPoint, DotProduct) {
    const nandina::geometry::NanPoint a{1.0f, 2.0f};
    const nandina::geometry::NanPoint b{3.0f, 4.0f};
    EXPECT_EQ(a.dot(b), 11.0f); // 1*3 + 2*4 = 11
}

TEST(NandinaPoint, CrossProduct) {
    const nandina::geometry::NanPoint a{1.0f, 2.0f};
    const nandina::geometry::NanPoint b{3.0f, 4.0f};
    EXPECT_EQ(a.cross(b), -2.0f); // 1*4 - 2*3 = -2
}

TEST(NandinaPoint, LengthAndDistance) {
    const nandina::geometry::NanPoint pt{3.0f, 4.0f};
    EXPECT_NEAR(pt.length(), 5.0f, kFloatTolerance);
    EXPECT_EQ(pt.lengthSquared(), 25.0f);
}

TEST(NandinaPoint, DistanceTo) {
    const nandina::geometry::NanPoint a{0.0f, 0.0f};
    const nandina::geometry::NanPoint b{3.0f, 4.0f};
    EXPECT_NEAR(a.distanceTo(b), 5.0f, kFloatTolerance);
    EXPECT_EQ(a.distanceSquaredTo(b), 25.0f);
}

TEST(NandinaPoint, Normalize) {
    nandina::geometry::NanPoint pt{3.0f, 4.0f};
    const auto normalized = pt.normalized();
    EXPECT_NEAR(normalized.x(), 0.6f, kFloatTolerance);
    EXPECT_NEAR(normalized.y(), 0.8f, kFloatTolerance);
    EXPECT_NEAR(normalized.length(), 1.0f, kFloatTolerance);
}

TEST(NandinaPoint, NormalizeZeroVectorThrows) {
    const nandina::geometry::NanPoint pt{0.0f, 0.0f};
    EXPECT_THROW([[maybe_unused]] auto result = pt.normalized(), std::domain_error);
}

TEST(NandinaPoint, EqualityOperators) {
    const nandina::geometry::NanPoint a{1.0f, 2.0f};
    const nandina::geometry::NanPoint b{1.0f, 2.0f};
    const nandina::geometry::NanPoint c{1.0f, 3.0f};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(NandinaPoint, ComparisonOperators) {
    const nandina::geometry::NanPoint a{1.0f, 2.0f};
    const nandina::geometry::NanPoint b{2.0f, 1.0f};

    EXPECT_TRUE(a < b);  // lexicographical: 1 < 2
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(b >= a);
}

TEST(NandinaPoint, StructuredBinding) {
    const nandina::geometry::NanPoint pt{1.0f, 2.0f};
    // Access via subscript operator (underlying array)
    const auto x = pt[0];
    const auto y = pt[1];
    EXPECT_EQ(x, 1.0f);
    EXPECT_EQ(y, 2.0f);
}

TEST(NandinaPoint, CastToDifferentType) {
    const nandina::geometry::NanPoint pt{1.5f, 2.5f};
    const auto casted = pt.cast<int>();
    EXPECT_EQ(casted[0], 1);
    EXPECT_EQ(casted[1], 2);
}

// ============================================================
// NanSize Tests
// ============================================================

TEST(NandinaSize, DefaultConstructCreatesZeroSize) {
    const nandina::geometry::NanSize sz;
    EXPECT_EQ(sz.width(), 0.0f);
    EXPECT_EQ(sz.height(), 0.0f);
    EXPECT_TRUE(sz.isEmpty());
}

TEST(NandinaSize, ConstructWithDimensions) {
    const nandina::geometry::NanSize sz{100.0f, 200.0f};
    EXPECT_EQ(sz.width(), 100.0f);
    EXPECT_EQ(sz.height(), 200.0f);
}

TEST(NandinaSize, SetDimensions) {
    nandina::geometry::NanSize sz;
    sz.set(50.0f, 60.0f);
    EXPECT_EQ(sz.width(), 50.0f);
    EXPECT_EQ(sz.height(), 60.0f);
    sz.setWidth(75.0f);
    sz.setHeight(85.0f);
    EXPECT_EQ(sz.width(), 75.0f);
    EXPECT_EQ(sz.height(), 85.0f);
}

TEST(NandinaSize, ArithmeticOperators) {
    const nandina::geometry::NanSize a{10.0f, 20.0f};
    const nandina::geometry::NanSize b{5.0f, 3.0f};

    const auto sum = a + b;
    EXPECT_EQ(sum.width(), 15.0f);
    EXPECT_EQ(sum.height(), 23.0f);

    const auto diff = a - b;
    EXPECT_EQ(diff.width(), 5.0f);
    EXPECT_EQ(diff.height(), 17.0f);

    const auto scaled = a * 2.0f;
    EXPECT_EQ(scaled.width(), 20.0f);
    EXPECT_EQ(scaled.height(), 40.0f);

    const auto divided = a / 5.0f;
    EXPECT_EQ(divided.width(), 2.0f);
    EXPECT_EQ(divided.height(), 4.0f);
}

TEST(NandinaSize, Area) {
    const nandina::geometry::NanSize sz{10.0f, 20.0f};
    EXPECT_EQ(sz.area(), 200.0f);
}

TEST(NandinaSize, AspectRatio) {
    const nandina::geometry::NanSize sz{16.0f, 9.0f};
    EXPECT_NEAR(sz.aspectRatio(), 16.0f / 9.0f, kFloatTolerance);

    // Zero height should return 0
    const nandina::geometry::NanSize zero_h{10.0f, 0.0f};
    EXPECT_EQ(zero_h.aspectRatio(), 0.0f);
}

TEST(NandinaSize, IsEmptyAndIsValid) {
    const nandina::geometry::NanSize valid{10.0f, 20.0f};
    EXPECT_FALSE(valid.isEmpty());
    EXPECT_TRUE(valid.isValid());

    const nandina::geometry::NanSize empty{0.0f, 20.0f};
    EXPECT_TRUE(empty.isEmpty());

    const nandina::geometry::NanSize negative{-1.0f, 10.0f};
    EXPECT_FALSE(negative.isValid());
}

TEST(NandinaSize, ConversionToNanPoint) {
    const nandina::geometry::NanSize sz{100.0f, 200.0f};
    nandina::geometry::NanPoint pt = sz;
    EXPECT_EQ(pt.x(), 100.0f);
    EXPECT_EQ(pt.y(), 200.0f);
}

// ============================================================
// NanRect Tests
// ============================================================

TEST(NandinaRect, DefaultConstructCreatesEmptyRect) {
    const nandina::geometry::NanRect rect;
    EXPECT_TRUE(rect.isEmpty());
    EXPECT_EQ(rect.width(), 0.0f);
    EXPECT_EQ(rect.height(), 0.0f);
}

TEST(NandinaRect, ConstructWithBounds) {
    const nandina::geometry::NanRect rect{10.0f, 20.0f, 110.0f, 120.0f};
    EXPECT_EQ(rect.left(), 10.0f);
    EXPECT_EQ(rect.top(), 20.0f);
    EXPECT_EQ(rect.right(), 110.0f);
    EXPECT_EQ(rect.bottom(), 120.0f);
    EXPECT_EQ(rect.width(), 100.0f);
    EXPECT_EQ(rect.height(), 100.0f);
}

TEST(NandinaRect, FromXYWH) {
    const auto rect = nandina::geometry::BaseRect<float>::fromXYWH(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_EQ(rect.left(), 10.0f);
    EXPECT_EQ(rect.top(), 20.0f);
    EXPECT_EQ(rect.width(), 100.0f);
    EXPECT_EQ(rect.height(), 50.0f);
}

TEST(NandinaRect, ConstructWithPoints) {
    const nandina::geometry::NanPoint topLeft{10.0f, 20.0f};
    const nandina::geometry::NanPoint bottomRight{110.0f, 120.0f};
    const nandina::geometry::NanRect rect{topLeft, bottomRight};

    EXPECT_EQ(rect.left(), 10.0f);
    EXPECT_EQ(rect.top(), 20.0f);
    EXPECT_EQ(rect.right(), 110.0f);
    EXPECT_EQ(rect.bottom(), 120.0f);
}

TEST(NandinaRect, ConstructWithPointAndSize) {
    const nandina::geometry::NanPoint origin{10.0f, 20.0f};
    const nandina::geometry::NanSize sz{100.0f, 50.0f};
    const nandina::geometry::NanRect rect{origin, sz};

    EXPECT_EQ(rect.left(), 10.0f);
    EXPECT_EQ(rect.top(), 20.0f);
    EXPECT_EQ(rect.width(), 100.0f);
    EXPECT_EQ(rect.height(), 50.0f);
}

TEST(NandinaRect, SetBounds) {
    nandina::geometry::NanRect rect;
    rect.setRect(5.0f, 10.0f, 105.0f, 60.0f);
    EXPECT_EQ(rect.left(), 5.0f);
    EXPECT_EQ(rect.top(), 10.0f);
    EXPECT_EQ(rect.right(), 105.0f);
    EXPECT_EQ(rect.bottom(), 60.0f);
    EXPECT_EQ(rect.width(), 100.0f);
    EXPECT_EQ(rect.height(), 50.0f);
}

TEST(NandinaRect, SetWidthAndHeight) {
    nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};
    rect.setWidth(200.0f);
    rect.setHeight(150.0f);
    EXPECT_EQ(rect.width(), 200.0f);
    EXPECT_EQ(rect.height(), 150.0f);
    EXPECT_EQ(rect.right(), 200.0f);
    EXPECT_EQ(rect.bottom(), 150.0f);
}

TEST(NandinaRect, PropertyAccessors) {
    const nandina::geometry::NanRect rect{10.0f, 20.0f, 110.0f, 120.0f};

    EXPECT_EQ(rect.x(), 10.0f);
    EXPECT_EQ(rect.y(), 20.0f);

    const auto topLeft = rect.topLeft();
    EXPECT_EQ(topLeft.x(), 10.0f);
    EXPECT_EQ(topLeft.y(), 20.0f);

    const auto botRight = rect.bottomRight();
    EXPECT_EQ(botRight.x(), 110.0f);
    EXPECT_EQ(botRight.y(), 120.0f);

    const auto center = rect.center();
    EXPECT_NEAR(center.x(), 60.0f, kFloatTolerance);
    EXPECT_NEAR(center.y(), 70.0f, kFloatTolerance);

    const auto size = rect.size();
    EXPECT_EQ(size.width(), 100.0f);
    EXPECT_EQ(size.height(), 100.0f);
}

TEST(NandinaRect, Translate) {
    const nandina::geometry::NanRect rect{10.0f, 20.0f, 110.0f, 120.0f};

    // Translated copy
    const auto translated = rect.translated(5.0f, 10.0f);
    EXPECT_EQ(translated.left(), 15.0f);
    EXPECT_EQ(translated.top(), 30.0f);
    EXPECT_EQ(translated.right(), 115.0f);
    EXPECT_EQ(translated.bottom(), 130.0f);

    // Original unchanged
    EXPECT_EQ(rect.left(), 10.0f);

    // Translate in place
    nandina::geometry::NanRect mutableRect = rect;
    mutableRect.translate(5.0f, 10.0f);
    EXPECT_EQ(mutableRect.left(), 15.0f);
}

TEST(NandinaRect, Scale) {
    const nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};
    const auto scaled = rect.scaled(2.0f, 0.5f);

    EXPECT_EQ(scaled.left(), 0.0f);
    EXPECT_EQ(scaled.top(), 0.0f);
    EXPECT_EQ(scaled.right(), 200.0f);
    EXPECT_EQ(scaled.bottom(), 50.0f);
}

TEST(NandinaRect, ExpandAndShrink) {
    const nandina::geometry::NanRect rect{10.0f, 20.0f, 110.0f, 120.0f};

    // Expand by 10
    const auto expanded = rect.expanded(10.0f);
    EXPECT_EQ(expanded.left(), 0.0f);
    EXPECT_EQ(expanded.top(), 10.0f);
    EXPECT_EQ(expanded.right(), 120.0f);
    EXPECT_EQ(expanded.bottom(), 130.0f);

    // Shrink by 10
    const auto shrunk = rect.shrinked(10.0f);
    EXPECT_EQ(shrunk.left(), 20.0f);
    EXPECT_EQ(shrunk.top(), 30.0f);
    EXPECT_EQ(shrunk.right(), 100.0f);
    EXPECT_EQ(shrunk.bottom(), 110.0f);
}

TEST(NandinaRect, ContainsPoint) {
    const nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};

    EXPECT_TRUE(rect.contains(nandina::geometry::NanPoint{50.0f, 50.0f}));
    EXPECT_TRUE(rect.contains(nandina::geometry::NanPoint{0.0f, 0.0f}));
    EXPECT_TRUE(rect.contains(nandina::geometry::NanPoint{100.0f, 100.0f}));
    EXPECT_FALSE(rect.contains(nandina::geometry::NanPoint{-1.0f, 50.0f}));
    EXPECT_FALSE(rect.contains(nandina::geometry::NanPoint{50.0f, -1.0f}));
    EXPECT_FALSE(rect.contains(nandina::geometry::NanPoint{101.0f, 50.0f}));
}

TEST(NandinaRect, ContainsRect) {
    const nandina::geometry::NanRect outer{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect inner{10.0f, 10.0f, 90.0f, 90.0f};
    const nandina::geometry::NanRect overlapping{50.0f, 50.0f, 150.0f, 150.0f};

    EXPECT_TRUE(outer.contains(inner));
    EXPECT_FALSE(outer.contains(overlapping));
}

TEST(NandinaRect, Intersects) {
    const nandina::geometry::NanRect a{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect b{50.0f, 50.0f, 150.0f, 150.0f};
    const nandina::geometry::NanRect c{200.0f, 200.0f, 300.0f, 300.0f};

    EXPECT_TRUE(a.intersects(b));
    EXPECT_FALSE(a.intersects(c));
}

TEST(NandinaRect, Intersected) {
    const nandina::geometry::NanRect a{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect b{50.0f, 50.0f, 150.0f, 150.0f};
    const auto intersection = a.intersected(b);

    EXPECT_EQ(intersection.left(), 50.0f);
    EXPECT_EQ(intersection.top(), 50.0f);
    EXPECT_EQ(intersection.right(), 100.0f);
    EXPECT_EQ(intersection.bottom(), 100.0f);
}

TEST(NandinaRect, United) {
    const nandina::geometry::NanRect a{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect b{50.0f, 50.0f, 150.0f, 150.0f};
    const auto unionRect = a.united(b);

    EXPECT_EQ(unionRect.left(), 0.0f);
    EXPECT_EQ(unionRect.top(), 0.0f);
    EXPECT_EQ(unionRect.right(), 150.0f);
    EXPECT_EQ(unionRect.bottom(), 150.0f);
}

TEST(NandinaRect, IntersectedNonOverlappingReturnsEmpty) {
    const nandina::geometry::NanRect a{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect b{200.0f, 200.0f, 300.0f, 300.0f};
    const auto intersection = a.intersected(b);

    EXPECT_TRUE(intersection.isEmpty());
}

TEST(NandinaRect, AlignedInside) {
    const nandina::geometry::NanRect container{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect small{0.0f, 0.0f, 20.0f, 20.0f};

    // Test all 9 alignments
    const auto topLeft = small.alignedInside(container, nandina::geometry::Alignment::TopLeft);
    EXPECT_EQ(topLeft.x(), 0.0f);
    EXPECT_EQ(topLeft.y(), 0.0f);

    const auto topRight = small.alignedInside(container, nandina::geometry::Alignment::TopRight);
    EXPECT_EQ(topRight.x(), 80.0f);
    EXPECT_EQ(topRight.y(), 0.0f);

    const auto center = small.alignedInside(container, nandina::geometry::Alignment::Center);
    EXPECT_EQ(center.x(), 40.0f);
    EXPECT_EQ(center.y(), 40.0f);

    const auto bottomCenter = small.alignedInside(container, nandina::geometry::Alignment::BottomCenter);
    EXPECT_EQ(bottomCenter.x(), 40.0f);
    EXPECT_EQ(bottomCenter.y(), 80.0f);
}

TEST(NandinaRect, WithMarginAndPadding) {
    const nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};

    // withMargin: shrink by margin * 2 (shrinks width/height by 2*margin)
    // left/right move inward by margin*2, top/bottom move inward by margin*2
    const auto withMargin = rect.withMargin(10.0f);
    EXPECT_EQ(withMargin.left(), 20.0f);
    EXPECT_EQ(withMargin.top(), 20.0f);
    EXPECT_EQ(withMargin.right(), 80.0f);
    EXPECT_EQ(withMargin.bottom(), 80.0f);

    // withPadding: same as margin
    const auto withPadding = rect.withPadding(5.0f);
    EXPECT_EQ(withPadding.left(), 10.0f);
    EXPECT_EQ(withPadding.top(), 10.0f);
    EXPECT_EQ(withPadding.right(), 90.0f);
    EXPECT_EQ(withPadding.bottom(), 90.0f);
}

TEST(NandinaRect, Inset) {
    const nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};
    const auto inset = rect.inset(10.0f, 20.0f);

    EXPECT_EQ(inset.left(), 10.0f);
    EXPECT_EQ(inset.top(), 20.0f);
    EXPECT_EQ(inset.right(), 90.0f);
    EXPECT_EQ(inset.bottom(), 80.0f);
}

TEST(NandinaRect, CenteredIn) {
    const nandina::geometry::NanRect rect{40.0f, 40.0f, 60.0f, 60.0f};  // 20x20 centered in 100x100
    const nandina::geometry::NanRect container{0.0f, 0.0f, 100.0f, 100.0f};
    const auto centered = rect.centeredIn(container);

    EXPECT_NEAR(centered.x(), 40.0f, kFloatTolerance);
    EXPECT_NEAR(centered.y(), 40.0f, kFloatTolerance);
}

TEST(NandinaRect, BoundedTo) {
    const nandina::geometry::NanRect rect{50.0f, 50.0f, 150.0f, 150.0f};
    const nandina::geometry::NanRect boundary{0.0f, 0.0f, 100.0f, 100.0f};
    const auto bounded = rect.boundedTo(boundary);

    EXPECT_EQ(bounded.left(), 50.0f);
    EXPECT_EQ(bounded.top(), 50.0f);
    EXPECT_EQ(bounded.right(), 100.0f);
    EXPECT_EQ(bounded.bottom(), 100.0f);
}

TEST(NandinaRect, EqualityOperators) {
    const nandina::geometry::NanRect a{10.0f, 20.0f, 110.0f, 120.0f};
    const nandina::geometry::NanRect b{10.0f, 20.0f, 110.0f, 120.0f};
    const nandina::geometry::NanRect c{10.0f, 20.0f, 110.0f, 130.0f};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(NandinaRect, IsValid) {
    const nandina::geometry::NanRect valid{0.0f, 0.0f, 100.0f, 100.0f};
    EXPECT_TRUE(valid.isValid());

    // Invalid: right < left
    const nandina::geometry::NanRect invalid{100.0f, 0.0f, 50.0f, 100.0f};
    EXPECT_FALSE(invalid.isValid());
}

TEST(NandinaRect, Operators) {
    const nandina::geometry::NanRect a{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect b{50.0f, 50.0f, 150.0f, 150.0f};
    const nandina::geometry::NanPoint offset{10.0f, 20.0f};

    // Rect + Point
    const auto translated = a + offset;
    EXPECT_EQ(translated.left(), 10.0f);
    EXPECT_EQ(translated.top(), 20.0f);

    // Rect - Point
    const auto moved = a - offset;
    EXPECT_EQ(moved.left(), -10.0f);
    EXPECT_EQ(moved.top(), -20.0f);

    // Rect & Rect (intersection)
    const auto intersection = a & b;
    EXPECT_EQ(intersection.left(), 50.0f);
    EXPECT_EQ(intersection.top(), 50.0f);

    // Rect | Rect (union)
    const auto unionRect = a | b;
    EXPECT_EQ(unionRect.left(), 0.0f);
    EXPECT_EQ(unionRect.top(), 0.0f);
    EXPECT_EQ(unionRect.right(), 150.0f);
    EXPECT_EQ(unionRect.bottom(), 150.0f);
}
