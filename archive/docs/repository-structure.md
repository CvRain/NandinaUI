# Repository Structure

> 状态校正（2026-05）：本文件已从“未来目录草案”改为“当前目录现状 + 仍然有效的模块边界说明”。顶层目录不再是占位骨架，绝大多数核心模块已经有实际代码落位。

## 当前目录现状

```text
NandinaUI/
├── runtime/      # UI runtime 基础能力
├── reactive/     # 响应式核心抽象与调度
├── render/       # 渲染抽象与后端适配
├── layout/       # 布局系统
├── theme/        # token/theme schema 与应用
├── widgets/      # 组件库
├── app/          # 应用层与 authoring 组织
├── bindings/     # 多语言绑定与脚本宿主接口
├── showcase/     # 展示与验证应用
├── tests/        # 分层测试
└── docs/         # 项目方向、架构、路线图
```

## 当前实现状态

| 目录 | 当前状态 | 说明 |
|------|----------|------|
| `foundation/` | 已落地 | 几何、颜色、约束、类型等基础能力已有实现与测试 |
| `runtime/` | 已落地 | 窗口、事件、Widget tree 与绘制主循环已形成闭环 |
| `reactive/` | 已落地 | `State` / `Effect` / `Computed` / `Prop` / `batch` 已实现 |
| `layout/` | 进行中 | 基础布局原语已落地，当前重点是主线收口 |
| `render/` | 早期 | 仍缺正式 Scene / DrawCommand 中间层与后端抽象 |
| `text/` | 早期可用 | 字体测量与基础绘制可用，完整 text layout 仍未收口 |
| `theme/` | 早期 | token 类型已存在，但统一 resolver/消费链路仍待推进 |
| `widgets/` | 已有实现，待收口 | primitives 与首批 controls 已存在，但部分仍保留手工布局 |
| `app/` | 已落地 | page/router/page_host 与第一版 authoring API 已形成闭环 |
| `bindings/` | 占位 | 仍主要保留扩展边界，尚未形成实质主线实现 |
| `showcase/` | 可运行 | 当前承担 shell/page/layout/widgets 的验证载体 |
| `tests/` | 持续扩展 | foundation/reactive/layout/runtime/app/showcase 已有覆盖 |
| `docs/` | 持续更新 | 需持续移除纯规划态口径，并按实现进度校正 |

## 目录存在意义

- 将“运行时能力”与“组件表达”解耦。
- 将“框架内核”与“应用开发层”解耦。
- 将“稳定边界”前置到目录与模块层级，降低后续返工成本。

## 仍然有效的规划边界

- `foundation -> runtime -> reactive/layout/theme/text -> widgets -> app -> showcase` 仍是主线依赖方向。
- `bindings/` 仍应视为未来扩展边界，而不是当前主实现入口。
- `render/` 虽已在构建系统中存在，但正式抽象层仍未完成，不应把当前 ThorVG 直连实现误认为最终形态。

