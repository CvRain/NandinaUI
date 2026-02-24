# Primitive-first M0 契约（v0.x）

## 1. 目标

在 v0.x 阶段将 Nandina 收敛为：

- 单一主题真源：`Nandina.Theme`（`NanStyle` / `NanTheme` / `ThemeManager`）
- 组合优先：业务控件由 Primitives 组合实现，而非深继承链
- 可破坏演进：允许调整控件内部实现与不稳定 API

## 2. 分层与边界

- `Nandina.Theme`：主题管理、调色盘、附加属性、主题作用域
- `Nandina.Tokens`：spacing/radius/motion/typography 设计 token
- `Nandina.Primitives`：基础交互与视觉原语（对 Controls 暴露）
- `Nandina.Controls`：语义组件（Button/Input/Switch/Checkbox/SideBar 等）
- `Nandina.Experimental`：验证性组件与迁移试验场

## 3. 允许与禁止

### 3.1 允许

- 在 QML 中通过 `NanStyle.themeManager` 读取主题
- 通过 Primitive 统一 hover/pressed/focused/disabled 等状态
- 在 Experimental 中先行验证原语，再迁移到 Controls

### 3.2 禁止

- 控件内重复实现主题解析链（多套 fallback 逻辑并存）
- 新增深继承路径（如 NanObject -> NanItem -> NanControl -> ...）
- 业务组件直接耦合底层 palette 的内部字段结构

## 4. 首批 Primitive（M0 冻结）

- `BaseControl`：主题入口、基础状态（focused/hovered/pressed/disabled）
- `Pressable`：鼠标/触控点击状态机与事件
- `Surface`：背景层、边框、圆角语义
- `Indicator`：勾选/开关的可视指示层
- `FormFieldBehavior`：输入状态优先级（disabled/error/success/focused/default）

## 5. Definition of Done（M0）

- 工程可导入 `Nandina.Primitives` 模块
- `Nandina.Experimental.Controls.NanItem` 基于 `BaseControl`
- 文档明确“单一主题真源 + 组合优先”约束
- 不修改现有示例功能行为
