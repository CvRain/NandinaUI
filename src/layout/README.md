# layout

布局系统层。**依赖 foundation**。

## 计划 API

- `Constraints`：`{ min_width, max_width, min_height, max_height }`。
- 协议：`measure(constraints) -> Size`（自底向上求 preferred size）、`layout(bounds: Rect)`（自顶向下分配几何）。
- 基础容器：`Row` / `Column` / `Stack` / `Padding`，支持 `gap` / `align` / `justify`。

## 三层模型

1. 语义层：开发者看到的布局原语，保持框架自身语义。
2. 协议层：constraints / preferred size / measure-layout 边界（核心，必须稳定）。
3. 求解层：自有轻量求解；未来复杂 flex 可接入 Yoga 作为可插拔后端。

## 设计要点

- 几何计算只发生在本层内部，**禁止泄漏到 widgets / app 业务代码**。
- 当前不 Yoga-first；先稳定协议层，为 Yoga 预留接入位。

## 状态

🚧 骨架。详见 [布局策略](../../docs/development/layout-strategy.md)。
