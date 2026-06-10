# NandinaUI 开发文档导航

> 本文档为开发文档的入口索引，提供按主题分类的文档导航。README 主要承担项目概述职责，详细技术文档统一在此导航。
>
> 状态校正（2026-05）：主线代码已超出“纯规划阶段”。阅读文档时，优先参考带有“状态校正 / 当前判断 / 当前进展”字段的文档；对明显仍停留在占位口径的文档，应以源码、测试和 issue 清单为准。

## 目录

### 项目概览

| 文档 | 说明 |
|------|------|
| [项目方向](project-direction.md) | 项目定位、设计哲学、演进原因 |
| [从 QML 迁移说明](migration-from-qml.md) | 历史背景、迁移原则、归档分支说明 |
| [仓库结构与模块边界](repository-structure.md) | 当前目录现状、保留的规划边界与实现状态 |

### 模块设计文档

| 文档 | 说明 |
|------|------|
| [NanPoint 设计文档](../foundation/docs/design-nan_point.md) | 基础点类型接口设计 `[已定稿]` |
| [NanRect 设计文档](../foundation/docs/design-nan_rect.md) | 矩形类型接口设计 `[已定稿]` |

### 架构与设计

| 文档 | 说明 |
|------|------|
| [架构规划](architecture-plan.md) | 顶层分层、依赖方向、稳定边界 `[已定稿]` |
| [Page / Router 合约（MVP）](page-contract.md) | 固定 `NanPage` / `NanRouter` / `NanPageHost` 当前职责边界与后续演进顺序 `[已定稿]` |
| [组件 Authoring 与挂载 API 设计](component-authoring-and-mounting.md) | 面向使用者的组件组合、挂载、引用与无显式 `move` 方向 `[已校正]` |
| [无显式 move 的组件组合 API（V1）](component-composition-api-v1.md) | 当前 V1 authoring API 的已落地子集、差异说明与未收口边界 `[已校正]` |
| [Widget Primitives 设计文档](widget-primitives.md) | 固定 Surface / Pressable / Text / FocusRing 的 primitive 边界与组合规则 `[已校正]` |
| [Design Tokens](design-tokens.md) | 固定 primitive token、semantic palette、preset/size/colorVariant 与 TypographyRole 的选型规则 `[已校正]` |
| [Overflow / Clip Contract 设计文档](overflow-and-clip-contract.md) | 固定父容器 child containment、`overflow` 语义与 `PushClip / PopClip` 的分层落点 `[草案]` |
| [Input / Field API 设计](input-and-field-api.md) | 固定 TextField / Field 的职责边界、authoring 契约与绑定语义 `[已校正]` |
| [布局策略](layout-strategy.md) | 当前布局阶段策略、自动布局目标与 Yoga 接入时机 `[已校正]` |
| [模块依赖规则](module-dependency-rules.md) | 模块间依赖方向约束，避免循环依赖 `[已定稿]` |
| [响应式策略](reactive-strategy.md) | 当前 reactive 主线的语义规范、实现进度与未收口边界 `[已校正]` |
| [Godot-like 开发范式（历史参考）](godot-like-authoring-draft.md) | 早期受 Godot 影响的应用/窗口继承式设计背景，非当前主线 authoring 路径 `[历史参考]` |

### 开发规范

| 文档 | 说明 |
|------|------|
| [编码与 API 规范](coding-and-api-conventions.md) | 命名风格、命名空间、public/internal API 约定 `[已定稿]` |
| [开发 Issue 清单](develop-issue.md) | 按模块拆分的细颗粒度开发任务清单 `[持续更新]` |

### 路线图

| 文档 | 说明 |
|------|------|
| [阶段路线图](roadmap.md) | 当前收口重点与后续阶段顺序 `[已按现状校正]` |

## 快速链接

- **[开发 Issue 清单](develop-issue.md)** — 当前开发任务追踪，按 Milestone 划分
- **[架构规划](architecture-plan.md)** — 了解各模块职责与依赖关系
- **[编码规范](coding-and-api-conventions.md)** — 实现前必读的开发约定

## 项目现状

### 已完成模块

