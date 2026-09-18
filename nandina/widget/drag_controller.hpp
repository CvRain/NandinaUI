//
// widget/drag_controller — 拖动一个已挂载节点到另一个容器。
//
// 典型场景（游戏 UI / 编辑器）：把按钮从 ListView 拖进 Grid、把卡片挪进面板。
// 它把「命中测试 → 落点解析 → 提交换父」收敛到一个窗口级服务：
//
//   on_drag_start : controller.start(node)
//   on_drag_move  : controller.update(pointer)   // 更新幽灵位置 + 解析落点
//   on_drag_end   : controller.commit()          // 真正换父，或 cancel()
//
// 换父本身走 `NanNode::reparent()`，因此旧父节点会正常收到 detach、新父节点收到
// attach；被拖动节点的内部状态（文本、数值、滚动位置、绑定）不受影响。
//
// 落点判定：从指针位置做 `NanSceneTree::hit_test`，沿祖先链找第一个
// `accepts_drop()` 为真且**不是被拖节点自身或其后代**的节点。解析是只读的，失败
// 只是"当前没有落点"，不改动树。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_DRAG_CONTROLLER_HPP
#define NANDINA_EXPERIMENT_WIDGET_DRAG_CONTROLLER_HPP

#include "../foundation/geometry.hpp"
#include "../scene/node.hpp"
#include "../scene/overlay_host.hpp"

#include <cstddef>
#include <functional>
#include <memory>

namespace nandina::scene
{
    class NanSceneTree;
}

namespace nandina::widget
{
    /// 一次拖动的会话句柄（由 DragController 持有，不是 RAII 守卫）。
    struct DragSession {
        /// 被拖动的节点。
        std::shared_ptr<scene::NanNode> node;

        /// 最多命中一级的落点容器；没有合法落点时为 nullptr。
        scene::NanNode* drop_target = nullptr;

        /// 在 drop_target 中的建议插入位置（按几何就近计算）。
        std::size_t drop_index = 0;

        /// 指针位置（全局坐标）。
        foundation::NanPoint pointer {};

        /// 指针相对被拖节点左上角的偏移。
        foundation::NanPoint grab_offset {};
    };

    class DragController {
    public:
        DragController() = default;
        ~DragController() = default;

        DragController(const DragController&) = delete;
        auto operator=(const DragController&) -> DragController& = delete;
        DragController(DragController&&) = delete;
        auto operator=(DragController&&) -> DragController& = delete;

        /**
         * 安装服务。由窗口在初始化时调用一次。
         *
         * @param tree      场景树，用于命中测试（必须比本控制器活得久）
         * @param overlays  浮层宿主，用于承载拖动幽灵（可为 nullptr，则无幽灵）
         * @param predicate 判断某节点是否可作为落点容器；为 null 时回退到
         *                  `node->accepts_drop()`
         */
        void install(
            scene::NanSceneTree& tree,
            scene::OverlayHost* overlays,
            std::function<bool(const scene::NanNode&)> predicate = {}
        );

        /**
         * 开始拖动。重复调用会先结束当前会话。
         *
         * @param node  被拖动的节点（必须已挂载；未挂载时 start 会失败并返回 false）
         * @param ghost 跟随指针的幽灵节点；为 null 时不显示幽灵
         * @param grab_offset 指针相对节点左上角的偏移，幽灵按它对齐
         * @return 会话是否成功开始
         */
        auto start(
            std::shared_ptr<scene::NanNode> node,
            std::shared_ptr<scene::NanControl> ghost = nullptr,
            foundation::NanPoint grab_offset = foundation::NanPoint()
        ) -> bool;

        /**
         * 指针移动：按 `base_pointer` 解析落点，并把幽灵对齐到那里。
         *
         * base_pointer 是「指针按在节点上的那个点」的当前位置（全局坐标），因此幽灵
         * 会跟随指针而不是贴到指针左上角。没有会话时是 no-op。
         */
        void update(foundation::NanPoint base_pointer);

        /// 提交当前落点：把节点换父到 drop_target 的 drop_index。没有落点时返回 false。
        auto commit() -> bool;

        /// 放弃拖动（移走幽灵，不改动树）。
        void cancel();

        [[nodiscard]] auto active() const noexcept -> bool {
            return session_ != nullptr;
        }

        /// 当前会话；未拖动时为 nullptr。
        [[nodiscard]] auto session() const noexcept -> const DragSession* {
            return session_.get();
        }

        /// 便捷查询：当前落点容器。
        [[nodiscard]] auto drop_target() const noexcept -> scene::NanNode* {
            return session_ != nullptr ? session_->drop_target : nullptr;
        }

    private:
        [[nodiscard]] auto accepts(const scene::NanNode& node) const -> bool;

        /// 落点是否仍然有效：仍在树内、不是被拖节点自身或其后代、类型边界匹配，
        /// 且目标的 accepts_child() 认可。commit() 改树前会重新调用它。
        [[nodiscard]] static auto can_accept_drop(scene::NanNode& node, scene::NanNode* target)
            -> bool;
        [[nodiscard]] auto resolve_drop_target(foundation::NanPoint pointer) -> scene::NanNode*;
        [[nodiscard]] auto resolve_drop_index(
            const scene::NanNode& container,
            const scene::NanNode& dragged,
            foundation::NanPoint pointer
        ) const -> std::size_t;
        void show_ghost();
        void hide_ghost();

        scene::NanSceneTree* tree_ = nullptr;
        scene::OverlayHost* overlays_ = nullptr;
        std::function<bool(const scene::NanNode&)> predicate_;
        std::unique_ptr<DragSession> session_;
        std::shared_ptr<scene::NanControl> ghost_;
        scene::OverlayHandle ghost_handle_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_DRAG_CONTROLLER_HPP
