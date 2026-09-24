# 菜单族条目模型

本文是**跨组件规则**，面向框架维护者。它规定阶段 4 的菜单族组件（DropdownMenu、ContextMenu、
Combobox、CommandPalette）如何共享同一份条目模型，避免每个组件各定义一套选项结构。

实现见 `nandina/widget/menu_item.hpp` / `.cpp`，契约测试见 `tests/menu_item_tests.cpp`。

## 为什么需要它

在引入本模型之前，仓库里唯一的选择类组件 `Select` 直接用 `std::vector<std::string>` 表示选项。
这在单层纯文本下拉里够用，但阶段 4 的组件需要：快捷键提示、禁用、勾选、图标、子菜单、单选与
多选。若每个组件自行扩展这个 vector，会出现四种互不兼容的选项类型，主题、无障碍与键盘行为
也无法共享。

因此把「条目长什么样」与「条目如何被聚焦、激活、勾选」抽成模型，组件只负责渲染与定位。

## 模型

```cpp
struct MenuItem {
    std::string id;          // 稳定身份：选择状态、keying、事件都用它
    std::string label;       // 展示文本，同时是 typeahead 匹配文本
    std::string shortcut;    // 仅供展示的快捷键提示
    std::string icon;        // 资源名，由视图通过 ResourceManager 解析
    MenuItemKind kind = MenuItemKind::action;
    bool disabled = false;
    bool checked = false;    // 仅 checkbox / radio 有意义
    std::vector<MenuItem> children;  // kind == submenu 时使用
};
```

| `MenuItemKind` | 可聚焦 | 可激活 | 勾选语义 |
| --- | --- | --- | --- |
| `action` | ✅ | ✅ | 无 |
| `checkbox` | ✅ | ✅ | 独立多选 |
| `radio` | ✅ | ✅ | 同层互斥单选 |
| `submenu` | ✅ | ✅ | 无（激活即展开） |
| `separator` | ❌ | ❌ | 无 |
| `label` | ❌ | ❌ | 无 |

`id` 在一层内必须唯一；`separator` 不需要 id，也不应被当作可寻址目标。当前菜单组件的选择
事件返回 leaf id，而不是完整路径；若业务把所有层级交给同一个回调，推荐让 id 在整棵菜单中
唯一。模型层仍允许父子层复用 id，因为 `find_menu_item()` 的查找契约始终是单层的。

## 规则

### 1. 「可聚焦」与「可激活」是两件事

- `separator` / `label` 是结构性条目，**不可聚焦**：方向键必须跳过它们。
- `disabled` 条目**仍然可聚焦**，只是不可激活。

第二条与 ARIA menu 一致，也是刻意的：用户要能用方向键走到禁用项上，才能知道它存在、以及它当前
不可用。把 disabled 条目从漫游里摘掉会让菜单项"凭空消失"，键盘用户无法发现它。

判定函数：`menu_item_is_focusable()` / `menu_item_is_activatable()`。

### 2. typeahead 只匹配 label

`shortcut` 是**展示提示**，不参与匹配。把 `"Ctrl+S"` 纳入匹配会导致输入 `c` 意外命中多个条目
（`Copy`、`Cut`、`Ctrl+...`）。`label` 的起始匹配由 `RovingFocus` 的既有 typeahead 实现负责。

`menu_item_typeahead_text()` 对结构性条目返回空串。这类条目在漫游中本就不可聚焦（规则 1），
因此 typeahead 不会命中它们；返回空串是额外保证，避免组件误把结构文本当成可匹配标签。

### 3. 查找不递归

`find_menu_item()` 只查给定列表这一层。子菜单是独立的一层，调用方手上已经有正确的层级；
递归查找会让「同一个 id 同时出现在父子两层」产生歧义。组件展开子菜单时，把 `children` 当作
一层新的条目列表处理。

### 4. 勾选状态的唯一事实来源是 `MenuItem::checked`

