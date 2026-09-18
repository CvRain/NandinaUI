//
// Created by cvrain on 2026/6/30.
//

#ifndef NANDINA_EXPERIMENT_NODE_HPP
#define NANDINA_EXPERIMENT_NODE_HPP

#include "../foundation/transform2d.hpp"
#include "../render/clip_stack.hpp"
#include "../semantics/semantics.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../theme/style_context.hpp"

namespace nandina::scene
{

    class InputEvent;
    class NanSceneTree;
    class NanControl;
    class NanNode2D;
    class CanvasLayer;
    class LayerStack;
} // namespace nandina::scene

namespace nandina::render
{
    class DrawContext;
    class TextureCache;
} // namespace nandina::render

namespace nandina::widget::primitives
{
    struct TextPipeline;
}

namespace nandina::text
{
    class FontPipelineCache;
}

namespace nandina::theme
{
    class ThemeManager;
}

namespace nandina::scene
{

    /**
     * Base class for all nodes in the scene tree.
     *
     * Nodes form a tree: each node owns zero or more children via unique_ptr.
     * The tree root is owned by NanSceneTree.  When a node is destroyed all
     * descendants are recursively destroyed.
     *
     * Lifecycle callbacks (override in subclasses):
     *   on_enter_tree()  — top-down, called when the node enters the active tree
     *   on_ready()       — bottom-up, called after ALL descendants have entered
     *   on_exit_tree()   — called before the node leaves the active tree
     *   on_process(dt)   — top-down, every frame (dt in seconds)
     *   on_draw()        — top-down, parent drawn before children
     *
     * Lifecycle order on add_child to an in-tree parent:
     *   1. child.on_enter_tree
     *   2. grandchild.on_enter_tree (recursively)
     *   3. grandchild.on_ready
     *   4. child.on_ready
     *
     * @note Nodes are non-copyable and non-movable while inside the tree.
     *       Ownership is managed through std::unique_ptr.
     */
    class NanNode: public std::enable_shared_from_this<NanNode> {
        friend class NanSceneTree;

    public:
        NanNode();
        virtual ~NanNode();

        NanNode(const NanNode&) = delete;
        auto operator=(const NanNode&) -> NanNode& = delete;
        NanNode(NanNode&&) = delete;
        auto operator=(NanNode&&) -> NanNode& = delete;

        // ---- hierarchy ----

        /// Parent node, or nullptr if this is the root or detached.
        [[nodiscard]] auto parent() const -> NanNode*;

        /// Number of direct children.
        [[nodiscard]] auto child_count() const -> size_t;

        /// Get child by index [0, child_count).
        [[nodiscard]] auto get_child(size_t index) const -> NanNode*;

        /// True if this node is an ancestor of `other`.
        [[nodiscard]] auto is_ancestor_of(const NanNode& other) const -> bool;

        // ---- tree membership ----

        /// True while this node is part of an active scene tree.
        [[nodiscard]] auto is_inside_tree() const -> bool;

        /// The scene tree this node belongs to, or nullptr.
        [[nodiscard]] auto get_tree() const -> NanSceneTree*;

        void set_style_context(theme::StyleContext context);
        void clear_style_context();
        [[nodiscard]] auto style_context() const -> const theme::StyleContext&;
        [[nodiscard]] auto resolved_style_context() const
            -> const theme::ResolvedStyleContext&;

        // ---- child management ----

        /**
         * Add a child node.  Ownership transfers to this node.
         * @return A reference to the added child (non-owning).
         *
         * @pre child is not null and not already owned by another node.
         * @throws std::logic_error if child already has a parent (use reparent())
         *         or is already in a tree.
         * @throws std::runtime_error if child is null, the parent rejects this
         *         child type, or NanNode/NanNode2D would be mixed on one edge.
         */
        auto add_child(std::shared_ptr<NanNode> child) -> NanNode&;

