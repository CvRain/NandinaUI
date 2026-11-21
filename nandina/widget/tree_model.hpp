//
// widget/tree_model — 树形行模型契约（experimental，批 6）。
//
// ITreeModel 在 IRowModel 之上补充父子结构：IRowModel::row_count()/row_id() 以先序
// 枚举**全部节点**（不要求节点连续排列），本接口提供 parent/child 查询。展开状态由
// TreeView 持有（视图拥有），模型只描述结构，首批不做懒加载。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TREE_MODEL_HPP
#define NANDINA_EXPERIMENT_WIDGET_TREE_MODEL_HPP

#include "../reactive/event.hpp"
#include "row_model.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>

namespace nandina::widget
{
    /// 树形行模型。模型必须无环；`parent(row)` 返回 nullopt 表示根节点。
    /// `child_count`/`child` 只对已知 RowId 调用；`has_children`/`depth` 有默认实现。
    class ITreeModel: public IRowModel {
    public:
        ~ITreeModel() override = default;

        [[nodiscard]] virtual auto parent(RowId row) const -> std::optional<RowId> = 0;
        [[nodiscard]] virtual auto child_count(RowId row) const -> std::size_t = 0;
        [[nodiscard]] virtual auto child(RowId row, std::size_t index) const -> RowId = 0;

        /// 是否有子节点；首批不做懒加载，默认等价于 child_count > 0。
        [[nodiscard]] virtual auto has_children(RowId row) const -> bool {
            return child_count(row) > 0;
        }

        /// 根到该节点的边数；默认沿 parent 行走，模型可覆盖为 O(1)。
        [[nodiscard]] virtual auto depth(RowId row) const -> std::size_t {
            std::size_t result = 0;
            for (auto current = parent(row); current.has_value(); current = parent(*current)) {
                ++result;
            }
            return result;
        }
    };

    /// 与 TypedRowModelBase 同形的树模型基类：事件存储 + 无 RTTI 类型令牌。
    /// 结构与 RowModelBase 的通知契约一致：reset 表示结构/行数变化，data_changed
    /// 只表示内容变化且不改结构。
    template<typename Derived>
    class TypedTreeModelBase: public ITreeModel {
    public:
        TypedTreeModelBase() = default;
        ~TypedTreeModelBase() override = default;

        // 与 RowModelBase 一致：Event 拷贝会共享订阅状态，模型统一由 shared_ptr 持有。
        TypedTreeModelBase(const TypedTreeModelBase&) = delete;
        auto operator=(const TypedTreeModelBase&) -> TypedTreeModelBase& = delete;
        TypedTreeModelBase(TypedTreeModelBase&&) = delete;
        auto operator=(TypedTreeModelBase&&) -> TypedTreeModelBase& = delete;

        [[nodiscard]] auto model_type_token() const noexcept -> const void* override {
            return model_type_token_of<Derived>();
        }
        [[nodiscard]] auto reset_event() const noexcept -> const reactive::Event<>& override {
            return reset_event_;
        }
        [[nodiscard]] auto data_changed_event() const noexcept
            -> const reactive::Event<RowRange>& override {
            return data_changed_event_;
        }

    protected:
        void notify_reset() {
            reset_event_.emit();
        }

        void notify_data_changed(const RowRange range) {
            if (range.first > range.last_exclusive || range.last_exclusive > row_count()) {
                throw std::invalid_argument("TreeModelBase data_changed range is invalid");
            }
            if (range.first == range.last_exclusive) {
                return;
            }
            data_changed_event_.emit(range);
        }

    private:
        reactive::Event<> reset_event_;
        reactive::Event<RowRange> data_changed_event_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TREE_MODEL_HPP
