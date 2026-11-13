/// showcase/pages/table_page.hpp — 静态 Table 的列轨道、自然行高与有限宽度行为。
#pragma once

#include <nandina/app/nan_page.hpp>
#include <nandina/widget/controls.hpp>

namespace nandina::showcase
{
    /// 演示 P4.2b 静态 Table：固定/权重列共享列轨道、单元格自然行高、有限宽度下的换行，
    /// 以及空表与行数变化的重排。表格本身不处理选择/排序/虚拟化（属于 DataTable）。
    class TablePage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(app::PageContext& context) -> widget::View override;
    };
} // namespace nandina::showcase
