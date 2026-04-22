//
// Created by cvrain on 2026/4/22.
//
module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fmt/core.h>
#include <functional>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>

export module nandina.foundation.nan_rect;

export import nandina.foundation.nan_point;
export import nandina.foundation.nan_size;

export namespace nandina::geometry {
    // ──────────────────────────────────────────────────────────
    // 对齐枚举
    // ──────────────────────────────────────────────────────────

    enum class Alignment {
        TopLeft, TopCenter, TopRight,
        CenterLeft, Center, CenterRight,
        BottomLeft, BottomCenter, BottomRight
    };

    // ──────────────────────────────────────────────────────────
    // BaseRect 模板类
    // ──────────────────────────────────────────────────────────

    /**
     * 通用二维矩形模板类，内部采用 LTRB（边界坐标）存储。
     * 提供丰富的几何运算和布局辅助功能。
     */
    template<SatisfiedPoint T>
    class BaseRect {
    public:
        using value_type = T;

        // 默认构造，矩形退化为空
        constexpr BaseRect() noexcept
            : bounds_{} {
        }

        // 通过边界坐标构造
        constexpr BaseRect(const T left, const T top, const T right, const T bottom) noexcept
            : bounds_{left, top, right, bottom} {
        }

        // ── 静态工厂方法 ──

        /// 从 LTRB 创建
        [[nodiscard]] static constexpr auto fromLTRB(const T l, const T t, const T r, const T b) noexcept -> BaseRect {
            return BaseRect{l, t, r, b};
        }

        /// 从 XYWH 创建
        [[nodiscard]] static constexpr auto fromXYWH(const T x, const T y, const T w, const T h) noexcept -> BaseRect {
            return BaseRect{x, y, x + w, y + h};
        }

        // ── 边界访问 ──

        [[nodiscard]] constexpr auto left() const noexcept -> T { return bounds_[0]; }
        [[nodiscard]] constexpr auto top() const noexcept -> T { return bounds_[1]; }
        [[nodiscard]] constexpr auto right() const noexcept -> T { return bounds_[2]; }
        [[nodiscard]] constexpr auto bottom() const noexcept -> T { return bounds_[3]; }

        [[nodiscard]] constexpr auto x() const noexcept -> T { return bounds_[0]; }
        [[nodiscard]] constexpr auto y() const noexcept -> T { return bounds_[1]; }

        [[nodiscard]] constexpr auto width() const noexcept -> T {
            return bounds_[2] >= bounds_[0] ? bounds_[2] - bounds_[0] : T{};
        }

        [[nodiscard]] constexpr auto height() const noexcept -> T {
            return bounds_[3] >= bounds_[1] ? bounds_[3] - bounds_[1] : T{};
        }

        // ── 设置边界 ──

        constexpr auto setLeft(const T value) noexcept -> void { bounds_[0] = value; }
        constexpr auto setTop(const T value) noexcept -> void { bounds_[1] = value; }
        constexpr auto setRight(const T value) noexcept -> void { bounds_[2] = value; }
        constexpr auto setBottom(const T value) noexcept -> void { bounds_[3] = value; }
        constexpr auto setX(const T value) noexcept -> void { setLeft(value); }
        constexpr auto setY(const T value) noexcept -> void { setTop(value); }

        constexpr auto setWidth(const T w) noexcept -> void {
            if (w >= T{}) bounds_[2] = bounds_[0] + w;
        }

        constexpr auto setHeight(const T h) noexcept -> void {
            if (h >= T{}) bounds_[3] = bounds_[1] + h;
        }

        constexpr auto setRect(const T l, const T t, const T r, const T b) noexcept -> void {
            bounds_[0] = l;
            bounds_[1] = t;
            bounds_[2] = r;
            bounds_[3] = b;
        }

        constexpr auto setEmpty() noexcept -> void {
            bounds_[0] = bounds_[1] = bounds_[2] = bounds_[3] = T{};
        }

        // ── 属性方法 ──

        [[nodiscard]] auto topLeft() const noexcept -> NanPoint {
            return NanPoint{left(), top()};
        }

        [[nodiscard]] auto bottomRight() const noexcept -> NanPoint {
            return NanPoint{right(), bottom()};
        }

        [[nodiscard]] auto center() const noexcept -> NanPoint {
            return NanPoint{(left() + right()) / 2.0f, (top() + bottom()) / 2.0f};
        }

        [[nodiscard]] auto size() const noexcept -> NanSize {
            return NanSize{width(), height()};
        }

