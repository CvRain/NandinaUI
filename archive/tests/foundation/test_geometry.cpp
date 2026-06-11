#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>

import nandina.foundation.nan_rect;
import nandina.foundation.nan_insets;
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
    EXPECT_TRUE(pt.is_zero());
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
    pt.set_x(7.0f);
    pt.set_y(8.0f);
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
    EXPECT_EQ(pt.length_squared(), 25.0f);
}

TEST(NandinaPoint, DistanceTo) {
    const nandina::geometry::NanPoint a{0.0f, 0.0f};
    const nandina::geometry::NanPoint b{3.0f, 4.0f};
    EXPECT_NEAR(a.distance_to(b), 5.0f, kFloatTolerance);
    EXPECT_EQ(a.distance_squared_to(b), 25.0f);
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
    EXPECT_TRUE(sz.is_empty());
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
    sz.set_width(75.0f);
    sz.set_height(85.0f);
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
    EXPECT_NEAR(sz.aspect_ratio(), 16.0f / 9.0f, kFloatTolerance);

    // Zero height should return 0
    const nandina::geometry::NanSize zero_h{10.0f, 0.0f};
    EXPECT_EQ(zero_h.aspect_ratio(), 0.0f);
}

TEST(NandinaSize, IsEmptyAndIsValid) {
    const nandina::geometry::NanSize valid{10.0f, 20.0f};
    EXPECT_FALSE(valid.is_empty());
    EXPECT_TRUE(valid.is_valid());

    const nandina::geometry::NanSize empty{0.0f, 20.0f};
    EXPECT_TRUE(empty.is_empty());

    const nandina::geometry::NanSize negative{-1.0f, 10.0f};
    EXPECT_FALSE(negative.is_valid());
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
    EXPECT_TRUE(rect.is_empty());
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
    const auto rect = nandina::geometry::BaseRect<float>::from_xywh(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_EQ(rect.left(), 10.0f);
    EXPECT_EQ(rect.top(), 20.0f);
    EXPECT_EQ(rect.width(), 100.0f);
    EXPECT_EQ(rect.height(), 50.0f);
}

TEST(NandinaRect, ConstructWithPoints) {
    const nandina::geometry::NanPoint top_left{10.0f, 20.0f};
    const nandina::geometry::NanPoint bottom_right{110.0f, 120.0f};
    const nandina::geometry::NanRect rect{top_left, bottom_right};

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
    rect.set_rect(5.0f, 10.0f, 105.0f, 60.0f);
    EXPECT_EQ(rect.left(), 5.0f);
    EXPECT_EQ(rect.top(), 10.0f);
    EXPECT_EQ(rect.right(), 105.0f);
    EXPECT_EQ(rect.bottom(), 60.0f);
    EXPECT_EQ(rect.width(), 100.0f);
    EXPECT_EQ(rect.height(), 50.0f);
}

TEST(NandinaRect, SetWidthAndHeight) {
    nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};
    rect.set_width(200.0f);
    rect.set_height(150.0f);
    EXPECT_EQ(rect.width(), 200.0f);
    EXPECT_EQ(rect.height(), 150.0f);
    EXPECT_EQ(rect.right(), 200.0f);
    EXPECT_EQ(rect.bottom(), 150.0f);
}

TEST(NandinaRect, PropertyAccessors) {
    const nandina::geometry::NanRect rect{10.0f, 20.0f, 110.0f, 120.0f};

    EXPECT_EQ(rect.x(), 10.0f);
    EXPECT_EQ(rect.y(), 20.0f);

    const auto top_left = rect.top_left();
    EXPECT_EQ(top_left.x(), 10.0f);
    EXPECT_EQ(top_left.y(), 20.0f);

    const auto bot_right = rect.bottom_right();
    EXPECT_EQ(bot_right.x(), 110.0f);
    EXPECT_EQ(bot_right.y(), 120.0f);

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

    EXPECT_TRUE(intersection.is_empty());
}

