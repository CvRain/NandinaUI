//
// Created by cvrain on 2026/4/21.
//
module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <functional>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

export module nandina.foundation.nan_point;

export namespace nandina::geometry {
    // ──────────────────────────────────────────────────────────
    // 概念定义
    // ──────────────────────────────────────────────────────────

    /// 适用于表示坐标点的单位，只能是整型和浮点型
    template<typename T>
    concept SatisfiedPoint = std::is_integral_v<T> || std::is_floating_point_v<T>;

    /// 坐标索引越界错误
    enum class PointError {
        index_out_of_range,
    };

    // ──────────────────────────────────────────────────────────
    // BasePoint 模板类
    // ──────────────────────────────────────────────────────────

    /**
     * 通用 N 维点模板类，存储于 std::array<T, N>。
     * 提供维度无关的坐标访问与操作。
     */
    template<SatisfiedPoint T, std::size_t N>
    class BasePoint {
    public:
        using value_type = T;

        // 默认构造，坐标置零
        constexpr BasePoint() noexcept = default;

        // 通过初始化列表构造
        constexpr explicit BasePoint(std::array<T, N> coords) noexcept
            : coords_(coords) {
        }

        // 拷贝/移动构造（默认生成）
        constexpr BasePoint(const BasePoint &) noexcept = default;

        constexpr BasePoint(BasePoint &&) noexcept = default;

        // 拷贝/移动赋值（默认生成）
        constexpr auto operator=(const BasePoint &) noexcept -> BasePoint& = default;

        constexpr auto operator=(BasePoint &&) noexcept -> BasePoint& = default;

        // ── 坐标访问 ──

        /// 下标访问（非 const，用于修改）
        [[nodiscard]] constexpr auto operator[](std::size_t index) -> T& {
            return coords_[index];
        }

        /// 下标访问（const 版本）
        [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const T& {
            return coords_[index];
        }

        /// 安全获取坐标值（越界返回 nullopt）
        [[nodiscard]] constexpr auto get(std::size_t index) const -> std::optional<T> {
            if (index >= N) return std::nullopt;
            return coords_[index];
        }

        /// 安全设置坐标值（越界抛出异常）
        constexpr auto set(std::size_t index, T value) -> void {
            if (index >= N) {
                throw std::out_of_range{"Point index out of range"};
            }
            coords_[index] = value;
        }

        // ── 比较运算符 ──

        [[nodiscard]] constexpr auto operator==(const BasePoint &rhs) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto operator!=(const BasePoint &rhs) const noexcept -> bool {
            return !(*this == rhs);
        }

        /// 字典序比较（便于用作 std::map 键）
        [[nodiscard]] constexpr auto operator<(const BasePoint &rhs) const noexcept -> bool {
            return std::lexicographical_compare(coords_.begin(), coords_.end(), rhs.coords_.begin(), rhs.coords_.end());
        }

        [[nodiscard]] constexpr auto operator<=(const BasePoint &rhs) const noexcept -> bool {
            return !(rhs < *this);
        }

        [[nodiscard]] constexpr auto operator>(const BasePoint &rhs) const noexcept -> bool {
            return rhs < *this;
        }

        [[nodiscard]] constexpr auto operator>=(const BasePoint &rhs) const noexcept -> bool {
            return !(*this < rhs);
        }

        // ── 辅助函数 ──

        /// 与另一个点交换内容
        constexpr auto swap(BasePoint &other) noexcept -> void {
            coords_.swap(other.coords_);
        }

        /// 返回形如 "(x, y, ...)" 的字符串
        [[nodiscard]] auto toString() const -> std::string {
            std::ostringstream oss;
            oss << "(";
            for (std::size_t i = 0; i < N; ++i) {
                if (i > 0) oss << ", ";
                oss << coords_[i];
            }
            oss << ")";
            return oss.str();
        }

        /// 获取底层数组的引用
        [[nodiscard]] constexpr auto data() noexcept -> std::array<T, N>& {
            return coords_;
        }

        [[nodiscard]] constexpr auto data() const noexcept -> const std::array<T, N>& {
            return coords_;
        }

    private:
        std::array<T, N> coords_{};
    };

    // ──────────────────────────────────────────────────────────
    // NanPoint - 二维点特化，使用 float 坐标
    // ──────────────────────────────────────────────────────────

