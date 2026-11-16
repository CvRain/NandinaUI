#include "item_host.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace nandina::widget
{
    ItemHost::ItemHost(const widget::BuildContext& ui): ui_(ui) {
        set_overflow(scene::ControlOverflow::clip);
    }

    auto ItemHost::binding_for(const ItemRequest& request) -> ItemBinding {
        if (request.model == nullptr) {
            throw std::invalid_argument("ItemHost request has no model");
        }
        return ItemBinding {
            .model = *request.model,
            .model_row = request.model_row,
            .display_index = request.display_index,
            .row = request.row,
            .column = request.column,
            .state = request.state,
        };
    }

    auto ItemHost::require_delegate(const ItemRequest& request)
        -> const std::shared_ptr<IItemDelegate>& {
        if (!request.delegate) {
            throw std::invalid_argument("ItemHost request has no delegate");
        }
        return request.delegate;
    }

    void ItemHost::set_items(const std::vector<ItemRequest>& items) {
        // 先把请求整体校验完，避免后一项非法时前面的 create/update 已经产生副作用。
        std::unordered_set<CellId, internal::CellIdHash> request_ids;
        request_ids.reserve(items.size());
        for (const auto& request: items) {
            if (!request_ids.insert(request.id).second) {
                throw std::invalid_argument("ItemHost item ids must be unique");
            }
            (void)require_delegate(request);
            (void)binding_for(request);
        }

        const bool recreate_all = scopes_released_;
        std::vector<bool> reused(entries_.size(), false);
        std::vector<std::size_t> existing(items.size(), entries_.size());
        std::vector<std::unique_ptr<Entry>> created(items.size());
        std::unordered_set<const scene::NanControl*> created_nodes;
        created_nodes.reserve(items.size());
        std::unordered_map<CellId, std::size_t, internal::CellIdHash> next_index;
        next_index.reserve(items.size());
        for (std::size_t item = 0; item < items.size(); ++item) {
            next_index.emplace(items[item].id, item);
        }

        // 先创建缺失项（create/update 都成功后才提交），已有项只做 update。退出树后
        // 所有旧 scope 已清空；即使 CellId 和 delegate 都未变，也必须完整重建才能恢复
        // create_item() 安装的 effect/subscription。
        for (std::size_t item = 0; item < items.size(); ++item) {
            const auto& request = items[item];
            const auto found = index_.find(request.id);
            if (!recreate_all && found != index_.end()
                && entries_[found->second].delegate == request.delegate)
            {
                reused[found->second] = true;
                existing[item] = found->second;
                continue;
            }
            const auto& delegate = request.delegate;
            auto scope = std::make_unique<reactive::ReactiveScope>(ui_.graph());
            auto node = delegate->create_item(ui_.with_scope(*scope), binding_for(request));
            if (!node) {
                throw std::runtime_error("Item delegate create_item returned null");
            }
            if (node.get() == this || node->parent() != nullptr || node->is_inside_tree()
                || node->is_ancestor_of(*this) || !created_nodes.insert(node.get()).second)
            {
                throw std::logic_error(
                    "Item delegate create_item must return a unique detached control"
                );
            }
            delegate->update_item(*node, binding_for(request));
            created[item] = std::make_unique<Entry>(Entry {
                .id = request.id,
                .node = std::move(node),
                .delegate = delegate,
                .scope = std::move(scope),
            });
        }

        // 已有项 update：失败时 entries_ 尚未改动，索引/所有权仍一致。
        for (std::size_t item = 0; item < items.size(); ++item) {
            if (existing[item] == entries_.size()) {
                continue;
            }
            const auto& request = items[item];
            const auto& delegate = request.delegate;
            delegate->update_item(*entries_[existing[item]].node, binding_for(request));
        }

        std::vector<Entry> next;
        next.reserve(items.size());
        for (std::size_t item = 0; item < items.size(); ++item) {
            if (existing[item] != entries_.size()) {
                next.push_back(std::move(entries_[existing[item]]));
            }
            else {
                next.push_back(std::move(*created[item]));
            }
        }

        for (auto& entry: next) {
            if (entry.node->parent() != this) {
                add_child(entry.node);
            }
        }
        for (std::size_t index = 0; index < entries_.size(); ++index) {
            if (reused[index]) {
                continue;
            }
            if (entries_[index].node && entries_[index].node->parent() == this) {
                // 遍历期也可能是 set_items 的调用点，remove_and_delete 会安全延迟。
                remove_and_delete(*entries_[index].node);
            }
            entries_[index].scope->clear();
        }

        entries_ = std::move(next);
        index_ = std::move(next_index);
        scopes_released_ = false;
        mark_layout_dirty();
    }

    void ItemHost::update_items(const std::vector<ItemRequest>& items) {
        for (const auto& request: items) {
            if (const auto found = index_.find(request.id);
                found != index_.end() && entries_[found->second].delegate == request.delegate)
            {
                const auto& delegate = request.delegate;
                delegate->update_item(*entries_[found->second].node, binding_for(request));
            }
        }
    }

    void ItemHost::reset_items() {
        for (auto& entry: entries_) {
            if (entry.node && entry.node->parent() == this) {
                remove_and_delete(*entry.node);
            }
            if (entry.scope) {
                entry.scope->clear();
            }
        }
        entries_.clear();
        index_.clear();
        scopes_released_ = false;
        mark_layout_dirty();
    }

    void ItemHost::release_scopes() {
        for (auto& entry: entries_) {
            if (entry.scope) {
                entry.scope->clear();
            }
        }
        scopes_released_ = true;
    }

    void ItemHost::on_layout() {
        // DataTable measures and positions each delegate control with its final column width.
    }

    auto ItemHost::node_for(const CellId& id) const -> scene::NanControl* {
        const auto found = index_.find(id);
        return found == index_.end() ? nullptr : entries_[found->second].node.get();
    }

    auto ItemHost::item_count() const noexcept -> std::size_t {
        return entries_.size();
    }

    void ItemHost::on_exit_tree() {
        release_scopes();
        scene::NanControl::on_exit_tree();
    }
} // namespace nandina::widget
