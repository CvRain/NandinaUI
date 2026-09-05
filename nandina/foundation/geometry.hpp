//
// nandina/foundation/geometry.hpp
// 基础坐标类型定义
// Created by cvrain on 2026/6/29.

#ifndef NANDINA_EXPERIMENT_GEOMETRY_HPP
#define NANDINA_EXPERIMENT_GEOMETRY_HPP

#include <array>
#include <tuple>
#include <utility>

namespace nandina::foundation
{
    /**
     * Two-dimensional point (or vector), the foundational coordinate type for all
     * layout and rendering operations.
     *
     * Internally stored as (x, y) with float precision.
     */
    class NanPoint {
    public:
        /// Construct a point at (x, y).
        explicit NanPoint(float x = 0.0F, float y = 0.0F);

        /// Construct a point from a std::pair.
        explicit NanPoint(const std::pair<float, float>& pos);

        /// Point at the origin.
        [[nodiscard]] static auto zero() -> NanPoint;

        // ---- mutation ----

        /// Set the x coordinate (chainable).
        auto set_x(float x) -> NanPoint&;

        /// Set the y coordinate (chainable).
        auto set_y(float y) -> NanPoint&;

        // ---- accessors ----

        /// Return x.
        [[nodiscard]] auto get_x() const -> float;

        /// Return y.
        [[nodiscard]] auto get_y() const -> float;

        /// Return (x, y) as a std::pair.
        [[nodiscard]] auto get_point() const -> std::pair<float, float>;

        // ---- point arithmetic ----

        /// Add another point in-place.
        auto add(const NanPoint& point) -> NanPoint&;
        auto operator+=(const NanPoint& point) -> NanPoint&;

        /// Subtract another point in-place.
        auto sub(const NanPoint& point) -> NanPoint&;
        auto operator-=(const NanPoint& point) -> NanPoint&;

        /// Uniform scale in-place.
        auto scale(float factor) -> NanPoint&;
        auto operator*=(float factor) -> NanPoint&;

        /// Non-uniform scale in-place (x * sx, y * sy).
        auto scale_xy(float sx, float sy) -> NanPoint&;

        auto operator+(const NanPoint& point) const -> NanPoint;
        auto operator-(const NanPoint& point) const -> NanPoint;
        auto operator*(float factor) const -> NanPoint;

        /// Negate the point (unary -).
        [[nodiscard]] auto negated() const -> NanPoint;
        auto operator-() const -> NanPoint;

        // ---- vector math ----

        /// Dot product with another point (treated as a vector).
        [[nodiscard]] auto dot(const NanPoint& point) const -> float;

        /// 2D cross product (determinant): x * p.y - y * p.x.
        [[nodiscard]] auto cross(const NanPoint& point) const -> float;

        /// Squared Euclidean length.
        [[nodiscard]] auto length_squared() const -> float;

        /// Euclidean length (magnitude).
        [[nodiscard]] auto length() const -> float;

        /// Euclidean distance to another point.
        [[nodiscard]] auto distance(const NanPoint& point) const -> float;

        /// Return a unit vector in the same direction, or zero if length is zero.
        [[nodiscard]] auto normalized() const -> NanPoint;

        /// Linear interpolation: (1 - t) * a + t * b.
        [[nodiscard]] static auto lerp(const NanPoint& a, const NanPoint& b, float t) -> NanPoint;

        // ---- comparison ----

        [[nodiscard]] auto equals(const NanPoint& point) const -> bool;
        auto operator==(const NanPoint& point) const -> bool;
        auto operator!=(const NanPoint& point) const -> bool;

    private:
        float x_axis {0};
        float y_axis {0};
    };

    /**
     * Two-dimensional size (width, height), guaranteed non-negative by convention.
     */
    class NanSize {
    public:
        explicit NanSize(float width = 0, float height = 0);
        explicit NanSize(const std::pair<float, float>& size);

