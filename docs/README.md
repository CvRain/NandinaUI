# NandinaUI 文档

文档按读者目的分为三条路线。应用开发者可以从入门指南开始，再按需查阅组件；参与框架开发时则应先阅读开发参考。

## 入门指南

[Getting Started](getting_started/README.md) 通过一个逐步完善的桌面应用介绍窗口、按钮、布局、响应式状态和页面导航。这里仅使用面向应用开发者的推荐 API。

## 主题指南

[Guide](guide/README.md) 面向已经跑通第一个界面的使用者，按主题深入讲清"用这套框架做好一类
事情"的方法（布局与约束、响应式状态、主题与样式、键盘与无障碍等）。它是 Getting Started 与
组件手册之间的那一层。

> 该层目前只有选题清单，篇目均未编写（见 [Guide 的建议篇目](guide/README.md#建议的篇目)）。
> 在首篇落地之前，请以 `getting_started/` 与 `components/` 为准。

## 组件文档

[Components](components/README.md) 汇总当前可用组件及其成熟度。每篇组件文档同时包含用途、构建器写法、公开 API、交互规则与无障碍行为。

## 开发参考

[项目进度与下一步](references/project_status.md) 是统一实施排期，覆盖 D1–D8 与依赖工程，
区分已接受设计、现有实现和待验收能力。D7 混合修复与批次 2 已完成，当前推进批次 3 生产 SDF；专题编号不代表执行顺序。

[渲染设备路径契约](references/render_device_path_contract.md) 记录批次 3 最小实验：参数预算、
像素与失败注入、状态/更新成本，以及尚未接入生产的边界。

[References](references/README.md) 面向框架维护者，记录公共组件契约、架构约束、开发顺序和兼容性规则。这些文档用于回答“新组件应该如何设计”，而不是作为应用层教程。

[D5 资源构建、部署与运行时定位](references/resource_build_and_deployment.md) 已冻结设计，
实现仍为 planned；批次和验收进度见 [项目进度与下一步](references/project_status.md)。
其中的目标接口和构建选项不能直接作为当前应用用法。

[D6 作者 DSL、文本能力与内容测量](references/authoring_dsl_and_content_measurement.md) 汇总了
Label、构建器子项简写、组件扩展与响应式内容测量的设计方向；API 和测量契约仍待审阅，尚未实现。

## 文档边界

- `getting_started/`：稳定、短小、连续的学习路线。
- `guide/`：按主题深入的使用者材料 —— 入门之后"怎么做好一类事情"。
- `components/`：单个组件或同族组件的公开使用参考。
- `references/`：**开发手册**，面向框架维护者：架构决策、公共契约、开发流程、迁移记录、
  状态盘点与路线图。它不是使用者的学习材料；需要解释内部机制时，由使用侧文档链接过来。
- `images/`：项目级封面与公共图片；章节图片放在对应模块的 `images/` 下。

## 开发故事

[stories/](../stories/README.md)（仓库根目录）记录**项目为什么变成现在这样**：方向调整、
踩过的坑、被推翻的旧结论。`docs/` 只描述当前事实，决策过程放在那里会变成流水账；
但它同样不该被丢掉，所以单开一处。它不属于上面任何一条文档路线，面向的是要接手这个仓库的人。
