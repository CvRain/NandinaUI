//
// Created by cvrain on 2026/4/24.
//
module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fmt/core.h>
#include <functional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

export module nandina.foundation.nan_insets;

export import nandina.foundation.nan_point;
export import nandina.foundation.nan_rect;

export namespace nandina::geometry {
    // ──────────────────────────────────────────────────────────
    // NanInsets - 四边内边距/缩进类
    // ──────────────────────────────────────────────────────────

    /**
     * 表示矩形四条边的等距或不等距缩进量。
     *
     * 语义定位：
     * - NanInsets 描述矩形每条边向内或向外的偏移量。
     * - 正值表示向内收缩（减小矩形面积），负值表示向外扩张。
     * - 常用场景：padding（内边距）、margin（外边距）、border（边框）。
     *
     * 与 NanRect 的关系：
     * - NanRect 定义矩形边界本身。
     * - NanInsets 定义边界如何"扩展"或"收缩"。
     * - 通过 applyToRect() 将 inset 应用到矩形上获得新的矩形。
     *
     * 设计思路：
     * - 使用 float 类型，与 NanPoint/NanSize/NanRect 一致。
     * - 提供统一构造（所有边相等）和四边独立构造。
     * - 支持算术运算和与矩形的交互。
     */
    class NanInsets final {
    public:
        // ── 构造函数 ──

        /// 默认构造，所有边为零
        constexpr NanInsets() noexcept
            : values_{0.0f, 0.0f, 0.0f, 0.0f} {
        }

        /// 统一缩进（四条边相同值）
        constexpr explicit NanInsets(const float all) noexcept
            : values_{all, all, all, all} {
        }

        /// 水平/垂直独立缩进
        constexpr NanInsets(const float horizontal, const float vertical) noexcept
            : values_{horizontal, vertical, horizontal, vertical} {
        }

        /// 四边独立缩进
        constexpr NanInsets(const float left, const float top, const float right, const float bottom) noexcept
            : values_{left, top, right, bottom} {
        }

        // ── 静态工厂方法 ──

        /// 从统一值创建
        [[nodiscard]] static constexpr auto all(const float value) noexcept -> NanInsets {
            return NanInsets{value};
        }

        /// 仅设置水平方向
        [[nodiscard]] static constexpr auto symmetricH(const float value) noexcept -> NanInsets {
            return NanInsets{value, 0.0f};
        }

        /// 仅设置垂直方向
        [[nodiscard]] static constexpr auto symmetricV(const float value) noexcept -> NanInsets {
            return NanInsets{0.0f, value};
        }

        /// 从单个值创建（同 all）
        [[nodiscard]] static constexpr auto fromAll(const float value) noexcept -> NanInsets {
            return NanInsets{value};
        }

        /// 仅左
        [[nodiscard]] static constexpr auto fromLeft(const float value) noexcept -> NanInsets {
            return NanInsets{value, 0.0f, 0.0f, 0.0f};
        }

        /// 仅上
        [[nodiscard]] static constexpr auto fromTop(const float value) noexcept -> NanInsets {
            return NanInsets{0.0f, value, 0.0f, 0.0f};
        }

        /// 仅右
        [[nodiscard]] static constexpr auto fromRight(const float value) noexcept -> NanInsets {
            return NanInsets{0.0f, 0.0f, value, 0.0f};
        }

        /// 仅下
        [[nodiscard]] static constexpr auto fromBottom(const float value) noexcept -> NanInsets {
            return NanInsets{0.0f, 0.0f, 0.0f, value};
        }

        // ── 便捷访问器 ──

        [[nodiscard]] constexpr auto left() const noexcept -> float { return values_[0]; }
        [[nodiscard]] constexpr auto top() const noexcept -> float { return values_[1]; }
        [[nodiscard]] constexpr auto right() const noexcept -> float { return values_[2]; }
        [[nodiscard]] constexpr auto bottom() const noexcept -> float { return values_[3]; }

        [[nodiscard]] constexpr auto horizontal() const noexcept -> float {
            return values_[0] + values_[2];
        }

        [[nodiscard]] constexpr auto vertical() const noexcept -> float {
            return values_[1] + values_[3];
        }

        /// 左上角视为一个点
        [[nodiscard]] constexpr auto topLeft() const noexcept -> NanPoint {
            return NanPoint{values_[0], values_[1]};
        }

        /// 右下角视为一个点
        [[nodiscard]] constexpr auto bottomRight() const noexcept -> NanPoint {
            return NanPoint{values_[2], values_[3]};
        }