TEST(NandinaRect, AlignedInside) {
    const nandina::geometry::NanRect container{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanRect small{0.0f, 0.0f, 20.0f, 20.0f};

    // Test all 9 alignments
    const auto topLeft = small.aligned_inside(container, nandina::geometry::Alignment::TopLeft);
    EXPECT_EQ(topLeft.x(), 0.0f);
    EXPECT_EQ(topLeft.y(), 0.0f);

    const auto topRight = small.aligned_inside(container, nandina::geometry::Alignment::TopRight);
    EXPECT_EQ(topRight.x(), 80.0f);
    EXPECT_EQ(topRight.y(), 0.0f);

    const auto center = small.aligned_inside(container, nandina::geometry::Alignment::Center);
    EXPECT_EQ(center.x(), 40.0f);
    EXPECT_EQ(center.y(), 40.0f);

    const auto bottomCenter = small.aligned_inside(container, nandina::geometry::Alignment::BottomCenter);
    EXPECT_EQ(bottomCenter.x(), 40.0f);
    EXPECT_EQ(bottomCenter.y(), 80.0f);
}

TEST(NandinaRect, WithMarginAndPadding) {
    const nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};

    // withMargin: shrink by margin * 2 (shrinks width/height by 2*margin)
    // left/right move inward by margin*2, top/bottom move inward by margin*2
    const auto withMargin = rect.with_margin(10.0f);
    EXPECT_EQ(withMargin.left(), 20.0f);
    EXPECT_EQ(withMargin.top(), 20.0f);
    EXPECT_EQ(withMargin.right(), 80.0f);
    EXPECT_EQ(withMargin.bottom(), 80.0f);

    // withPadding: same as margin
    const auto withPadding = rect.with_padding(5.0f);
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
    const auto centered = rect.centered_in(container);

    EXPECT_NEAR(centered.x(), 40.0f, kFloatTolerance);
    EXPECT_NEAR(centered.y(), 40.0f, kFloatTolerance);
}

