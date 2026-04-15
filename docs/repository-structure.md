# Repository Structure (Draft)

## 未来目录草案

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

## 目录存在意义

- 将“运行时能力”与“组件表达”解耦。
- 将“框架内核”与“应用开发层”解耦。
- 将“稳定边界”前置到目录与模块层级，降低后续返工成本。

## 当前状态说明

这些目录目前是占位骨架，用于承接后续正式实现，不代表对应模块已完成开发。

