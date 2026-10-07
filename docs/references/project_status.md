# 项目进度与下一步

> 截至 2026-10-07 的源码与文档核对。本文区分“实现存在”“作者可用”和“仍是草案”；不把旧头文件的存在视为永久兼容承诺。组件细目见 [组件路线图](component_roadmap.md)。Anchors 作者模型已有正式设计 reference，但代码尚未实现。

## 当前能力边界

| 方向 | 当前状态 | 下一道边界 |
| --- | --- | --- |
| 组件与浮层 | 阶段 1–4 的基础设施与清单已经落地；阶段 5 未开始 | 复合数据组件需要稳定的布局、选择和虚拟化契约 |
| 文本与分层 | 纯布局约束在 `foundation`，文本布局协议在 `text`，字体请求值在 `theme`；旧布局、文本与字体转发入口已退出 | `theme::StyleDocument` 对字体注册的依赖仍待收口 |
| 动画 | 值级 motion 在 `foundation/motion`，节点 endpoint、宿主与组调度在 `scene`；旧 `animation/` 入口已退出；L2 opacity/translate/scale 可用 | `NodeBuilder::group` 仅开放同节点、扁平 Tween 组合；跨节点、组内 Spring、Keyframes 作者入口与浮层自动过渡未完成，见 [节点表现层 §6.1](node_presentation.md) |
| 布局 | 排列容器及 `NanControl::layout_to` 已用在生产路径；浮层有独立锚定定位器 | 场景树 anchors 尚无实现，不能依赖草案 API 编写应用 |
| 分发 | Linux 桌面源码分发 profile 已有构建与 SDK 流程 | Modules、CMake package 与跨平台仍待重新评估 |

## 近期顺序

1. **开工前验证并收敛规范。** 作者模型已记录在 [Anchors 布局设计](anchors.md)：显式锚定画布、`.anchor.*` 与 `parent` 关系、`NodeRef<T>` 匿名目标、`ControlSizeSpec` 尺寸归属及同画布兄弟引用。先验证无界百分比、隐藏子项、目标生命周期/reparent、画布约束传播与 ScrollView 边界，再把每项结论固化为实现规则。
2. **实现锚定布局内核。** 自下而上补齐锚线/关系值、显式锚定画布、尺寸解析与冲突校验、兄弟依赖排序及环检测；求解结果统一经 `layout_to` 写入，不引入第二套几何或 presentation 位移。
3. **集成、验收与手工体验。** 测试父锚与兄弟锚、百分比边界、嵌套排列、非法混用、重复布局稳定性，以及 global bounds / hit test / semantics 一致性；每项测试注明故障注入点。随后更新 showcase/playground，以侧边栏/编辑区案例供作者检查实际手感。
4. **再评估后续阶段。** Anchors 核心边界通过后，重新排序阶段 5 复合组件与浮层过渡动效；它们不是 anchors 求解器的前置条件。

过渡源码入口已分批退出；这不表示兼容层清理是 anchors 求解器的技术前置。后续仍按“在扩张复杂组件前先稳定基础设施”的顺序推进。

### 兼容层退出的分批边界

第一批退出 `nandina/animation/` 的别名头：值与规格统一使用 `nandina::motion`，endpoint、宿主与组统一使用 `nandina::scene`。库与测试调用、legacy 独立头测试和 SDK 导出清单同步迁移；`animation::motion` 也不再保留。已有 `foundation/motion/spec.hpp` 公开头承担独立包含测试。

第二批已退出 `scene::LayoutConstraints`：公开 API、库内实现和测试统一写 `foundation::NanLayoutConstraints`；删除 `scene/control.hpp` 中的别名与只验证旧拼写的兼容测试，保留 foundation 数值回归。不改字段、函数签名的实际类型、约束计算或布局行为。文本与字体的旧转发头另列第三批，不与这一批机械改写混合。历史故事保留原样。

第三批已退出 `widget/primitives/text_layout.hpp`、`text_layout_backend.hpp` 与 `text/font_request.hpp` 三个纯别名头。文本布局协议及默认 backend 的唯一公开归属是 `text/text_layout*.hpp`；轻量字体请求值的唯一公开归属是 `theme/font_request.hpp`。组件公开签名、测试和示例改用 `text::` / `theme::`，不搬动字体注册器、布局算法或 StyleDocument 的引擎适配。删旧头及旧入口专用测试，保留 canonical 独立包含与行为测试；变更后的库和使用者须一同重新编译。

## Anchors 设计关口

作者模型和已达成的边界以 [Anchors 布局设计](anchors.md) 为准；[历史草案](anchor_draft.md)中的 Q1–Q7 已过期。实现前仍需验证百分比遇到无界约束、隐藏子项参与测量、弱引用生命周期与 reparent、画布尺寸约束传播，以及 overflow / ScrollView 的交叉边界。场景树已有 z 序，不应把新增 z 系统列为 anchors 前置。当前没有可供应用依赖的 anchors 代码 API。

作者模型已经记录在正式 reference 中；剩余边界验证完成后，按[开发循环](development_loop.md)将规则补进规范，再自下而上实现。不要先公开会“设置成功但不生效”的 DSL。

## 验证与交付

兼容层退出会跨多个模块，需跑主套件、ASan/UBSan unit、no-RTTI unit，核对 suite 注册、SDK 导出与独立 include/消费者；构建和测试不得通过管道掩盖失败。anchors 实现还需覆盖重复布局、兄弟依赖、环、重挂载、绘制/命中同序与语义 bounds。真实窗口手感仍由作者在 playground/showcase 验收，自动测试不能替代。
