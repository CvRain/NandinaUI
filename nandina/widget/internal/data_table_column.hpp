//
// widget/internal/data_table_column — DataTable 列定义（批 2B 内部协议）。
//
// 现有静态 Table 的 TableColumn 保持不变，只作为轨道定义嵌入。列是 DataTable 的
// 二维结构唯一来源：轨道、可见性、排序策略/比较器、delegate 与默认文本取值器都在这里。
// 所有模型类型相关字段都携带无 RTTI 令牌，set_columns/set_model 在提交前统一校验。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_TABLE_COLUMN_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_TABLE_COLUMN_HPP

#include "../../widget/table.hpp"
#include "data_identity.hpp"
#include "item_delegate.hpp"
#include "row_model.hpp"

#include <compare>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace nandina::widget
{
    class IHeaderDelegate;

    enum class SortDirection : std::uint8_t { ascending, descending };
    enum class SortStrategy : std::uint8_t { none, local, external };

    /// 当前排序状态（按 ColumnId）。默认相等、方向默认 ascending。
    struct SortSpec {
        ColumnId column;
        SortDirection direction = SortDirection::ascending;

        friend auto operator==(const SortSpec&, const SortSpec&) -> bool = default;
    };

    enum class SortRequestReason : std::uint8_t { user_header_click, programmatic };

    /// 排序请求事件载荷；nullopt = 清除外部排序。reason 区分表头点击与程序化操作。
    struct SortRequest {
        std::optional<SortSpec> sort;
        SortRequestReason reason = SortRequestReason::programmatic;
    };

    /// 列取值器：token 与 text 必须同时有值或同时为空。
    struct ColumnValue {
        const void* model_token = nullptr;
        std::function<std::string(const IRowModel&, std::size_t model_row)> text;
    };

    /// 本地排序比较器：token 与 compare 同样必须成对出现。
    struct ColumnComparator {
        const void* model_token = nullptr;
        std::function<
            std::weak_ordering(const IRowModel&, std::size_t lhs_row, std::size_t rhs_row)>
            compare;
    };

    struct DataTableColumn {
        ColumnId id;
        std::string title;
        TableColumn track; // 固定/权重轨道，复用静态 Table 定义
        bool visible = true; // false：可排序、不进布局/命中/选择/导航
        SortStrategy sort = SortStrategy::none;
        ColumnComparator comparator; // local 使用；external/none 为空
        std::shared_ptr<IItemDelegate> delegate; // 空 = 默认 TextItemDelegate
        std::shared_ptr<IHeaderDelegate> header_delegate; // 空 = 默认 TextHeaderDelegate
        ColumnValue value; // 默认 delegate 的文本来源（构造糖填充）
    };

    /// 类型化取值器糖：框架负责类型擦除与令牌，作者不写 static_cast。
    template<typename ModelT>
        requires std::derived_from<ModelT, IRowModel>
    auto text_column(
        ColumnId id,
        std::string title,
        std::function<std::string(const ModelT&, std::size_t model_row)> value,
        TableColumn track = {},
        bool visible = true
    ) -> DataTableColumn {
        return DataTableColumn {
            .id = std::move(id),
            .title = std::move(title),
            .track = track,
            .visible = visible,
            .value = ColumnValue {
                .model_token = model_type_token_of<ModelT>(),
                .text = [value =
                             std::move(value)](const IRowModel& model, const std::size_t model_row)
                    -> std::string { return value(static_cast<const ModelT&>(model), model_row); },
            },
        };
    }

    /// 类型化本地排序糖：同时写入 sort=local、比较器与令牌。
    template<typename ModelT>
        requires std::derived_from<ModelT, IRowModel>
    void set_local_sort(
        DataTableColumn& column,
        std::function<std::weak_ordering(const ModelT&, std::size_t lhs_row, std::size_t rhs_row)>
            compare
    ) {
        column.sort = SortStrategy::local;
        column.comparator = ColumnComparator {
            .model_token = model_type_token_of<ModelT>(),
            .compare = [compare = std::move(compare)](
                           const IRowModel& model,
                           const std::size_t lhs_row,
                           const std::size_t rhs_row
                       ) -> std::weak_ordering {
                return compare(static_cast<const ModelT&>(model), lhs_row, rhs_row);
            },
        };
    }

    /// 自定义列没有取值器：内容完全由 delegate 提供，模型类型由 delegate token 表达。
    inline auto custom_column(
        ColumnId id,
        std::string title,
        TableColumn track,
        std::shared_ptr<IItemDelegate> delegate,
        bool visible = true
    ) -> DataTableColumn {
        return DataTableColumn {
            .id = std::move(id),
            .title = std::move(title),
            .track = track,
            .visible = visible,
            .delegate = std::move(delegate),
        };
    }
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_TABLE_COLUMN_HPP
