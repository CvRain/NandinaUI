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

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_FRAME_SCHEDULER_HPP
