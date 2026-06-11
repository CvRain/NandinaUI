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
#include <stdexcept>
#include <string>
#include <type_traits>

export module nandina.foundation.nan_size;

export import nandina.foundation.nan_point;

export namespace nandina::geometry {
    // ──────────────────────────────────────────────────────────
    // NanSize - 二维尺寸类
    // ──────────────────────────────────────────────────────────

    /**
     * 表示二维尺寸（宽度和高度）。
     * 与 NanPoint 形成完整的几何体系：
     * - NanPoint：位置
     * - NanSize：尺寸
     * - NanRect：位置 + 尺寸（或边界）
     */
    class NanSize final : public BasePoint<float, 2> {
    public:
        using BasePoint::BasePoint; // 继承基类构造函数

        // 默认构造，尺寸为零
        constexpr NanSize() noexcept : BasePoint{{0.0f, 0.0f}} {
        }

        // 通过宽高构造
        constexpr NanSize(const float w, const float h) noexcept
            : BasePoint{{w, h}} {
        }

        // 从基类转换构造
        constexpr explicit NanSize(const BasePoint<float, 2> &other) noexcept
            : BasePoint(other) {
        }

        // 从其他数值类型的二维点显式转换
        template<SatisfiedPoint U>
        explicit constexpr NanSize(const BasePoint<U, 2> &other) noexcept
            : BasePoint{{static_cast<float>(other[0]), static_cast<float>(other[1])}} {
        }

        // ── 便捷属性访问 ──

        /// 返回宽度
        [[nodiscard]] constexpr auto width() const noexcept -> float {
            return (*this)[0];
        }

        /// 返回高度
        [[nodiscard]] constexpr auto height() const noexcept -> float {
            return (*this)[1];
        }

        /// 设置宽度
        constexpr auto set_width(const float w) noexcept -> void {
            (*this)[0] = w;
        }

        /// 设置高度
        constexpr auto set_height(const float h) noexcept -> void {
            (*this)[1] = h;
        }

        /// 同时设置宽高
        constexpr auto set(const float w, const float h) noexcept -> void {
            (*this)[0] = w;
            (*this)[1] = h;
        }

        // ── 算术运算符 ──

        /// 加法
        [[nodiscard]] constexpr auto operator+(const NanSize &other) const noexcept -> NanSize {
            return NanSize{(*this)[0] + other[0], (*this)[1] + other[1]};
        }

        /// 减法
        [[nodiscard]] constexpr auto operator-(const NanSize &other) const noexcept -> NanSize {
            return NanSize{(*this)[0] - other[0], (*this)[1] - other[1]};
        }

        /// 标量乘法
        [[nodiscard]] constexpr auto operator*(const float scalar) const noexcept -> NanSize {
            return NanSize{(*this)[0] * scalar, (*this)[1] * scalar};
        }

        /// 标量除法
        [[nodiscard]] constexpr auto operator/(const float scalar) const -> NanSize {
            if (scalar == 0.0f) {
                throw std::domain_error{"Division by zero"};
            }
            return NanSize{(*this)[0] / scalar, (*this)[1] / scalar};
        }

        // ── 复合赋值运算符 ──

        constexpr auto operator+=(const NanSize &other) noexcept -> NanSize& {
            *this = *this + other;
            return *this;
        }

        constexpr auto operator-=(const NanSize &other) noexcept -> NanSize& {
            *this = *this - other;
            return *this;
        }

        constexpr auto operator*=(const float scalar) noexcept -> NanSize& {
            *this = *this * scalar;
            return *this;
        }

        constexpr auto operator/=(const float scalar) -> NanSize& {
            *this = *this / scalar;
            return *this;
        }

        // ── 几何运算 ──

        /// 面积
        [[nodiscard]] constexpr auto area() const noexcept -> float {
            return (*this)[0] * (*this)[1];
        }

        /// 宽高比
        [[nodiscard]] auto aspect_ratio() const noexcept -> float {
            if ((*this)[1] == 0.0f) {
                return 0.0f;
            }
            return (*this)[0] / (*this)[1];
        }

        /// 是否为空
        [[nodiscard]] constexpr auto is_empty() const noexcept -> bool {
            return (*this)[0] <= 0.0f || (*this)[1] <= 0.0f;
        }

        /// 是否为正尺寸
        [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
            return (*this)[0] >= 0.0f && (*this)[1] >= 0.0f;
        }

        // ── 类型转换 ──

        /// 转换为另一种数值类型
        template<SatisfiedPoint U>
        [[nodiscard]] auto cast() const noexcept -> BasePoint<U, 2> {
            return BasePoint<U, 2>{{static_cast<U>((*this)[0]), static_cast<U>((*this)[1])}};
        }

        /// 显式转换运算符
        template<SatisfiedPoint U>
        explicit constexpr operator BasePoint<U, 2>() const noexcept {
            return cast<U>();
        }

        // ── 转换为 NanPoint ──

        /// 隐式转换为 NanPoint
        constexpr operator NanPoint() const noexcept {
            return NanPoint{(*this)[0], (*this)[1]};
        }
    };

    // ──────────────────────────────────────────────────────────
    // 非成员运算符
    // ──────────────────────────────────────────────────────────

    /// 标量左乘
    [[nodiscard]] constexpr auto operator*(const float scalar, const NanSize &sz) noexcept -> NanSize {
        return sz * scalar;
    }

    /// 流输出
    [[nodiscard]] auto operator<<(std::ostream &os, const NanSize &sz) -> std::ostream& {
        os << sz.to_string();
        return os;
    }
} // namespace nandina::geometry

// ──────────────────────────────────────────────────────────
// fmt::formatter 特化（放在全局命名空间）
// ──────────────────────────────────────────────────────────

using namespace nandina::geometry;

template<>
struct fmt::formatter<NanSize> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const NanSize &sz, FormatContext &ctx) -> FormatContext::iterator {
        return fmt::format_to(ctx.out(), "{}", sz.to_string());
    }
};

// ──────────────────────────────────────────────────────────
// std::tuple_size / std::tuple_element 特化
// ──────────────────────────────────────────────────────────

namespace std {
    template<>
    struct tuple_size<NanSize> : integral_constant<size_t, 2> {
    };

    template<size_t Index>
    struct tuple_element<Index, NanSize> {
        using type = float;
    };
}

namespace nandina::geometry {
    template<size_t Index>
    [[nodiscard]] constexpr auto get(const NanSize &sz) noexcept -> float {
        static_assert(Index < 2);
        return sz[Index];
    }
}