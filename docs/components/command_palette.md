# CommandPalette

> **本文档的框架、符号表与默认值已按代码核对；带 `<!-- TODO -->` 的叙述性正文待手写。**
> 分工见 [覆盖率看板](coverage.md)：AI 提供结构、提纲与符号核对，正文由人工完成。

<!-- TODO: 一句话说明这个组件是什么、解决什么问题。不要复述 API，说"人话"。
     参考措辞方向：它是"键盘优先的入口"——把应用里散落的命令收进一个可搜索的列表，
     让用户不离开键盘就能到达，而不是又一个下拉菜单。 -->

## 何时使用

<!-- TODO: 正向场景 2~3 条 + 反向场景（该用别的组件的情况）。反向场景最有价值。 -->

| 场景 | 建议 |
| --- | --- |
| <!-- TODO --> | 本组件 |
| 少数几个固定选项、已有触发按钮 | 改用 [`DropdownMenu`](dropdown_menu.md) |
| 需要在输入框里选一个**值**（而不是执行动作） | 改用 `Combobox` |
| 需要让用户确认/填写内容后再继续 | 改用 [`Dialog`](dialog.md) |

## 快速开始

<!-- TODO: 用一段话说明这个例子的行为，以及为什么打开动作由应用自己触发。 -->

```cpp
#include <nandina/widget/controls.hpp>

auto palette = ui.make<widget::CommandPalette>(
    std::vector<widget::MenuItem> {
        widget::MenuItem {.id = "file.new", .label = "新建文件", .shortcut = "Ctrl+N"},
        widget::MenuItem {.id = "file.open", .label = "打开文件", .shortcut = "Ctrl+O"},
        widget::MenuItem {.id = "sep", .kind = widget::MenuItemKind::separator},
        widget::MenuItem {.id = "group.view", .label = "视图", .kind = widget::MenuItemKind::label},
        widget::MenuItem {.id = "view.zoom", .label = "缩放", .shortcut = "Ctrl+0"},
    },
    "搜索命令"
).build();

palette->set_on_select([](const std::string_view id) {
    // 激活某条命令：这里按 id 分派到你自己的动作上。
});

// 打开动作由应用触发：框架不提供全局快捷键注册（见「已知空白」）。
palette->open();
```

## 公开 API

<!-- TODO: 按"配置 / 值 / 事件"分组讲解使用节奏，表格字段已按代码核对。 -->

| 方法 | 说明 |
| --- | --- |
| `create(items, placeholder, theme)` | 构造入口；`ui.make<CommandPalette>(items, placeholder)` 会注入主题与覆盖层服务 |
| `set_items(vector<MenuItem>)` / `items()` / `item_count()` | 条目模型；打开时替换会**就地**重过滤，不打断输入 |
| `filtered_ids()` | 当前结果里可聚焦条目的 id，按列表顺序 |
| `set_query(string)` / `query()` / `clear_query()` | 查询文本；`set_query` 静默（不触发 `on_query_change`） |
| `set_placeholder(string)` / `placeholder()` | 占位文本 |
| `set_max_visible_results(n)` / `max_visible_results()` | 结果上限，默认 **8** |
| `hidden_result_count()` | 因上限未渲染的条数 |
| `open()` / `close()` / `toggle()` / `is_open()` | 开关；每次 `open()` 都会清空查询 |
| `active_index()` / `active_id()` | 当前高亮；`active_index()` 索引的是**渲染出的结果列表**（含分组标题与分隔线），`active_id()` 是无歧义的那个 |
| `set_selection_mode(MenuSelectionMode)` / `selection_mode()` | 勾选语义，默认 `none`（纯动作面板） |
| `checked_ids()` | 已勾选条目 id；事实来源是 `items()` 的 `MenuItem::checked` |
| `set_on_select(cb)` / `item_selected()` | 用户激活条目；可订阅事件与会话回调同名 |
| `set_on_query_change(cb)` | 用户输入触发；`set_query` / `clear_query` 不触发 |
| `set_on_close(cb)` | 关闭时触发（Escape、外部点击、激活后自动关闭都会走） |
| `set_disabled(bool)` / `disabled()` | 禁用时不可打开，打开状态下会被关掉 |
| `set_override(CommandPaletteRecipeRule)` / `resolved_style()` | 实例级样式覆盖 |

## 过滤规则

