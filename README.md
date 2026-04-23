# NandinaUI

NandinaUI 的旧版 Qt/QML 组件库实现已归档到分支 `archive-0.0.1-qml`。

`main` 分支已重置为：**正式项目启动前的规划与架构主线**。当前仓库不提供可运行框架实现，专注于方向统一、架构分层、迁移策略与开发路线。

## 当前状态

- **已决定**：旧 QML 主线不再继续在 `main` 演进。
- **已决定**：历史实现代码由 `archive-0.0.1-qml` 承担保留职责。
- **已决定**：`main` 用于正式项目重启前的文档化规划与仓库骨架整理。
- **待实现**：runtime/reactive/render/layout/theme/widgets 等新架构代码。

## 项目目标（Goals）

- 建立可长期维护的 UI runtime + 组件系统分层架构。
- 以 design-system-first 方式定义 theme/token/widget 边界。
- 保持对多语言接入（C++ 优先，脚本层可扩展）的架构可行性。
- 为后续实现提供可执行、可拆分、可验证的里程碑计划。

## 非目标（Non-Goals）

- 不在本阶段继续维护旧 QML 组件实现。
- 不在本阶段交付可运行的正式框架。
- 不把实验仓库内容直接视为已定稿实现。

## 为什么从 QML 组件库阶段进入重启规划

旧仓库在主题系统、语义 API、原语组合和示例组织上已积累经验，但也暴露出主线实现与未来目标（runtime 层抽象、渲染后端可扩展、脚本层能力）之间的结构性差距。

因此 `main` 先回到“文档先行 + 架构先行”的状态：先统一边界与路线，再进入新一轮实现。

## 文档导航

- [开发文档导航页](docs/index.md) — 统一入口，按主题分类所有技术文档
- [项目方向](docs/project-direction.md)
- [架构规划](docs/architecture-plan.md)
- [从 QML 迁移策略](docs/migration-from-qml.md)
- [阶段路线图](docs/roadmap.md)
- [仓库结构草案](docs/repository-structure.md)

## 历史说明

- 旧实现历史：`archive-0.0.1-qml`
- 当前主线：`main`（正式项目重启规划）

