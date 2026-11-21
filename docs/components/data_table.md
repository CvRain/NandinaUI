# DataTable（experimental）

> 3b③ 公共导出已完成（2026-11-18）；数据视图作者体验批（2026-11-21）已将全部 9 个
> experimental 公开头从 `nandina/widget/internal/` 迁至 `nandina/widget/`，并为
> `TextItemDelegate` 补齐默认选中/当前格视觉、为 `DataTable` 补齐 `selection_changed()` /
> `sort_changed()` 与 `ComponentTraits`。

`DataTable` 是**虚拟化**的数据表格：行数据由 `IRowModel` 提供，列由
`DataTableColumn` 定义，单元格视觉由 `IItemDelegate` 创建和更新。表头固定，body 只物化
当前视口内的单元格，适合上千行数据。静态、全量物化的表格使用 [`Table`](table.md)。

## 最小用法

```cpp
#include <nandina/widget/data_table.hpp>
#include <nandina/widget/label.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class NotesModel final: public nandina::widget::TypedRowModelBase<NotesModel> {
public:
    std::vector<std::string> titles;
    std::vector<nandina::widget::RowId> ids;

    [[nodiscard]] auto row_count() const noexcept -> std::size_t override {
        return titles.size();
    }

    [[nodiscard]] auto row_id(std::size_t row) const -> nandina::widget::RowId override {
        return ids.at(row);
    }

    void rename(std::size_t row, std::string title) {
        titles.at(row) = std::move(title);
        notify_data_changed({.first = row, .last_exclusive = row + 1});
    }
};

auto model = std::make_shared<NotesModel>();
model->titles = {"First", "Second"};
model->ids = {
    nandina::widget::RowId{.value = 1},
    nandina::widget::RowId{.value = 2},
};

auto table = ui.make<nandina::widget::DataTable>(
    std::vector<nandina::widget::DataTableColumn>{
        nandina::widget::text_column<NotesModel>(
            nandina::widget::ColumnId{.value = "title"},
            "标题",
            [](const NotesModel& current, std::size_t row) {
                return current.titles.at(row);
            },
            nandina::widget::TableColumn{.flex = 1.0F}
        ),
    }
).build();
table->set_model(model);
```

`IRowModel` 的 `RowId` 在插入、排序和过滤后必须保持不变且唯一。行集合、行数或显示顺序变化
调用 `notify_reset()`；只修改现有行内容时调用 `notify_data_changed(range)`。

## Delegate

默认 `TextItemDelegate` 从列取值器取得 `std::string`，渲染基于 `Label` 的单行省略文本，
并消费 `ItemState::selected` / `current` 绘制主题语义选中背景（`accent` / `accent_foreground`）
与当前格边框（`ring`）。需要自定义卡片、徽标或多控件组合时，继承 `TypedItemDelegate<ModelT>`：

```cpp
class NoteDelegate final: public nandina::widget::TypedItemDelegate<NotesModel> {
protected:
    auto create_typed(
        const nandina::widget::BuildContext& ui,
        const NotesModel&,
        const nandina::widget::ItemBinding&
    ) -> std::shared_ptr<nandina::scene::NanControl> override {
        return nandina::widget::Label::create(ui.graph(), "…");
    }

    void update_typed(
        nandina::scene::NanControl& control,
        const NotesModel& model,
        const nandina::widget::ItemBinding& binding
    ) override {
        static_cast<nandina::widget::Label&>(control).set_text(
            model.titles.at(binding.model_row)
        );
    }
};
```

`create_item` 获得的 `BuildContext` 已绑定到该单元格作用域，可安装 effect/subscription；
`update_item` 只更新控件，不得重复安装订阅。`ItemBinding`、`BuildContext`、列身份和主题都是
同步借用，不能在回调结束后保存。

表头视觉可通过 `DataTableColumn::header_delegate` 替换 `IHeaderDelegate`。焦点、键盘、
`column_header` 语义与 `aria-sort` 始终由框架 wrapper 拥有，delegate 只替换 wrapper 内的视觉根。

## 排序、选择与滚动

- `SortStrategy::local`：列需通过 `set_local_sort<ModelT>()` 提供弱序比较器，框架只重排显示索引。
- `SortStrategy::external`：`sort_requested()` 发出 `SortRequest`，应用重查数据后调用
  `notify_reset()`；也可用 `set_on_sort_requested()` 安装便捷回调。
- `sort_changed()` / `set_on_sort_changed()`：排序状态变化（`local` / `external` / 清除 /
  `set_columns` 移除已排序列）提交后触发，订阅方读取 `sort_spec()`。
- `selection()` 是按 `CellId` 的身份选择；`selection_changed()` / `set_on_selection_changed()`
  直接转发选择变更事件；`current`/`anchor` 不是第二个选择事实。
- `apply_selection()`、`set_current_cell()` 只接受当前模型中存在的可见单元格身份；未知身份返回
  `false` 且不改状态。
- `scroll_to_row()` 让显示序行进入视口；`set_scroll_offset()` 自动夹取到合法范围。

`std::hash<RowId/ColumnId/CellId>` 可直接用于 `unordered_map`/`unordered_set`。它只保证
相等身份得到相等哈希；哈希数值不是跨进程、跨标准库版本或跨框架版本的持久身份。

完整契约见[数据组件 §9](../references/data_components_and_virtualization.md#9-p42d-数据视图契约datatable-与-listviewv31-定稿)。