TEST(NandinaRect, BoundedTo) {
    const nandina::geometry::NanRect rect{50.0f, 50.0f, 150.0f, 150.0f};
    const nandina::geometry::NanRect boundary{0.0f, 0.0f, 100.0f, 100.0f};
    const auto bounded = rect.bounded_to(boundary);

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
    EXPECT_TRUE(valid.is_valid());

    // Invalid: right < left
    const nandina::geometry::NanRect invalid{100.0f, 0.0f, 50.0f, 100.0f};
    EXPECT_FALSE(invalid.is_valid());
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

// ============================================================
// NanInsets Tests
// ============================================================

TEST(NandinaInsets, DefaultConstructCreatesZeroInsets) {
    const nandina::geometry::NanInsets insets;
    EXPECT_EQ(insets.left(), 0.0f);
    EXPECT_EQ(insets.top(), 0.0f);
    EXPECT_EQ(insets.right(), 0.0f);
    EXPECT_EQ(insets.bottom(), 0.0f);
    EXPECT_TRUE(insets.is_zero());
    EXPECT_TRUE(insets.is_empty());
}

TEST(NandinaInsets, ConstructWithUniformValue) {
    const nandina::geometry::NanInsets insets{10.0f};
    EXPECT_EQ(insets.left(), 10.0f);
    EXPECT_EQ(insets.top(), 10.0f);
    EXPECT_EQ(insets.right(), 10.0f);
    EXPECT_EQ(insets.bottom(), 10.0f);
    EXPECT_TRUE(insets.is_uniform());
}

TEST(NandinaInsets, ConstructWithHorizontalVertical) {
    const nandina::geometry::NanInsets insets{5.0f, 10.0f};
    EXPECT_EQ(insets.left(), 5.0f);
    EXPECT_EQ(insets.top(), 10.0f);
    EXPECT_EQ(insets.right(), 5.0f);
    EXPECT_EQ(insets.bottom(), 10.0f);
}

TEST(NandinaInsets, ConstructWithFourValues) {
    const nandina::geometry::NanInsets insets{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_EQ(insets.left(), 1.0f);
    EXPECT_EQ(insets.top(), 2.0f);
    EXPECT_EQ(insets.right(), 3.0f);
    EXPECT_EQ(insets.bottom(), 4.0f);
}

TEST(NandinaInsets, StaticFactoryMethods) {
    const auto all = nandina::geometry::NanInsets::all(10.0f);
    EXPECT_EQ(all.left(), 10.0f);
    EXPECT_EQ(all.top(), 10.0f);

    const auto symmetric_h = nandina::geometry::NanInsets::symmetric_h(5.0f);
    EXPECT_EQ(symmetric_h.left(), 5.0f);
    EXPECT_EQ(symmetric_h.right(), 5.0f);
    EXPECT_EQ(symmetric_h.top(), 0.0f);
    EXPECT_EQ(symmetric_h.bottom(), 0.0f);

    const auto symmetric_v = nandina::geometry::NanInsets::symmetric_v(8.0f);
    EXPECT_EQ(symmetric_v.top(), 8.0f);
    EXPECT_EQ(symmetric_v.bottom(), 8.0f);
    EXPECT_EQ(symmetric_v.left(), 0.0f);
    EXPECT_EQ(symmetric_v.right(), 0.0f);

    const auto from_left = nandina::geometry::NanInsets::from_left(3.0f);
    EXPECT_EQ(from_left.left(), 3.0f);
    EXPECT_EQ(from_left.top(), 0.0f);
    EXPECT_EQ(from_left.right(), 0.0f);
    EXPECT_EQ(from_left.bottom(), 0.0f);

    const auto from_top = nandina::geometry::NanInsets::from_top(4.0f);
    EXPECT_EQ(from_top.top(), 4.0f);
    EXPECT_EQ(from_top.left(), 0.0f);

    const auto from_right = nandina::geometry::NanInsets::from_right(5.0f);
    EXPECT_EQ(from_right.right(), 5.0f);
    EXPECT_EQ(from_right.left(), 0.0f);

    const auto from_bottom = nandina::geometry::NanInsets::from_bottom(6.0f);
    EXPECT_EQ(from_bottom.bottom(), 6.0f);
    EXPECT_EQ(from_bottom.top(), 0.0f);
}

TEST(NandinaInsets, AccessorsAndProperties) {
    const nandina::geometry::NanInsets insets{10.0f, 20.0f, 30.0f, 40.0f};

    EXPECT_EQ(insets.horizontal(), 40.0f);   // 10 + 30
    EXPECT_EQ(insets.vertical(), 60.0f);      // 20 + 40

    const auto tl = insets.top_left();
    EXPECT_EQ(tl.x(), 10.0f);
    EXPECT_EQ(tl.y(), 20.0f);

    const auto br = insets.bottom_right();
    EXPECT_EQ(br.x(), 30.0f);
    EXPECT_EQ(br.y(), 40.0f);
}

TEST(NandinaInsets, SetMethods) {
    nandina::geometry::NanInsets insets;

    insets.set_left(1.0f);
    insets.set_top(2.0f);
    insets.set_right(3.0f);
    insets.set_bottom(4.0f);
    EXPECT_EQ(insets.left(), 1.0f);
    EXPECT_EQ(insets.top(), 2.0f);
    EXPECT_EQ(insets.right(), 3.0f);
    EXPECT_EQ(insets.bottom(), 4.0f);

    insets.set_all(10.0f);
    EXPECT_TRUE(insets.is_uniform());
    EXPECT_EQ(insets.left(), 10.0f);

    insets.set(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ(insets.left(), 1.0f);
    EXPECT_EQ(insets.right(), 3.0f);
}

TEST(NandinaInsets, PropertyChecks) {
    const nandina::geometry::NanInsets zero;
    EXPECT_TRUE(zero.is_zero());
    EXPECT_TRUE(zero.is_empty());
    EXPECT_FALSE(zero.has_positive());
    EXPECT_FALSE(zero.has_negative());

    const nandina::geometry::NanInsets positive{5.0f};
    EXPECT_TRUE(positive.has_positive());
    EXPECT_FALSE(positive.has_negative());
    EXPECT_TRUE(positive.is_uniform());
    EXPECT_TRUE(positive.is_horizontal_symmetric());
    EXPECT_TRUE(positive.is_vertical_symmetric());

    const nandina::geometry::NanInsets asymmetric{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FALSE(asymmetric.is_uniform());
    EXPECT_FALSE(asymmetric.is_horizontal_symmetric());
    EXPECT_FALSE(asymmetric.is_vertical_symmetric());

    const nandina::geometry::NanInsets negative{-5.0f};
    EXPECT_TRUE(negative.has_negative());
    EXPECT_FALSE(negative.has_positive());
}

TEST(NandinaInsets, ApplyToRect) {
    const nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanInsets insets{10.0f, 20.0f, 30.0f, 40.0f};

    const auto applied = insets.apply_to_rect(rect);
    EXPECT_EQ(applied.left(), 10.0f);
    EXPECT_EQ(applied.top(), 20.0f);
    EXPECT_EQ(applied.right(), 70.0f);   // 100 - 30
    EXPECT_EQ(applied.bottom(), 60.0f);  // 100 - 40
}

TEST(NandinaInsets, InflateRect) {
    const nandina::geometry::NanRect rect{10.0f, 20.0f, 90.0f, 80.0f};
    const nandina::geometry::NanInsets insets{5.0f};

    const auto inflated = insets.inflate_rect(rect);
    EXPECT_EQ(inflated.left(), 5.0f);     // 10 - 5
    EXPECT_EQ(inflated.top(), 15.0f);     // 20 - 5
    EXPECT_EQ(inflated.right(), 95.0f);   // 90 + 5
    EXPECT_EQ(inflated.bottom(), 85.0f);  // 80 + 5
}

TEST(NandinaInsets, ToSize) {
    const nandina::geometry::NanInsets insets{10.0f, 20.0f, 30.0f, 40.0f};
    const auto size = insets.to_size();
    EXPECT_EQ(size.width(), 40.0f);   // 10 + 30
    EXPECT_EQ(size.height(), 60.0f);  // 20 + 40
}

TEST(NandinaInsets, ArithmeticOperators) {
    const nandina::geometry::NanInsets a{10.0f, 20.0f, 30.0f, 40.0f};
    const nandina::geometry::NanInsets b{1.0f, 2.0f, 3.0f, 4.0f};

    const auto sum = a + b;
    EXPECT_EQ(sum.left(), 11.0f);
    EXPECT_EQ(sum.top(), 22.0f);
    EXPECT_EQ(sum.right(), 33.0f);
    EXPECT_EQ(sum.bottom(), 44.0f);

    const auto diff = a - b;
    EXPECT_EQ(diff.left(), 9.0f);
    EXPECT_EQ(diff.top(), 18.0f);
    EXPECT_EQ(diff.right(), 27.0f);
    EXPECT_EQ(diff.bottom(), 36.0f);

    const auto scaled = a * 2.0f;
    EXPECT_EQ(scaled.left(), 20.0f);
    EXPECT_EQ(scaled.top(), 40.0f);
    EXPECT_EQ(scaled.right(), 60.0f);
    EXPECT_EQ(scaled.bottom(), 80.0f);

    const auto divided = a / 2.0f;
    EXPECT_EQ(divided.left(), 5.0f);
    EXPECT_EQ(divided.top(), 10.0f);
    EXPECT_EQ(divided.right(), 15.0f);
    EXPECT_EQ(divided.bottom(), 20.0f);

    const auto neg = -a;
    EXPECT_EQ(neg.left(), -10.0f);
    EXPECT_EQ(neg.top(), -20.0f);
    EXPECT_EQ(neg.right(), -30.0f);
    EXPECT_EQ(neg.bottom(), -40.0f);
}

TEST(NandinaInsets, CompoundAssignmentOperators) {
    nandina::geometry::NanInsets insets{10.0f, 20.0f, 30.0f, 40.0f};

    insets += nandina::geometry::NanInsets{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_EQ(insets.left(), 11.0f);
    EXPECT_EQ(insets.top(), 22.0f);

    insets -= nandina::geometry::NanInsets{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_EQ(insets.left(), 10.0f);
    EXPECT_EQ(insets.top(), 20.0f);

    insets *= 3.0f;
    EXPECT_EQ(insets.left(), 30.0f);
    EXPECT_EQ(insets.right(), 90.0f);

    insets /= 3.0f;
    EXPECT_EQ(insets.left(), 10.0f);
    EXPECT_EQ(insets.right(), 30.0f);
}

TEST(NandinaInsets, DivisionByZeroThrows) {
    const nandina::geometry::NanInsets insets{10.0f};
    EXPECT_THROW([[maybe_unused]] auto result = insets / 0.0f, std::domain_error);
}

TEST(NandinaInsets, EqualityOperators) {
    const nandina::geometry::NanInsets a{10.0f, 20.0f, 30.0f, 40.0f};
    const nandina::geometry::NanInsets b{10.0f, 20.0f, 30.0f, 40.0f};
    const nandina::geometry::NanInsets c{1.0f, 2.0f, 3.0f, 4.0f};

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(NandinaInsets, ScalarLeftMultiplication) {
    const nandina::geometry::NanInsets insets{10.0f, 20.0f, 30.0f, 40.0f};
    const auto result = 2.0f * insets;
    EXPECT_EQ(result.left(), 20.0f);
    EXPECT_EQ(result.top(), 40.0f);
    EXPECT_EQ(result.right(), 60.0f);
    EXPECT_EQ(result.bottom(), 80.0f);
}

TEST(NandinaInsets, RectWithInsetsOperators) {
    const nandina::geometry::NanRect rect{0.0f, 0.0f, 100.0f, 100.0f};
    const nandina::geometry::NanInsets insets{10.0f, 20.0f, 30.0f, 40.0f};

    // rect + insets = applyToRect (shrink)
    const auto shrunk = rect + insets;
    EXPECT_EQ(shrunk.left(), 10.0f);
    EXPECT_EQ(shrunk.top(), 20.0f);
    EXPECT_EQ(shrunk.right(), 70.0f);
    EXPECT_EQ(shrunk.bottom(), 60.0f);

    // rect - insets = inflateRect (expand)
    const auto expanded = rect - insets;
    EXPECT_EQ(expanded.left(), -10.0f);
    EXPECT_EQ(expanded.top(), -20.0f);
    EXPECT_EQ(expanded.right(), 130.0f);
    EXPECT_EQ(expanded.bottom(), 140.0f);
}

TEST(NandinaInsets, Swap) {
    nandina::geometry::NanInsets a{1.0f, 2.0f, 3.0f, 4.0f};
    nandina::geometry::NanInsets b{5.0f, 6.0f, 7.0f, 8.0f};

    a.swap(b);
    EXPECT_EQ(a.left(), 5.0f);
    EXPECT_EQ(b.left(), 1.0f);
}

TEST(NandinaInsets, ToString) {
    const nandina::geometry::NanInsets insets{10.0f, 20.0f, 30.0f, 40.0f};
    const auto str = insets.to_string();
    EXPECT_NE(str.find("L=10"), std::string::npos);
    EXPECT_NE(str.find("T=20"), std::string::npos);
    EXPECT_NE(str.find("R=30"), std::string::npos);
    EXPECT_NE(str.find("B=40"), std::string::npos);
}