        /// Size with both dimensions set to zero.
        [[nodiscard]] static auto zero() -> NanSize;

        /// Size with both dimensions set to one.
        [[nodiscard]] static auto one() -> NanSize;

        // ---- mutation ----

        auto set_width(float width) -> NanSize&;
        auto set_height(float height) -> NanSize&;
        auto set_size(float width, float height) -> NanSize&;

        // ---- accessors ----

        [[nodiscard]] auto get_width() const -> float;
        [[nodiscard]] auto get_height() const -> float;
        [[nodiscard]] auto get_size() const -> std::pair<float, float>;

        /// Width / height; returns 0 if height is zero.
        [[nodiscard]] auto aspect_ratio() const -> float;

        /// Width * height.
        [[nodiscard]] auto area() const -> float;

        /// True if either dimension is zero.
        [[nodiscard]] auto is_empty() const -> bool;

        /// True if both dimensions are strictly positive.
        [[nodiscard]] auto is_valid() const -> bool;

        /// True if this size fits within `bounds` (width <= bounds.width && height <= bounds.height).
        [[nodiscard]] auto fits_in(const NanSize& bounds) const -> bool;

        // ---- arithmetic (return new value) ----

        auto operator+(const NanSize& other) const -> NanSize;
        auto operator-(const NanSize& other) const -> NanSize;
        auto operator*(float factor) const -> NanSize;
        auto operator/(float factor) const -> NanSize;

        // ---- arithmetic (mutate in-place) ----

        auto operator+=(const NanSize& other) -> NanSize&;
        auto operator-=(const NanSize& other) -> NanSize&;
        auto operator*=(float factor) -> NanSize&;
        auto operator/=(float factor) -> NanSize&;

        // ---- transforms ----

        /// Uniform scale (width * factor, height * factor).
        auto scaled(float factor) -> NanSize&;

        /// Non-uniform scale (width * sx, height * sy).
        auto scaled_xy(float sx, float sy) -> NanSize&;

        /// Add (w, h) to width and height.
        auto expanded(float w, float h) -> NanSize&;

        /// Clamp each dimension to [min, max].
        auto constrain(const NanSize& min, const NanSize& max) -> NanSize&;

        /// Component-wise maximum with another size.
        [[nodiscard]] auto max(const NanSize& other) const -> NanSize;

        /// Component-wise minimum with another size.
        [[nodiscard]] auto min(const NanSize& other) const -> NanSize;

        // ---- comparison ----

        [[nodiscard]] auto equals(const NanSize& size) const -> bool;
        auto operator==(const NanSize& size) const -> bool;
        auto operator!=(const NanSize& size) const -> bool;

    private:
        float width {0};
        float height {0};
    };

    // Forward declaration so NanRect can reference NanInsets.
    class NanInsets;

    /**
     * Two-dimensional rectangle, stored internally as boundary coordinates
     * (left, top, right, bottom) in float precision.
     *
     * The coordinate space follows screen convention: x increases rightward,
     * y increases downward. Right must be >= left and bottom must be >= top
     * for the rectangle to be valid.
     */
    class NanRect {
    public:
        explicit NanRect(float left = 0, float top = 0, float right = 0, float bottom = 0);
        explicit NanRect(const std::array<float, 4>& rect);
        explicit NanRect(const std::tuple<float, float, float, float>& rect);

        /// Empty rectangle at origin.
        [[nodiscard]] static auto empty() -> NanRect;

        /// Construct from boundary coordinates.
        [[nodiscard]] static auto from_ltrb(float left, float top, float right, float bottom)
            -> NanRect;

        /// Construct from position (x, y) and size (width, height).
        [[nodiscard]] static auto from_xywh(float x, float y, float width, float height) -> NanRect;

        /// Construct from origin point and size.
        [[nodiscard]] static auto from_origin_size(const NanPoint& origin, const NanSize& size)
            -> NanRect;

