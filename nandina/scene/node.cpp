//
// Created by cvrain on 2026/6/30.
//

#include "node.hpp"
#include "../animation/animation_host.hpp"
#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "control.hpp"
#include "node2d.hpp"
#include "scene_tree.hpp"

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <vector>

    namespace nandina::scene
{

    namespace
    {
        std::atomic<semantics::SemanticsId> next_semantics_id {1};
    }

    NanNode::NanNode(): semantics_id_(next_semantics_id.fetch_add(1, std::memory_order_relaxed)) {}

    NanNode::~NanNode() {
        // Remove from tree before destruction (calls on_exit_tree on children).
        if (tree_) {
            _propagate_exit_tree();
        }
        // Children are automatically destroyed by unique_ptr.
    }

    auto NanNode::parent() const -> NanNode* {
        return parent_.lock().get();
    }

    auto NanNode::child_count() const -> size_t {
        return children_.size();
    }

    auto NanNode::get_child(const size_t index) const -> NanNode* {
        if (index < children_.size()) {
            return children_[index].get();
        }
        return nullptr;
    }

    auto NanNode::is_ancestor_of(const NanNode& other) const -> bool {
        for (auto* p = other.parent(); p != nullptr; p = p->parent()) {
            if (p == this) {
                return true;
            }
        }
        return false;
    }

    auto NanNode::is_inside_tree() const -> bool {
        return tree_ != nullptr;
    }

    auto NanNode::get_tree() const -> NanSceneTree* {
        return tree_;
    }

    void NanNode::set_style_context(theme::StyleContext context) {
        style_context_ = std::move(context);
        const auto* inherited = parent() != nullptr ? &parent()->resolved_style_context_ : nullptr;
        _resolve_style_context(inherited);
    }

    void NanNode::clear_style_context() {
        set_style_context({});
    }

    auto NanNode::style_context() const -> const theme::StyleContext& {
        return style_context_;
    }

    auto NanNode::resolved_style_context() const -> const theme::ResolvedStyleContext& {
        return resolved_style_context_;
    }

    auto NanNode::add_child(std::shared_ptr<NanNode> child) -> NanNode& {
        return insert_child(children_.size(), std::move(child));
    }

    auto NanNode::insert_child(const std::size_t index, std::shared_ptr<NanNode> child)
        -> NanNode& {
        if (!child) {
            throw std::runtime_error("NanNode::insert_child: child is null");
        }
        if (!child->parent_.expired()) {
            throw std::runtime_error("NanNode::insert_child: child already has a parent");
        }
        if (child->tree_ != nullptr) {
            throw std::runtime_error("NanNode::insert_child: child is already in a tree");
        }

        const auto* parent_2d = as_node2d();
        const auto* child_2d = child->as_node2d();
        if ((parent_2d != nullptr) != (child_2d != nullptr)) {
            throw std::runtime_error(
                "NanNode::insert_child: cannot mix NanNode and NanNode2D on the same edge"
            );
        }
        if (!accepts_child(*child)) {
            throw std::runtime_error("NanNode::insert_child: parent rejects this child type");
        }

        if (tree_ != nullptr && tree_->defers_tree_mutation()) {
            auto* raw = child.get();
            auto parent = shared_from_this();
            tree_->defer_tree_mutation(
                [parent = std::move(parent), index, child = std::move(child)]() mutable {
                    parent->insert_child(index, std::move(child));
                }
            );
            return *raw;
        }

        child->parent_ = weak_from_this();
        auto* raw = child.get();
        children_.insert(
            children_.begin() + static_cast<std::ptrdiff_t>(std::min(index, children_.size())),
            std::move(child)
        );

        raw->_resolve_style_context(&resolved_style_context_);

        // If this node is already in a tree, propagate enter_tree to the new subtree.
        if (tree_) {
            raw->_propagate_enter_tree(tree_);
            raw->_propagate_ready();
        }

        if (auto* control = as_control(); control != nullptr) {
            control->mark_layout_dirty();
        }
        mark_semantics_dirty();

        return *raw;
    }