        /**
         * Insert a detached child at a stable sibling position. Existing siblings
         * keep their lifecycle state; an out-of-range index appends the child.
         *
         * @pre child is detached (no parent, not inside a tree).
         * @throws std::logic_error if child already has a parent (use reparent())
         *         or is already in a tree.
         */
        auto insert_child(std::size_t index, std::shared_ptr<NanNode> child) -> NanNode&;

        /**
         * Move `child` here, detaching it from its current parent first.
         *
         * 这是「拖拽换父」的正规入口：把一个已经挂在别处的节点挂到本节点下。
         * 相比 add_child/insert_child，它会先走一次完整 detach（旧父节点收到
         * layout/semantics 失效标记，子树的 on_exit_tree 被调用），再走完整 attach
         * （on_enter_tree / on_ready）。节点自身的内部状态（文本、数值、滚动位置等）
         * 不受影响。
         *
         * 与 set_root / set_child 这类「只允许一个子节点」的容器也兼容：旧父节点
         * 会正常失去这个子节点。
         *
         * @param child 目标节点；不能为 null、不能是自己、不能是自己的祖先
         *              （否则会形成环）。
         * @param index 在新父节点中的插入位置；越界则追加。
         * @return 本节点（便于链式调用）。
         *
         * @note 在树遍历期间（process / layout / paint 等阶段）这个操作会被**延迟**
         *       到本帧的安全提交点执行，因此调用后立刻查询 `child->parent()` 可能
         *       仍是旧父节点；需要精确同步语义时请查询 is_reparent_deferred()。
         */
        auto reparent(const std::shared_ptr<NanNode>& child, std::size_t index = npos) -> NanNode&;

        /// reparent() 未指定位置时的默认值：追加到末尾。
        static constexpr std::size_t npos = static_cast<std::size_t>(-1);

        /// True when the most recent reparent() call was deferred to the next commit.
        [[nodiscard]] auto is_reparent_deferred() const noexcept -> bool;

        /// Reorder an attached child without exit/enter/ready notifications.
        /// Returns false when child is not attached to this parent.
        auto move_child(NanNode& child, std::size_t index) -> bool;

        /**
         * 本节点是否可以作为拖放的容器 —— **显式属性，默认 false**。
         *
         * 默认必须为 false：`DragController` 从命中节点沿祖先链向上找落点，若默认
         * 为 true，第一个祖先容器（往往是不相关的行/页面）就会被当成落点，与
         * "拖到那个列表里"的直觉不符。容器需要显式声明自己接受放置：
         *
         * ```cpp
         * list->set_accepts_drop(true);   // 或用 DragController::install 的谓词
         * ```
         *
         * 覆写它也可以表达更复杂的条件（例如只读列表按状态返回）。
         */
        [[nodiscard]] virtual auto accepts_drop() const -> bool {
            return accepts_drop_;
        }

        /// 声明本节点是否接受拖放（容器用）。`DragController` 据此筛选落点。
        void set_accepts_drop(bool accepts) noexcept {
            accepts_drop_ = accepts;
        }

        /// 插入位置提示：给定指针位置，返回建议的兄弟插入下标（用于画插入线）。
        /// 默认实现按几何就近选择；返回 npos 表示容器不提供插入提示。
        [[nodiscard]] virtual auto drop_slot_at(foundation::NanPoint /*pointer*/) const
            -> std::size_t {
            return npos;
        }

        /**
         * Remove a child node and return ownership to the caller.
         * @return The removed child as shared_ptr, or nullptr if not found.
         */
        auto remove_child(NanNode& child) -> std::shared_ptr<NanNode>;

        /// Replace an existing child without returning its ownership. During tree
        /// traversal the complete replacement is deferred to the next safe commit.
        auto replace_child(NanNode* current, std::shared_ptr<NanNode> replacement) -> NanNode&;

        /// Remove and immediately destroy a child.
        void remove_and_delete(NanNode& child);

        // ---- name ----

        [[nodiscard]] auto name() const -> std::string_view;
        void set_name(std::string name);

        // ---- accessibility semantics ----