        /// Construct centered on a point with a given size.
        [[nodiscard]] static auto from_center(const NanPoint& center, const NanSize& size)
            -> NanRect;

        /// Construct from any two opposite corner points.
        [[nodiscard]] static auto from_points(const NanPoint& p1, const NanPoint& p2) -> NanRect;

        // ---- mutation ----

        auto set_left(float left) -> NanRect&;
        auto set_top(float top) -> NanRect&;
        auto set_right(float right) -> NanRect&;
        auto set_bottom(float bottom) -> NanRect&;
        auto set_rect(float left, float top, float right, float bottom) -> NanRect&;

        // ---- boundary accessors ----

        [[nodiscard]] auto get_left() const -> float;
        [[nodiscard]] auto get_top() const -> float;
        [[nodiscard]] auto get_right() const -> float;
        [[nodiscard]] auto get_bottom() const -> float;
        [[nodiscard]] auto get_rect() const -> std::tuple<float, float, float, float>;

        // ---- derived geometry ----

        [[nodiscard]] auto get_x() const -> float;
        [[nodiscard]] auto get_y() const -> float;
        [[nodiscard]] auto get_width() const -> float;
        [[nodiscard]] auto get_height() const -> float;
        [[nodiscard]] auto get_size() const -> NanSize;
        [[nodiscard]] auto get_top_left() const -> NanPoint;
        [[nodiscard]] auto get_top_right() const -> NanPoint;
        [[nodiscard]] auto get_bottom_left() const -> NanPoint;
        [[nodiscard]] auto get_bottom_right() const -> NanPoint;
        [[nodiscard]] auto get_center() const -> NanPoint;

        /// True if right <= left or bottom <= top.
        [[nodiscard]] auto is_empty() const -> bool;

        /// True if right > left and bottom > top.
        [[nodiscard]] auto is_valid() const -> bool;

        // ---- relationships ----

        [[nodiscard]] auto contains_point(const NanPoint& point) const -> bool;
        [[nodiscard]] auto contains_rect(const NanRect& rect) const -> bool;
        [[nodiscard]] auto intersects(const NanRect& rect) const -> bool;

        /// Intersection of two rectangles; returns empty if they don't overlap.
        [[nodiscard]] auto intersected(const NanRect& rect) const -> NanRect;

        /// Smallest rectangle that contains both rectangles.
        [[nodiscard]] auto united(const NanRect& rect) const -> NanRect;

        // ---- transforms (return new value) ----

        /// Translate by an offset vector.
        [[nodiscard]] auto translated(const NanPoint& offset) const -> NanRect;
        [[nodiscard]] auto translated(float offset_x, float offset_y) const -> NanRect;

        /// Expand outward by `amount` on all four sides.
        [[nodiscard]] auto expanded(float amount) const -> NanRect;

        /// Shrink by insets (moves each boundary inward).
        [[nodiscard]] auto inset_by(const NanInsets& insets) const -> NanRect;

        /// Center this rectangle inside `outer`.
        [[nodiscard]] auto centered_in(const NanRect& outer) const -> NanRect;

        /// Uniform scale around the center point.
        [[nodiscard]] auto scaled(float factor) const -> NanRect;

        /// Uniform scale around an arbitrary anchor point.
        [[nodiscard]] auto scaled_from(float factor, const NanPoint& anchor) const -> NanRect;

        // ---- with-* builders (return new value with one field changed) ----

        /// Move to origin without changing size.
        [[nodiscard]] auto with_origin(const NanPoint& origin) const -> NanRect;
        [[nodiscard]] auto with_origin(float x, float y) const -> NanRect;

        /// Change size while keeping the top-left origin.
        [[nodiscard]] auto with_size(const NanSize& size) const -> NanRect;

        /// Change width while keeping the left edge and height intact.
        [[nodiscard]] auto with_width(float width) const -> NanRect;

        /// Change height while keeping the top edge and width intact.
        [[nodiscard]] auto with_height(float height) const -> NanRect;

