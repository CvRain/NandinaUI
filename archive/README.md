# NandinaUI

NandinaUI 的旧版 Qt/QML 组件库实现已归档到分支 `archive-0.0.1-qml`。

> 状态校正（2026-05）：主线已经不再是“仅规划”的空骨架。`foundation`、`runtime`、`reactive`、`layout`、`widgets`、`app`、`showcase` 均已有实际实现；当前阶段应理解为“实现推进中 + 主线收口中”，而不是“从零待建”。

## 当前状态

- **已决定**：旧 QML 主线不再继续在 `main` 演进。
- **已决定**：历史实现代码由 `archive-0.0.1-qml` 承担保留职责。
- **当前事实**：主线已有可运行的 showcase 验证应用，以及 runtime -> reactive/layout -> widgets -> app 的基础闭环。
- **当前重点**：优先完成 layout 主线收口、widgets/theme 统一消费，以及 authoring API 的继续验证。
- **仍处早期**：render 抽象、完整 text layout、脚本层与 bindings 路线仍未收口。

## 项目目标（Goals）

- 建立可长期维护的 UI runtime + 组件系统分层架构。
- 以 design-system-first 方式定义 theme/token/widget 边界。
- 保持对多语言接入（C++ 优先，脚本层可扩展）的架构可行性。
- 为后续实现提供可执行、可拆分、可验证的里程碑计划。

## 非目标（Non-Goals）

- 不在本阶段继续维护旧 QML 组件实现。
- 不把当前主线误解为“已经进入 API 冻结或可直接对外发布的正式稳定版”。
- 不把实验仓库内容直接视为已定稿实现。

## 为什么从 QML 组件库阶段进入重启规划

旧仓库在主题系统、语义 API、原语组合和示例组织上已积累经验，但也暴露出主线实现与未来目标（runtime 层抽象、渲染后端可扩展、脚本层能力）之间的结构性差距。

因此 `main` 先经历了“文档先行 + 架构先行”的阶段；而当前已经进入“边实现边收口”的阶段，文档需要以现有源码和测试覆盖为准持续校正。

## 文档导航

- **[开发文档导航页](docs/index.md)** — 所有开发文档的统一入口，按主题分类
- [开发 Issue 清单](docs/develop-issue.md) — 任务追踪
- [阶段路线图](docs/roadmap.md) — 当前收口重点与后续阶段顺序

## 历史说明

- 旧实现历史：`archive-0.0.1-qml`
- 当前主线：`main`（新架构实现推进中）