        [[nodiscard]] auto semantics_id() const noexcept -> semantics::SemanticsId;
        void set_semantics_composition(semantics::Composition composition);
        [[nodiscard]] auto semantics_composition() const noexcept -> semantics::Composition;
        void set_semantics_override(semantics::Properties properties);
        void clear_semantics_override();
        [[nodiscard]] auto resolved_semantics_properties() const -> semantics::Properties;
        void mark_semantics_dirty();

        /// Paint-only invalidation: marks the nearest `NanControl` ancestor (or this
        /// node itself when it is a Control) paint-dirty. Node-local visual changes
        /// such as opacity use this instead of touching layout/semantics.
        void mark_paint_dirty();

        // ---- lifecycle (override in subclasses) ----

        /// Called when the node enters the tree (top-down: parent before children).
        virtual void on_enter_tree();

        /// Called after all descendants' on_enter_tree have completed (bottom-up).
        virtual void on_ready();

        /// Called before the node (and all descendants) leave the tree.
        virtual void on_exit_tree();

        /**
         * Handle an input event routed to this node via hit-testing.
         * Return true if the event was consumed (stops bubbling to parent).
         * Default: false (event continues to parent).
         */
        virtual auto on_input(InputEvent& event) -> bool;

        /**
         * Observe input while it travels from the root toward the hit target.
         * Containers can use this even when a descendant consumes bubbling.
         */
        virtual auto on_input_capture(InputEvent& event) -> bool;

        /// Called every frame.  dt is the elapsed time in seconds.
        virtual void on_process(float dt);

        /// Optional fixed-step subsystem hook. Base nodes do nothing.
        virtual void physics_step(float dt);

        /// Called during draw traversal (top-down: parent drawn before children).
        /// The context carries the world transform, inherited opacity, and clip stack.
        virtual void on_draw(render::DrawContext& ctx);

        /// Hint for sibling draw/hit-test ordering (higher = on top).
        /// Override in subclasses that have a z-ordering concept.
        [[nodiscard]] virtual auto z_index_hint() const -> int;

        /// 子树内最大 z_index_hint（含自身）。用于把「打开的浮层」提升到祖先链的
        /// 兄弟排序之上，使弹出内容（dropdown/dialog）能盖住后续兄弟。
        [[nodiscard]] auto subtree_z_index_hint() const -> int;

        /// True if this node can become the focus target.
        [[nodiscard]] virtual auto is_focusable() const -> bool;

        /**
         * Focus target to use when a click lands on this node or its subtree.
         * Floating content lives in another subtree than the control it belongs to;
         * pointing back at that control keeps the click from clearing focus, so using
         * the floating content does not blur its owner. Default: none.
         */
        [[nodiscard]] virtual auto focus_delegate() const -> NanNode2D*;

        /// True if this node should be drawn / hit-tested in the active tree.
        /// A false return means this node AND all descendants are skipped.
        [[nodiscard]] virtual auto is_visible_in_tree() const -> bool;

        /// Local alpha in [0,1] applied once to this node during draw traversal.
        /// Base nodes are opaque (1.0); NanNode2D overrides to expose a mutable value.
        /// The DrawContext effective opacity is `parent effective × local_opacity()`.
        [[nodiscard]] virtual auto local_opacity() const -> float;

        /// Safe down-cast to NanNode2D without RTTI.
        /// Base returns nullptr; NanNode2D overrides to return itself.
        /// Prefer this over dynamic_cast for traversal-time type discrimination.
        [[nodiscard]] virtual auto as_node2d() -> NanNode2D* {
            return nullptr;
        }
        [[nodiscard]] virtual auto as_node2d() const -> const NanNode2D* {
            return nullptr;
        }

        /// Safe down-cast to NanControl without RTTI.
        [[nodiscard]] virtual auto as_control() -> NanControl* {
            return nullptr;
        }
        [[nodiscard]] virtual auto as_control() const -> const NanControl* {
            return nullptr;
        }

