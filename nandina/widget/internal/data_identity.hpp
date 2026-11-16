//
// widget/internal/data_identity — 数据视图的稳定身份与显示序坐标（批 2A 内部协议）。
//
// 2A 先放在 internal/ 且头文件表面登记为 internal；批 3 导出 DataTable 时再一起转为
// experimental，因此当前不承诺 std::hash 是稳定的公共 API（2A 只提供相等语义）。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_IDENTITY_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_IDENTITY_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace nandina::widget
{
    /// 模型行的稳定身份。由 IRowModel::row_id() 提供，插入/排序/过滤后保持不变。
    struct RowId {
        std::uint64_t value = 0;

        friend auto operator==(const RowId&, const RowId&) -> bool = default;
    };

    /// 列的稳定身份。跨列插删、持久化、排序状态都按 id 而不是索引。
    struct ColumnId {
        std::string value;

        friend auto operator==(const ColumnId&, const ColumnId&) -> bool = default;
    };

    /// 单元格身份 = 行身份 + 列身份。选择、current、anchor 全部按身份存储。
    struct CellId {
        RowId row;
        ColumnId column;

        friend auto operator==(const CellId&, const CellId&) -> bool = default;
    };

    /// 显示序坐标（排序/过滤后的行、列位置），不是身份。与身份分开存储。
    struct ModelIndex {
        std::size_t row = 0;
        std::size_t column = 0;

        friend auto operator==(const ModelIndex&, const ModelIndex&) -> bool = default;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_DATA_IDENTITY_HPP