        // ── 设置器 ──

        constexpr auto setLeft(const float value) noexcept -> void { values_[0] = value; }
        constexpr auto setTop(const float value) noexcept -> void { values_[1] = value; }
        constexpr auto setRight(const float value) noexcept -> void { values_[2] = value; }
        constexpr auto setBottom(const float value) noexcept -> void { values_[3] = value; }

        /// 统一设置所有边
        constexpr auto setAll(const float value) noexcept -> void {
            values_[0] = values_[1] = values_[2] = values_[3] = value;
        }

        /// 同时设置四条边
        constexpr auto set(const float left, const float top, const float right, const float bottom) noexcept -> void {
            values_[0] = left;
            values_[1] = top;
            values_[2] = right;
            values_[3] = bottom;
        }

        // ── 属性方法 ──

        /// 是否所有缩进为零
        [[nodiscard]] constexpr auto isZero() const noexcept -> bool {
            return values_[0] == 0.0f && values_[1] == 0.0f &&
                   values_[2] == 0.0f && values_[3] == 0.0f;
        }

        /// 缩进是否为空（所有边 <= 0？实际上 Insets 的"空"通常指零值）
        /// 此方法检查是否所有边都为零或负值（即无正向内缩）
        [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool {
            return values_[0] <= 0.0f && values_[1] <= 0.0f &&
                   values_[2] <= 0.0f && values_[3] <= 0.0f;
        }

        /// 是否有任何正缩进
        [[nodiscard]] constexpr auto hasPositive() const noexcept -> bool {
            return values_[0] > 0.0f || values_[1] > 0.0f ||
                   values_[2] > 0.0f || values_[3] > 0.0f;
        }

        /// 是否有任何负缩进（扩张）
        [[nodiscard]] constexpr auto hasNegative() const noexcept -> bool {
            return values_[0] < 0.0f || values_[1] < 0.0f ||
                   values_[2] < 0.0f || values_[3] < 0.0f;
        }

        /// 水平方向是否对称（left == right）
        [[nodiscard]] constexpr auto isHorizontalSymmetric() const noexcept -> bool {
            return std::abs(values_[0] - values_[2]) < 1.0e-6f;
        }

        /// 垂直方向是否对称（top == bottom）
        [[nodiscard]] constexpr auto isVerticalSymmetric() const noexcept -> bool {
            return std::abs(values_[1] - values_[3]) < 1.0e-6f;
        }

        /// 是否完全对称（所有边相等）
        [[nodiscard]] constexpr auto isUniform() const noexcept -> bool {
            return isHorizontalSymmetric() && isVerticalSymmetric() &&
                   std::abs(values_[0] - values_[1]) < 1.0e-6f;
        }

        // ── 与 NanRect 交互 ──

        /// 将缩进应用到矩形上（正值缩小，负值扩大）
        [[nodiscard]] constexpr auto applyToRect(const NanRect &rect) const noexcept -> NanRect {
            return NanRect{
                rect.left() + values_[0],
                rect.top() + values_[1],
                rect.right() - values_[2],
                rect.bottom() - values_[3]
            };
        }

        /// 将缩进反向应用到矩形上（正值扩大，负值缩小）
        [[nodiscard]] constexpr auto inflateRect(const NanRect &rect) const noexcept -> NanRect {
            return NanRect{
                rect.left() - values_[0],
                rect.top() - values_[1],
                rect.right() + values_[2],
                rect.bottom() + values_[3]
            };
        }

        /// 缩进总和作为矩形尺寸偏移量
        [[nodiscard]] constexpr auto toSize() const noexcept -> NanSize {
            return NanSize{horizontal(), vertical()};
        }

        // ── 算术运算符 ──

        /// 加法（对应边相加）
        [[nodiscard]] constexpr auto operator+(const NanInsets &rhs) const noexcept -> NanInsets {
            return NanInsets{
                values_[0] + rhs.values_[0],
                values_[1] + rhs.values_[1],
                values_[2] + rhs.values_[2],
                values_[3] + rhs.values_[3]
            };
        }

        /// 减法（对应边相减）
        [[nodiscard]] constexpr auto operator-(const NanInsets &rhs) const noexcept -> NanInsets {
            return NanInsets{
                values_[0] - rhs.values_[0],
                values_[1] - rhs.values_[1],
                values_[2] - rhs.values_[2],
                values_[3] - rhs.values_[3]
            };
        }

        /// 标量乘法
        [[nodiscard]] constexpr auto operator*(const float scalar) const noexcept -> NanInsets {
            return NanInsets{
                values_[0] * scalar,
                values_[1] * scalar,
                values_[2] * scalar,
                values_[3] * scalar
            };
        }

        /// 标量除法
        [[nodiscard]] constexpr auto operator/(const float scalar) const -> NanInsets {
            if (scalar == 0.0f) {
                throw std::domain_error{"Insets division by zero"};
            }
            return NanInsets{
                values_[0] / scalar,
                values_[1] / scalar,
                values_[2] / scalar,
                values_[3] / scalar
            };
        }

        /// 一元负号（反转缩进方向）
        [[nodiscard]] constexpr auto operator-() const noexcept -> NanInsets {
            return NanInsets{
                -values_[0], -values_[1],
                -values_[2], -values_[3]
            };
        }

        // ── 复合赋值运算符 ──

        constexpr auto operator+=(const NanInsets &rhs) noexcept -> NanInsets& {
            values_[0] += rhs.values_[0];
            values_[1] += rhs.values_[1];
            values_[2] += rhs.values_[2];
            values_[3] += rhs.values_[3];
            return *this;
        }

        constexpr auto operator-=(const NanInsets &rhs) noexcept -> NanInsets& {
            values_[0] -= rhs.values_[0];
            values_[1] -= rhs.values_[1];
            values_[2] -= rhs.values_[2];
            values_[3] -= rhs.values_[3];
            return *this;
        }

        constexpr auto operator*=(const float scalar) noexcept -> NanInsets& {
            values_[0] *= scalar;
            values_[1] *= scalar;
            values_[2] *= scalar;
            values_[3] *= scalar;
            return *this;
        }

        constexpr auto operator/=(const float scalar) -> NanInsets& {
            *this = *this / scalar;
            return *this;
        }

        // ── 比较运算符 ──

        [[nodiscard]] constexpr auto operator==(const NanInsets &rhs) const noexcept -> bool = default;
        [[nodiscard]] constexpr auto operator!=(const NanInsets &rhs) const noexcept -> bool = default;

        // ── 辅助函数 ──

        /// 返回形如 "(L=x, T=x, R=x, B=x)" 的字符串
        [[nodiscard]] auto toString() const -> std::string {
            std::ostringstream oss;
            oss << "(L=" << values_[0] << ", T=" << values_[1]
                << ", R=" << values_[2] << ", B=" << values_[3] << ")";
            return oss.str();
        }

        /// 交换内容
        constexpr auto swap(NanInsets &other) noexcept -> void {
            values_.swap(other.values_);
        }

    private:
        std::array<float, 4> values_{}; // [left, top, right, bottom]
    };

