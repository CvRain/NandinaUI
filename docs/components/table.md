# Table

> 🟡 P4.2b experimental 使用手册提纲；API、列规则和语义结构按实现核对，叙述正文与 Showcase 页面待维护者手写。

## 核对表

| 项目 | 当前接口/行为 |
| --- | --- |
| 创建 | `ui.make<widget::Table>()` / `Table::create(columns)`；默认两列 |
| 列定义 | `TableColumn{.width = logical_width}` 固定宽；缺省 width 时按 `flex` 权重分配有限剩余宽度 |
| 表头 | `.header_cells(cell...)` / `set_header(Cells)`；可选，单行 |
| 正文 | `.row_cells(cell...)` / `add_row(Cells)` / `set_rows(vector<Cells>)` |
| 查询 | `columns()`、`row_count()`、`header_cell(column)`、`cell(row,column)` |
| 间距 | `set_column_gap()` / `set_row_gap()`；默认 0；轨道/行数增加时也校验间距总和 |
| 数值边界 | 自然列宽/行高与间距总和超出 float 范围时，测量或排列抛 `invalid_argument` |
| 布局 | 有限宽度共享列轨道；无界宽度取各列内容自然宽度最大值；每行高度取该行单元格最大自然高度 |
| 内容 | 普通 `NanControl`；同一表格内不得重复，必须非空、脱离树、无父节点且不含锚点约束 |
| 替换 | 列数只能在无表头/正文时改变；非法行或列操作抛异常并保留原状态 |
| 语义 | `table → row → column_header/cell → cell content`；Table 不可聚焦，内容 action 保留 |
| 边界 | 静态、全量物化、不滚动；不含选择、排序、虚拟化、合并单元格、粘性表头、列拖动 |

## 最小结构

<!-- TODO: 维护者补充可直接编译的 BuildContext / NodeBuilder 示例。 -->

## 布局与滚动

<!-- TODO: 维护者说明 flex 列、固定列、有限宽度文本换行及外层 ScrollView 的组合。 -->

## 语义与边界

<!-- TODO: 维护者补充读屏顺序和静态 Table 与 DataTable 的选用说明。 -->

实现契约见 [数据组件 §8](../references/data_components_and_virtualization.md#8-p42b-静态-table布局契约)。
Showcase 视觉验收尚未完成。
