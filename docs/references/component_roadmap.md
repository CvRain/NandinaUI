# 组件开发路线图

NandinaUI 不以逐项复制其他组件库为目标。[shadcn/ui Components](https://ui.shadcn.com/docs/components)、Qt、Angular 等项目用于帮助识别常见需求，但组件是否进入核心库取决于桌面应用价值、可复用性和现有基础设施。

## 当前判断

项目已经拥有一套可运行的内容、输入、布局、主题和响应式基础。当前最大的结构性缺口不是单个组件，而是多个浮层组件共享的定位、关闭与焦点设施。

现有 Select、Tooltip 和 Dialog 已可使用，但内部各自处理部分浮层行为。直接继续增加 Menu、Popover 与 Combobox 会产生重复逻辑，并可能迫使这些组件随后共同修改 API。

## 阶段 0：稳定公共边界

- 使用 [组件公共契约](component_contract.md) 评审新增或重构组件；
- 明确 recommended、experimental 与 internal API；
- 统一值组件的 setter、callback、reactive event 和 Signal 绑定；
- 统一 disabled、read-only、invalid、focus 和语义行为；
- 让 Getting Started 只依赖 recommended API。

完成标准：新增组件可以套用统一检查表，而不是重新决定一遍命名和生命周期。

## 阶段 1：浮层基础设施

当前进度：`OverlayHost` / portal 与 `AnchoredPositioner` 已经完成；窗口服务接入、关闭和焦点能力尚未实现。

优先实现内部能力，而不是立即增加多个公开控件：

- `OverlayHost` / portal：将浮层托管到顶层，避免父级裁剪（内部基础已完成）；
- `AnchoredPositioner`：anchor、placement、alignment、offset、flip 和 shift（已完成）；
- `DismissLayer`：点击外部、Escape、模态阻断与关闭原因（基础已完成）；
- `FocusScope`：焦点限制、初始焦点和关闭后的焦点恢复（基础已完成）；
- roving focus/typeahead：菜单和列表选择的键盘导航。

完成标准：嵌套浮层、窗口边缘定位、Escape、点击外部与焦点恢复具有独立测试。

## 阶段 2：迁移已有浮层组件

- Tooltip 使用统一定位和 OverlayHost；
- Select 使用统一 popup、关闭规则与键盘导航；
- Dialog 使用 FocusScope、DismissLayer 和命名槽位；
- 保持已有应用层构建方式兼容，内部实现迁移不要求教程改写。

## 阶段 3：高频基础组件

推荐顺序：

1. `Spinner`、`Skeleton`、`EmptyState`；
2. `Alert` 与基于 Dialog 语义约束的 `AlertDialog`；
3. `Toggle`、`ToggleGroup`、`ButtonGroup`；
4. `TextArea`，与 TextField 共享 EditableText primitive；
5. `Breadcrumb` 与 `Pagination`。

这些组件能补齐常见应用页面，同时不会引入庞大的新子系统。

## 阶段 4：菜单与选择组件族

在浮层基础设施稳定后实现：

- `Popover`；
- `DropdownMenu`；
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

## 每个阶段的交付要求

- 公开头文件与 ComponentTraits；
- 语义状态和键盘行为说明；
- 主题 recipe 与实例覆盖路径；
- 针对组件契约的测试；
- `docs/components/` 中的使用参考；
- 若建立新的跨组件规则，同步更新 `docs/references/`。
