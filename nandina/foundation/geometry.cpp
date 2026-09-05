//
// Created by cvrain on 2026/6/29.
//

#include "geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace nandina::foundation
{

    NanPoint::NanPoint(const float x, const float y): x_axis(x), y_axis(y) {}

    NanPoint::NanPoint(const std::pair<float, float>& pos): x_axis(pos.first), y_axis(pos.second) {}

    auto NanPoint::zero() -> NanPoint {
        return NanPoint(0, 0);
    }

    auto NanPoint::set_x(const float x) -> NanPoint& {
        this->x_axis = x;
        return *this;
    }

    auto NanPoint::set_y(const float y) -> NanPoint& {
        this->y_axis = y;
        return *this;
    }

    auto NanPoint::get_point() const -> std::pair<float, float> {
        return std::make_pair(this->x_axis, this->y_axis);
    }

    auto NanPoint::get_x() const -> float {
        return this->x_axis;
    }

    auto NanPoint::get_y() const -> float {
        return this->y_axis;
    }

    auto NanPoint::add(const NanPoint& point) -> NanPoint& {
        this->x_axis += point.x_axis;
        this->y_axis += point.y_axis;
        return *this;
    }

    auto NanPoint::operator+=(const NanPoint& point) -> NanPoint& {
        return add(point);
    }

    auto NanPoint::operator+(const NanPoint& point) const -> NanPoint {
        return NanPoint(this->x_axis + point.x_axis, this->y_axis + point.y_axis);
    }

    auto NanPoint::sub(const NanPoint& point) -> NanPoint& {
        this->x_axis -= point.x_axis;
        this->y_axis -= point.y_axis;
        return *this;
    }

    auto NanPoint::operator-=(const NanPoint& point) -> NanPoint& {
        return sub(point);
    }

    auto NanPoint::operator-(const NanPoint& point) const -> NanPoint {
        return NanPoint(this->x_axis - point.x_axis, this->y_axis - point.y_axis);
    }

    auto NanPoint::scale(const float factor) -> NanPoint& {
        this->x_axis *= factor;
        this->y_axis *= factor;
        return *this;
    }

    auto NanPoint::operator*=(const float factor) -> NanPoint& {
        return scale(factor);
    }

    auto NanPoint::scale_xy(const float sx, const float sy) -> NanPoint& {
        this->x_axis *= sx;
        this->y_axis *= sy;
        return *this;
    }

    auto NanPoint::operator*(const float factor) const -> NanPoint {
        return NanPoint(this->x_axis * factor, this->y_axis * factor);
    }

    auto NanPoint::negated() const -> NanPoint {
        return NanPoint(-this->x_axis, -this->y_axis);
    }

    auto NanPoint::operator-() const -> NanPoint {
        return negated();
    }

    auto NanPoint::dot(const NanPoint& point) const -> float {
        return this->x_axis * point.x_axis + this->y_axis * point.y_axis;
    }

    auto NanPoint::cross(const NanPoint& point) const -> float {
        return this->x_axis * point.y_axis - this->y_axis * point.x_axis;
    }

    auto NanPoint::length_squared() const -> float {
        return dot(*this);
    }

    auto NanPoint::length() const -> float {
        return std::sqrt(length_squared());
    }

    auto NanPoint::normalized() const -> NanPoint {
        const auto len = length();
        if (len == 0.0F) {
            return zero();
        }
        return NanPoint(this->x_axis / len, this->y_axis / len);
    }

    auto NanPoint::lerp(const NanPoint& a, const NanPoint& b, const float t) -> NanPoint {
        return NanPoint(std::lerp(a.x_axis, b.x_axis, t), std::lerp(a.y_axis, b.y_axis, t));
    }

    auto NanPoint::equals(const NanPoint& point) const -> bool {
        return this->x_axis == point.x_axis && this->y_axis == point.y_axis;
    }

    auto NanPoint::operator==(const NanPoint& point) const -> bool {
        return this->equals(point);
    }

    auto NanPoint::operator!=(const NanPoint& point) const -> bool {
        return !this->equals(point);
    }

    auto NanPoint::distance(const NanPoint& point) const -> float {
        return static_cast<float>(std::sqrt(
            std::pow(this->x_axis - point.x_axis, 2) + std::pow(this->y_axis - point.y_axis, 2)
        ));
    }

    // ===============NanSize==============

    NanSize::NanSize(const float width, const float height): width(width), height(height) {}

    NanSize::NanSize(const std::pair<float, float>& size): width(size.first), height(size.second) {}

    auto NanSize::zero() -> NanSize {
        return NanSize(0, 0);
    }

    auto NanSize::one() -> NanSize {
        return NanSize(1, 1);
    }

    auto NanSize::set_width(const float width) -> NanSize& {
        this->width = width;
        return *this;
    }

    auto NanSize::set_height(const float height) -> NanSize& {
        this->height = height;
        return *this;
    }

    auto NanSize::set_size(const float width, const float height) -> NanSize& {
        this->width = width;
        this->height = height;
        return *this;
    }

    auto NanSize::get_width() const -> float {
        return this->width;
    }

    auto NanSize::get_height() const -> float {
        return this->height;
    }

    auto NanSize::get_size() const -> std::pair<float, float> {
        return std::make_pair(this->width, this->height);
    }

    auto NanSize::aspect_ratio() const -> float {
        if (this->height == 0.0F) {
            return 0.0F;
        }
        return this->width / this->height;
    }

    auto NanSize::area() const -> float {
        return this->width * this->height;
    }

    auto NanSize::is_empty() const -> bool {
        return this->width == 0 || this->height == 0;
    }

    auto NanSize::is_valid() const -> bool {
        return this->width > 0.0F && this->height > 0.0F;
    }

    auto NanSize::fits_in(const NanSize& bounds) const -> bool {
        return this->width <= bounds.width && this->height <= bounds.height;
    }

    auto NanSize::operator+(const NanSize& other) const -> NanSize {
        return NanSize(this->width + other.width, this->height + other.height);
    }

    auto NanSize::operator-(const NanSize& other) const -> NanSize {
        return NanSize(this->width - other.width, this->height - other.height);
    }

    auto NanSize::operator*(const float factor) const -> NanSize {
        return NanSize(this->width * factor, this->height * factor);
    }

    auto NanSize::operator/(const float factor) const -> NanSize {
        return NanSize(this->width / factor, this->height / factor);
    }

    auto NanSize::operator+=(const NanSize& other) -> NanSize& {
        this->width += other.width;
        this->height += other.height;
        return *this;
    }

    auto NanSize::operator-=(const NanSize& other) -> NanSize& {
        this->width -= other.width;
        this->height -= other.height;
        return *this;
    }

    auto NanSize::operator*=(const float factor) -> NanSize& {
        this->width *= factor;
        this->height *= factor;
        return *this;
    }

    auto NanSize::operator/=(const float factor) -> NanSize& {
        this->width /= factor;
        this->height /= factor;
        return *this;
    }

    auto NanSize::scaled(const float factor) -> NanSize& {
        return operator*=(factor);
    }

    auto NanSize::scaled_xy(const float sx, const float sy) -> NanSize& {
        this->width *= sx;
        this->height *= sy;
        return *this;
    }

    auto NanSize::expanded(const float w, const float h) -> NanSize& {
        this->width += w;
        this->height += h;
        return *this;
    }

    auto NanSize::constrain(const NanSize& min, const NanSize& max) -> NanSize& {
        this->width = std::clamp(this->width, min.width, max.width);
        this->height = std::clamp(this->height, min.height, max.height);
        return *this;
    }

    auto NanSize::max(const NanSize& other) const -> NanSize {
        return NanSize(std::max(this->width, other.width), std::max(this->height, other.height));
    }

    auto NanSize::min(const NanSize& other) const -> NanSize {
        return NanSize(std::min(this->width, other.width), std::min(this->height, other.height));
    }

    auto NanSize::equals(const NanSize& size) const -> bool {
        return this->width == size.width && this->height == size.height;
    }

    auto NanSize::operator==(const NanSize& size) const -> bool {
        return this->equals(size);
    }

    auto NanSize::operator!=(const NanSize& size) const -> bool {
        return !this->equals(size);
    }

    NanRect::NanRect(const float left, const float top, const float right, const float bottom):
        left(left),
        top(top),
        right(right),
        bottom(bottom) {}

    NanRect::NanRect(const std::array<float, 4>& rect):
        left(rect.at(0)),
        top(rect.at(1)),
        right(rect.at(2)),
        bottom(rect.at(3)) {}

    NanRect::NanRect(const std::tuple<float, float, float, float>& rect):
        left(std::get<0>(rect)),
        top(std::get<1>(rect)),
        right(std::get<2>(rect)),
        bottom(std::get<3>(rect)) {}

    auto NanRect::empty() -> NanRect {
        return NanRect(0, 0, 0, 0);
    }

    auto NanRect::set_left(const float left) -> NanRect& {
        this->left = left;
        return *this;
    }

    auto NanRect::set_top(const float top) -> NanRect& {
        this->top = top;
        return *this;
    }

    auto NanRect::set_right(const float right) -> NanRect& {
        this->right = right;
        return *this;
    }

    auto NanRect::set_bottom(const float bottom) -> NanRect& {
        this->bottom = bottom;
        return *this;
    }

    auto NanRect::set_rect(const float left, const float top, const float right, const float bottom)
        -> NanRect& {
        this->left = left;
        this->top = top;
        this->right = right;
        this->bottom = bottom;
        return *this;
    }

    auto NanRect::get_left() const -> float {
        return this->left;
    }

    auto NanRect::get_top() const -> float {
        return this->top;
    }

    auto NanRect::get_right() const -> float {
        return this->right;
    }

    auto NanRect::get_bottom() const -> float {
        return this->bottom;
    }

    auto NanRect::get_rect() const -> std::tuple<float, float, float, float> {
        return std::make_tuple(this->left, this->top, this->right, this->bottom);
    }

    auto
    NanRect::from_ltrb(const float left, const float top, const float right, const float bottom)
        -> NanRect {
        return NanRect(left, top, right, bottom);
    }

    auto NanRect::from_xywh(const float x, const float y, const float width, const float height)
        -> NanRect {
        return NanRect(x, y, x + width, y + height);
    }

    auto NanRect::from_origin_size(const NanPoint& origin, const NanSize& size) -> NanRect {
        return from_xywh(origin.get_x(), origin.get_y(), size.get_width(), size.get_height());
    }

    auto NanRect::from_center(const NanPoint& center, const NanSize& size) -> NanRect {
        const auto half_w = size.get_width() / 2.0F;
        const auto half_h = size.get_height() / 2.0F;
        return NanRect(
            center.get_x() - half_w,
            center.get_y() - half_h,
            center.get_x() + half_w,
            center.get_y() + half_h
        );
    }

    auto NanRect::from_points(const NanPoint& p1, const NanPoint& p2) -> NanRect {
        return NanRect(
            std::min(p1.get_x(), p2.get_x()),
            std::min(p1.get_y(), p2.get_y()),
            std::max(p1.get_x(), p2.get_x()),
            std::max(p1.get_y(), p2.get_y())
        );
    }

    auto NanRect::get_x() const -> float {
        return this->left;
    }

    auto NanRect::get_y() const -> float {
        return this->top;
    }

    auto NanRect::get_width() const -> float {
        return this->right - this->left;
    }

    auto NanRect::get_height() const -> float {
        return this->bottom - this->top;
    }

    auto NanRect::get_size() const -> NanSize {
        return NanSize(get_width(), get_height());
    }

    auto NanRect::get_bottom_right() const -> NanPoint {
        return NanPoint(this->right, this->bottom);
    }

    auto NanRect::get_top_right() const -> NanPoint {
        return NanPoint(this->right, this->top);
    }

    auto NanRect::get_bottom_left() const -> NanPoint {
        return NanPoint(this->left, this->bottom);
    }

    auto NanRect::get_top_left() const -> NanPoint {
        return NanPoint(this->left, this->top);
    }

    auto NanRect::get_center() const -> NanPoint {
        return NanPoint((this->left + this->right) / 2.0F, (this->top + this->bottom) / 2.0F);
    }

    auto NanRect::is_empty() const -> bool {
        return this->right <= this->left || this->bottom <= this->top;
    }

    auto NanRect::is_valid() const -> bool {
        return this->right > this->left && this->bottom > this->top;
    }

    auto NanRect::translated(const NanPoint& offset) const -> NanRect {
        return NanRect(
            this->left + offset.get_x(),
            this->top + offset.get_y(),
            this->right + offset.get_x(),
            this->bottom + offset.get_y()
        );
    }

    auto NanRect::translated(const float offset_x, const float offset_y) const -> NanRect {
        return NanRect(
            this->left + offset_x,
            this->top + offset_y,
            this->right + offset_x,
            this->bottom + offset_y
        );
    }

    auto NanRect::expanded(const float amount) const -> NanRect {
        return NanRect(
            this->left - amount,
            this->top - amount,
            this->right + amount,
            this->bottom + amount
        );
    }

    auto NanRect::contains_point(const NanPoint& point) const -> bool {
        return point.get_x() >= this->left && point.get_x() <= this->right
            && point.get_y() >= this->top && point.get_y() <= this->bottom;
    }

    auto NanRect::contains_rect(const NanRect& rect) const -> bool {
        return rect.left >= this->left && rect.top >= this->top && rect.right <= this->right
            && rect.bottom <= this->bottom;
    }

    auto NanRect::intersects(const NanRect& rect) const -> bool {
        return this->left<rect.right&& this->right> rect.left
            && this->top<rect.bottom&& this->bottom> rect.top;
    }

    auto NanRect::intersected(const NanRect& rect) const -> NanRect {
        const auto new_left = std::max(this->left, rect.left);
        const auto new_top = std::max(this->top, rect.top);
        const auto new_right = std::min(this->right, rect.right);
        const auto new_bottom = std::min(this->bottom, rect.bottom);

        if (new_left >= new_right || new_top >= new_bottom) {
            return NanRect(0, 0, 0, 0);
        }

        return NanRect(new_left, new_top, new_right, new_bottom);
    }

    auto NanRect::united(const NanRect& rect) const -> NanRect {
        return NanRect(
            std::min(this->left, rect.left),
            std::min(this->top, rect.top),
            std::max(this->right, rect.right),
            std::max(this->bottom, rect.bottom)
        );
    }

    auto NanRect::centered_in(const NanRect& outer) const -> NanRect {
        const auto current_width = get_width();
        const auto current_height = get_height();
        const auto x = outer.left + (outer.get_width() - current_width) / 2.0F;
        const auto y = outer.top + (outer.get_height() - current_height) / 2.0F;
        return from_xywh(x, y, current_width, current_height);
    }

    auto NanRect::inset_by(const NanInsets& insets) const -> NanRect {
        return insets.apply_to_rect(*this);
    }

    auto NanRect::scaled(const float factor) const -> NanRect {
        return scaled_from(factor, get_center());
    }

    auto NanRect::scaled_from(const float factor, const NanPoint& anchor) const -> NanRect {
        const auto new_width = get_width() * factor;
        const auto new_height = get_height() * factor;
        const auto ratio_x = (anchor.get_x() - this->left) / get_width();
        const auto ratio_y = (anchor.get_y() - this->top) / get_height();
        const auto new_left = anchor.get_x() - new_width * ratio_x;
        const auto new_top = anchor.get_y() - new_height * ratio_y;
        return NanRect(new_left, new_top, new_left + new_width, new_top + new_height);
    }

    auto NanRect::with_origin(const NanPoint& origin) const -> NanRect {
        return from_xywh(origin.get_x(), origin.get_y(), get_width(), get_height());
    }

    auto NanRect::with_origin(const float x, const float y) const -> NanRect {
        return from_xywh(x, y, get_width(), get_height());
    }

    auto NanRect::with_size(const NanSize& size) const -> NanRect {
        return from_xywh(this->left, this->top, size.get_width(), size.get_height());
    }

    auto NanRect::with_width(const float width) const -> NanRect {
        return from_xywh(this->left, this->top, width, get_height());
    }

    auto NanRect::with_height(const float height) const -> NanRect {
        return from_xywh(this->left, this->top, get_width(), height);
    }

    auto NanRect::equals(const NanRect& rect) const -> bool {
        return this->left == rect.left && this->top == rect.top && this->right == rect.right
            && this->bottom == rect.bottom;
    }

    auto NanRect::operator==(const NanRect& rect) const -> bool {
        return this->equals(rect);
    }

    auto NanRect::operator!=(const NanRect& rect) const -> bool {
        return !this->equals(rect);
    }

    // ===============NanInsets==============

    NanInsets::NanInsets(const float left, const float right, const float top, const float bottom):
        left(left),
        right(right),
        top(top),
        bottom(bottom) {}

    auto NanInsets::all(const float value) -> NanInsets {
        return NanInsets(value, value, value, value);
    }

    auto NanInsets::symmetric(const float horizontal, const float vertical) -> NanInsets {
        return NanInsets(horizontal, horizontal, vertical, vertical);
    }

    auto NanInsets::horizontal(const float value) -> NanInsets {
        return NanInsets(value, value, 0, 0);
    }

    auto NanInsets::vertical(const float value) -> NanInsets {
        return NanInsets(0, 0, value, value);
    }

    auto NanInsets::only_left(const float value) -> NanInsets {
        return NanInsets(value, 0, 0, 0);
    }

    auto NanInsets::only_right(const float value) -> NanInsets {
        return NanInsets(0, value, 0, 0);
    }

    auto NanInsets::only_top(const float value) -> NanInsets {
        return NanInsets(0, 0, value, 0);
    }

    auto NanInsets::only_bottom(const float value) -> NanInsets {
        return NanInsets(0, 0, 0, value);
    }

    auto NanInsets::set_left(const float left) -> NanInsets& {
        this->left = left;
        return *this;
    }

    auto NanInsets::set_right(const float right) -> NanInsets& {
        this->right = right;
        return *this;
    }

    auto NanInsets::set_top(const float top) -> NanInsets& {
        this->top = top;
        return *this;
    }

    auto NanInsets::set_bottom(const float bottom) -> NanInsets& {
        this->bottom = bottom;
        return *this;
    }

    auto NanInsets::get_left() const -> float {
        return this->left;
    }

    auto NanInsets::get_right() const -> float {
        return this->right;
    }

    auto NanInsets::get_top() const -> float {
        return this->top;
    }

    auto NanInsets::get_bottom() const -> float {
        return this->bottom;
    }

    auto NanInsets::get_insets() const -> std::tuple<float, float, float, float> {
        return std::make_tuple(this->left, this->right, this->top, this->bottom);
    }

    auto NanInsets::is_zero() const -> bool {
        return this->left == 0 && this->right == 0 && this->top == 0 && this->bottom == 0;
    }

    auto NanInsets::horizontal_sum() const -> float {
        return this->left + this->right;
    }

    auto NanInsets::vertical_sum() const -> float {
        return this->top + this->bottom;
    }

    auto NanInsets::apply_to_rect(const NanRect& rect) const -> NanRect {
        return NanRect(
            rect.get_left() + this->left,
            rect.get_top() + this->top,
            rect.get_right() - this->right,
            rect.get_bottom() - this->bottom
        );
    }

    auto NanInsets::inflate_rect(const NanRect& rect) const -> NanRect {
        return NanRect(
            rect.get_left() - this->left,
            rect.get_top() - this->top,
            rect.get_right() + this->right,
            rect.get_bottom() + this->bottom
        );
    }

    auto NanInsets::add(const NanInsets& other) const -> NanInsets {
        return NanInsets(
            this->left + other.left,
            this->right + other.right,
            this->top + other.top,
            this->bottom + other.bottom
        );
    }

    auto NanInsets::operator+(const NanInsets& other) const -> NanInsets {
        return this->add(other);
    }

    auto NanInsets::operator+=(const NanInsets& other) -> NanInsets& {
        this->left += other.left;
        this->right += other.right;
        this->top += other.top;
        this->bottom += other.bottom;
        return *this;
    }

    auto NanInsets::operator*(const float factor) const -> NanInsets {
        return NanInsets(
            this->left * factor,
            this->right * factor,
            this->top * factor,
            this->bottom * factor
        );
    }

    auto NanInsets::operator*=(const float factor) -> NanInsets& {
        this->left *= factor;
        this->right *= factor;
        this->top *= factor;
        this->bottom *= factor;
        return *this;
    }

    auto NanInsets::max(const NanInsets& other) const -> NanInsets {
        return NanInsets(
            std::max(this->left, other.left),
            std::max(this->right, other.right),
            std::max(this->top, other.top),
            std::max(this->bottom, other.bottom)
        );
    }

    auto NanInsets::min(const NanInsets& other) const -> NanInsets {
        return NanInsets(
            std::min(this->left, other.left),
            std::min(this->right, other.right),
            std::min(this->top, other.top),
            std::min(this->bottom, other.bottom)
        );
    }

    auto NanInsets::lerp(const NanInsets& a, const NanInsets& b, const float t) -> NanInsets {
        return NanInsets(
            std::lerp(a.left, b.left, t),
            std::lerp(a.right, b.right, t),
            std::lerp(a.top, b.top, t),
            std::lerp(a.bottom, b.bottom, t)
        );
    }

    auto NanInsets::equals(const NanInsets& other) const -> bool {
        return this->left == other.left && this->right == other.right && this->top == other.top
            && this->bottom == other.bottom;
    }

    auto NanInsets::operator==(const NanInsets& other) const -> bool {
        return this->equals(other);
    }

    auto NanInsets::operator!=(const NanInsets& other) const -> bool {
        return !this->equals(other);
    }
} // namespace nandina::foundation