| 模块 | 状态 | 说明 |
|------|------|------|
| 目录骨架 (Issue 001) | ✅ 完成 | 所有顶层目录已创建 |
| 模块依赖规则 (Issue 002) | ✅ 完成 | 详见 `module-dependency-rules.md` |
| 编码与 API 规范 (Issue 003) | ✅ 完成 | 详见 `coding-and-api-conventions.md` |
| 几何类型 (Issue 006) | ✅ 完成 | `NanPoint`、`NanSize`、`NanRect` 已实现并测试 |
| Log 服务 | ✅ 完成 | 基于 spdlog 的日志框架 |
| Color 模块 | ✅ 完成 | `NanColor` 及颜色转换 |
| Window 基础 | ✅ 完成 | `NanWindow` 窗口基类 |
| Reactive 核心 | ✅ 主体完成 | `State`、`Effect`、`Computed`、`Prop`、`batch` 已实现并测试 |
| Layout 主线 | ✅ 当前阶段完成 | `constraints -> measure/layout -> root reflow -> widgets/showcase -> regression tests` 主线已接通 |
| App 层抽象 | ✅ 完成 | `NanAppWindow`、`NanComponent` 应用层生命周期封装 |
| Showcase 验证应用 | ✅ 可运行 | 基于 ThorVG/SDL 的验证应用，当前主要承担 shell + page + layout 回归面 |

### 进行中

| 模块 | 状态 | 说明 |
|------|------|------|
| Layout 主线收口 | ✅ 已完成 | 当前阶段验收口径已达成；后续重点转入复杂 flex 语义、widgets 专项测试与 Yoga 评估前置条件 |
| Widgets 自动布局收口 | ⚠️ 主体完成 | Issue 086 已完成；当前重点转入 widgets 专项测试、primitive/theme 收口，以及延后的 Sidebar 结构化演进 |
| Theme / Design System | ⚠️ 主体推进中 | primitive tokens、palette、ThemeManager 与 Button/TextField/SidebarMenuButton/ProgressBar 的 colorVariant 公开 API 已接通；后续重点转入更高层 style layer |

### 待开发

详见 [开发 Issue 清单](develop-issue.md)。当前建议优先关注 primitive/control 收口、widgets 专项测试、theme 统一消费与后续表单垂直切片。

## 里程碑进度

| 里程碑 | 状态 | 关键 Issue |
|--------|------|------------|
| M0: 文档与工程基线 | ✅ 已完成 | Issue 001-004 |
| M1: runtime + reactive | ⚠️ 核心闭环完成 | Issue 005-028 |
| M2: layout + theme | ⚠️ layout 主线完成，theme 继续推进 | Issue 029-042, Issue 082-089 |
| M3: first widgets | ⚠️ 已有可运行实现，继续收口 | Widgets / Showcase 相关章节 |
| M4: app shell + router/page | ⚠️ 核心骨架已落地，待收口 | App / Showcase 相关章节 |
| M5: render abstraction / text / scriptability | 🚧 早期 | Render / Text / Docs 相关章节 |

> 注：这里的里程碑口径与 [阶段路线图](roadmap.md) 保持一致；[开发 Issue 清单](develop-issue.md) 中的 Milestone 划分更细，用于跟踪模块级实现进度。
> 其中 M4 当前应理解为：Page / Router / PageHost 与 showcase shell 已经接通，但 registry、测试覆盖与更通用的 app 收口工作仍在后续范围内。

## 文档状态说明

- **已定稿**：已通过评审，可作为实现依据
- **草案**：待进一步讨论或验证
- **持续更新**：以当前源码与测试为准持续修订的活文档
- **历史参考**：保留背景与设计来源，但不是当前主线推荐路径
- **待撰写**：计划中但尚未完成

## 贡献指南

1. 新增文档前请先确认是否已有相关主题
2. 架构类文档需标注"已决定/建议方向/待验证"状态
3. 实现类文档应包含完成定义（Definition of Done）
4. 文档更新后请同步维护本文档导航

## 外部链接

- [GitHub 仓库](https://github.com/CvRain/NandinaUI)
- [文档源码目录](https://github.com/CvRain/NandinaUI/tree/main/docs)
- [QML 历史分支](https://github.com/CvRain/NandinaUI/tree/archive-0.0.1-qml)