    auto NanNode::move_child(NanNode& child, const std::size_t index) -> bool {
        if (child.parent() != this) {
            return false;
        }
        if (tree_ != nullptr && tree_->defers_tree_mutation()) {
            auto parent = shared_from_this();
            auto target = child.weak_from_this();
            tree_->defer_tree_mutation([parent = std::move(parent), target, index] {
                if (auto current = target.lock();
                    current != nullptr && current->parent() == parent.get())
                {
                    (void)parent->move_child(*current, index);
                }
            });
            return true;
        }

        const auto current = std::ranges::find(children_, &child, &std::shared_ptr<NanNode>::get);
        if (current == children_.end()) {
            return false;
        }
        const auto old_index = static_cast<std::size_t>(std::distance(children_.begin(), current));
        const auto target_index = std::min(index, children_.size() - 1);
        if (old_index == target_index) {
            return true;
        }
        auto owned = std::move(*current);
        children_.erase(current);
        children_.insert(
            children_.begin() + static_cast<std::ptrdiff_t>(target_index),
            std::move(owned)
        );
        if (auto* control = as_control(); control != nullptr) {
            control->mark_layout_dirty();
        }
        mark_semantics_dirty();
        return true;
    }

    auto NanNode::remove_child(NanNode& child) -> std::shared_ptr<NanNode> {
        if (tree_ != nullptr && tree_->defers_tree_mutation()) {
            throw std::logic_error(
                "NanNode::remove_child cannot return ownership during tree traversal; "
                "use remove_and_delete or queue_delete"
            );
        }
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (it->get() == &child) {
                // Exit tree before removing.
                if (tree_) {
                    child._propagate_exit_tree();
                }
                child.parent_.reset();
                child._resolve_style_context(nullptr);
                auto result = std::move(*it);
                children_.erase(it);
                if (auto* control = as_control(); control != nullptr) {
                    control->mark_layout_dirty();
                }
                mark_semantics_dirty();
                return result;
            }
        }
        return nullptr;
    }

    auto NanNode::replace_child(NanNode* current, std::shared_ptr<NanNode> replacement)
        -> NanNode& {
        if (!replacement) {
            throw std::runtime_error("NanNode::replace_child: replacement is null");
        }
        if (current == replacement.get()) {
            return *replacement;
        }
        if (tree_ != nullptr && tree_->defers_tree_mutation()) {
            auto* raw = replacement.get();
            auto parent = shared_from_this();
            auto current_weak =
                current != nullptr ? current->weak_from_this() : std::weak_ptr<NanNode> {};
            tree_->defer_tree_mutation([parent = std::move(parent),
                                        current_weak,
                                        replacement = std::move(replacement)]() mutable {
                auto current_shared = current_weak.lock();
                auto* attached =
                    current_shared != nullptr && current_shared->parent() == parent.get()
                    ? current_shared.get()
                    : nullptr;
                parent->replace_child(attached, std::move(replacement));
            });
            return *raw;
        }
        if (current != nullptr && current->parent() == this) {
            remove_and_delete(*current);
        }
        return add_child(std::move(replacement));
    }

    void NanNode::remove_and_delete(NanNode& child) {
        if (tree_ != nullptr && tree_->defers_tree_mutation()) {
            auto parent = shared_from_this();
            auto target = child.shared_from_this();
            tree_->defer_tree_mutation([parent = std::move(parent), target = std::move(target)] {
                if (target->parent() == parent.get()) {
                    parent->remove_and_delete(*target);
                }
            });
            return;
        }
        remove_child(child); // unique_ptr goes out of scope, child is destroyed.
    }

    auto NanNode::name() const -> std::string_view {
        return name_;
    }

    void NanNode::set_name(std::string name) {
        name_ = std::move(name);
    }

    auto NanNode::semantics_id() const noexcept -> semantics::SemanticsId {
        return semantics_id_;
    }

    void NanNode::set_semantics_composition(const semantics::Composition composition) {
        if (semantics_composition_ == composition) {
            return;
        }
        semantics_composition_ = composition;
        mark_semantics_dirty();
    }

    auto NanNode::semantics_composition() const noexcept -> semantics::Composition {
        return semantics_composition_;
    }

    void NanNode::set_semantics_override(semantics::Properties properties) {
        semantics_override_ = std::move(properties);
        mark_semantics_dirty();
    }

    void NanNode::clear_semantics_override() {
        if (!semantics_override_) {
            return;
        }
        semantics_override_.reset();
        mark_semantics_dirty();
    }

    auto NanNode::resolved_semantics_properties() const -> semantics::Properties {
        return semantics_override_.value_or(semantics_properties());
    }

    void NanNode::mark_semantics_dirty() {
        if (tree_ != nullptr) {
            tree_->mark_semantics_dirty();
        }
    }

    auto NanNode::local_opacity() const -> float {
        return 1.0F;
    }

    void NanNode::mark_paint_dirty() {
        for (auto* node = this; node != nullptr; node = node->parent()) {
            if (auto* control = node->as_control(); control != nullptr) {
                control->mark_dirty(DirtyFlags::paint);
                return;
            }
        }
    }

    void NanNode::on_enter_tree() {}
    void NanNode::on_ready() {}
    void NanNode::on_exit_tree() {}

    auto NanNode::on_input(InputEvent& /*event*/) -> bool {
        return false;
    }

    void NanNode::on_process(float /*dt*/) {}
    void NanNode::physics_step(float /*dt*/) {}
    void NanNode::on_draw(render::DrawContext& /*ctx*/) {}

    auto NanNode::z_index_hint() const -> int {
        return 0;
    }

    auto NanNode::subtree_z_index_hint() const -> int {
        int max_z = z_index_hint();
        for (const auto& child: children_) {
            max_z = std::max(max_z, child->subtree_z_index_hint());
        }
        return max_z;
    }

    auto NanNode::is_focusable() const -> bool {
        return false;
    }

    auto NanNode::is_visible_in_tree() const -> bool {
        auto* p = parent();
        return p == nullptr || p->is_visible_in_tree();
    }

    void NanNode::apply_default_text_pipeline(
        const widget::primitives::TextPipeline& /*pipeline*/
    ) {}

    void NanNode::apply_font_context(text::FontPipelineCache& /*context*/) {}

    void NanNode::apply_texture_cache(render::TextureCache& /*cache*/) {}

    void NanNode::on_style_context_changed(
        const theme::ResolvedStyleContext& /*context*/
    ) {}

    void NanNode::on_theme_changed(const theme::ThemeManager& /*manager*/) {}

    void NanNode::on_theme_context_removed() {}

    auto NanNode::semantics_properties() const -> semantics::Properties {
        return {};
    }

    auto NanNode::on_semantics_action(const semantics::ActionRequest& /*request*/) -> bool {
        return false;
    }

    void NanNode::_set_tree(NanSceneTree* tree) {
        tree_ = tree;
    }

    void NanNode::_propagate_enter_tree(NanSceneTree* tree) {
        // Idempotent: if a user calls add_child() from inside on_enter_tree(),
        // add_child already propagated enter_tree into the new subtree (because
        // tree_ was set). The parent's own children loop must not enter it again.
        if (tree_ == tree) {
            return;
        }
        // 子类可以在构造函数中组装子树；此时父节点尚未由 shared_ptr 持有，
        // insert_child() 无法取得有效 weak_from_this()。挂树前统一补全父链，
        // 保证全局变换、命中测试和输入冒泡看到真实层级。
        const auto self = weak_from_this();
        for (auto& child: children_) {
            child->parent_ = self;
        }
        tree_ = tree;
        const auto* inherited = parent() != nullptr ? &parent()->resolved_style_context_ : nullptr;
        _resolve_style_context(inherited);
        if (const auto* pipeline = tree->default_text_pipeline(); pipeline != nullptr) {
            apply_default_text_pipeline(*pipeline);
        }
        if (auto* context = tree->font_context(); context != nullptr) {
            apply_font_context(*context);
        }
        if (auto* cache = tree->texture_cache(); cache != nullptr) {
            apply_texture_cache(*cache);
        }
        if (const auto* manager = tree->theme_manager(); manager != nullptr) {
            on_theme_changed(*manager);
        }
        on_enter_tree();
        for (auto& child: children_) {
            child->_propagate_enter_tree(tree);
        }
    }

    void NanNode::_propagate_ready() {
        // Children first (bottom-up).
        for (auto& child: children_) {
            child->_propagate_ready();
        }
        // Idempotent: a node added during on_enter_tree() is readied immediately by
        // add_child; the later top-level ready sweep must not fire on_ready twice.
        if (!ready_notified_) {
            ready_notified_ = true;
            on_ready();
        }
    }

    void NanNode::_propagate_exit_tree() {
        if (tree_ != nullptr) {
            tree_->animation_host().cancel_owner(*this);
        }
        on_exit_tree();
        on_theme_context_removed();
        for (auto& child: children_) {
            child->_propagate_exit_tree();
        }
        tree_ = nullptr;
        ready_notified_ = false; // Allow re-entry (remove + re-add) to fire ready again.
    }

    void NanNode::_propagate_process(const float dt) {
        on_process(dt);
        for (auto& child: children_) {
            child->_propagate_process(dt);
        }
    }

    void NanNode::_propagate_physics(const float dt) {
        physics_step(dt);
        for (auto& child: children_) {
            child->_propagate_physics(dt);
        }
    }

    void NanNode::_resolve_style_context(const theme::ResolvedStyleContext* inherited) {
        resolved_style_context_ = theme::resolve_style_context(style_context_, inherited);
        on_style_context_changed(resolved_style_context_);
        for (auto& child: children_) {
            child->_resolve_style_context(&resolved_style_context_);
        }
    }

    void NanNode::_propagate_theme_changed(const theme::ThemeManager& manager) {
        on_theme_changed(manager);
        for (auto& child: children_) {
            child->_propagate_theme_changed(manager);
        }
    }

    void NanNode::_propagate_theme_context_removed() {
        on_theme_context_removed();
        for (auto& child: children_) {
            child->_propagate_theme_context_removed();
        }
    }

    auto NanNode::_push_draw_transform(render::DrawContext& /*ctx*/) -> foundation::NanTransform2D {
        // Base node has no spatial transform; leave ctx.world unchanged.
        // NanNode2D overrides this to compose its local transform onto the parent.
        return {};
    }

    void NanNode::_pop_draw_transform(
        render::DrawContext& /*ctx*/,
        const foundation::NanTransform2D& /*saved*/
    ) {}

    auto NanNode::_push_child_clip(render::DrawContext& /*ctx*/) -> render::ClipStack::Guard {
        return {nullptr, false};
    }

    void NanNode::_propagate_draw(render::DrawContext& ctx) {
        if (!is_visible_in_tree()) {
            return; // Whole subtree skipped (visibility is a subtree property).
        }

        // Update world transform for this node (virtual: no RTTI in traversal).
        const auto saved_world = _push_draw_transform(ctx);
        const float saved_opacity = ctx.opacity_;
        ctx.opacity_ *= local_opacity();

        on_draw(ctx);

        auto child_clip = _push_child_clip(ctx);

        if (!children_.empty()) {
            // z-order: stable_sort keeps insertion order among equal z (unchanged).
            std::vector<size_t> indices(children_.size());
            for (size_t i = 0; i < children_.size(); ++i) {
                indices[i] = i;
            }
            std::stable_sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
                return children_[a]->subtree_z_index_hint() < children_[b]->subtree_z_index_hint();
            });

            for (size_t idx: indices) {
                children_[idx]->_propagate_draw(ctx);
            }
        }

        ctx.opacity_ = saved_opacity;
        _pop_draw_transform(ctx, saved_world);
    }

} // namespace nandina::scene
