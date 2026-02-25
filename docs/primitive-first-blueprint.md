# Primitive-first 重构蓝图（v0.x）

## 1. 背景与目标

当前项目允许破坏性迭代，核心目标是先夯实底层，再快速重建组件层。

- 单一主题真源：`Nandina.Theme`（`NanStyle` / `NanTheme` / `ThemeManager`）
- 组合优先：组件由 Primitive 组合，不走深继承树
- 可维护优先：统一状态机、事件语义、样式计算契约

---

## 2. 组件展示的基本流程（统一模型）

一个组件从“定义”到“可见”的标准流程：

1. **模块注册**：组件文件被 `qt_add_qml_module` 导出到 URI（如 `Nandina.Controls`）。
2. **实例创建**：业务页 `import` 后实例化组件并传入属性。
3. **主题解析**：组件从 `NanStyle.themeManager`（或显式 themeManager）获取当前 palette。
4. **状态初始化**：建立基础状态（`hovered/pressed/focused/disabled`）。
5. **样式计算**：由 `variant/size/state + palette + tokens` 计算视觉值。
6. **渲染合成**：`Surface + Content + Indicator` 等 Primitive 输出最终 UI。
7. **交互驱动**：输入事件更新状态，触发信号和重新渲染。

> 规范要求：组件内不再维护多套主题解析链，不直接耦合 palette 内部字段结构。

---

## 3. 分层架构

### 3.1 Theme 层（唯一真源）
- 责任：主题上下文、调色盘切换、继承作用域。
- 对外：提供 `themeManager`、`font` 的统一读取入口。

### 3.2 Tokens 层（设计系统常量）
- 责任：spacing/radius/typography/motion 等语义 token。
- 约束：业务控件不硬编码尺寸与动效时长。

### 3.3 Primitives 层（基础能力）
- `BaseControl`：统一主题入口和基础状态字段。
- `Pressable`：鼠标/键盘按压语义与事件序列。
- `Surface`：背景、边框、圆角语义容器。
- `Indicator`：二态/三态指示器。
- `FormFieldBehavior`：输入型状态优先级。

### 3.4 Controls 层（语义组件）
- 按钮、输入框、开关、复选框、侧边栏等。
- 原则：只负责“语义 API + 结构拼装”，不重复造状态机。

### 3.5 Example/Docs 层
- 验证场与用法示例。
- 约束：示例不直接依赖底层 palette 细节字段。

---

## 4. 统一设计契约

### 4.1 状态字段契约
基础控件至少具备：

- `disabled`
- `hovered`
- `pressed`
- `focused`

输入型控件额外具备：

- `invalid` / `hasError`
- `hasSuccess`
- `helperText`

### 4.2 事件语义契约
交互组件应统一信号语义：

- `pressStarted`
- `clicked`
- `released`
- `canceled`

### 4.3 样式计算契约
统一采用“纯输入 -> 纯输出”的样式计算方式：

- 输入：`variant/size/state/theme/tokens`
- 输出：`foreground/background/border/hover/pressed`

---

## 5. 重构开发模块（Task / Action）

## 模块 A：基础治理（已启动）

### Task A1：主题解析单一化
- Action A1.1：移除控件内部无效 fallback ThemeManager 模板段。
- Action A1.2：收敛到 `NanStyle/NanTheme` 统一入口。
- 验收：控件不再出现重复主题解析实现。

### Task A2：Primitive 边界冻结
- Action A2.1：冻结首批 Primitive 职责，不新增模糊组件。
- Action A2.2：形成“组件必须复用的能力清单”。
- 验收：新控件不得绕过 Primitive 直接复制状态逻辑。

## 模块 B：低风险组件重建（优先）

### Task B1：Switch / Checkbox / Label
- Action B1.1：全部接入 `BaseControl`。
- Action B1.2：抽离重复状态与配色分支。
- 验收：行为与现有 Demo 一致，代码重复率明显下降。

## 模块 C：核心交互组件重建

### Task C1：NanButton 分层重构
- Action C1.1：拆分“行为层（Pressable）/样式层/视图层”。
- Action C1.2：保持公开 API 不变，内部改造。
- 验收：按钮行为事件与视觉回归通过。

### Task C2：NanInput 语义化重构
- Action C2.1：统一校验时机与状态优先级。
- Action C2.2：把输入状态迁移到 `FormFieldBehavior`。
- 验收：focus/error/success/helperText 行为一致。

## 模块 D：复杂容器与维护闭环

### Task D1：SideBar 体系重建
- Action D1.1：统一 item/group/trigger 的状态和主题读取。
- Action D1.2：减少 SideBar 家族内部重复逻辑。
- 验收：折叠、焦点、激活态在左右 dock 都一致。

### Task D2：文档与示例收口
- Action D2.1：更新对外用法文档。
- Action D2.2：清理过期文档、无效示例和遗留说明。
- 验收：docs 与代码现状一致，无明显过期描述。

---

## 6. 新组件开发模板（必须遵守）

新增任意组件时按以下顺序实现：

1. 定义语义 API（属性/信号）
2. 接入 `BaseControl` 与主题上下文
3. 接入状态能力（按需组合 `Pressable` / `FormFieldBehavior`）
4. 用 token 构建尺寸与动效
5. 抽离样式计算函数
6. 在示例页验证交互与主题切换
7. 写入文档与迁移说明

---

## 7. 质量门槛（DoD）

任一完成迁移的组件必须满足：

- 主题切换无明显状态错乱
- 键盘可用（Tab 聚焦，Enter/Space 触发）
- 状态视觉一致（normal/hover/pressed/focus/disabled）
- 代码不含重复 fallback 主题模板段
- 示例页可运行并保持行为一致
