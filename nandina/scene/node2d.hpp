//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_NODE2D_HPP
#define NANDINA_EXPERIMENT_NODE2D_HPP

#include "../foundation/transform2d.hpp"
#include "node.hpp"

#include <cstdint>

namespace nandina::scene
{

    /**
     * 2D scene node with a local transform (position, rotation, scale).
     *
     * Inherits from NanNode and adds:
     *   - A local transform relative to the parent node.
     *   - Visibility flag and z-index for draw ordering.
     *   - Global transform computation by walking the parent chain.
     *
     * This is the base class for all spatially-located 2D nodes
     * (sprites, labels, collision shapes, etc.).
     *
     * The global transform is lazily cached: the first call to
     * global_transform() after a local change recomputes by composing
     * the parent chain. Subsequent calls return the cached value.
     * The cache is invalidated automatically when any transform mutator
     * is called (set_position, set_rotation, set_scale, etc.).
     */
    class NanNode2D: public NanNode {
    public:
        NanNode2D();

        // ---- local transform ----

        [[nodiscard]] auto transform() const -> const foundation::NanTransform2D&;
        void set_transform(const foundation::NanTransform2D& t);

        [[nodiscard]] auto position() const -> foundation::NanPoint;
        void set_position(foundation::NanPoint pos);

        [[nodiscard]] auto rotation() const -> float;
        void set_rotation(float radians);

        [[nodiscard]] auto scale() const -> foundation::NanPoint;
        void set_scale(foundation::NanPoint s);
        void set_scale(float sx, float sy);

        /// Translate by offset in local space.
        void translate(foundation::NanPoint offset);

        /// Rotate by radians (adds to current rotation).
        void rotate(float radians);

        /// Multiply current scale by factor.
        void apply_scale(foundation::NanPoint factor);

        // ---- global transform (world-space) ----

        /// World-space transform.  Lazily recomputed from the parent chain.
        [[nodiscard]] auto global_transform() const -> foundation::NanTransform2D;

        [[nodiscard]] auto global_position() const -> foundation::NanPoint;
        void set_global_position(foundation::NanPoint pos);

        [[nodiscard]] auto global_rotation() const -> float;

        /// Convert a point from local space to global (world) space.
        [[nodiscard]] auto to_global(foundation::NanPoint local_point) const
            -> foundation::NanPoint;

        /// Convert a point from global (world) space to local space.
        [[nodiscard]] auto to_local(foundation::NanPoint global_point) const
            -> foundation::NanPoint;

        /// World-space axis-aligned bounding rectangle of this node.
        /// Override in subclasses to return the actual interactive/visible area.
        /// Default: empty rect at global_position.
        [[nodiscard]] virtual auto global_bounds() const -> foundation::NanRect;

        // ---- visibility ----

        [[nodiscard]] auto visible() const -> bool;
        void set_visible(bool v);

        // ---- opacity ----

        /// Local alpha in [0,1]; default 1.0 (opaque). Multiplied once with the
        /// parent effective opacity during draw traversal. Affects the whole
        /// subtree's paint, but not visibility, input, or semantics.
        [[nodiscard]] auto local_opacity() const -> float override;
        void set_local_opacity(float opacity);

        // ---- draw order ----

        /// z_index controls sibling draw order. Higher values draw on top.
        [[nodiscard]] auto z_index() const -> int;
        void set_z_index(int z);

        [[nodiscard]] auto z_index_hint() const -> int override {
            return z_index_;
        }

        [[nodiscard]] auto is_visible_in_tree() const -> bool override {
            return visible_ && NanNode::is_visible_in_tree();
        }

        [[nodiscard]] auto as_node2d() -> NanNode2D* override {
            return this;
        }
        [[nodiscard]] auto as_node2d() const -> const NanNode2D* override {
            return this;
        }

        /// Request focus after the node is mounted and the current layout completes.
        /// Calls made while detached are remembered until the next tree entry.
        void request_focus();

        // ---- hit testing ----

        /// Check whether a point in local space is inside this node.
        /// Override in subclasses to define the node's interactive area.
        [[nodiscard]] virtual auto contains_point(foundation::NanPoint local_point) const -> bool;

        // ---- lifecycle ----

        void on_draw(render::DrawContext& ctx) override;

    protected:
        void on_enter_tree() override;
        void on_exit_tree() override;

        /// Compose this node's local transform onto the parent world in ctx.
        /// Returns the parent world so _propagate_draw can restore it afterward.
        [[nodiscard]] auto _push_draw_transform(render::DrawContext& ctx)
            -> foundation::NanTransform2D override;
        void _pop_draw_transform(
            render::DrawContext& ctx,
            const foundation::NanTransform2D& saved
        ) override;

        /// Invalidate this node's cached global transform.
        void _invalidate_global();

        /// Invalidate global transform on this node and all descendants.
        void _propagate_invalidate_global();

        void schedule_focus_request();

    private:
        foundation::NanTransform2D transform_;
        bool visible_ = true;
        int z_index_ = 0;
        float local_opacity_ = 1.0F;

        mutable foundation::NanTransform2D cached_global_;
        mutable bool global_invalid_ = true;
        bool focus_requested_ = false;
        bool focus_request_scheduled_ = false;
        std::uint64_t focus_request_generation_ = 0;
    };

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_NODE2D_HPP
