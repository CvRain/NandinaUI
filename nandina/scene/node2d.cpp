//
// Created by cvrain on 2026/6/30.
//

#include "node2d.hpp"
#include "../render/draw_context.hpp"
#include "canvas_layer.hpp"
#include "control.hpp"
#include "scene_tree.hpp"

namespace nandina::scene
{

    NanNode2D::NanNode2D(): presentation_(*this) {}
    NanNode2D::~NanNode2D() = default;

    // ---- local transform (all mutators invalidate global cache) ----

    auto NanNode2D::transform() const -> const foundation::NanTransform2D& {
        return transform_;
    }

    void NanNode2D::set_transform(const foundation::NanTransform2D& t) {
        transform_ = t;
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    auto NanNode2D::position() const -> foundation::NanPoint {
        return transform_.position();
    }

    void NanNode2D::set_position(const foundation::NanPoint pos) {
        transform_.set_position(pos);
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    auto NanNode2D::rotation() const -> float {
        return transform_.rotation();
    }

    void NanNode2D::set_rotation(const float radians) {
        transform_.set_rotation(radians);
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    auto NanNode2D::scale() const -> foundation::NanPoint {
        return transform_.scale();
    }

    void NanNode2D::set_scale(const foundation::NanPoint s) {
        transform_.set_scale(s);
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    void NanNode2D::set_scale(const float sx, const float sy) {
        transform_.set_scale_xy(sx, sy);
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    void NanNode2D::translate(const foundation::NanPoint offset) {
        transform_.translate(offset);
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    void NanNode2D::rotate(const float radians) {
        transform_.rotate(radians);
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    void NanNode2D::apply_scale(const foundation::NanPoint factor) {
        transform_.scale_by_xy(factor.get_x(), factor.get_y());
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    auto NanNode2D::visual_part(visual::node_t) noexcept -> NodePresentation& {
        return presentation_;
    }

    auto NanNode2D::visual_part(visual::node_t) const noexcept -> const NodePresentation& {
        return presentation_;
    }

    void NanNode2D::set_presentation_translate(foundation::NanPoint translate) {
        presentation_.property(visual::translate_t {}).set(translate);
    }

    void NanNode2D::set_presentation_scale(foundation::NanPoint scale) {
        presentation_.property(visual::scale_t {}).set(scale);
    }

    void NanNode2D::set_transform_origin(const TransformOrigin origin) {
        if (transform_origin_ == origin) {
            return;
        }
        transform_origin_ = origin;
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
    }

    auto NanNode2D::transform_origin() const noexcept -> TransformOrigin {
        return transform_origin_;
    }

    auto NanNode2D::resolved_transform_origin() const -> foundation::NanPoint {
        const auto* control = as_control();
        const float width = control != nullptr ? control->width() : 0.0F;
        const float height = control != nullptr ? control->height() : 0.0F;
        const bool center_x = transform_origin_ == TransformOrigin::top
            || transform_origin_ == TransformOrigin::center
            || transform_origin_ == TransformOrigin::bottom;
        const bool right = transform_origin_ == TransformOrigin::top_right
            || transform_origin_ == TransformOrigin::right
            || transform_origin_ == TransformOrigin::bottom_right;
        const bool center_y = transform_origin_ == TransformOrigin::left
            || transform_origin_ == TransformOrigin::center
            || transform_origin_ == TransformOrigin::right;
        const bool bottom = transform_origin_ == TransformOrigin::bottom_left
            || transform_origin_ == TransformOrigin::bottom
            || transform_origin_ == TransformOrigin::bottom_right;

        const auto center_x_width = center_x ? width * 0.5F : 0.0F;
        const auto center_y_height = center_y ? height * 0.5F : 0.0F;
        const auto right_width = right ? width : 0.0F;
        const auto bottom_height = bottom ? height : 0.0F;
        return foundation::NanPoint(right_width + center_x_width, bottom_height + center_y_height);
    }

    auto NanNode2D::effective_transform() const -> foundation::NanTransform2D {
        const auto translate = presentation_.translate();
        const auto scale = presentation_.scale();
        const auto origin = resolved_transform_origin();
        const auto pivot_offset = foundation::NanPoint(
            origin.get_x() * (1.0F - scale.get_x()),
            origin.get_y() * (1.0F - scale.get_y())
        );
        const auto presentation = foundation::NanTransform2D(translate + pivot_offset, 0.0F, scale);
        return transform_.compose(presentation);
    }

    // ---- cached global transform ----

    auto NanNode2D::global_transform() const -> foundation::NanTransform2D {
        if (global_invalid_) {
            cached_global_ = effective_transform();
            for (const auto* parent_node = parent(); parent_node != nullptr;
                 parent_node = parent_node->parent())
            {
                if (const auto* layer = parent_node->as_canvas_layer(); layer != nullptr) {
                    cached_global_ = layer->canvas_transform() * cached_global_;
                    break;
                }
                if (const auto* p = parent_node->as_node2d(); p != nullptr) {
                    cached_global_ = p->effective_transform() * cached_global_;
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
            else if (
                const auto* spatial_parent = parent_node->as_node2d(); spatial_parent != nullptr
            )
            {
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
             parent_node = parent_node->parent())
        {
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

    auto NanNode2D::dirty_flags() const -> DirtyFlags {
        return dirty_flags_;
    }

    auto NanNode2D::is_dirty(const DirtyFlags flags) const -> bool {
        return has_any(dirty_flags_, flags);
    }

    void NanNode2D::mark_dirty(const DirtyFlags flags) {
        // 两条依赖是分开的，方向也不同：
        //   transform     ⇒ 几何缓存失效（自身与后代），**并且**语义快照里的 bounds 也旧了
        //                    —— 所以 transform 单向蕴含 semantics，这是安全的加宽；
        //   semantics 单独 ⇒ 只是名称/描述/角色这类非几何信息变了，绝不去动变换缓存
        //                    （否则每改一次无障碍文案都要重算整棵子树的变换）。
        auto effective = flags;
        if (has_any(effective, DirtyFlags::transform)) {
            _propagate_invalidate_global();
            effective |= DirtyFlags::semantics;
        }
        if (has_any(effective, DirtyFlags::semantics)) {
            mark_semantics_dirty();
        }
        const auto newly_dirty = static_cast<DirtyFlags>(
            static_cast<std::uint8_t>(effective) & ~static_cast<std::uint8_t>(dirty_flags_)
        );
        dirty_flags_ |= effective;
        if (!has_any(newly_dirty, layout_dirty_flags)) {
            return;
        }
        for (auto* ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent()) {
            if (auto* node = ancestor->as_node2d(); node != nullptr) {
                node->dirty_flags_ |= layout_dirty_flags;
            }
        }
    }

    void NanNode2D::clear_dirty(const DirtyFlags flags) {
        dirty_flags_ = static_cast<DirtyFlags>(
            static_cast<std::uint8_t>(dirty_flags_) & ~static_cast<std::uint8_t>(flags)
        );
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
        return presentation_.opacity();
    }

    void NanNode2D::set_local_opacity(const float opacity) {
        presentation_.property(visual::opacity_t {}).set(opacity);
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

    void NanNode2D::_propagate_invalidate_global() {
        // 只做几何缓存失效：自身与后代都标记为需要重算。**不碰任何脏位、不碰语义** ——
        // 语义 bounds 的失效由 `mark_dirty(DirtyFlags::transform)` 负责，两者是分开的
        // 依赖（见 mark_dirty 的注释）。
        global_invalid_ = true;
        for (size_t i = 0; i < child_count(); ++i) {
            auto* child = get_child(i);
            if (child == nullptr) {
                continue;
            }
            if (auto* child_2d = child->as_node2d(); child_2d != nullptr) {
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
        ctx.world_ = ctx.world_.compose(effective_transform());
        return saved;
    }

    void NanNode2D::_pop_draw_transform(
        render::DrawContext& ctx,
        const foundation::NanTransform2D& saved
    ) {
        ctx.world_ = saved;
    }

} // namespace nandina::scene