    /**
     * Nandina 库中默认使用表示"点"概念的类型。
     * 使用 float 坐标值，适用于窗口像素坐标、UI布局和几何计算。
     *
     * 优势：
     * - 避免 uint8_t 溢出的处理复杂性和精度损失
     * - 支持负坐标和连续空间变换
     * - 与图形API和动画系统无缝集成
     */
    class NanPoint final : public BasePoint<float, 2> {
    public:
        using BasePoint::BasePoint; // 继承基类构造函数

        // 默认构造，坐标置零
        constexpr NanPoint() noexcept : BasePoint{{0.0f, 0.0f}} {
        }

        // 通过两个坐标值构造
        constexpr NanPoint(const float x, const float y) noexcept
            : BasePoint{{x, y}} {
        }

        // 从基类转换构造
        constexpr explicit NanPoint(const BasePoint<float, 2> &other) noexcept
            : BasePoint(other) {
        }

        // 从其他数值类型的二维点显式转换
        template<SatisfiedPoint U>
        explicit constexpr NanPoint(const BasePoint<U, 2> &other) noexcept
            : BasePoint{{static_cast<float>(other[0]), static_cast<float>(other[1])}} {
        }

        // ── 便捷坐标访问 ──

        /// 返回 x 坐标
        [[nodiscard]] constexpr auto x() const noexcept -> float {
            return (*this)[0];
        }

        /// 返回 y 坐标
        [[nodiscard]] constexpr auto y() const noexcept -> float {
            return (*this)[1];
        }

        /// 设置 x 坐标
        constexpr auto setX(const float value) noexcept -> void {
            (*this)[0] = value;
        }

        /// 设置 y 坐标
        constexpr auto setY(const float value) noexcept -> void {
            (*this)[1] = value;
        }

        /// 同时设置两个坐标
        constexpr auto set(const float x, const float y) noexcept -> void {
            (*this)[0] = x;
            (*this)[1] = y;
        }

        // ── 算术运算符 ──

        /// 向量加法
        [[nodiscard]] constexpr auto operator+(const NanPoint &rhs) const noexcept -> NanPoint {
            return NanPoint{(*this)[0] + rhs[0], (*this)[1] + rhs[1]};
        }

        /// 向量减法
        [[nodiscard]] constexpr auto operator-(const NanPoint &rhs) const noexcept -> NanPoint {
            return NanPoint{(*this)[0] - rhs[0], (*this)[1] - rhs[1]};
        }

        /// 标量乘法
        [[nodiscard]] constexpr auto operator*(const float scalar) const noexcept -> NanPoint {
            return NanPoint{(*this)[0] * scalar, (*this)[1] * scalar};
        }

        /// 标量除法
        [[nodiscard]] constexpr auto operator/(const float scalar) const -> NanPoint {
            if (scalar == 0.0f) {
                throw std::domain_error{"Division by zero"};
            }
            return NanPoint{(*this)[0] / scalar, (*this)[1] / scalar};
        }

        /// 一元负号
        [[nodiscard]] constexpr auto operator-() const noexcept -> NanPoint {
            return NanPoint{-(*this)[0], -(*this)[1]};
        }

        // ── 复合赋值运算符 ──

        constexpr auto operator+=(const NanPoint &rhs) noexcept -> NanPoint& {
            *this = *this + rhs;
            return *this;
        }

        constexpr auto operator-=(const NanPoint &rhs) noexcept -> NanPoint& {
            *this = *this - rhs;
            return *this;
        }

        constexpr auto operator*=(const float scalar) noexcept -> NanPoint& {
            *this = *this * scalar;
            return *this;
        }

        constexpr auto operator/=(const float scalar) -> NanPoint& {
            *this = *this / scalar;
            return *this;
        }

        // ── 几何运算 ──

        /// 点积（内积）
        [[nodiscard]] constexpr auto dot(const NanPoint &rhs) const noexcept -> float {
            return (*this)[0] * rhs[0] + (*this)[1] * rhs[1];
        }

        /// 二维叉积（标量值）
        [[nodiscard]] constexpr auto cross(const NanPoint &rhs) const noexcept -> float {
            return (*this)[0] * rhs[1] - (*this)[1] * rhs[0];
        }

        /// 向量长度（模）
        [[nodiscard]] auto length() const noexcept -> float {
            return std::hypot((*this)[0], (*this)[1]);
        }

