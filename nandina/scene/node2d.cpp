//
// Created by cvrain on 2026/6/30.
//

#include "node2d.hpp"
#include "../render/draw_context.hpp"
#include "canvas_layer.hpp"
#include "scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nandina::scene
{

    NanNode2D::NanNode2D() = default;

    // ---- local transform (all mutators invalidate global cache) ----

    auto NanNode2D::transform() const -> const foundation::NanTransform2D& {
        return transform_;
    }

    void NanNode2D::set_transform(const foundation::NanTransform2D& t) {
        transform_ = t;
        _propagate_invalidate_global();
    }

    auto NanNode2D::position() const -> foundation::NanPoint {
        return transform_.position();
    }

    void NanNode2D::set_position(const foundation::NanPoint pos) {
        transform_.set_position(pos);
        _propagate_invalidate_global();
    }

    auto NanNode2D::rotation() const -> float {
        return transform_.rotation();
    }

    void NanNode2D::set_rotation(const float radians) {
        transform_.set_rotation(radians);
        _propagate_invalidate_global();
    }

    auto NanNode2D::scale() const -> foundation::NanPoint {
        return transform_.scale();
    }

    void NanNode2D::set_scale(const foundation::NanPoint s) {
        transform_.set_scale(s);
        _propagate_invalidate_global();
    }

    void NanNode2D::set_scale(const float sx, const float sy) {
        transform_.set_scale_xy(sx, sy);
        _propagate_invalidate_global();
    }

    void NanNode2D::translate(const foundation::NanPoint offset) {
        transform_.translate(offset);
        _propagate_invalidate_global();
    }

    void NanNode2D::rotate(const float radians) {
        transform_.rotate(radians);
        _propagate_invalidate_global();
    }

    void NanNode2D::apply_scale(const foundation::NanPoint factor) {
        transform_.scale_by_xy(factor.get_x(), factor.get_y());
        _propagate_invalidate_global();
    }

    // ---- cached global transform ----

    auto NanNode2D::global_transform() const -> foundation::NanTransform2D {
        if (global_invalid_) {
            cached_global_ = transform_;
            for (const auto* parent_node = parent(); parent_node != nullptr;
                 parent_node = parent_node->parent()) {
                if (const auto* layer = parent_node->as_canvas_layer(); layer != nullptr) {
                    cached_global_ = layer->canvas_transform() * cached_global_;
                    break;
                }
                if (const auto* p = parent_node->as_node2d(); p != nullptr) {
                    cached_global_ = p->transform_ * cached_global_;
                }
            }
            global_invalid_ = false;
        }
        return cached_global_;
    }

    auto NanNode2D::global_position() const -> foundation::NanPoint {
        return global_transform().position();
    }

    void NanNode2D::set_global_position(const foundation::NanPoint pos) {
        if (const auto* parent_node = parent(); parent_node != nullptr) {
            if (const auto* layer = parent_node->as_canvas_layer(); layer != nullptr) {
                set_position(layer->canvas_transform().inverse_transform_point(pos));
            }
            else if (const auto* spatial_parent = parent_node->as_node2d(); spatial_parent != nullptr) {
                set_position(spatial_parent->global_transform().inverse_transform_point(pos));
            }
            else {
                set_position(pos);
            }
        }
        else {
            set_position(pos);
        }
    }

    auto NanNode2D::global_rotation() const -> float {
        auto rot = rotation();
        for (const auto* parent_node = parent(); parent_node != nullptr;
             parent_node = parent_node->parent()) {
            if (const auto* layer = parent_node->as_canvas_layer(); layer != nullptr) {
                rot += layer->canvas_transform().rotation();
                break;
            }
            if (const auto* p = parent_node->as_node2d(); p != nullptr) {
                rot += p->rotation();
            }
        }
        return rot;
    }

    auto NanNode2D::to_global(const foundation::NanPoint local_point) const
        -> foundation::NanPoint {
        return global_transform().transform_point(local_point);
    }

    auto NanNode2D::to_local(const foundation::NanPoint global_point) const
        -> foundation::NanPoint {
        return global_transform().inverse_transform_point(global_point);
    }

    auto NanNode2D::global_bounds() const -> foundation::NanRect {
        const auto pos = global_position();
        return foundation::NanRect::from_xywh(pos.get_x(), pos.get_y(), 0, 0);
    }

    // ---- visibility ----

    auto NanNode2D::visible() const -> bool {
        return visible_;
    }

    void NanNode2D::set_visible(const bool v) {
        visible_ = v;
        mark_semantics_dirty();
    }

    // ---- opacity ----

    auto NanNode2D::local_opacity() const -> float {
        return local_opacity_;
    }

    void NanNode2D::set_local_opacity(const float opacity) {
        if (!std::isfinite(opacity)) {
            throw std::invalid_argument("NanNode2D::set_local_opacity: opacity must be finite");
        }
        const float clamped = std::clamp(opacity, 0.0F, 1.0F);
        if (local_opacity_ == clamped) {
            return;
        }
        local_opacity_ = clamped;
        mark_paint_dirty();
    }

    // ---- draw order ----

    auto NanNode2D::z_index() const -> int {
        return z_index_;
    }

    void NanNode2D::set_z_index(const int z) {
        z_index_ = z;
    }

    void NanNode2D::request_focus() {
        focus_requested_ = true;
        schedule_focus_request();
    }

    // ---- hit testing ----

    auto NanNode2D::contains_point(foundation::NanPoint /*local_point*/) const -> bool {
        return false;
    }

    // ---- cache invalidation ----

    void NanNode2D::_invalidate_global() {
        global_invalid_ = true;
    }

    void NanNode2D::_propagate_invalidate_global() {
        global_invalid_ = true;
        mark_semantics_dirty();
        for (size_t i = 0; i < child_count(); ++i) {
            auto* child = get_child(i);
            if (auto* child_2d = child ? child->as_node2d() : nullptr) {
                child_2d->_propagate_invalidate_global();
            }
        }
    }

    // ---- lifecycle ----

    void NanNode2D::on_enter_tree() {
        NanNode::on_enter_tree();
        global_invalid_ = true; // Force recompute now that we have a parent chain.
        schedule_focus_request();
    }

    void NanNode2D::on_exit_tree() {
        focus_requested_ = false;
        focus_request_scheduled_ = false;
        ++focus_request_generation_;
        NanNode::on_exit_tree();
    }

    void NanNode2D::schedule_focus_request() {
        if (!focus_requested_ || focus_request_scheduled_ || !is_inside_tree()) {
            return;
        }
        auto* tree = get_tree();
        auto weak =
            std::weak_ptr<NanNode2D>(std::static_pointer_cast<NanNode2D>(shared_from_this()));
        const auto generation = focus_request_generation_;
        focus_request_scheduled_ = true;
        tree->post_layout([weak = std::move(weak), tree, generation] {
            const auto node = weak.lock();
            if (!node || node->focus_request_generation_ != generation) {
                return;
            }
            node->focus_request_scheduled_ = false;
            if (!node->focus_requested_ || node->get_tree() != tree) {
                return;
            }
            node->focus_requested_ = false;
            tree->set_focus(node.get());
        });
    }

    void NanNode2D::on_draw(render::DrawContext& ctx) {
        NanNode::on_draw(ctx);
    }

    // ---- draw-time transform propagation ----

    auto NanNode2D::_push_draw_transform(render::DrawContext& ctx) -> foundation::NanTransform2D {
        // ctx.world_ currently holds the parent's world transform. Compose this
        // node's local transform onto it (parent * local) and store as the new world.
        // This avoids each node re-walking the parent chain via global_transform().
        auto saved = ctx.world_;
        ctx.world_ = ctx.world_.compose(transform_);
        return saved;
    }

    void NanNode2D::_pop_draw_transform(
        render::DrawContext& ctx,
        const foundation::NanTransform2D& saved
    ) {
        ctx.world_ = saved;
    }

} // namespace nandina::scene
