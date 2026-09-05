//
// widget/declarative - low-level reactive regions over concrete scene widgets.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_DECLARATIVE_HPP
#define NANDINA_EXPERIMENT_WIDGET_DECLARATIVE_HPP

#include "../reactive/effect.hpp"
#include "../reactive/graph.hpp"
#include "../reactive/scope.hpp"
#include "../scene/control.hpp"
#include "layout.hpp"

#include <algorithm>
#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nandina::widget
{

    /// Keyed list region. Each key owns one concrete control and one reactive scope.
    /// Reordering preserves the node, its focus/edit state, and its mounted lifecycle.
    template<typename Item, std::equality_comparable Key, typename NodeT = scene::NanControl>
        requires std::derived_from<NodeT, scene::NanControl>
    class ForEach: public Flex {
    public:
        using KeyFunction = std::function<Key(const Item&)>;
        using CreateFunction =
            std::function<std::shared_ptr<NodeT>(reactive::ReactiveScope&, const Item&)>;
        using UpdateFunction = std::function<void(NodeT&, const Item&)>;

        ForEach(
            reactive::Graph& graph,
            KeyFunction key,
            CreateFunction create,
            UpdateFunction update = {},
            LayoutAxis axis = LayoutAxis::vertical
        ):
            Flex(axis),
            graph_(&graph),
            binding_scope_(graph),
            key_(std::move(key)),
            create_(std::move(create)),
            update_(std::move(update)) {
            if (!key_ || !create_) {
                throw std::invalid_argument("ForEach requires key and create functions");
            }
        }

        [[nodiscard]] static auto create(
            reactive::Graph& graph,
            KeyFunction key,
            CreateFunction create,
            UpdateFunction update = {},
            LayoutAxis axis = LayoutAxis::vertical
        ) -> std::shared_ptr<ForEach> {
            return std::make_shared<ForEach>(
                graph,
                std::move(key),
                std::move(create),
                std::move(update),
                axis
            );
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::same_as<const std::vector<Item>&>;
            }
        void bind(Source& source) {
            binding_ = [&source](reactive::EffectScope& scope, ForEach& region) {
                scope.add([&source, &region] { region.set_items(source.get()); });
            };
            if (is_inside_tree()) {
                activate_binding();
            }
        }

        void set_items(const std::vector<Item>& items) {
            std::vector<Key> keys;
            keys.reserve(items.size());
            std::unordered_map<Key, std::size_t> next_indices;
            next_indices.reserve(items.size());
            for (const auto& item: items) {
                auto key = key_(item);
                if (!next_indices.try_emplace(key, keys.size()).second) {
                    throw std::invalid_argument("ForEach keys must be unique");
                }
                keys.push_back(std::move(key));
            }

            std::vector<bool> reused(entries_.size(), false);
            std::vector<std::size_t> existing_entries(items.size(), entries_.size());
            std::vector<std::unique_ptr<Entry>> created_entries(items.size());
            for (std::size_t item_index = 0; item_index < items.size(); ++item_index) {
                const auto existing = find_entry(keys[item_index], reused);
                if (existing != entries_.size()) {
                    reused[existing] = true;
                    existing_entries[item_index] = existing;
                    continue;
                }

                auto scope = std::make_unique<reactive::ReactiveScope>(*graph_);
                auto node = create_(*scope, items[item_index]);
                if (!node) {
                    throw std::runtime_error("ForEach create function returned null");
                }
                if (update_) {
                    update_(*node, items[item_index]);
                }
                created_entries[item_index] = std::make_unique<Entry>(Entry {
                    .key = keys[item_index],
                    .node = std::move(node),
                    .scope = std::move(scope),
                });
            }

            for (std::size_t item_index = 0; item_index < items.size(); ++item_index) {
                if (existing_entries[item_index] != entries_.size() && update_) {
                    update_(*entries_[existing_entries[item_index]].node, items[item_index]);
                }
            }
            for (auto& created: created_entries) {
                if (created) {
                    Flex::add(created->node);
                }
            }

            std::vector<Entry> next;
            next.reserve(items.size());
            for (std::size_t item_index = 0; item_index < items.size(); ++item_index) {
                if (existing_entries[item_index] != entries_.size()) {
                    next.push_back(std::move(entries_[existing_entries[item_index]]));
                }
                else {
                    next.push_back(std::move(*created_entries[item_index]));
                }
            }

            for (std::size_t index = 0; index < entries_.size(); ++index) {
                if (reused[index]) {
                    continue;
                }
                if (entries_[index].node && entries_[index].node->parent() == this) {
                    (void)remove_child(*entries_[index].node);
                }
                entries_[index].scope->clear();
            }

            entries_ = std::move(next);
            indices_ = std::move(next_indices);
            for (std::size_t index = 0; index < entries_.size(); ++index) {
                (void)move_child(*entries_[index].node, index);
            }
        }

        void clear_items() {
            set_items({});
        }

        [[nodiscard]] auto item_count() const -> std::size_t {
            return entries_.size();
        }

        [[nodiscard]] auto node_for(const Key& key) const -> NodeT* {
            const auto found = indices_.find(key);
            return found == indices_.end() ? nullptr : entries_[found->second].node.get();
        }

    protected:
        void on_ready() override {
            Flex::on_ready();
            activate_binding();
        }

        void on_exit_tree() override {
            binding_scope_.clear();
            binding_ = {};
            for (auto& entry: entries_) {
                entry.scope->clear();
            }
            Flex::on_exit_tree();
        }

    private:
        struct Entry {
            Key key;
            std::shared_ptr<NodeT> node;
            std::unique_ptr<reactive::ReactiveScope> scope;
        };

        [[nodiscard]] auto find_entry(const Key& key, const std::vector<bool>& reused) const
            -> std::size_t {
            const auto found = indices_.find(key);
            if (found != indices_.end() && !reused[found->second]) {
                return found->second;
            }
            return entries_.size();
        }

        void activate_binding() {
            binding_scope_.clear();
            if (binding_) {
                binding_(binding_scope_, *this);
            }
        }

        reactive::Graph* graph_;
        reactive::EffectScope binding_scope_;
        KeyFunction key_;
        CreateFunction create_;
        UpdateFunction update_;
        std::function<void(reactive::EffectScope&, ForEach&)> binding_;
        std::vector<Entry> entries_;
        std::unordered_map<Key, std::size_t> indices_;
    };

    /// Conditional single-child region. The active branch owns a dedicated scope.
    template<typename NodeT = scene::NanControl>
        requires std::derived_from<NodeT, scene::NanControl>
    class IfRegion final: public scene::NanControl {
    public:
        using CreateFunction = std::function<std::shared_ptr<NodeT>(reactive::ReactiveScope&)>;

        IfRegion(reactive::Graph& graph, CreateFunction when_true, CreateFunction when_false = {}):
            graph_(&graph),
            binding_scope_(graph),
            when_true_(std::move(when_true)),
            when_false_(std::move(when_false)) {
            if (!when_true_) {
                throw std::invalid_argument("IfRegion requires a true branch factory");
            }
        }

        [[nodiscard]] static auto
        create(reactive::Graph& graph, CreateFunction when_true, CreateFunction when_false = {})
            -> std::shared_ptr<IfRegion> {
            return std::make_shared<IfRegion>(graph, std::move(when_true), std::move(when_false));
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<bool>;
            }
        void bind(Source& source) {
            binding_ = [&source](reactive::EffectScope& scope, IfRegion& region) {
                scope.add([&source, &region] {
                    region.set_condition(static_cast<bool>(source.get()));
                });
            };
            if (is_inside_tree()) {
                activate_binding();
            }
        }

        void set_condition(const bool condition) {
            if (condition_ == condition && (active_ != nullptr || !factory_for(condition))) {
                return;
            }

            if (active_ != nullptr && active_->parent() == this) {
                (void)remove_child(*active_);
            }
            active_.reset();
            if (branch_scope_) {
                branch_scope_->clear();
                branch_scope_.reset();
            }
            condition_ = condition;

            auto& factory = factory_for(condition);
            if (!factory) {
                mark_layout_dirty();
                return;
            }
            branch_scope_ = std::make_unique<reactive::ReactiveScope>(*graph_);
            active_ = factory(*branch_scope_);
            if (!active_) {
                branch_scope_.reset();
                throw std::runtime_error("IfRegion branch factory returned null");
            }
            add_child(active_);
        }

        [[nodiscard]] auto condition() const -> bool {
            return condition_;
        }

        [[nodiscard]] auto active_node() const -> NodeT* {
            return active_.get();
        }

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override {
            return active_ != nullptr ? active_->measure_layout(constraints)
                                      : constraints.constrain(foundation::NanSize {});
        }

        auto on_layout() -> void override {
            if (active_ != nullptr) {
                (void)active_->measure_layout(scene::LayoutConstraints::tight(size()));
                active_->layout_to(local_rect());
            }
        }

        void on_ready() override {
            scene::NanControl::on_ready();
            activate_binding();
        }

        void on_exit_tree() override {
            binding_scope_.clear();
            binding_ = {};
            if (branch_scope_) {
                branch_scope_->clear();
            }
            scene::NanControl::on_exit_tree();
        }

    private:
        [[nodiscard]] auto factory_for(const bool condition) -> CreateFunction& {
            return condition ? when_true_ : when_false_;
        }

        void activate_binding() {
            binding_scope_.clear();
            if (binding_) {
                binding_(binding_scope_, *this);
            }
        }

        reactive::Graph* graph_;
        reactive::EffectScope binding_scope_;
        CreateFunction when_true_;
        CreateFunction when_false_;
        std::function<void(reactive::EffectScope&, IfRegion&)> binding_;
        std::unique_ptr<reactive::ReactiveScope> branch_scope_;
        std::shared_ptr<NodeT> active_;
        bool condition_ = false;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_DECLARATIVE_HPP