        /// 长度平方
        [[nodiscard]] constexpr auto lengthSquared() const noexcept -> float {
            return (*this)[0] * (*this)[0] + (*this)[1] * (*this)[1];
        }

        /// 两点欧氏距离
        [[nodiscard]] auto distanceTo(const NanPoint &other) const noexcept -> float {
            const auto dx = (*this)[0] - other[0];
            const auto dy = (*this)[1] - other[1];
            return std::hypot(dx, dy);
        }

        /// 距离平方
        [[nodiscard]] constexpr auto distanceSquaredTo(const NanPoint &other) const noexcept -> float {
            const auto dx = (*this)[0] - other[0];
            const auto dy = (*this)[1] - other[1];
            return dx * dx + dy * dy;
        }

        /// 返回单位向量（若零向量则抛异常）
        [[nodiscard]] auto normalized() const -> NanPoint {
            const auto len = length();
            if (len == 0.0f) {
                throw std::domain_error{"Cannot normalize zero vector"};
            }
            return NanPoint{(*this)[0] / len, (*this)[1] / len};
        }

        /// 原地归一化
        auto normalize() -> NanPoint& {
            *this = normalized();
            return *this;
        }

        // ── 类型转换 ──

        /// 转换为另一种数值类型的点
        template<SatisfiedPoint U>
        [[nodiscard]] auto cast() const noexcept -> BasePoint<U, 2> {
            return BasePoint<U, 2>{{static_cast<U>((*this)[0]), static_cast<U>((*this)[1])}};
        }

        /// 显式转换运算符
        template<SatisfiedPoint U>
        explicit constexpr operator BasePoint<U, 2>() const noexcept {
            return cast<U>();
        }

        // ── 其他工具函数 ──

        /// 检查是否为零向量
        [[nodiscard]] constexpr auto isZero() const noexcept -> bool {
            return (*this)[0] == 0.0f && (*this)[1] == 0.0f;
        }
    };

    // ──────────────────────────────────────────────────────────
    // 非成员运算符与工具函数
    // ──────────────────────────────────────────────────────────

    /// 标量左乘
    [[nodiscard]] constexpr auto operator*(const float scalar, const NanPoint &pt) noexcept -> NanPoint {
        return pt * scalar;
    }

    /// 流输出
    [[nodiscard]] auto operator<<(std::ostream &os, const NanPoint &pt) -> std::ostream& {
        os << pt.toString();
        return os;
    }

    // ──────────────────────────────────────────────────────────
    // std::hash 特化
    // ──────────────────────────────────────────────────────────

    template<SatisfiedPoint T, std::size_t N>
    struct BasePointHash {
        [[nodiscard]] auto operator()(const BasePoint<T, N> &pt) const noexcept -> std::size_t {
            std::size_t seed = 0;
            for (std::size_t i = 0; i < N; ++i) {
                seed ^= std::hash<T>{}(pt[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
} // namespace nandina::geometry

// ──────────────────────────────────────────────────────────
// fmt::formatter 特化（放在全局命名空间）
// ──────────────────────────────────────────────────────────

using namespace nandina::geometry;

template<SatisfiedPoint T, std::size_t N>
struct fmt::formatter<BasePoint<T, N>, void> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const BasePoint<T, N> &pt, FormatContext &ctx) -> FormatContext::iterator {
        std::ostringstream oss;
        oss << "(";
        for (std::size_t i = 0; i < N; ++i) {
            if (i > 0) oss << ", ";
            oss << pt[i];
        }
        oss << ")";
        return fmt::format_to(ctx.out(), "{}", oss.str());
    }
};

template<>
struct fmt::formatter<NanPoint> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const NanPoint &pt, FormatContext &ctx) -> FormatContext::iterator {
        return fmt::format_to(ctx.out(), "{}", pt.toString());
    }
};

// ──────────────────────────────────────────────────────────
// std::tuple_size / std::tuple_element 特化
// ──────────────────────────────────────────────────────────

namespace std {
    template<>
    struct tuple_size<NanPoint> : integral_constant<size_t, 2> {
    };

    template<size_t Index>
    struct tuple_element<Index, NanPoint> {
        using type = float;
    };
}

namespace nandina::geometry {
    template<size_t Index>
    [[nodiscard]] constexpr auto get(const NanPoint &pt) noexcept -> float {
        static_assert(Index < 2);
        return pt[Index];
    }
}