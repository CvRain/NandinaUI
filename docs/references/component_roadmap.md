# 组件开发路线图

NandinaUI 不以逐项复制其他组件库为目标。[shadcn/ui Components](https://ui.shadcn.com/docs/components)、Qt、Angular 等项目用于帮助识别常见需求，但组件是否进入核心库取决于桌面应用价值、可复用性和现有基础设施。

## 当前判断

项目已经拥有一套可运行的内容、输入、布局、主题和响应式基础。阶段 1 的定位、关闭与焦点设施已经落地，阶段 2 的 Tooltip、Select 与 Dialog 也全部接入，因此「每个浮层组件各自处理定位与关闭」的重复逻辑已经消除。

阶段 1 的基础设施已全部落地：roving focus / typeahead（`widget::RovingFocus`，接入
`RadioGroup` / `Tabs` / `Select` / `ToggleGroup`，见 [选择与导航的键盘模型](../components/selection_and_navigation.md)）、
嵌套浮层的父子关闭关系（`OverlayOptions::parent` + `OverlayCloseReason`，`Select` / `Tooltip` 已接入），
键码常量也收敛到单一定义处。

阶段 4 的 Popover、DropdownMenu 与 Combobox 依赖这些能力，现在条件已经具备，可以直接开始。

## 阶段 0：稳定公共边界

- 使用 [组件公共契约](component_contract.md) 评审新增或重构组件；
- 明确 recommended、experimental 与 internal API；
- 统一值组件的 setter、callback、reactive event 和 Signal 绑定；
- 统一 disabled、read-only、invalid、focus 和语义行为；
- 让 Getting Started 只依赖 recommended API。

完成标准：新增组件可以套用统一检查表，而不是重新决定一遍命名和生命周期。

## 阶段 1：浮层基础设施

当前进度：`OverlayHost` / portal、`AnchoredPositioner`、窗口服务接入、关闭和焦点能力均已完成基础实现。

优先实现内部能力，而不是立即增加多个公开控件：

- `OverlayHost` / portal：将浮层托管到顶层，避免父级裁剪（内部基础已完成）；
- `AnchoredPositioner`：anchor、placement、alignment、offset、flip 和 shift（已完成）；
- `DismissLayer`：点击外部、Escape、模态阻断与关闭原因（基础已完成）；
- `FocusScope`：焦点限制、初始焦点和关闭后的焦点恢复（基础已完成）；
- 嵌套浮层父子关闭：`OverlayOptions::parent` 声明从属关系，父层关闭时先递归关闭后代并带上
  `OverlayCloseReason::parent`（已完成，`Select` / `Tooltip` 接入）；
- roving focus/typeahead：菜单和列表选择的键盘导航（已完成：`RovingFocus`，显式移动模型
  `focus_and_selection` / `focus_only` / `selection_only` 与键码无关的 `step()`，含 Home/End
  与 typeahead；RTL 极性列为已知空白）。

完成标准：嵌套浮层、窗口边缘定位、Escape、点击外部与焦点恢复具有独立测试。

## 阶段 2：迁移已有浮层组件（已完成）

- Tooltip 使用统一定位和 OverlayHost（已完成，原有 API 与 detached 绘制保持兼容）；
- Select popup 使用统一定位、外部关闭和 OverlayHost（已完成，保留无窗口上下文下的 detached 行为）；
- Dialog 使用 FocusScope、DismissLayer 和命名槽位（已完成，保留无窗口上下文下的树内模态回退）；
- 保持已有应用层构建方式兼容，内部实现迁移不要求教程改写。

三个组件的迁移都已完成。阶段 1 的基础设施至此全部落地：roving focus / typeahead（`Select` 的
弹出列表同时获得了 Home/End 与 typeahead）与嵌套浮层的父子关闭关系均已接入，阶段 1 不再有遗留项。

## 阶段 3：高频基础组件

推荐顺序：

1. `Spinner`、`Skeleton`、`EmptyState`；
2. `Alert`（已完成）与基于 Dialog 语义约束的 `AlertDialog`；
3. `Toggle`、`ToggleGroup`、`ButtonGroup`（均已完成）；
4. `TextArea`（已完成），与 TextField 共享 EditableText primitive；
5. `Breadcrumb` 与 `Pagination`（均已完成）。

这些组件能补齐常见应用页面，同时不会引入庞大的新子系统。

## 阶段 4：菜单与选择组件族

进行中。三项公共依赖已落地，后续组件直接消费它们：

- 统一 MenuItem model（`nandina/widget/menu_item.hpp`，规则见
  [菜单族条目模型](menu_model.md)，契约测试 `tests/menu_item_tests.cpp`）；
- `Popover` 浮层基座（锚定、外部关闭、焦点作用域、内容为任意控件；契约测试
  `tests/popover_tests.cpp`），菜单族把它当作自己的浮层表面；
- `DropdownMenu`（动作 / 勾选 / 单选条目，键盘漫游、typeahead 与无障碍语义；契约测试
  `tests/dropdown_menu_tests.cpp`；playground 有「菜单与下拉」演示页）；
- `DropdownMenu` 递归子菜单（悬停与 Right/Left/Escape、父子浮层关闭、焦点返回、父层点击
  穿透及子层选择状态回写均已覆盖测试）。

待实现：

- `ContextMenu`；
- `Combobox`；
- `CommandPalette`；
- `HoverCard`。

这一阶段应复用统一 MenuItem model、roving focus、typeahead 和 selection model，避免每个组件定义不同的选项结构。

## 阶段 5：信息组织与数据展示

- `Accordion`、`Collapsible`；
- `Sheet`、`Drawer`；
- `Resizable`、`AspectRatio`；
- 静态 `Table`；
- 在选择模型、虚拟化和排序能力成熟后再实现 `DataTable`。

## 暂不进入核心优先级

Chart、Calendar、DatePicker、Carousel、Sidebar、Questionnaire、Message、Bubble 和 Attachment 更适合作为后续 recipes、扩展包或独立子系统。它们不应阻塞核心组件契约和浮层基础设施。

## 新增组件时

机制（改哪些文件、四步同步清单、什么时候不需要配方）见 [新增一个组件](adding_a_component.md)；
质量标准见 [组件公共契约](component_contract.md)。

## 每个阶段的交付要求

- 公开头文件与 ComponentTraits；
- 语义状态和键盘行为说明；
- 主题 recipe 与实例覆盖路径；
- 针对组件契约的测试；
- `docs/components/` 中的使用参考；
- 若建立新的跨组件规则，同步更新 `docs/references/`。