        [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool {
            return width() <= T{} || height() <= T{};
        }

        [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
            return bounds_[0] <= bounds_[2] && bounds_[1] <= bounds_[3];
        }

        // ── 修改器 ──

        constexpr auto setTopLeft(const NanPoint &pt) noexcept -> void {
            bounds_[0] = pt.x();
            bounds_[1] = pt.y();
        }

        constexpr auto setBottomRight(const NanPoint &pt) noexcept -> void {
            bounds_[2] = pt.x();
            bounds_[3] = pt.y();
        }

        // ── 比较运算符 ──

        [[nodiscard]] constexpr auto operator==(const BaseRect &rhs) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto operator!=(const BaseRect &rhs) const noexcept -> bool = default;

        // ── 辅助函数 ──

        [[nodiscard]] auto toString() const -> std::string {
            std::ostringstream oss;
            oss << "(L=" << left() << ", T=" << top() << ", R=" << right() << ", B=" << bottom() << ")";
            return oss.str();
        }

        constexpr auto swap(BaseRect &other) noexcept -> void {
            bounds_.swap(other.bounds_);
        }

    private:
        std::array<T, 4> bounds_{}; // [left, top, right, bottom]
    };

    // ──────────────────────────────────────────────────────────
    // NanRect - 特化类型
    // ──────────────────────────────────────────────────────────

    class NanRect final : public BaseRect<float> {
    public:
        using BaseRect::BaseRect;

        // 默认构造
        constexpr NanRect() noexcept : BaseRect{0.0f, 0.0f, 0.0f, 0.0f} {
        }

        // 边界构造
        constexpr NanRect(const float l, const float t, const float r, const float b) noexcept
            : BaseRect{l, t, r, b} {
        }

        // 从两个点构造
        constexpr NanRect(const NanPoint &topLeft, const NanPoint &bottomRight) noexcept
            : BaseRect{topLeft.x(), topLeft.y(), bottomRight.x(), bottomRight.y()} {
        }

        // 从点和尺寸构造
        constexpr NanRect(const NanPoint &topLeft, const NanSize &sz) noexcept
            : BaseRect{topLeft.x(), topLeft.y(), topLeft.x() + sz.width(), topLeft.y() + sz.height()} {
        }

        // ── 几何运算（变换） ──

        [[nodiscard]] constexpr auto translated(const float dx, const float dy) const noexcept -> NanRect {
            return NanRect{left() + dx, top() + dy, right() + dx, bottom() + dy};
        }

        constexpr auto translate(const float dx, const float dy) noexcept -> NanRect& {
            setRect(left() + dx, top() + dy, right() + dx, bottom() + dy);
            return *this;
        }

        [[nodiscard]] constexpr auto translated(const NanPoint &offset) const noexcept -> NanRect {
            return translated(offset.x(), offset.y());
        }

        constexpr auto translate(const NanPoint &offset) noexcept -> NanRect& {
            return translate(offset.x(), offset.y());
        }

        [[nodiscard]] constexpr auto scaled(const float sx, const float sy) const noexcept -> NanRect {
            return NanRect{left() * sx, top() * sy, right() * sx, bottom() * sy};
        }

        constexpr auto scale(const float sx, const float sy) noexcept -> NanRect& {
            setRect(left() * sx, top() * sy, right() * sx, bottom() * sy);
            return *this;
        }

        [[nodiscard]] constexpr auto expanded(const float amount) const noexcept -> NanRect {
            return NanRect{left() - amount, top() - amount, right() + amount, bottom() + amount};
        }

        constexpr auto expand(const float amount) noexcept -> NanRect& {
            setRect(left() - amount, top() - amount, right() + amount, bottom() + amount);
            return *this;
        }

        [[nodiscard]] constexpr auto shrinked(const float amount) const noexcept -> NanRect {
            return expanded(-amount);
        }

        constexpr auto shrink(const float amount) noexcept -> NanRect& {
            return expand(-amount);
        }

        // ── 几何关系判断 ──

        [[nodiscard]] auto contains(const NanPoint &pt) const noexcept -> bool {
            return pt.x() >= left() && pt.x() <= right() && pt.y() >= top() && pt.y() <= bottom();
        }

        [[nodiscard]] auto contains(const NanRect &other) const noexcept -> bool {
            return other.left() >= left() && other.right() <= right() &&
                   other.top() >= top() && other.bottom() <= bottom();
        }

        [[nodiscard]] auto intersects(const NanRect &other) const noexcept -> bool {
            return left() < other.right() && right() > other.left() &&
                   top() < other.bottom() && bottom() > other.top();
        }

        [[nodiscard]] auto overlaps(const NanRect &other) const noexcept -> bool {
            return intersects(other);
        }

        [[nodiscard]] auto intersected(const NanRect &other) const noexcept -> NanRect {
            if (!intersects(other)) return NanRect{};
            return NanRect{
                std::max(left(), other.left()), std::max(top(), other.top()),
                std::min(right(), other.right()), std::min(bottom(), other.bottom())
            };
        }

