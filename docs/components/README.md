# 组件参考

- 文档覆盖率与编写进度见 [覆盖率看板](coverage.md)；
- 新增组件文档请复制 [组件文档模板](_template.md)。

键盘交互模型（选择与导航类组件）见 [选择与导航的键盘模型](selection_and_navigation.md)。

这里记录 NandinaUI 面向应用开发者公开的组件。项目仍处于 alpha 阶段；“可用”表示已有实现、主题与测试覆盖，不等同于已经冻结全部 API。

## 当前组件

### 内容与展示

| 组件 | 用途 | 状态 |
| --- | --- | --- |
| `Label` | 展示文本并支持响应式文本绑定 | 可用 |
| `Image` | 从资源系统加载和展示图片 | 可用 |
| `Avatar` | 展示头像占位与名称语义 | 可用 |
| `Badge` | 展示轻量状态或分类 | 可用 |
| `Chip` | 展示可移除标签 | 可用 |
| `Divider` | 分隔内容区域 | 可用 |
| `ProgressBar` | 展示确定性进度 | 可用 |
| [`Skeleton`](skeleton.md) | 内容加载前占据最终版式的占位块 | 可用 |
| [`EmptyState`](empty_state.md) | 列表或集合暂空时的占位与引导 | 可用 |
| `Card` | 带主题外观的单子内容容器 | 可用，等待语义槽位 |

### 输入与选择

| 组件 | 用途 | 状态 |
| --- | --- | --- |
| [`Button`](button.md) | 执行语义操作 | 可用 |
| `Checkbox` | 独立布尔选择 | 可用 |
| `Switch` | 即时启用或关闭设置 | 可用 |
| [`Toggle`](toggle.md) | 按钮外观的两态开关（工具栏开/关） | 可用 |
| [`ToggleGroup`](toggle_group.md) | 协调一组 Toggle 的单选/多选与键盘漫游 | 可用 |
| [`ButtonGroup`](button_group.md) | 把相关按钮按统一间距排成一行/一列 | 可用 |
| `RadioButton` / `RadioGroup` | 单选项组 | 可用 |
| [`Slider`](slider.md) | 在数值范围内选择 | 可用 |
| `TextField` | 单行文本输入 | 可用 |
| [`TextArea`](text_area.md) | 多行纯文本输入 | 可用 |
| `Select` | 从字符串选项中单选 | 可用，已接入统一浮层 |
| `Tabs` | 水平标签选择 | 可用，等待内容槽位 |
| [`Breadcrumb`](breadcrumb.md) | 用链接路径标出当前页在层级中的位置 | 可用 |
| [`Pagination`](pagination.md) | 上一页/下一页与带省略号的页码导航 | 可用 |

### 浮层与反馈

| 组件 | 用途 | 状态 |
| --- | --- | --- |
| `Dialog` | 模态内容、遮罩与焦点限制 | 可用，已接入统一浮层 |
| [`Popover`](popover.md) | 锚定在触发控件旁的非模态浮层容器（内容为任意控件） | 可用，已接入统一浮层 |
| [`DropdownMenu`](dropdown_menu.md) | 锚定菜单：动作 / 勾选 / 单选条目，键盘漫游与 typeahead | 可用，基于 Popover |
| `Tooltip` | 悬停后显示简短提示 | 可用，已接入统一浮层定位 |
| [`Alert`](alert.md) | 内联消息条，传达状态或反馈 | 可用 |

### 交互扩展

| 组件 | 用途 | 状态 |
| --- | --- | --- |
| `PointerArea` | 为任意单个子控件观察原始指针输入 | 实验性 |
| `GestureArea` | 为任意单个子控件识别点击、双击、长按和拖拽 | 实验性 |

参见 [Dialog](dialog.md) 与 [PointerArea 与 GestureArea](pointer_and_gesture_areas.md)。

### 布局与滚动

当前提供 `Column`、`Row`、`Flex`、`Wrap`、`Stack`、`Padding`、`Center`、`Expanded`、`FlexItem`、`Grid`、`ScrollView` 和列表声明能力。布局系统将在独立专题中统一说明。

## 计划补充

- 高频基础组件：`AlertDialog`。
- 浮层组件族：`ContextMenu`、`Combobox`、`CommandPalette`、`HoverCard`
  （基座 `Popover`、统一 MenuItem model 与 `DropdownMenu` 已落地，见
  [菜单族条目模型](../references/menu_model.md)）。
- 信息组织组件：`Accordion`、`Collapsible`、`Sheet`、`Table`。

计划顺序与依赖关系见 [组件开发路线图](../references/component_roadmap.md)。

## 组件文档模板

新增组件文档时建议保持以下顺序：

1. 组件解决的问题，以及不适合使用它的场景；
2. 最小可运行示例；
3. 常用构建器写法；
4. 状态、事件和响应式绑定；
5. 键盘、焦点与无障碍行为；
6. 主题变体与实例级微调；
7. 完整公开 API 和仍处于实验阶段的能力。