        [[nodiscard]] virtual auto as_canvas_layer() -> CanvasLayer* {
            return nullptr;
        }
        [[nodiscard]] virtual auto as_canvas_layer() const -> const CanvasLayer* {
            return nullptr;
        }
        [[nodiscard]] virtual auto as_layer_stack() -> LayerStack* {
            return nullptr;
        }
        [[nodiscard]] virtual auto as_layer_stack() const -> const LayerStack* {
            return nullptr;
        }

        /// Parent-specific child admission hook. Base nodes accept any child that
        /// satisfies the NanNode/NanNode2D edge rule.
        [[nodiscard]] virtual auto accepts_child(const NanNode& child) const -> bool {
            return true;
        }

        virtual void apply_default_text_pipeline(const widget::primitives::TextPipeline& pipeline);
        virtual void apply_font_context(text::FontPipelineCache& context);
        virtual void apply_texture_cache(render::TextureCache& cache);

        virtual void on_style_context_changed(const theme::ResolvedStyleContext& context);
        virtual void on_theme_changed(const theme::ThemeManager& manager);
        virtual void on_theme_context_removed();
        [[nodiscard]] virtual auto semantics_properties() const -> semantics::Properties;
        virtual auto on_semantics_action(const semantics::ActionRequest& request) -> bool;

    protected:
        /// Internal: set the owning scene tree (called by NanSceneTree).
        void _set_tree(NanSceneTree* tree);

        /// Internal: propagate enter_tree to this node and all descendants.
        void _propagate_enter_tree(NanSceneTree* tree);

        /// Internal: propagate ready to all descendants then this node (bottom-up).
        void _propagate_ready();

        /// Internal: propagate exit_tree to this node and all descendants.
        void _propagate_exit_tree();

        /// Internal: process this node then recursively process children.
        void _propagate_process(float dt);
        void _propagate_physics(float dt);
        void _resolve_style_context(const theme::ResolvedStyleContext* inherited);
        void _propagate_theme_changed(const theme::ThemeManager& manager);
        void _propagate_theme_context_removed();

        /// Internal: draw this node then recursively draw children (top-down).
        /// Threads world transform + inherited opacity + clip via the context.
        void _propagate_draw(render::DrawContext& ctx);

        /// Internal hook: update ctx world transform for this node before drawing.
        /// Base is identity (no spatial transform); NanNode2D composes its local
        /// transform onto the parent world. Returns the parent world to restore.
        /// Avoids RTTI in the traversal by using a virtual instead of a cast.
        [[nodiscard]] virtual auto _push_draw_transform(render::DrawContext& ctx)
            -> foundation::NanTransform2D;
        virtual void
        _pop_draw_transform(render::DrawContext& ctx, const foundation::NanTransform2D& saved);

        /// Internal hook: optionally push a clip before drawing children.
        /// Base nodes do not clip. Controls override this for overflow policy.
        [[nodiscard]] virtual auto _push_child_clip(render::DrawContext& ctx)
            -> render::ClipStack::Guard;

    private:
        // parent is a non-owning back-reference (children own parent would be a
        // cycle); weak_ptr expires gracefully if a detached child outlives its parent.
        std::weak_ptr<NanNode> parent_;
        std::vector<std::shared_ptr<NanNode>> children_;
        // tree_ is a non-owning back-pointer: NanSceneTree owns the root and always
        // outlives the nodes, so a raw pointer is safe here (a tree is not a node).
        NanSceneTree* tree_ = nullptr;
        std::string name_;
        theme::StyleContext style_context_;
        theme::ResolvedStyleContext resolved_style_context_;
        semantics::SemanticsId semantics_id_ = 0;
        semantics::Composition semantics_composition_ = semantics::Composition::automatic;
        std::optional<semantics::Properties> semantics_override_;

        /// True once on_ready() has fired for the current tree membership.
        /// Guards against double-ready when children are added during on_enter_tree().
        /// Reset on exit_tree so a removed + re-added node readies again.
        bool ready_notified_ = false;

        /// True when the last reparent() call had to be queued (tree traversal phase).
        bool reparent_deferred_ = false;

        /// Explicit drop-container opt-in (see accepts_drop()).
        bool accepts_drop_ = false;
    };

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_NODE_HPP
