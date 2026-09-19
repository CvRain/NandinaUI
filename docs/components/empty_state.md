# EmptyState

EmptyState 在列表或集合暂无内容时占据内容区：可选的图标、标题、描述，以及可选的操作槽位（通常是"新建"或"重试"按钮）。

它是**纯展示**组件：自身不可聚焦、不接收输入，交互全部由 `action` 槽位里的控件承载。标题为空且没有图标时整体测量为 0，因此可以无条件挂载，不会在列表里留下空隙。

> 组件属于 recommended；`icon` / `action` 是命名槽位，alpha 阶段仍可能细化。

## 不适合使用它的场景

- 内容正在加载、只是还没到：用 `Skeleton` 或 `Spinner`；
- 需要用户立即确认或阻断页面：用 `Dialog`；
- 只是提示一段信息、不需要占位：用页内 `Label`。

## 最小示例

```cpp
auto empty = ui.make<widget::EmptyState>("暂无数据", "试试调整筛选条件").build();
empty->set_action(ui.make<widget::Button>("新建"));
```

## 内容与槽位

标题与描述是文本入口，`icon` / `action` 是两个固定语义槽位，缺省为空且不占高度：

| 接口 | 说明 |
| --- | --- |
| `set_title()` / `title()` | 标题；为空且无图标时整体不占位 |
| `set_description()` / `description()` | 描述，使用 `muted_foreground` 弱化文本色 |
| `set_icon()` / `icon()` | 可选图标槽位，接受任意控件 |
| `set_action()` / `action()` | 可选操作槽位，通常放按钮 |

传入槽位的控件必须是尚未挂载的节点（`create()` / `ui.make()` 的产物）；传 `nullptr` 或已挂载的节点会抛异常，替换槽位时旧内容会被移除——与 `Dialog` 的命名槽位约定一致。

布局是垂直堆叠、水平居中：图标 → 标题 → 描述 → 操作，块与块之间用配方的 `gap`，四周用 `padding_x` / `padding_y`。

## 测量与显式尺寸

- 有界宽度下铺满约束上限；无界时取配方首选宽度（默认 240）；
- 高度由内容决定，再用 `min_height` 托底；
- 标题为空且没有图标 → 测量为 `0 × 0`，`on_draw` 也直接返回。

## 主题与实例覆盖

```cpp
auto empty = widget::EmptyState::create();
empty->set_override(theme::EmptyStateRecipeRule {
    .container_radius = theme::ThemeScalar::literal(4.0F),
    .description_color = theme::ThemeColor::token(theme::ColorToken::on_surface_variant),
    .metrics_gap = theme::ThemeScalar::literal(8.0F),
});
const auto style = empty->resolved_style();
```

解析顺序遵循[组件公共契约](../references/component_contract.md)：DesignSystem 默认值 → recipe → 实例覆盖。默认值只用语义角色：`title` 取 `foreground` + `typography_label_lg`，`description` 取 `muted_foreground` + `typography_label_sm`，间距取 `spacing_md` / `spacing_lg`，容器默认透明（空状态通常嵌在卡片或列表内部，避免"框中框"）。

## 无障碍

EmptyState 暴露 `generic` 语义角色，标签取标题，`hint` 取描述；没有标题时不暴露语义节点，由图标 / 描述 / 操作子节点各自报告。操作槽位里的按钮保持自己的 `button` 角色与 action，不会被合并。

## 完整公开 API

| 接口 | 说明 |
| --- | --- |
| `EmptyState(theme)` / `create(theme)` | 构造 |
| `set_theme()` / `theme_ref()` | 整份主题覆盖 / 当前主题视图 |
| `set_override(theme::EmptyStateRecipeRule)` | 只覆盖明确指定的配方字段，系统切换后跟随新快照 |
| `visual_state()` / `resolved_style()` | 当前状态与解析后的具体样式 |
| `on_theme_changed()` / `on_style_context_changed()` / `on_draw()` / `on_measure()` / `on_layout()` | 控件协议实现 |

描述文本支持自动换行（最多 4 行，防止超长文本撑出容器）；行数上限不是配方字段。
