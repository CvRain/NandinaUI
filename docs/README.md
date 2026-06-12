# NandinaUI 文档导航

文档分为三类，按受众与用途区分：

## 1. 使用参考文档（面向库使用者）

位于 [`guide/`](guide/)。回答「我怎么用 NandinaUI 写界面」。

| 文档 | 说明 |
|------|------|
| [快速开始](guide/getting-started.md) | 环境、构建、第一个程序 |
| [核心概念](guide/core-concepts.md) | 分层、signal 响应式、组件组合、page/router 心智模型 |
| [主题与设计令牌](guide/theming.md) | token / palette / preset / size 的使用方式 |

> 每个组件/代码文件的**就近使用说明**放在对应的 `src/<layer>/README.md` 里，便于在代码旁直接查看。

## 2. 开发文档（面向贡献者）

位于 [`development/`](development/)。回答「内核为什么这样设计、该怎么实现」。

| 文档 | 说明 |
|------|------|
| [架构](development/architecture.md) | 定位、设计哲学、分层职责、依赖方向、稳定边界 |
| [重写路线图](development/roadmap.md) | 从哪个方向开始重写、阶段顺序、关键决策 |
| [模块依赖规则](development/module-dependency-rules.md) | 单向依赖约束，避免循环依赖 |
| [编码与 API 规范](development/coding-conventions.md) | Zig 命名、模块组织、API 风格 |
| [响应式策略](development/reactive-strategy.md) | signal / computed / effect 的语义规范（Angular 风格命名） |
| [设计令牌](development/design-tokens.md) | primitive token、semantic palette、preset/size/colorVariant 选型规则 |
| [组件 Primitives](development/widget-primitives.md) | Surface / Pressable / Text / FocusRing 的边界与组合规则 |
| [布局策略](development/layout-strategy.md) | 语义层 / 协议层 / 求解层分层与演进 |
| [Page / Router 合约](development/page-and-router.md) | page / router / 挂载的职责边界 |
| [Authoring 与挂载](development/authoring-and-mounting.md) | 声明式组件树、Ref/Handle、无显式所有权编排 |

## 3. 历史参照

旧 C++/QML 主线的设计文档保留在 [`../archive/docs/`](../archive/docs/)。这些文档记录了已被验证的思路（响应式语义、token 体系、primitive 边界），是本次 Zig 重写的语义来源，但**不作为当前实现依据**——以本目录文档与源码为准。

## 文档状态标记

- **已定稿**：可作为实现依据
- **草案**：待讨论或验证
- **持续更新**：随源码与测试修订的活文档
- **历史参照**：保留背景，非当前主线
