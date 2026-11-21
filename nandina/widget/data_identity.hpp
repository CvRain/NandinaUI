//
// widget/data_identity — 数据视图的稳定身份与显示序坐标（experimental）。
//
// 身份值本身是容器语义的稳定身份；std::hash 数值只用于当前进程内哈希容器，
// 不是可持久化的标识。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_DATA_IDENTITY_HPP
#define NANDINA_EXPERIMENT_WIDGET_DATA_IDENTITY_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
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

namespace std
{
    /// 公共哈希承诺：相等身份得到相等哈希，可直接用于 unordered_* 容器。哈希数值
    /// 不承诺跨进程、跨标准库版本或跨框架版本稳定，也不是序列化身份。
    template<>
    struct hash<nandina::widget::RowId> {
        [[nodiscard]] auto operator()(const nandina::widget::RowId& id) const noexcept
            -> std::size_t;
    };

    template<>
    struct hash<nandina::widget::ColumnId> {
        [[nodiscard]] auto operator()(const nandina::widget::ColumnId& id) const noexcept
            -> std::size_t;
    };

    template<>
    struct hash<nandina::widget::CellId> {
        [[nodiscard]] auto operator()(const nandina::widget::CellId& id) const noexcept
            -> std::size_t;
    };
} // namespace std

namespace nandina::widget::internal
{
    /// 内部兼容别名：统一指向公共 std::hash，避免内部与作者容器出现两套哈希语义。
    using RowIdHash = std::hash<RowId>;
    using ColumnIdHash = std::hash<ColumnId>;
    using CellIdHash = std::hash<CellId>;
} // namespace nandina::widget::internal

#endif // NANDINA_EXPERIMENT_WIDGET_DATA_IDENTITY_HPP
