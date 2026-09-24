# DropdownMenu

DropdownMenu 是锚定在触发控件旁的操作菜单。它复用 Popover 的托管、定位和关闭能力，并在内容层提供动作条目、勾选项、单选项、分组标题、分隔线、快捷键提示和键盘漫游。

## 何时使用

| 场景 | 建议 |
| --- | --- |
| 一组临时操作或显示选项 | `DropdownMenu` |
| 从固定候选值中选择一个表单值 | `Select` |
| 展示任意自定义内容或小型表单 | `Popover` |
| 页面上持续可见的导航集合 | `Tabs`、`ButtonGroup` 或普通布局 |

## 快速开始

```cpp
std::vector<widget::MenuItem> items {
    {.id = "new", .label = "新建", .shortcut = "Ctrl+N"},
    {.id = "open", .label = "打开…", .shortcut = "Ctrl+O"},
    {
        .id = "export",
        .label = "导出为",
        .kind = widget::MenuItemKind::submenu,
        .children = {
            {.id = "export_pdf", .label = "PDF 文档"},
            {.id = "export_png", .label = "PNG 图像"},
        },
    },
};

auto menu = ui.make<widget::DropdownMenu>(
    ui.make<widget::Button>("文件").build(),
    std::move(items)
).configure([](widget::DropdownMenu& menu) {
    menu.set_on_select([](std::string_view id) {
        // 执行 id 对应的命令
    });
}).build();
```

触发按钮的鼠标单击和 Enter/Space 会自动切换菜单。动作条目激活后关闭整棵菜单；checkbox 与 radio 条目保持当前层打开，方便连续调整。submenu 可以继续包含 submenu，组件会按层递归展开。

## 条目模型

`MenuItem` 是菜单族共享的值类型：

| 字段 | 说明 |
| --- | --- |
| `id` | 同一层内稳定且唯一的身份，也是事件参数 |
| `label` | 展示文本和 typeahead 匹配文本 |
| `shortcut` | 只展示，不参与 typeahead |
| `icon` | 资源名；当前 DropdownMenu 尚未绘制图标 |
| `kind` | `action` / `checkbox` / `radio` / `submenu` / `separator` / `label` |
| `disabled` | 仍可被键盘漫游发现，但不能激活 |
| `checked` | checkbox/radio 的唯一状态来源 |
| `children` | submenu 的子条目 |

结构条目 `separator` 和 `label` 不参与键盘漫游。详细规则见[菜单族条目模型](../references/menu_model.md)。

`set_on_select()`、`item_selected()` 与 `set_on_submenu()` 当前返回被激活条目的 leaf id，不携带完整路径。同一层必须保持 id 唯一；若多个层级共用一个集中式回调，建议让 id 在整棵菜单中唯一，避免业务侧歧义。

## 选择模式

```cpp
menu->set_selection_mode(widget::MenuSelectionMode::multiple);
menu->set_checked("grid", true);
const auto ids = menu->checked_ids();
```

| 模式 | 行为 |
| --- | --- |
| `none` | 默认；激活 checkbox/radio 仍触发选择事件，但不修改 `checked` |
| `single` | 选中 radio 时清除同层其他 radio；checkbox 不受影响 |
| `multiple` | checkbox/radio 各自独立切换 |

`set_checked()` 是静默的程序同步入口，不触发事件，也不执行 single 互斥清理。

## 事件

| 接口 | 说明 |
| --- | --- |
| `set_on_select()` | action、checkbox 或 radio 被用户激活时回调 id |
| `item_selected()` | 同一次激活对应的响应式事件 |
| `set_on_submenu()` | submenu 被 Enter、Space、Right 或点击激活时回调 leaf id；单纯悬停展开不通知 |
| `set_on_close()` | 菜单因程序调用、Escape 或外部点击关闭后触发 |

回调保存在菜单自身时，不要让回调强捕获菜单本身；需要读回 `checked_ids()` 时应捕获 `weak_ptr`，避免形成引用环。

## 键盘与指针行为

| 输入 | 行为 |
| --- | --- |
| 单击触发器 | 打开或关闭菜单 |
| ↑ / ↓ | 在可聚焦条目间循环，跳过标题和分隔线 |
| Home / End | 跳到第一个或最后一个可聚焦条目 |
| PageUp / PageDown | 按共享漫游模型向首尾移动 |
| 输入文字 | 按 `label` 前缀进行大小写不敏感的 typeahead |
| Enter / Space | 激活当前高亮条目 |
| Right | 当前项是可用且非空的 submenu 时展开并进入子层 |
| Left | 在子层中关闭当前层并把焦点还给父菜单 |
| Escape | 子层中返回父层；根层中关闭整棵菜单并恢复触发器焦点 |
| 点击外部 | 关闭整棵菜单并恢复触发器焦点 |
| 鼠标移动 | 更新高亮；进入 submenu 时立即展开，进入普通项时收起现有子层 |
| disabled 条目 | 可以被高亮和朗读，但激活是 no-op |

菜单表面本身持有焦点，条目通过 active index 表达高亮；方向键移动不会隐式改变 `checked`。

## 定位与公开状态

`set_placement()`、`set_alignment()` 和 `set_gap()` 直接转发给内部 Popover。`open()`、`close()`、`toggle()`、`is_open()` 可程序化控制菜单；`active_index()` 返回当前高亮项，`-1` 表示没有可聚焦条目。

`set_items()` 可以替换根层条目；若菜单已打开，组件会先关闭子层，再在安全的场景树提交点重建表面、重新测量并重置焦点。子层 checkbox/radio 的用户变更会同步回根 `items()` 对应的 `children`。

## 主题与实例覆盖

外层面板由 Popover 配方负责，DropdownMenu 配方只控制条目列表：

- 常规文本、快捷键与分组标题排版；
- 禁用文本色、鼠标悬停填充、键盘高亮填充；
- 勾选指示和分隔线颜色；
- 条目高度、列表内边距、分隔留白、条目圆角、分隔线厚度和最小宽度。

```cpp
menu->set_override(theme::DropdownMenuRecipeRule {
    .metrics_item_height = theme::ThemeScalar::literal(36.0F),
    .metrics_min_width = theme::ThemeScalar::literal(200.0F),
});
```

默认条目高度为 32 个逻辑单位，最小宽度为 160；其余默认值引用当前 DesignSystem 的语义 token。

## 无障碍

菜单表面报告 `list` 角色并持有焦点。action/submenu 使用 `list_item`，checkbox 和 radio 使用对应角色及 checked 状态，分隔线使用 `separator`，分组标题使用 `static_text`。`shortcut` 进入 hint，禁用条目报告 disabled 且不暴露 activate 动作。

## 已知空白

- `MenuItem::icon` 已进入共享模型，但 DropdownMenu 暂未渲染图标；
- 子菜单悬停立即展开，尚未实现延时意图判断或 safe-polygon 轨迹容错；
- 选择事件只提供 leaf id，尚未提供完整菜单路径事件；
- 尚未提供打开/关闭和子菜单切换动画；
- 语义角色集合暂时没有专用 `menu` / `menuitem`，当前使用 `list` / `list_item` 表达。
