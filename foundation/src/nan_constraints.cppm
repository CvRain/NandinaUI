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
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

export module nandina.foundation.nan_constraints;

export import nandina.foundation.nan_point;
export import nandina.foundation.nan_size;

export namespace nandina::geometry {
    // ──────────────────────────────────────────────────────────
    // NanConstraints - 布局约束类
    // ──────────────────────────────────────────────────────────

    /**
     * 表示 widget 在布局过程中可以接受的最小/最大尺寸范围。
     *
     * 语义定位：
     * - NanConstraints 描述尺寸的"允许范围"，而非具体尺寸。
     * - 包含 minWidth/maxWidth/minHeight/maxHeight 四个边界。
     * - tight 约束：min == max，widget 必须精确填充该尺寸。
     * - loose 约束：min == 0，max == 约束值，widget 在 0~max 间自由选择。
     * - unbounded：某一方向上 max 为 kInfinity，表示无上限。
     *
     * 与 NanSize 的关系：
     * - NanSize 表示具体尺寸值。
     * - NanConstraints 是 NanSize 的"范围"描述。
     * - constrain(NanSize) 将尺寸限制在约束范围内。
     *
     * 设计思路：
     * - 使用 float 类型，与 NanPoint/NanSize 一致。
     * - kInfinity 使用正无穷表示无上限。
     * - 提供丰富的工厂方法（tight/loose/expand）和变换方法。
     */
    class NanConstraints final {
    public:
        /// 无界限常量
        static constexpr float kInfinity = std::numeric_limits<float>::infinity();

        // ── 构造函数 ──

        /// 默认构造：tight(0,0)
        constexpr NanConstraints() noexcept
            : minWidth_{0.0f}, maxWidth_{0.0f},
              minHeight_{0.0f}, maxHeight_{0.0f} {
        }

        /// 全字段构造
        constexpr NanConstraints(const float minWidth, const float maxWidth,
                                 const float minHeight, const float maxHeight) noexcept
            : minWidth_{minWidth}, maxWidth_{maxWidth},
              minHeight_{minHeight}, maxHeight_{maxHeight} {
        }

        /// 从 NanSize 构造 tight 约束
        constexpr explicit NanConstraints(const NanSize &size) noexcept
            : minWidth_{size.width()}, maxWidth_{size.width()},
              minHeight_{size.height()}, maxHeight_{size.height()} {
        }

        /// tight(width, height) 便捷构造
        constexpr NanConstraints(const float width, const float height) noexcept
            : minWidth_{width}, maxWidth_{width},
              minHeight_{height}, maxHeight_{height} {
        }

        // ── 静态工厂方法 ──

        /// 创建 tight 约束（精确尺寸）
        [[nodiscard]] static constexpr auto tight(const NanSize &size) noexcept -> NanConstraints {
            return NanConstraints{size};
        }

        /// 创建 tight 约束（精确尺寸，直接传宽高）
        [[nodiscard]] static constexpr auto tight(const float width, const float height) noexcept -> NanConstraints {
            return NanConstraints{width, height};
        }

        /// 创建 loose 约束（0 ~ size）
        [[nodiscard]] static constexpr auto loose(const NanSize &size) noexcept -> NanConstraints {
            return NanConstraints{0.0f, size.width(), 0.0f, size.height()};
        }

        /// 创建 loose 约束（0 ~ width, 0 ~ height）
        [[nodiscard]] static constexpr auto loose(const float width, const float height) noexcept -> NanConstraints {
            return NanConstraints{0.0f, width, 0.0f, height};
        }

        /// 创建全部 unbounded 的约束
        [[nodiscard]] static constexpr auto expand() noexcept -> NanConstraints {
            return NanConstraints{0.0f, kInfinity, 0.0f, kInfinity};
        }

        // ── 访问器 ──

