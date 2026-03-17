# NandinaUI Component Roadmap

这份文档用于约束 NandinaUI 后续组件开发方向，目标不是简单罗列 shadcn/ui 的组件名称，而是把它们翻译成适合 Qt Quick 原生实现、并符合 Nandina 当前架构的开发顺序。

## 核心约束

### 1. 命名约束

- 组件命名统一使用 `Nan` 前缀。
- 对齐 shadcn/ui 的组件语义，但不直接照搬无前缀名称。
- 例如：`NanButton`、`NanInput`、`NanLabel`、`NanCheckbox`、`NanDialog`、`NanPopover`。

### 2. 实现约束

- 实现优先使用最基础的 Qt Quick 类型。
- 优先使用：`Item`、`Rectangle`、`Text`、`FocusScope`、`Loader`、`Row`、`Column`、`Flickable`、`Repeater`、`State`、`Transition`、`DragHandler`、`TapHandler`。
- 避免把组件直接建立在 Qt Quick Controls 的现成控件皮肤之上。
- 允许借助少量基础能力，例如键盘焦点、输入法、光标与选择支持，但组件结构与视觉表达应保持原生可控。

### 3. API 约束

- API 语义优先，不让视觉细节成为主接口。
- 跨组件统一使用共享枚举：`ColorVariantTypes`、`PresetTypes`、`SizeTypes`。
- 默认零配置可用，复杂能力通过可选属性逐步打开。
- 同类组件保持一致字段，例如：`size`、`preset`、`colorVariant`、`disabled`、`invalid`。

### 4. 架构约束

- 先补足基础原语，再往上叠加复杂组件。
- 所有表面型组件尽量复用 `NanSurface`。
- 所有可交互组件尽量复用 `NanPressable` 或同层级交互基础能力。
- 主题切换、亮暗色、语义色映射必须由 `ThemeManager` 驱动，而不是局部硬编码。

## 当前已完成组件

- `NanPressable`
- `NanSurface`
- `NanButton`
- `NanBadge`
- `NanPanel`
- `NanCard`
- `NanSideBar`
- `NanSideBarGroup`
- `NanSideBarItem`
- `NanSideBarTrigger`
- `NanWindow`
- `NanTitleBar`
- `NanTitleBarButton`
- `NanWindowResizer`
- `NanResizeEdge`

## 开发顺序总览

### Phase 0: 基础能力收口

这一阶段不是新增大量组件，而是保证后续输入类组件不会返工。

- [ ] 收敛共享字段命名：`size` / `preset` / `colorVariant`
- [ ] 明确禁用态、聚焦态、错误态的通用视觉语义
- [ ] 固化输入类组件的边框、文本、占位符、提示文案 token
- [ ] 约束亮暗色切换下的响应式绑定写法
- [ ] 统一示例页中的组件 API 风格

### Phase 1: 输入基础件

这是下一阶段最优先的开发任务。

- [ ] `NanLabel`
- [ ] `NanInput`
- [ ] `NanTextarea`
- [ ] `NanCheckbox`
- [ ] `NanSwitch`
- [ ] `NanRadioGroup`

原因：

- 这批组件会直接定义项目的状态模型。
- 它们能倒逼表单类 API 一致性尽早稳定。
- 后续 `Form`、`Dialog`、`Select`、`Combobox` 都依赖这批基础件。

### Phase 2: 低成本高价值基础件

这一阶段用于补足页面拼装能力。

- [ ] `NanSeparator`
- [ ] `NanScrollArea`
- [ ] `NanSkeleton`
- [ ] `NanAvatar`
- [ ] `NanBreadcrumb`
- [ ] `NanCollapsible`

原因：

- 实现成本低。
- 对示例页和真实页面的组合能力提升明显。
- 能为后续更复杂组件提供结构支撑。

### Phase 3: 浮层与反馈基础设施

在输入基础件稳定后，开始补弹层与反馈类能力。

- [ ] `NanDialog`
- [ ] `NanAlertDialog`
- [ ] `NanPopover`
- [ ] `NanTooltip`
- [ ] `NanSheet`
- [ ] `NanDropdownMenu`
- [ ] `NanContextMenu`

原因：

- 这些组件依赖焦点管理、遮罩、层级、点击外部关闭、键盘退出。
- 先做这一层，后续 `Select`、`Combobox`、命令菜单才能复用弹层基础设施。

### Phase 4: 表单增强件

- [ ] `NanSelect`
- [ ] `NanCombobox`
- [ ] `NanCalendar`
- [ ] `NanDatePicker`
- [ ] `NanForm`

原因：

- 这些组件对输入状态、弹层系统、键盘交互依赖很强。
- 放在前面做，后续极大概率返工。

### Phase 5: 页面结构与数据展示件

- [ ] `NanAccordion`
- [ ] `NanTabs`
- [ ] `NanPagination`
- [ ] `NanTable`
- [ ] `NanEmpty`
- [ ] `NanCarousel`
- [ ] `NanChart`

原因：

- 这批组件价值高，但不是当前最短板。
- 等输入、浮层、反馈链路稳定后再推进更合适。

## 近期最建议立刻启动的组件

如果只选一批立刻开工，推荐顺序如下：

1. `NanLabel`
2. `NanInput`
3. `NanTextarea`
4. `NanCheckbox`
5. `NanSwitch`
6. `NanRadioGroup`
7. `NanSeparator`

这是当前最稳的路线。完成它们后，NandinaUI 会从“已有若干漂亮基础件”进入“已经具备页面级组装能力”的阶段。

## 单个组件的开发模板

每开发一个新组件，建议统一遵循下面的流程：

1. 定义名称与目标语义，例如 `NanInput` 对齐 shadcn 的 Input。
2. 明确它依赖哪些已有原语，例如 `NanSurface`、`NanPressable`、`ThemeManager`。
3. 先确定最小 API，再决定视觉细节。
4. 先完成 `default / hover / focus / disabled / invalid` 五类状态。
5. 补示例页，而不是只补单个孤立示例代码。
6. 完成后再决定是否抽出新的内部原语。

## NanInput 预研建议

因为 `NanInput` 会是下一阶段的核心入口，建议它的第一版只做这些能力：

- [ ] 单行文本输入
- [ ] `placeholderText`
- [ ] `text`
- [ ] `disabled`
- [ ] `readOnly`
- [ ] `invalid`
- [ ] `leading` / `trailing` 插槽
- [ ] 焦点环与错误态边框
- [ ] 与 `NanLabel` 配合使用的表单示例页

第一版不要急着加入：

- 密码强度
- 自动补全
- 复杂校验框架
- 掩码输入
- 多段格式化输入

这些都应该在基础输入体验稳定后再加。

## 维护规则

- README 中保留高层组件 Todo。
- 本文档维护完整路线图与阶段说明。
- 每完成一个组件，就同时更新 README 与本文档状态。