<!-- TODO: 解释"为什么 shortcut 不参与匹配"，以及分组为什么在过滤后仍然保留。 -->

- 匹配对象是 `MenuItem::label`，ASCII 大小写不敏感子串；`shortcut` **不参与**匹配
  （否则输入 `c` 会同时命中 `Copy` 与 `Ctrl+...`）。
- 分组结构（`MenuItemKind::label` 组标题、`MenuItemKind::separator` 分隔线）在过滤后保留；
  **整组都无匹配时，该组连同标题与分隔线一起消失**。
- `MenuItemKind::submenu` 条目会被跳过：命令面板要的是"一次激活就执行"，子菜单在这里没有意义。
- 结果超过上限时只渲染前 N 条，并显示「还有 N 条，继续输入以缩小范围」。

## 键盘与焦点

<!-- TODO: 说明为什么焦点始终在查询框、以及为什么这里不启用 typeahead。 -->

| 按键 | 行为 |
| --- | --- |
| 可打印字符 / Backspace / ←→ / Ctrl+A … | 交给查询输入框 |
| ↑ / ↓、Home / End、PageUp / PageDown | 移动高亮；**跳过**分隔线与组标题 |
| Enter | 激活高亮条目 |
| Escape | 关闭（由浮层的 DismissLayer 处理） |
| Tab | 在面板内部循环（FocusScope） |

- 焦点始终留在查询输入框，结果行不接收焦点。
- 漫游**不启用 typeahead**：查询本身就是过滤器，否则输入的字母会既进输入框又跳高亮。
- 禁用条目**仍然可聚焦**，方向键能走到它，只是不可激活（`menu_model.md` 规则 1）。
- 激活后：`action` 关闭整块面板；`checkbox` / `radio` 按 `selection_mode` 原地切换并保持打开。

## 无障碍语义

<!-- TODO: 结合实际的读屏验证补充说明。 -->

| 节点 | role | 说明 |
| --- | --- | --- |
| 面板 | `dialog` | label「命令面板」 |
| 结果列表 | `list` | 有结果时 label「命令结果」 |
| 条目行 | `list_item`（`checkbox` / `radio` 条目用对应 role） | `value` 放快捷键提示；禁用时 action 为 `none` |
| 组标题 / 分隔线 | `static_text` / `separator` | 不可聚焦 |
| 查询框 | `text_field` | 由内嵌 `TextField` 提供 |

组件自身还暴露一个语义 `activate` action，作用于当前高亮条目。

## 主题

`CommandPaletteRecipe` 的字段（默认值见 `default_command_palette_recipe()`，全部引用语义角色）：

| 分组 | 字段 |
| --- | --- |
| 面板 | `panel`（`BoxStyle`：填充 / 边框 / 圆角）、`metrics.panel_width`、`metrics.panel_top_offset`、`metrics.panel_padding`、`metrics.panel_radius` |
| 查询行 | `query`、`placeholder`（`TypeStyle`） |
| 结果行 | `item_label`、`item_shortcut`、`group_label`、`disabled_label`、`hover_fill`、`focus_fill`、`separator` |
| 空状态 | `empty`（`TypeStyle`） |
| 度量 | `metrics.gap`、`metrics.item_height`、`metrics.item_padding_x`、`metrics.item_radius`、`metrics.separator_thickness`、`metrics.min_width` |

实例级微调用 `set_override(CommandPaletteRecipeRule)`，只覆盖显式指定的字段。

## 已知空白

<!-- TODO: 确认这些空白是否仍然是当前决定；若有取舍理由，补充说明。 -->

- **没有滚动**：结果只按 `set_max_visible_results()` 截断并提示剩余数量。仓库里没有"漫游时
  滚动进视野"的设施，`DropdownMenu` 也不滚动；这一版选择如实提示而不是假装能滚动。
- **不注册全局快捷键**：打开动作由应用触发（例如在窗口里绑定 Ctrl/Cmd+K）。框架不提供快捷键
  注册表，因为"谁来分发按键"取决于应用自己的输入焦点策略。
- **不接受子菜单**：`MenuItemKind::submenu` 条目在过滤阶段被跳过。
- **RTL 方向键极性**：与其它选择类组件一样，沿用 `RovingFocus` 的已知空白
  （见 [选择与导航的键盘模型](selection_and_navigation.md)）。