`MenuSelection` **不额外保存一份状态**。它只提供变更规则与通知，组件渲染时直接读条目自身的
`checked`。这样不会出现「模型说选中、条目说没选中」的分裂。

```cpp
MenuSelection selection {MenuSelectionMode::single};
if (selection.toggle(items, clicked_id)) {
    // items 已被原地更新；按需重绘
}
```

| `MenuSelectionMode` | 语义 | `toggle` 行为 |
| --- | --- | --- |
| `none` | 纯动作菜单 | 一律返回 false，不改变任何状态 |
| `single` | 单选 | 勾选一项会清除**同层的其他 radio 项** |
| `multiple` | 多选 | 各项独立 |

`toggle()` 在以下情况返回 `false` 且不触发事件：mode 为 `none`、id 不存在、条目不是
`checkbox` / `radio`、条目被 `disabled`。

**single 的互斥只发生在 radio 之间**：同一层允许同时存在 radio 组与独立 checkbox，选中一个
radio 不应清掉用户的 checkbox 选择。

`set_checked()` 是程序侧同步入口：**静默**（不触发事件）且**不做互斥清理**，因此可以把两个
radio 同时置为真。需要互斥语义时用 `toggle()`。

## 视图组件如何消费

1. 用条目层建立漫游成员，把规则直接交给 `RovingFocus::sync`：

   ```cpp
   focus_.sync(
       items_.size(),
       [this](std::size_t i) { return menu_item_is_focusable(items_[i]); },
       [this](std::size_t i) -> std::string_view {
           return menu_item_typeahead_text(items_[i]);
       }
   );
   ```

2. 漫游的移动模型按组件语义选择（见 [选择与导航的键盘模型](../components/selection_and_navigation.md)）：
   菜单用 `focus_only`（焦点移动不改值），勾选由 Enter/Space 显式触发，而不是随方向键变化。
3. 激活时先判 `menu_item_is_activatable()`，再按 kind 分派：`checkbox` / `radio` 走
   `MenuSelection::toggle()`，`submenu` 展开子层，`action` 触发回调。
4. 渲染需要的每项视觉状态（hovered / focused / disabled / checked）由组件配方表达，
   模型不参与造型。

DropdownMenu 的子层继续消费同一组规则：子层以父条目为外部锚点，借助 OverlayHost 的
parent 关系递归关闭；Left / Escape 只退出当前层，任意深度的 action 关闭整棵菜单，
checkbox / radio 则保持展开并把修改后的 children 同步回父层模型。

## 浮层基座

菜单族都需要"锚定在触发控件旁、点外部关闭、不被父级裁剪"的浮层表面，这一层由 `Popover`
提供（见 [浮层架构](overlay_architecture.md)）。分工是：

- `Popover`：托管、锚点定位、关闭与焦点作用域，内容可以是任意控件；
- 菜单族组件：把自己的条目列表渲染成内容，并消费本模型。

## 本模型不覆盖什么

- **造型与布局**：条目高度、内边距、状态层属于组件配方，不属于模型；
- **任意节点图标槽位**：`MenuItem::icon` 只是资源名。需要传入任意控件时，由组件提供槽位，
  不把场景节点塞进值类型（模型必须能脱离 scene 单测）；
- **虚拟化与异步数据**：`CommandPalette` 的大列表过滤在组件侧完成，模型只描述已确定的条目。

## 后续

阶段 4 已完成 `Popover`、`DropdownMenu`、递归子菜单、`ContextMenu` 与 `Combobox`，下一步按
`CommandPalette` → `HoverCard` 顺序实现。`Combobox` 额外验证了一点：本模型可以同时服务
「激活即执行」的菜单与「输入即筛选」的选择器 —— 前者用 `MenuItem` 的 kind 与 `MenuSelection`，
后者只需按 `label` 过滤后仍交给同一套 `RovingFocus` 与条目渲染。每新增一个消费方，都应复用本模型与 `MenuSelection`，不得新增平行的选项类型；
若模型确需扩展字段，先改这里再改实现，并同步 `tests/menu_item_tests.cpp`。
