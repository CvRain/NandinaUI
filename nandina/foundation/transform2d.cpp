//
// Created by cvrain on 2026/6/30.
//

#include "transform2d.hpp"

#include <cmath>
#include <numbers>

namespace nandina::foundation
{

    // ---------------------------------------------------------------------------
    // accessors
    // ---------------------------------------------------------------------------

    auto NanTransform2D::is_identity() const -> bool {
        return position_.get_x() == 0.0F && position_.get_y() == 0.0F && rotation_ == 0.0F
            && scale_.get_x() == 1.0F && scale_.get_y() == 1.0F;
    }

    // ---------------------------------------------------------------------------
    // mutation
    // ---------------------------------------------------------------------------

    auto NanTransform2D::set_position(const NanPoint pos) -> NanTransform2D& {
        position_ = pos;
        return *this;
    }

    auto NanTransform2D::set_rotation(const float radians) -> NanTransform2D& {
        rotation_ = radians;
        return *this;
    }

    auto NanTransform2D::set_scale(const NanPoint s) -> NanTransform2D& {
        scale_ = s;
        return *this;
    }

    auto NanTransform2D::set_scale_xy(const float sx, const float sy) -> NanTransform2D& {
        scale_ = NanPoint(sx, sy);
        return *this;
    }

    auto NanTransform2D::translate(const NanPoint offset) -> NanTransform2D& {
        position_ = position_ + offset;
        return *this;
    }

    auto NanTransform2D::translated(const NanPoint offset) const -> NanTransform2D {
        return NanTransform2D(position_ + offset, rotation_, scale_);
    }

    auto NanTransform2D::rotate(const float radians) -> NanTransform2D& {
        rotation_ += radians;
        return *this;
    }

    auto NanTransform2D::rotated(const float radians) const -> NanTransform2D {
        return NanTransform2D(position_, rotation_ + radians, scale_);
    }

    auto NanTransform2D::scale_by(const float factor) -> NanTransform2D& {
        scale_ = scale_ * factor;
        return *this;
    }

    auto NanTransform2D::scaled(const float factor) const -> NanTransform2D {
        return NanTransform2D(position_, rotation_, scale_ * factor);
    }

    auto NanTransform2D::scale_by_xy(const float sx, const float sy) -> NanTransform2D& {
        scale_ = NanPoint(scale_.get_x() * sx, scale_.get_y() * sy);
        return *this;
    }

    auto NanTransform2D::scaled_xy(const float sx, const float sy) const -> NanTransform2D {
        return NanTransform2D(
            position_,
            rotation_,
            NanPoint(scale_.get_x() * sx, scale_.get_y() * sy)
        );
    }

    // ---------------------------------------------------------------------------
    // composition  (parent * child)
    // ---------------------------------------------------------------------------

    auto NanTransform2D::compose(const NanTransform2D& child) const -> NanTransform2D {
        // transform child.position into this transform's space
        const auto new_pos = transform_point(child.position_);
        const auto new_rot = rotation_ + child.rotation_;
        const auto new_scl =
            NanPoint(scale_.get_x() * child.scale_.get_x(), scale_.get_y() * child.scale_.get_y());
        return NanTransform2D(new_pos, new_rot, new_scl);
    }

    auto NanTransform2D::operator*(const NanTransform2D& child) const -> NanTransform2D {
        return compose(child);
    }

    auto NanTransform2D::operator*=(const NanTransform2D& child) -> NanTransform2D& {
        *this = compose(child);
        return *this;
    }

    // ---------------------------------------------------------------------------
    // inverse
    // ---------------------------------------------------------------------------

    auto NanTransform2D::inverse() const -> NanTransform2D {
        const auto inv_rot = -rotation_;
        constexpr auto eps = 0.000001F;
        const auto inv_sx = (std::abs(scale_.get_x()) < eps) ? 0.0F : 1.0F / scale_.get_x();
        const auto inv_sy = (std::abs(scale_.get_y()) < eps) ? 0.0F : 1.0F / scale_.get_y();
        const auto inv_scl = NanPoint(inv_sx, inv_sy);

        // inv.position = inverse_scale(inverse_rotate(-position))
        const auto cos_r = std::cos(inv_rot);
        const auto sin_r = std::sin(inv_rot);
        const auto px = -position_.get_x();
        const auto py = -position_.get_y();

        // standard rotation matrix (CW appearance in screen coords)
        const auto rx = cos_r * px - sin_r * py;
        const auto ry = sin_r * px + cos_r * py;

        return NanTransform2D(NanPoint(rx * inv_sx, ry * inv_sy), inv_rot, inv_scl);
    }

    // ---------------------------------------------------------------------------
    // point transformation
    // ---------------------------------------------------------------------------

    auto NanTransform2D::transform_point(const NanPoint point) const -> NanPoint {
        const auto px = point.get_x();
        const auto py = point.get_y();

        // scale
        const auto sx = px * scale_.get_x();
        const auto sy = py * scale_.get_y();

        // rotate (standard rotation — CW appearance in screen coords)
        const auto cos_r = std::cos(rotation_);
        const auto sin_r = std::sin(rotation_);
        const auto rx = cos_r * sx - sin_r * sy;
        const auto ry = sin_r * sx + cos_r * sy;

        // translate
        return NanPoint(rx + position_.get_x(), ry + position_.get_y());
    }

    auto NanTransform2D::inverse_transform_point(const NanPoint point) const -> NanPoint {
        const auto cos_r = std::cos(-rotation_);
        const auto sin_r = std::sin(-rotation_);
        const auto dx = point.get_x() - position_.get_x();
        const auto dy = point.get_y() - position_.get_y();

        // inverse rotate
        const auto rx = cos_r * dx - sin_r * dy;
        const auto ry = sin_r * dx + cos_r * dy;

        // inverse scale
        constexpr auto eps = 0.000001F;
        const auto inv_sx = (std::abs(scale_.get_x()) < eps) ? 0.0F : 1.0F / scale_.get_x();
        const auto inv_sy = (std::abs(scale_.get_y()) < eps) ? 0.0F : 1.0F / scale_.get_y();

        return NanPoint(rx * inv_sx, ry * inv_sy);
    }

    // ---------------------------------------------------------------------------
    // interpolation
    // ---------------------------------------------------------------------------

    auto NanTransform2D::lerp(const NanTransform2D& a, const NanTransform2D& b, const float t)
        -> NanTransform2D {
        // shortest-path rotation
        auto delta = b.rotation_ - a.rotation_;
        constexpr auto pi = std::numbers::pi_v<float>;
        constexpr auto tau = 2.0F * pi;

        // wrap delta to [-pi, pi]
        delta = std::fmod(delta, tau);
        if (delta > pi) {
            delta -= tau;
        }
        else if (delta < -pi) {
            delta += tau;
        }

        return NanTransform2D(
            NanPoint::lerp(a.position_, b.position_, t),
            a.rotation_ + delta * t,
            NanPoint::lerp(a.scale_, b.scale_, t)
        );
    }

    // ---------------------------------------------------------------------------
    // comparison
    // ---------------------------------------------------------------------------

    auto NanTransform2D::operator==(const NanTransform2D& other) const -> bool {
        return position_ == other.position_ && rotation_ == other.rotation_
            && scale_ == other.scale_;
    }

    auto NanTransform2D::operator!=(const NanTransform2D& other) const -> bool {
        return !(*this == other);
    }

} // namespace nandina::foundation
