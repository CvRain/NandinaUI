//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_TRANSFORM2D_HPP
#define NANDINA_EXPERIMENT_TRANSFORM2D_HPP

#include "geometry.hpp"

namespace nandina::foundation
{

    /**
     * Decomposed 2D affine transform: position (translation), rotation (radians),
     * and non-uniform scale.
     *
     * The transform maps local coordinates to world coordinates:
     *   world = translate(position) * rotate(rotation) * scale(scale_x, scale_y) * local
     *
     * Stored in decomposed form rather than a raw matrix so that individual
     * components can be read, animated, and interpolated without decomposition
     * ambiguity. The decomposition order is fixed: scale first, then rotate,
     * then translate.
     *
     * @note Rotation is in radians.  In screen coordinates (x right, y down)
     *       a positive rotation appears clockwise.
     * @note Scale components can be negative to support mirroring/flipping.
     */
    class NanTransform2D {
    public:
        /// Identity transform (position 0, rotation 0, scale 1).
        NanTransform2D() = default;

        /// Fully specified transform.
        NanTransform2D(NanPoint position, float rotation, NanPoint scale):
            position_(position),
            rotation_(rotation),
            scale_(scale) {}

        // ---- named constructors ----

        /// Identity.
        [[nodiscard]] static auto identity() -> NanTransform2D {
            return {};
        }

        /// Translation only.
        [[nodiscard]] static auto from_position(NanPoint pos) -> NanTransform2D {
            return {pos, 0.0F, NanPoint(1.0F, 1.0F)};
        }

        /// Rotation only (clock-wise radians).
        [[nodiscard]] static auto from_rotation(float radians) -> NanTransform2D {
            return {NanPoint(0, 0), radians, NanPoint(1.0F, 1.0F)};
        }

        /// Uniform scale only.
        [[nodiscard]] static auto from_scale(float s) -> NanTransform2D {
            return {NanPoint(0, 0), 0.0F, NanPoint(s, s)};
        }

        /// Non-uniform scale only.
        [[nodiscard]] static auto from_scale_xy(float sx, float sy) -> NanTransform2D {
            return {NanPoint(0, 0), 0.0F, NanPoint(sx, sy)};
        }

        // ---- accessors ----

        [[nodiscard]] auto position() const -> NanPoint {
            return position_;
        }
        [[nodiscard]] auto rotation() const -> float {
            return rotation_;
        }
        [[nodiscard]] auto scale() const -> NanPoint {
            return scale_;
        }
        [[nodiscard]] auto scale_x() const -> float {
            return scale_.get_x();
        }
        [[nodiscard]] auto scale_y() const -> float {
            return scale_.get_y();
        }

        /// True if this is the identity transform.
        [[nodiscard]] auto is_identity() const -> bool;

        // ---- mutation (chainable) ----

        auto set_position(NanPoint pos) -> NanTransform2D&;
        auto set_rotation(float radians) -> NanTransform2D&;
        auto set_scale(NanPoint s) -> NanTransform2D&;
        auto set_scale_xy(float sx, float sy) -> NanTransform2D&;

        /// Translate in local space (applied after current rotation/scale).
        auto translate(NanPoint offset) -> NanTransform2D&;
        [[nodiscard]] auto translated(NanPoint offset) const -> NanTransform2D;

        /// Rotate (add radians).
        auto rotate(float radians) -> NanTransform2D&;
        [[nodiscard]] auto rotated(float radians) const -> NanTransform2D;

        /// Uniform scale.
        auto scale_by(float factor) -> NanTransform2D&;
        [[nodiscard]] auto scaled(float factor) const -> NanTransform2D;

        /// Non-uniform scale.
        auto scale_by_xy(float sx, float sy) -> NanTransform2D&;
        [[nodiscard]] auto scaled_xy(float sx, float sy) const -> NanTransform2D;

        // ---- composition ----

        /**
         * Compose child-transform with this parent-transform.
         *
         *   result = parent * child
         *
         * A point in the child's local space is first transformed by `child`,
         * then by this transform. This mirrors how a scene-tree transform chain
         * works: child-local → parent-local → world.
         */
        [[nodiscard]] auto compose(const NanTransform2D& child) const -> NanTransform2D;
        [[nodiscard]] auto operator*(const NanTransform2D& child) const -> NanTransform2D;

        /// Compose in-place (equivalant to this = this * child).
        auto operator*=(const NanTransform2D& child) -> NanTransform2D&;

        // ---- inverse ----

        /// Inverse transform (world → local).
        [[nodiscard]] auto inverse() const -> NanTransform2D;

        // ---- point / vector transformation ----

        /// Transform a point from local to world space.
        [[nodiscard]] auto transform_point(NanPoint point) const -> NanPoint;

        /// Transform a point from world to local space.
        [[nodiscard]] auto inverse_transform_point(NanPoint point) const -> NanPoint;

        // ---- interpolation ----

        /**
         * Linear interpolation of each component independently.
         * Rotation uses the shortest path.
         */
        [[nodiscard]] static auto lerp(const NanTransform2D& a, const NanTransform2D& b, float t)
            -> NanTransform2D;

        // ---- comparison ----

        [[nodiscard]] auto operator==(const NanTransform2D& other) const -> bool;
        [[nodiscard]] auto operator!=(const NanTransform2D& other) const -> bool;

    private:
        NanPoint position_ {0, 0};
        float rotation_ {0}; // radians
        NanPoint scale_ {1, 1};
    };

} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_TRANSFORM2D_HPP
