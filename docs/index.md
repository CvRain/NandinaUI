# NandinaUI 开发文档导航

> 本文档为开发文档的入口索引，提供按主题分类的文档导航。README 主要承担项目概述职责，详细技术文档统一在此导航。

## 目录

### 项目概览

| 文档 | 说明 |
|------|------|
| [项目方向](project-direction.md) | 项目定位、设计哲学、演进原因 |
| [从 QML 迁移说明](migration-from-qml.md) | 历史背景、迁移原则、归档分支说明 |
| [仓库结构草案](repository-structure.md) | 顶层目录规划与职责划分 |

### 模块设计文档

| 文档 | 说明 |
|------|------|
| [NanPoint 设计文档](../foundation/design-nan_point.md) | 基础点类型接口设计 `[已定稿]` |
| [NanRect 设计文档](../foundation/design-nan_rect.md) | 矩形类型接口设计 `[已定稿]` |

### 架构与设计

| 文档 | 说明 |
|------|------|
| [架构规划](architecture-plan.md) | 顶层分层、依赖方向、稳定边界 `[已定稿]` |
| [模块依赖规则](module-dependency-rules.md) | 模块间依赖方向约束，避免循环依赖 `[已定稿]` |
| [响应式策略](reactive-strategy.md) | State/Effect/Prop 等响应式抽象设计说明 `[草案]` |
| [Godot 式 Authoring 草案](godot-like-authoring-draft.md) | 组件化创作模式设计草案 `[草案]` |

### 开发规范

| 文档 | 说明 |
|------|------|
| [编码与 API 规范](coding-and-api-conventions.md) | 命名风格、命名空间、public/internal API 约定 `[已定稿]` |
| [开发 Issue 清单](develop-issue.md) | 按模块拆分的细颗粒度开发任务清单 `[已定稿]` |

### 路线图

| 文档 | 说明 |
|------|------|
| [阶段路线图](roadmap.md) | M0-M5 各阶段目标与依赖关系 `[草案]` |

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
| App 层抽象 | ✅ 完成 | `NanAppWindow`、`NanComponent` 应用层生命周期封装 |
| Showcase 验证应用 | ✅ 完成 | 基于 ThorVG 的组件展示窗口，含 MainComponent/MainWindow |

### 进行中

| 模块 | 状态 | 说明 |
|------|------|------|
| 当前暂无进行中的模块 | — | — |

### 待开发

详见 [开发 Issue 清单](develop-issue.md)，按 Milestone M1-M10 规划。

## 里程碑进度

| 里程碑 | 状态 | 关键 Issue |
|--------|------|------------|
| M0: 文档与工程基线 | ✅ 已完成 | Issue 001-004 |
| M1: Foundation 与 Runtime MVP | 📋 规划中 | Issue 005-017 |
| M2: Reactive 模块 | 📋 规划中 | Issue 018-028 |
| M3: Layout 模块 | 📋 规划中 | Issue 029-036 |
| M4: Theme 与 Design System | 📋 规划中 | Issue 037-042 |
| M5-M10 | 📋 规划中 | 渲染、组件、应用层等 |

## 文档状态说明

- **已定稿**：已通过评审，可作为实现依据
- **草案**：待进一步讨论或验证
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
- [实验代码目录](temp/) — 包含 Reactive/Layout/Core 等模块的实验实现