        [[nodiscard]] constexpr auto minWidth() const noexcept -> float { return minWidth_; }
        [[nodiscard]] constexpr auto maxWidth() const noexcept -> float { return maxWidth_; }
        [[nodiscard]] constexpr auto minHeight() const noexcept -> float { return minHeight_; }
        [[nodiscard]] constexpr auto maxHeight() const noexcept -> float { return maxHeight_; }

        // ── 设置器 ──

        constexpr auto setMinWidth(const float value) noexcept -> void { minWidth_ = value; }
        constexpr auto setMaxWidth(const float value) noexcept -> void { maxWidth_ = value; }
        constexpr auto setMinHeight(const float value) noexcept -> void { minHeight_ = value; }
        constexpr auto setMaxHeight(const float value) noexcept -> void { maxHeight_ = value; }

        // ── 查询方法 ──

        /// 约束是否在宽高上都是 tight（min == max）
        [[nodiscard]] constexpr auto isTight() const noexcept -> bool {
            return isTightWidth() && isTightHeight();
        }

        /// 宽度方向是否 tight
        [[nodiscard]] constexpr auto isTightWidth() const noexcept -> bool {
            return minWidth_ == maxWidth_;
        }

        /// 高度方向是否 tight
        [[nodiscard]] constexpr auto isTightHeight() const noexcept -> bool {
            return minHeight_ == maxHeight_;
        }

        /// 约束是否在宽高上都是 loose（min == 0 且 max > 0）
        [[nodiscard]] constexpr auto isLoose() const noexcept -> bool {
            return minWidth_ == 0.0f && maxWidth_ > 0.0f &&
                   minHeight_ == 0.0f && maxHeight_ > 0.0f;
        }

        /// 是否有界（非 unbounded）
        [[nodiscard]] constexpr auto isBounded() const noexcept -> bool {
            return !hasUnboundedWidth() && !hasUnboundedHeight();
        }

        /// 宽度方向是否无上限
        [[nodiscard]] constexpr auto hasUnboundedWidth() const noexcept -> bool {
            return maxWidth_ == kInfinity;
        }

        /// 高度方向是否无上限
        [[nodiscard]] constexpr auto hasUnboundedHeight() const noexcept -> bool {
            return maxHeight_ == kInfinity;
        }

        /// 约束是否有效（所有 min <= max）
        [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
            return minWidth_ <= maxWidth_ && minHeight_ <= maxHeight_;
        }

        // ── 约束变换 ──

        /// 收紧到给定 size：将 max 降低到 size，min 提升到 size（即变为 tight）
        [[nodiscard]] constexpr auto tighten(const NanSize &size) const noexcept -> NanConstraints {
            return NanConstraints{
                std::max(minWidth_, size.width()),
                std::min(maxWidth_, size.width()),
                std::max(minHeight_, size.height()),
                std::min(maxHeight_, size.height())
            };
        }

        /// 转换为 loose：将 min 置为 0，保持 max 不变
        [[nodiscard]] constexpr auto loosen() const noexcept -> NanConstraints {
            return NanConstraints{0.0f, maxWidth_, 0.0f, maxHeight_};
        }

        /// 取两个约束的交集（更严格的 min 和更宽松的 max）
        [[nodiscard]] constexpr auto intersect(const NanConstraints &other) const noexcept -> NanConstraints {
            return NanConstraints{
                std::max(minWidth_, other.minWidth_),
                std::min(maxWidth_, other.maxWidth_),
                std::max(minHeight_, other.minHeight_),
                std::min(maxHeight_, other.maxHeight_)
            };
        }

        // ── 尺寸适配 ──

        /// 将尺寸约束在 [min, max] 范围内
        [[nodiscard]] constexpr auto constrain(const NanSize &size) const noexcept -> NanSize {
            return NanSize{
                std::clamp(size.width(), minWidth_, maxWidth_),
                std::clamp(size.height(), minHeight_, maxHeight_)
            };
        }

        /// 将宽度约束在 [minWidth, maxWidth] 范围内
        [[nodiscard]] constexpr auto constrainWidth(const float width) const noexcept -> float {
            return std::clamp(width, minWidth_, maxWidth_);
        }