        [[nodiscard]] auto united(const NanRect &other) const noexcept -> NanRect {
            return NanRect{
                std::min(left(), other.left()), std::min(top(), other.top()),
                std::max(right(), other.right()), std::max(bottom(), other.bottom())
            };
        }

        // ── 布局辅助 ──

        [[nodiscard]] auto alignedInside(const NanRect &container, const Alignment align) const noexcept -> NanRect {
            const auto w = width();
            const auto h = height();
            auto result = *this;

            switch (align) {
                case Alignment::TopLeft:
                case Alignment::CenterLeft:
                case Alignment::BottomLeft:
                    result.setX(container.left());
                    break;
                case Alignment::TopCenter:
                case Alignment::Center:
                case Alignment::BottomCenter:
                    result.setX((container.left() + container.right()) / 2.0f - w / 2.0f);
                    break;
                case Alignment::TopRight:
                case Alignment::CenterRight:
                case Alignment::BottomRight:
                    result.setX(container.right() - w);
                    break;
            }

            switch (align) {
                case Alignment::TopLeft:
                case Alignment::TopCenter:
                case Alignment::TopRight:
                    result.setY(container.top());
                    break;
                case Alignment::CenterLeft:
                case Alignment::Center:
                case Alignment::CenterRight:
                    result.setY((container.top() + container.bottom()) / 2.0f - h / 2.0f);
                    break;
                case Alignment::BottomLeft:
                case Alignment::BottomCenter:
                case Alignment::BottomRight:
                    result.setY(container.bottom() - h);
                    break;
            }

            return result;
        }

        [[nodiscard]] constexpr auto withMargin(const float margin) const noexcept -> NanRect {
            return shrinked(margin * 2.0f);
        }

        [[nodiscard]] constexpr auto withPadding(const float padding) const noexcept -> NanRect {
            return shrinked(padding * 2.0f);
        }

        [[nodiscard]] constexpr auto inset(const float dx, const float dy) const noexcept -> NanRect {
            return NanRect{left() + dx, top() + dy, right() - dx, bottom() - dy};
        }

        [[nodiscard]] auto centeredIn(const NanRect &outer) const noexcept -> NanRect {
            const auto w = width();
            const auto h = height();
            return NanRect{
                (outer.left() + outer.right()) / 2.0f - w / 2.0f,
                (outer.top() + outer.bottom()) / 2.0f - h / 2.0f,
                (outer.left() + outer.right()) / 2.0f + w / 2.0f,
                (outer.top() + outer.bottom()) / 2.0f + h / 2.0f
            };
        }

        [[nodiscard]] auto boundedTo(const NanRect &boundary) const noexcept -> NanRect {
            return intersected(boundary);
        }
    };

    // ──────────────────────────────────────────────────────────
    // 非成员运算符
    // ──────────────────────────────────────────────────────────

    [[nodiscard]] constexpr auto operator+(const NanRect &rect, const NanPoint &offset) noexcept -> NanRect {
        return rect.translated(offset);
    }

    [[nodiscard]] constexpr auto operator-(const NanRect &rect, const NanPoint &offset) noexcept -> NanRect {
        return rect.translated(-offset.x(), -offset.y());
    }

    [[nodiscard]] constexpr auto operator&(const NanRect &lhs, const NanRect &rhs) noexcept -> NanRect {
        return lhs.intersected(rhs);
    }

    [[nodiscard]] constexpr auto operator|(const NanRect &lhs, const NanRect &rhs) noexcept -> NanRect {
        return lhs.united(rhs);
    }

    [[nodiscard]] auto operator<<(std::ostream &os, const NanRect &rect) -> std::ostream& {
        return os << rect.toString();
    }
} // namespace nandina::geometry

// ──────────────────────────────────────────────────────────
// fmt::formatter 特化（放在全局命名空间）
// ──────────────────────────────────────────────────────────

using namespace nandina::geometry;

template<SatisfiedPoint T>
struct fmt::formatter<BaseRect<T>> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const BaseRect<T> &rect, FormatContext &ctx) -> FormatContext::iterator {
        return fmt::format_to(ctx.out(), "{}", rect.toString());
    }
};

template<>
struct fmt::formatter<NanRect> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const NanRect &rect, FormatContext &ctx) -> FormatContext::iterator {
        return fmt::format_to(ctx.out(), "{}", rect.toString());
    }
};

// ──────────────────────────────────────────────────────────
// std::hash 特化
// ──────────────────────────────────────────────────────────

namespace nandina::geometry {
    template<SatisfiedPoint T>
    struct BaseRectHash {
        [[nodiscard]] auto operator()(const BaseRect<T> &rect) const noexcept -> std::size_t {
            std::size_t seed = 0;
            seed ^= std::hash<T>{}(rect.left()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<T>{}(rect.top()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<T>{}(rect.right()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<T>{}(rect.bottom()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}