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
     * - 包含 min_width/max_width/min_height/max_height 四个边界。
     * - tight 约束：min == max，widget 必须精确填充该尺寸。
     * - loose 约束：min == 0，max == 约束值，widget 在 0~max 间自由选择。
     * - unbounded：某一方向上 max 为 k_infinity，表示无上限。
     *
     * 与 NanSize 的关系：
     * - NanSize 表示具体尺寸值。
     * - NanConstraints 是 NanSize 的"范围"描述。
     * - constrain(NanSize) 将尺寸限制在约束范围内。
     *
     * 设计思路：
     * - 使用 float 类型，与 NanPoint/NanSize 一致。
     * - k_infinity 使用正无穷表示无上限。
     * - 提供丰富的工厂方法（tight/loose/expand）和变换方法。
     */
    class NanConstraints final {
    public:
        /// 无界限常量
        static constexpr float k_infinity = std::numeric_limits<float>::infinity();

        // ── 构造函数 ──

        /// 默认构造：tight(0,0)
        constexpr NanConstraints() noexcept
            : min_width_{0.0f}, max_width_{0.0f}, min_height_{0.0f}, max_height_{0.0f} {
        }

        /// 全字段构造
        constexpr NanConstraints(const float min_width, const float max_width,
            const float min_height, const float max_height) noexcept
            : min_width_{min_width}, max_width_{max_width}, min_height_{min_height}, max_height_{max_height} {
        }

        /// 从 NanSize 构造 tight 约束
        constexpr explicit NanConstraints(const NanSize& size) noexcept
            : min_width_{size.width()}, max_width_{size.width()}, min_height_{size.height()}, max_height_{size.height()} {
        }

        /// tight(width, height) 便捷构造
        constexpr NanConstraints(const float width, const float height) noexcept
            : min_width_{width}, max_width_{width}, min_height_{height}, max_height_{height} {
        }

        // ── 静态工厂方法 ──

        /// 创建 tight 约束（精确尺寸）
        [[nodiscard]] static constexpr auto tight(const NanSize& size) noexcept -> NanConstraints {
            return NanConstraints{size};
        }

        /// 创建 tight 约束（精确尺寸，直接传宽高）
        [[nodiscard]] static constexpr auto tight(const float width, const float height) noexcept -> NanConstraints {
            return NanConstraints{width, height};
        }

        /// 创建 loose 约束（0 ~ size）
        [[nodiscard]] static constexpr auto loose(const NanSize& size) noexcept -> NanConstraints {
            return NanConstraints{0.0f, size.width(), 0.0f, size.height()};
        }

        /// 创建 loose 约束（0 ~ width, 0 ~ height）
        [[nodiscard]] static constexpr auto loose(const float width, const float height) noexcept -> NanConstraints {
            return NanConstraints{0.0f, width, 0.0f, height};
        }

        /// 创建全部 unbounded 的约束
        [[nodiscard]] static constexpr auto expand() noexcept -> NanConstraints {
            return NanConstraints{0.0f, k_infinity, 0.0f, k_infinity};
        }

        // ── 访问器 ──

        [[nodiscard]] constexpr auto min_width() const noexcept -> float {
            return min_width_;
        }

        [[nodiscard]] constexpr auto max_width() const noexcept -> float {
            return max_width_;
        }

        [[nodiscard]] constexpr auto min_height() const noexcept -> float {
            return min_height_;
        }

        [[nodiscard]] constexpr auto max_height() const noexcept -> float {
            return max_height_;
        }

        // ── 设置器 ──

        constexpr auto set_min_width(const float value) noexcept -> void {
            min_width_ = value;
        }

        constexpr auto set_max_width(const float value) noexcept -> void {
            max_width_ = value;
        }

        constexpr auto set_min_height(const float value) noexcept -> void {
            min_height_ = value;
        }

        constexpr auto set_max_height(const float value) noexcept -> void {
            max_height_ = value;
        }

        // ── 查询方法 ──

        /// 约束是否在宽高上都是 tight（min == max）
        [[nodiscard]] constexpr auto is_tight() const noexcept -> bool {
            return is_tight_width() && is_tight_height();
        }

        /// 宽度方向是否 tight
        [[nodiscard]] constexpr auto is_tight_width() const noexcept -> bool {
            return min_width_ == max_width_;
        }

        /// 高度方向是否 tight
        [[nodiscard]] constexpr auto is_tight_height() const noexcept -> bool {
            return min_height_ == max_height_;
        }

        /// 约束是否在宽高上都是 loose（min == 0 且 max > 0）
        [[nodiscard]] constexpr auto is_loose() const noexcept -> bool {
            return min_width_ == 0.0f && max_width_ > 0.0f && min_height_ == 0.0f && max_height_ > 0.0f;
        }

        /// 是否有界（非 unbounded）
        [[nodiscard]] constexpr auto is_bounded() const noexcept -> bool {
            return !has_unbounded_width() && !has_unbounded_height();
        }

        /// 宽度方向是否无上限
        [[nodiscard]] constexpr auto has_unbounded_width() const noexcept -> bool {
            return max_width_ == k_infinity;
        }

        /// 高度方向是否无上限
        [[nodiscard]] constexpr auto has_unbounded_height() const noexcept -> bool {
            return max_height_ == k_infinity;
        }

        /// 约束是否有效（所有 min <= max）
        [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
            return min_width_ <= max_width_ && min_height_ <= max_height_;
        }

        // ── 约束变换 ──

        /// 收紧到给定 size：将 max 降低到 size，min 提升到 size（即变为 tight）
        [[nodiscard]] constexpr auto tighten(const NanSize& size) const noexcept -> NanConstraints {
            return NanConstraints{std::max(min_width_, size.width()), std::min(max_width_, size.width()),
                                  std::max(min_height_, size.height()), std::min(max_height_, size.height())};
        }

        /// 转换为 loose：将 min 置为 0，保持 max 不变
        [[nodiscard]] constexpr auto loosen() const noexcept -> NanConstraints {
            return NanConstraints{0.0f, max_width_, 0.0f, max_height_};
        }

        /// 取两个约束的交集（更严格的 min 和更宽松的 max）
        [[nodiscard]] constexpr auto intersect(const NanConstraints& other) const noexcept -> NanConstraints {
            return NanConstraints{std::max(min_width_, other.min_width_), std::min(max_width_, other.max_width_),
                                  std::max(min_height_, other.min_height_), std::min(max_height_, other.max_height_)};
        }

        // ── 尺寸适配 ──

        /// 将尺寸约束在 [min, max] 范围内
        [[nodiscard]] constexpr auto constrain(const NanSize& size) const noexcept -> NanSize {
            return NanSize{
                std::clamp(size.width(), min_width_, max_width_), std::clamp(size.height(), min_height_, max_height_)};
        }

        /// 将宽度约束在 [min_width, max_width] 范围内
        [[nodiscard]] constexpr auto constrain_width(const float width) const noexcept -> float {
            return std::clamp(width, min_width_, max_width_);
        }

        /// 将高度约束在 [min_height, max_height] 范围内
        [[nodiscard]] constexpr auto constrain_height(const float height) const noexcept -> float {
            return std::clamp(height, min_height_, max_height_);
        }

        /// 将点约束在约束范围内
        [[nodiscard]] constexpr auto constrain(const NanPoint& point) const noexcept -> NanPoint {
            return NanPoint{std::clamp(point.x(), min_width_, max_width_), std::clamp(point.y(), min_height_, max_height_)};
        }

        // ── 最佳尺寸 ──

        /// 返回最小允许尺寸 (min_width, min_height)
        [[nodiscard]] constexpr auto min_size() const noexcept -> NanSize {
            return NanSize{min_width_, min_height_};
        }

        /// 返回最大允许尺寸 (max_width, max_height)
        [[nodiscard]] constexpr auto max_size() const noexcept -> NanSize {
            return NanSize{max_width_, max_height_};
        }

        /// 返回中间值（宽松布局时的推荐尺寸）
        [[nodiscard]] constexpr auto middle() const noexcept -> NanSize {
            const float midW = (has_unbounded_width() || is_tight_width()) ? min_width_ : (min_width_ + max_width_) * 0.5f;
            const float midH =
                (has_unbounded_height() || is_tight_height()) ? min_height_ : (min_height_ + max_height_) * 0.5f;
            return NanSize{midW, midH};
        }

        // ── 比较运算符 ──

        [[nodiscard]] constexpr auto operator==(const NanConstraints& rhs) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto operator!=(const NanConstraints& rhs) const noexcept -> bool = default;

        // ── 辅助函数 ──

        /// 返回形如 "Constraints(minW=..., maxW=..., minH=..., maxH=...)" 的字符串
        [[nodiscard]] auto to_string() const -> std::string {
            std::ostringstream oss;
            oss << "Constraints(minW=" << min_width_ << ", maxW=";
            if (max_width_ == k_infinity) {
                oss << "INF";
            } else {
                oss << max_width_;
            }
            oss << ", minH=" << min_height_ << ", maxH=";
            if (max_height_ == k_infinity) {
                oss << "INF";
            } else {
                oss << max_height_;
            }
            oss << ")";
            return oss.str();
        }

    private:
        float min_width_;
        float max_width_;
        float min_height_;
        float max_height_;
    };

    // ──────────────────────────────────────────────────────────
    // 非成员运算符
    // ──────────────────────────────────────────────────────────

    /// 流输出
    [[nodiscard]] auto operator<<(std::ostream& os, const NanConstraints& c) -> std::ostream& {
        return os << c.to_string();
    }

    // ──────────────────────────────────────────────────────────
    // std::hash 特化
    // ──────────────────────────────────────────────────────────

    struct NanConstraintsHash {
        [[nodiscard]] auto operator()(const NanConstraints& c) const noexcept -> std::size_t {
            std::size_t seed = 0;
            seed             ^= std::hash<float>{}(c.min_width()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed             ^= std::hash<float>{}(c.max_width()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed             ^= std::hash<float>{}(c.min_height()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed             ^= std::hash<float>{}(c.max_height()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
} // namespace nandina::geometry

// ──────────────────────────────────────────────────────────
// fmt::formatter 特化（放在全局命名空间）
// ──────────────────────────────────────────────────────────

using namespace nandina::geometry;

template <>
struct fmt::formatter<NanConstraints> {
    template <typename ParseContext>
    static constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const NanConstraints& c, FormatContext& ctx) -> FormatContext::iterator {
        return fmt::format_to(ctx.out(), "{}", c.to_string());
    }
};