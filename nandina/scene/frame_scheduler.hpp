#ifndef NANDINA_EXPERIMENT_FRAME_SCHEDULER_HPP
#define NANDINA_EXPERIMENT_FRAME_SCHEDULER_HPP

#include <cstdint>

namespace nandina::scene
{

    enum class FramePhase : std::uint8_t {
        idle,
        input,
        tasks,
        process,
        tree_commit,
        physics,
        reactive,
        animation,
        layout,
        post_layout,
        semantics,
        paint,
        dispose,
    };

    enum class DirtyFlags : std::uint8_t {
        none = 0,
        style = 1U << 0U,
        measure = 1U << 1U,
        layout = 1U << 2U,
        paint = 1U << 3U,
        semantics = 1U << 4U,
        /// 节点的**有效变换**变了（local transform、表现层 translate/scale、或缩放中心），
        /// 因此几何缓存必须失效，语义 bounds 也需要重建。它和 `semantics` 正交：
        /// 纯语义变化（名称、描述、角色）不该让整棵子树的变换重算。
        transform = 1U << 5U,
    };

    [[nodiscard]] constexpr auto operator|(DirtyFlags lhs, DirtyFlags rhs) -> DirtyFlags {
        return static_cast<DirtyFlags>(
            static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs)
        );
    }

    [[nodiscard]] constexpr auto operator&(DirtyFlags lhs, DirtyFlags rhs) -> DirtyFlags {
        return static_cast<DirtyFlags>(
            static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs)
        );
    }

    constexpr auto operator|=(DirtyFlags& lhs, DirtyFlags rhs) -> DirtyFlags& {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr auto operator&=(DirtyFlags& lhs, DirtyFlags rhs) -> DirtyFlags& {
        lhs = lhs & rhs;
        return lhs;
    }

    [[nodiscard]] constexpr auto any(DirtyFlags flags) -> bool {
        return flags != DirtyFlags::none;
    }

    [[nodiscard]] constexpr auto has_any(DirtyFlags flags, DirtyFlags mask) -> bool {
        return any(flags & mask);
    }

    inline constexpr auto layout_dirty_flags = DirtyFlags::measure | DirtyFlags::layout;

    /// 几何比较阈值（逻辑像素）。小于它的尺寸差异不算一次几何变化。
    ///
    /// 存在的理由：measure 与 arrange 会用不同的浮点运算顺序算出**同一个**尺寸
    /// （例如 wrap 的行高 `font_size * 1.2 * lines` 与父级按测量值反推的紧约束），
    /// 实测差值在 1e-5 量级。`set_size()` 若按精确相等判断「是否变了」，这种噪声会让
    /// 节点每一轮布局都被判为"改了尺寸"，次次把祖先链标脏 —— 布局永不收敛。
    /// 千分之一逻辑像素在任何显示尺度下都不可见，作为「几何是否变化」的门槛是安全的。
    /// 注意：阈值只影响**是否标脏**，不影响尺寸本身的取值（见 `NanControl::set_size`）。
    /// 见 docs/references/layout_convergence_and_text_measurement.md §4.1。
    inline constexpr float layout_size_epsilon = 0.001F;

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_FRAME_SCHEDULER_HPP
