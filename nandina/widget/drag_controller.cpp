//
// widget/drag_controller — 拖动换父服务的实现。
//

#include "drag_controller.hpp"

#include "../scene/control.hpp"
#include "../scene/node2d.hpp"
#include "../scene/scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    void DragController::install(
        scene::NanSceneTree& tree,
        scene::OverlayHost* overlays,
        std::function<bool(const scene::NanNode&)> predicate
    ) {
        cancel();
        tree_ = &tree;
        overlays_ = overlays;
        predicate_ = std::move(predicate);
    }

    auto DragController::accepts(const scene::NanNode& node) const -> bool {
        if (predicate_) {
            return predicate_(node);
        }
        return node.accepts_drop();
    }

    auto DragController::start(
        std::shared_ptr<scene::NanNode> node,
        std::shared_ptr<scene::NanControl> ghost,
        const foundation::NanPoint grab_offset
    ) -> bool {
        cancel();
        if (!node || !node->is_inside_tree()) {
            return false;
        }
        session_ = std::make_unique<DragSession>();
        session_->node = std::move(node);
        session_->grab_offset = grab_offset;
        ghost_ = std::move(ghost);
        return true;
    }

    void DragController::update(const foundation::NanPoint base_pointer) {
        if (session_ == nullptr) {
            return;
        }
        session_->pointer = base_pointer;
        // 命中测试用指针位置，不用抓取偏移后的幽灵位置 —— 用户看的是指针在哪。
        session_->drop_target = resolve_drop_target(base_pointer);
        session_->drop_index = session_->drop_target != nullptr
            ? resolve_drop_index(*session_->drop_target, *session_->node, base_pointer)
            : 0;
        show_ghost();
    }

    auto DragController::commit() -> bool {
        // 落点是 update() 期间记录的**裸指针**，可能在拖动过程中因为别处的
        // remove / reparent / 关闭浮层而失效或脱离树。这里在改动树之前重新确认一次：
        // 失效就安全取消，绝不把僵死指针交给 reparent()，也不让异常穿到事件主循环。
        if (session_ == nullptr || !can_accept_drop(*session_->node, session_->drop_target)) {
            cancel();
            return false;
        }
        auto target = session_->drop_target;
        auto node = session_->node;
        const auto index = session_->drop_index;
        hide_ghost();
        session_.reset();
        ghost_.reset();

        target->reparent(node, index);
        // reparent 在树遍历期间会延后到本帧安全点；这里只报告"已受理"。
        return true;
    }

    auto DragController::can_accept_drop(scene::NanNode& node, scene::NanNode* target)
        -> bool {
        if (target == nullptr || target == &node) {
            return false;
        }
        // 目标必须仍在当前树里（可能已被移出 / 关闭），否则 reparent 会把节点挂到
        // 一棵“孤儿”子树上。
        if (!target->is_inside_tree()) {
            return false;
        }
        // 被拖节点自身与后代永远不是落点，拖动过程中树发生变化后要重新判定。
        if (node.is_ancestor_of(*target)) {
            return false;
        }
        // 类型边界：NanNode / NanNode2D 不能混挂。
        const auto* target_2d = target->as_node2d();
        const auto* node_2d = node.as_node2d();
        if ((target_2d != nullptr) != (node_2d != nullptr)) {
            return false;
        }
        // 父节点的准入钩子（例如只读列表、单子节点容器）在这里提前兑现。
        return target->accepts_child(node);
    }

    void DragController::cancel() {
        hide_ghost();
        session_.reset();
        ghost_.reset();
    }

    auto DragController::resolve_drop_target(const foundation::NanPoint pointer)
        -> scene::NanNode* {
        if (tree_ == nullptr || session_ == nullptr) {
            return nullptr;
        }
        auto* hit = tree_->hit_test(pointer);
        for (auto* candidate = static_cast<scene::NanNode*>(hit); candidate != nullptr;
             candidate = candidate->parent())
        {
            // 不能落到自己或自己的后代上（那会成环 / 没有意义）。
            if (candidate == session_->node.get() || session_->node->is_ancestor_of(*candidate)) {
                continue;
            }
            if (accepts(*candidate) && can_accept_drop(*session_->node, candidate)) {
                return candidate;
            }
        }
        return nullptr;
    }

    auto DragController::resolve_drop_index(
        const scene::NanNode& container,
        const scene::NanNode& dragged,
        const foundation::NanPoint pointer
    ) const -> std::size_t {
        // 容器可以自己给提示（例如 Grid 的网格落位）。
        if (const auto slot = container.drop_slot_at(pointer); slot != scene::NanNode::npos) {
            return slot;
        }

        // 默认按几何就近：沿主轴找第一个"指针位于其中线之前"的兄弟，插到它前面。
        const auto container_bounds = container.as_node2d() != nullptr
            ? container.as_node2d()->global_bounds()
            : foundation::NanRect {};
        const bool horizontal = container_bounds.get_width() > container_bounds.get_height();

        std::size_t index = 0;
        for (std::size_t i = 0; i < container.child_count(); ++i) {
            const auto* child = container.get_child(i);
            if (child == &dragged) {
                continue;
            }
            const auto* child_2d = child != nullptr ? child->as_node2d() : nullptr;
            if (child_2d == nullptr || !child_2d->visible()) {
                ++index;
                continue;
            }
            const auto bounds = child_2d->global_bounds();
            const auto middle = horizontal ? bounds.get_left() + bounds.get_width() * 0.5F
                                           : bounds.get_top() + bounds.get_height() * 0.5F;
            const auto position = horizontal ? pointer.get_x() : pointer.get_y();
            if (position < middle) {
                break;
            }
            ++index;
        }
        return index;
    }

    void DragController::show_ghost() {
        if (ghost_ == nullptr || overlays_ == nullptr || session_ == nullptr) {
            return;
        }
        if (!ghost_handle_.mounted()) {
            ghost_handle_ = overlays_->present(
                ghost_,
                scene::OverlayOptions {
                    .level = scene::OverlayLevel::nested_popup,
                    .block_below = false,
                }
            );
        }
        if (auto* control = ghost_->as_control(); control != nullptr) {
            // 幽灵按抓取偏移对齐指针，避免拖动时内容"跳"到指针左上角。
            control->set_position(
                foundation::NanPoint {
                    session_->pointer.get_x() - session_->grab_offset.get_x(),
                    session_->pointer.get_y() - session_->grab_offset.get_y(),
                }
            );
        }
    }

    void DragController::hide_ghost() {
        if (ghost_handle_.mounted()) {
            ghost_handle_.close();
        }
    }
} // namespace nandina::widget
