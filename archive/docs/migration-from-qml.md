# Migration from QML

> 状态校正（2026-05）：`main` 已不再只是“正式项目规划仓库”，而是当前新架构的实际实现主线。本文应作为历史迁移动机与保留原则阅读，而不是把主分支理解成纯文档占位仓库。

## 迁移目标

将 `main` 从旧 QML 实现仓库迁移为当前新架构的主开发分支；历史代码由 `archive-0.0.1-qml` 保留。

## 保留的经验

- 主题系统与 token 分层思路
- primitive-first 的组件构建方式
- showcase / 多页面示例对开发验证的价值
- 语义 API 与一致命名规范
- 组件状态与交互反馈的设计经验

## 放弃或重做的内容

- 对 Qt/QML 绑定机制的主线依赖
- 旧控件与示例应用实现代码
- 直接依附 Qt Quick Controls 的实现心智
- 原 `main` 上以 QML 组件目录为中心的仓库结构

## archive 分支角色

- `archive-0.0.1-qml`：保存旧实现、可用于历史追溯与经验参考。
- `main`：承载正式项目新架构的当前实现与后续演进入口。

## 迁移原则

1. 不把实验代码“原样搬运”为正式标准。
2. 文档优先于代码重启，先统一边界和阶段目标。
3. 明确“已决定 / 建议方向 / 待验证”三类状态，避免误导。

## 当前迁移结果

- `main` 上的 `foundation/runtime/reactive/layout/theme/text/widgets/app/showcase` 已有实际实现
- QML 经验仍主要作为 design system、组件语义与 showcase 组织方式的参考来源
- 当前主线的工作重心已经从“迁移立项”转为“layout / widgets / authoring 收口”

