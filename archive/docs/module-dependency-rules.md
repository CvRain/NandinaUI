# NandinaUI 模块依赖约束规范

> 状态校正（2026-05）：本文件描述当前仍然有效的依赖方向约束。主线模块已经实际落位，不应再把它理解成“未来目录规划”；真正尚早期的主要是 render abstraction 与 bindings。

为了保持架构清晰、避免循环依赖并确保层级职责明确，本项目严格遵守以下依赖规则。

## 模块分层与职责

| 模块 | 职责描述 |
| :--- | :--- |
| **foundation** | 基础公用工具（Log、几何类型、基础宏/配置）。 |
| **runtime** | 底层平台抽象。窗口、事件、`NanWidget` 基础原语；ThorVG 目前仍直接在此层及上层消费。 |
| **reactive** | 响应式核心。State、Effect、Computed、Prop 等。 |
| **layout** | 布局计算。Row、Column、Stack、constraints 与 flex widgets。 |
| **theme** | 主题与 Token。Theme context、Token 映射。 |
| **text** | 字体、文本 shaping、测量与绘制。 |
| **render** | 渲染抽象与后端适配边界；当前正式抽象层仍未收口。 |
| **widgets** | 基础组件库。Button、Label、Card、Panel 等。 |
| **app** | 应用开发层。`NanAppWindow`、`NanComponent`、authoring、Page、Router。 |
| **showcase** | 验证应用。仅用于演示和测试，不被其它模块依赖。 |

## 依赖方向规则（单向）

1. **禁止向上依赖**：底层模块严禁依赖高层模块（例如 `runtime` 绝不能引用 `widgets` 或 `app` 中的类型）。
2. **禁止循环依赖**：如果 A 依赖 B，则 B 及其所有子依赖都不允许依赖 A。
3. **runtime 独立性**：`runtime` 仅依赖 `foundation`（以及被隐藏在实现单元内的平台/绘制库）。它不感知响应式系统或具体业务组件。
4. **reactive 独立性**：`reactive` 作为一个纯逻辑层，不依赖 `render` 或 `layout`。
5. **text / theme / layout 作为中层能力**：这些模块可以向下依赖 `foundation/runtime/reactive` 中的必要子集，但不能反向依赖 `widgets/app`。
6. **app 作为汇聚层**：`app` 模块可以依赖 `widgets`、`runtime`、`reactive`、`layout`、`theme`、`text` 等底层模块，用于编排完整的应用。
6. **showcase 隔离性**：`showcase` 依赖 `app`，但任何核心模块（foundation 到 app）都严禁依赖 `showcase`。

## 物理实现约束

- **C++ Modules 导出**：所有模块应通过 `export module` 暴露公共接口。
- **PIMPL 隔离**：在 `runtime` 中，SDL3 等第三方库的头文件应尽可能隐藏在 `.cpp` 实现单元中，避免污染消费者的全局命名空间。
- **GMF (Global Module Fragment)**：三方头文件（如 ThorVG）应置于模块文件的 `module;` 段落。

## 违反约束的后果

任何违反上述依赖方向的提交都将被视为技术债，必须在进入下一 Milestone 前予以重构。
