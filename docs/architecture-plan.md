# Architecture Plan

## 顶层分层（目标态）

- `runtime`
- `reactive`
- `render`
- `layout`
- `theme`
- `widgets`
- `app`
- `bindings`
- `showcase`
- `docs`

## 分层职责

### runtime
UI 运行时基础：生命周期、节点树、事件循环、调度边界。

### reactive
响应式核心抽象（如 State / Effect / Prop / StateList）及依赖追踪与调度。

### render
渲染抽象层与后端适配接口。当前策略是“后端抽象优先”，而非立刻自研完整绘制库。

### layout
从 Row/Column/Stack 的基础语义起步，先稳定自身的 constraints / preferred size / slot layout 协议，再逐步对齐 Flex/Yoga 路线。Yoga 作为后续复杂容器的求解后端预留，不直接主导当前语义层设计。

### theme
token/theme schema 与运行时主题应用，不与具体 widget 实现耦合。

### widgets
组件库层，消费 runtime/reactive/layout/theme 能力，保持语义 API 一致性。

### app
应用开发层（authoring layer），承载页面、路由、窗口/控制器编排。

### bindings
多语言绑定与脚本宿主接口（C++ 默认优先，脚本为可扩展方向）。

### showcase
用于验证组件与应用模式的展示应用，不作为核心运行时依赖。

## 依赖方向（单向）

`runtime/reactive/render/layout/theme` → `widgets` → `app/showcase`

`bindings` 与 `app` 对接，但不反向侵入 runtime 内核。

## 稳定边界（优先建立）

1. runtime 与 reactive 的调度边界。
2. render 与具体后端的抽象边界。
3. theme/token 与 widgets 的接口边界。
4. app authoring 与 widgets 的语义 API 边界。
5. layout 语义层、协议层与未来求解层（含 Yoga）之间的边界。

## 当前布局策略

- 当前阶段优先消除 widgets 与 showcase 装配层中的手工几何计算。
- 基础布局原语保持自有语义，不直接等同于 Yoga API。
- Yoga 应作为未来复杂 flex 容器的可插拔求解后端接入，而不是当前 layout 核心。

详见 [Layout Strategy](layout-strategy.md)。

## 来自实验阶段的吸收点

- 响应式核心抽象（State/Effect/Prop/StateList）
- page/router 思维与 showcase app 组织方式
- window/controller 责任拆分
- 渲染后端可替换抽象

> 注：以上为当前建议方向，不代表实现已定型。