    // ──────────────────────────────────────────────────────────
    // 非成员运算符
    // ──────────────────────────────────────────────────────────

    /// 标量左乘
    [[nodiscard]] constexpr auto operator*(const float scalar, const NanInsets &insets) noexcept -> NanInsets {
        return insets * scalar;
    }

    /// 流输出
    [[nodiscard]] auto operator<<(std::ostream &os, const NanInsets &insets) -> std::ostream& {
        return os << insets.toString();
    }

    /// 将 insets 应用于 rect（等同 rect + insets 的语义）
    [[nodiscard]] constexpr auto operator+(const NanRect &rect, const NanInsets &insets) noexcept -> NanRect {
        return insets.applyToRect(rect);
    }

    /// 从 rect 中减去 insets（等同 rect - insets 的语义：反向应用）
    [[nodiscard]] constexpr auto operator-(const NanRect &rect, const NanInsets &insets) noexcept -> NanRect {
        return insets.inflateRect(rect);
    }

    // ──────────────────────────────────────────────────────────
    // std::hash 特化
    // ──────────────────────────────────────────────────────────

    struct NanInsetsHash {
        [[nodiscard]] auto operator()(const NanInsets &insets) const noexcept -> std::size_t {
            std::size_t seed = 0;
            seed ^= std::hash<float>{}(insets.left()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<float>{}(insets.top()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<float>{}(insets.right()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<float>{}(insets.bottom()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
} // namespace nandina::geometry

// ──────────────────────────────────────────────────────────
// fmt::formatter 特化（放在全局命名空间）
// ──────────────────────────────────────────────────────────

using namespace nandina::geometry;

template<>
struct fmt::formatter<NanInsets> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const NanInsets &insets, FormatContext &ctx) -> FormatContext::iterator {
        return fmt::format_to(ctx.out(), "{}", insets.toString());
    }
};