        /// 将高度约束在 [minHeight, maxHeight] 范围内
        [[nodiscard]] constexpr auto constrainHeight(const float height) const noexcept -> float {
            return std::clamp(height, minHeight_, maxHeight_);
        }

        /// 将点约束在约束范围内
        [[nodiscard]] constexpr auto constrain(const NanPoint &point) const noexcept -> NanPoint {
            return NanPoint{
                std::clamp(point.x(), minWidth_, maxWidth_),
                std::clamp(point.y(), minHeight_, maxHeight_)
            };
        }

        // ── 最佳尺寸 ──

        /// 返回最小允许尺寸 (minWidth, minHeight)
        [[nodiscard]] constexpr auto minSize() const noexcept -> NanSize {
            return NanSize{minWidth_, minHeight_};
        }

        /// 返回最大允许尺寸 (maxWidth, maxHeight)
        [[nodiscard]] constexpr auto maxSize() const noexcept -> NanSize {
            return NanSize{maxWidth_, maxHeight_};
        }

        /// 返回中间值（宽松布局时的推荐尺寸）
        [[nodiscard]] constexpr auto middle() const noexcept -> NanSize {
            const float midW = (hasUnboundedWidth() || isTightWidth())
                ? minWidth_
                : (minWidth_ + maxWidth_) * 0.5f;
            const float midH = (hasUnboundedHeight() || isTightHeight())
                ? minHeight_
                : (minHeight_ + maxHeight_) * 0.5f;
            return NanSize{midW, midH};
        }

        // ── 比较运算符 ──

        [[nodiscard]] constexpr auto operator==(const NanConstraints &rhs) const noexcept -> bool = default;
        [[nodiscard]] constexpr auto operator!=(const NanConstraints &rhs) const noexcept -> bool = default;

        // ── 辅助函数 ──

        /// 返回形如 "Constraints(minW=..., maxW=..., minH=..., maxH=...)" 的字符串
        [[nodiscard]] auto toString() const -> std::string {
            std::ostringstream oss;
            oss << "Constraints(minW=" << minWidth_ << ", maxW=";
            if (maxWidth_ == kInfinity) {
                oss << "INF";
            } else {
                oss << maxWidth_;
            }
            oss << ", minH=" << minHeight_ << ", maxH=";
            if (maxHeight_ == kInfinity) {
                oss << "INF";
            } else {
                oss << maxHeight_;
            }
            oss << ")";
            return oss.str();
        }

    private:
        float minWidth_;
        float maxWidth_;
        float minHeight_;
        float maxHeight_;
    };

    // ──────────────────────────────────────────────────────────
    // 非成员运算符
    // ──────────────────────────────────────────────────────────

    /// 流输出
    [[nodiscard]] auto operator<<(std::ostream &os, const NanConstraints &c) -> std::ostream& {
        return os << c.toString();
    }

    // ──────────────────────────────────────────────────────────
    // std::hash 特化
    // ──────────────────────────────────────────────────────────

    struct NanConstraintsHash {
        [[nodiscard]] auto operator()(const NanConstraints &c) const noexcept -> std::size_t {
            std::size_t seed = 0;
            seed ^= std::hash<float>{}(c.minWidth()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<float>{}(c.maxWidth()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<float>{}(c.minHeight()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<float>{}(c.maxHeight()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
} // namespace nandina::geometry

// ──────────────────────────────────────────────────────────
// fmt::formatter 特化（放在全局命名空间）
// ──────────────────────────────────────────────────────────

using namespace nandina::geometry;

template<>
struct fmt::formatter<NanConstraints> {
    template<typename ParseContext>
    static constexpr auto parse(ParseContext &ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const NanConstraints &c, FormatContext &ctx) -> FormatContext::iterator {
        return fmt::format_to(ctx.out(), "{}", c.toString());
    }
};