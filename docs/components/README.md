# 组件参考

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
| `Card` | 带主题外观的单子内容容器 | 可用，等待语义槽位 |

### 输入与选择

| 组件 | 用途 | 状态 |
| --- | --- | --- |
| `Button` | 执行语义操作 | 可用 |
| `Checkbox` | 独立布尔选择 | 可用 |
| `Switch` | 即时启用或关闭设置 | 可用 |
| `RadioButton` / `RadioGroup` | 单选项组 | 可用 |
| `Slider` | 在数值范围内选择 | 可用 |
| `TextField` | 单行文本输入 | 可用 |
| `Select` | 从字符串选项中单选 | 可用，等待统一浮层 |
| `Tabs` | 水平标签选择 | 可用，等待内容槽位 |

### 浮层与反馈

| 组件 | 用途 | 状态 |
| --- | --- | --- |
| `Dialog` | 模态内容、遮罩与焦点限制 | 可用，等待统一浮层 |
| `Tooltip` | 悬停后显示简短提示 | 可用，等待统一定位 |

### 交互扩展

| 组件 | 用途 | 状态 |
| --- | --- | --- |
| `PointerArea` | 为任意单个子控件观察原始指针输入 | 实验性 |
| `GestureArea` | 为任意单个子控件识别点击、双击、长按和拖拽 | 实验性 |

参见 [PointerArea 与 GestureArea](pointer_and_gesture_areas.md)。

### 布局与滚动

当前提供 `Column`、`Row`、`Flex`、`Wrap`、`Stack`、`Padding`、`Center`、`Expanded`、`FlexItem`、`Grid`、`ScrollView` 和列表声明能力。布局系统将在独立专题中统一说明。

## 计划补充

- 高频基础组件：`TextArea`、`Toggle`、`ToggleGroup`、`Alert`、`AlertDialog`、`Spinner`、`Skeleton`、`EmptyState`。
- 浮层组件族：`Popover`、`DropdownMenu`、`ContextMenu`、`Combobox`、`CommandPalette`、`HoverCard`。
- 信息组织组件：`Accordion`、`Collapsible`、`Sheet`、`Breadcrumb`、`Pagination`、`Table`。

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
