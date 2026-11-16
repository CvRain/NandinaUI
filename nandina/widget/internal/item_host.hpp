//
// widget/internal/item_host — delegate 控件 host（批 2B，以 CellId 为 key）。
//
// 负责 create/update/销毁/keyed 复用与每项 scope；布局由 DataTable 通过 node_for() +
// layout_to() 完成。create 失败不留下半挂载节点或 scope；update 失败保持索引与所有权
// 一致（delegate 已执行的控件写入不承诺回滚）。退出树清理 item scope，重入树由
// DataTable 执行完整 reset 重建。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_ITEM_HOST_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_ITEM_HOST_HPP

#include "../../reactive/scope.hpp"
#include "../../scene/control.hpp"
#include "../../widget/build_context.hpp"
#include "data_identity.hpp"
#include "item_delegate.hpp"
#include "row_model.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace nandina::widget
{
    class ItemHost final: public scene::NanControl {
    public:
        /// 一次物化请求：DataTable 把显示序/身份/状态与 delegate 一起交给 host。
        struct ItemRequest {
            CellId id;
            const IRowModel* model = nullptr;
            std::size_t model_row = 0;
            ModelIndex display_index;
            RowId row;
            const ColumnId* column = nullptr;
            ItemState state;
            std::shared_ptr<IItemDelegate> delegate;
        };

        explicit ItemHost(const widget::BuildContext& ui);

        /// 按键对账物化：复用同 CellId 的节点并走 update；缺失的创建；多余的销毁。
        /// create/update 抛异常时索引与已有条目保持一致，不留下半挂载节点。
        void set_items(const std::vector<ItemRequest>& items);

        /// 只更新已存在条目（data_changed 快速路径）；缺失键忽略。
        void update_items(const std::vector<ItemRequest>& items);

        /// 清空控件与条目（重入树完整 reset 用；可在遍历期调用）。
        void reset_items();

        /// 退出树：取消每项 scope 上的 effect/subscription，但保留节点。
        void release_scopes();

        [[nodiscard]] auto node_for(const CellId& id) const -> scene::NanControl*;
        [[nodiscard]] auto item_count() const noexcept -> std::size_t;

    protected:
        /// DataTable owns the grid placement; the host must not lay items out with host-wide
        /// constraints before the final per-column rectangles are known.
        void on_layout() override;
        void on_exit_tree() override;

    private:
        struct Entry {
            CellId id;
            std::shared_ptr<scene::NanControl> node;
            std::shared_ptr<IItemDelegate> delegate;
            std::unique_ptr<reactive::ReactiveScope> scope;
        };

        [[nodiscard]] static auto binding_for(const ItemRequest& request) -> ItemBinding;
        [[nodiscard]] static auto require_delegate(const ItemRequest& request)
            -> const std::shared_ptr<IItemDelegate>&;

        widget::BuildContext ui_;
        std::vector<Entry> entries_;
        std::unordered_map<CellId, std::size_t, internal::CellIdHash> index_;
        /// on_exit_tree() clears per-item reactive scopes. A later set_items() must recreate
        /// delegate items, because update_item() is not allowed to install effects/subscriptions.
        bool scopes_released_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_ITEM_HOST_HPP