        // ---- comparison ----

        [[nodiscard]] auto equals(const NanRect& rect) const -> bool;
        auto operator==(const NanRect& rect) const -> bool;
        auto operator!=(const NanRect& rect) const -> bool;

    private:
        float left {0};
        float top {0};
        float right {0};
        float bottom {0};
    };

    /**
     * Four-side inset values for padding, margin, or border widths.
     * Constructor order is (left, right, top, bottom).
     */
    class NanInsets {
    public:
        explicit NanInsets(float left = 0, float right = 0, float top = 0, float bottom = 0);

        // ---- named constructors ----

        /// Same value on all four sides.
        [[nodiscard]] static auto all(float value) -> NanInsets;

        /// Symmetric horizontal and vertical values.
        [[nodiscard]] static auto symmetric(float horizontal, float vertical) -> NanInsets;

        /// Only horizontal insets (left, right).
        [[nodiscard]] static auto horizontal(float value) -> NanInsets;

        /// Only vertical insets (top, bottom).
        [[nodiscard]] static auto vertical(float value) -> NanInsets;

        /// Only the left side.
        [[nodiscard]] static auto only_left(float value) -> NanInsets;

        /// Only the right side.
        [[nodiscard]] static auto only_right(float value) -> NanInsets;

        /// Only the top side.
        [[nodiscard]] static auto only_top(float value) -> NanInsets;

        /// Only the bottom side.
        [[nodiscard]] static auto only_bottom(float value) -> NanInsets;

        // ---- mutation ----

        auto set_left(float left) -> NanInsets&;
        auto set_right(float right) -> NanInsets&;
        auto set_top(float top) -> NanInsets&;
        auto set_bottom(float bottom) -> NanInsets&;

        // ---- accessors ----

        [[nodiscard]] auto get_left() const -> float;
        [[nodiscard]] auto get_right() const -> float;
        [[nodiscard]] auto get_top() const -> float;
        [[nodiscard]] auto get_bottom() const -> float;
        [[nodiscard]] auto get_insets() const -> std::tuple<float, float, float, float>;

        /// True if all four sides are zero.
        [[nodiscard]] auto is_zero() const -> bool;

        /// Sum of left + right.
        [[nodiscard]] auto horizontal_sum() const -> float;

        /// Sum of top + bottom.
        [[nodiscard]] auto vertical_sum() const -> float;

        // ---- geometry application ----

        /// Shrink `rect` by these insets (move boundaries inward).
        [[nodiscard]] auto apply_to_rect(const NanRect& rect) const -> NanRect;

        /// Expand `rect` by these insets (move boundaries outward).
        [[nodiscard]] auto inflate_rect(const NanRect& rect) const -> NanRect;

        // ---- arithmetic ----

        [[nodiscard]] auto add(const NanInsets& other) const -> NanInsets;
        [[nodiscard]] auto operator+(const NanInsets& other) const -> NanInsets;
        auto operator+=(const NanInsets& other) -> NanInsets&;

        /// Scale all four sides by a factor.
        [[nodiscard]] auto operator*(float factor) const -> NanInsets;
        auto operator*=(float factor) -> NanInsets&;

        /// Component-wise maximum.
        [[nodiscard]] auto max(const NanInsets& other) const -> NanInsets;

        /// Component-wise minimum.
        [[nodiscard]] auto min(const NanInsets& other) const -> NanInsets;

        /// Linear interpolation: (1 - t) * a + t * b.
        [[nodiscard]] static auto lerp(const NanInsets& a, const NanInsets& b, float t)
            -> NanInsets;

        // ---- comparison ----

        [[nodiscard]] auto equals(const NanInsets& other) const -> bool;
        auto operator==(const NanInsets& other) const -> bool;
        auto operator!=(const NanInsets& other) const -> bool;

    private:
        float left {0};
        float right {0};
        float top {0};
        float bottom {0};
    };
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_GEOMETRY_HPP
