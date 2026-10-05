# 项目进度与下一步

> 截至 2026-10-05 的源码与文档核对。本文区分“实现存在”“作者可用”和“仍是草案”；不把旧头文件的存在视为永久兼容承诺。组件细目见 [组件路线图](component_roadmap.md)。锚点设计仍是工作草案，尚未并入正式规范。

## 当前能力边界

| 方向 | 当前状态 | 下一道边界 |
| --- | --- | --- |
| 组件与浮层 | 阶段 1–4 的基础设施与清单已经落地；阶段 5 未开始 | 复合数据组件需要稳定的布局、选择和虚拟化契约 |
| 文本与分层 | 纯布局约束在 `foundation`，文本布局协议在 `text`，字体请求值在 `theme`；旧布局、文本与字体转发入口已退出 | `theme::StyleDocument` 对字体注册的依赖仍待收口 |
| 动画 | 值级 motion 在 `foundation/motion`，节点 endpoint、宿主与组调度在 `scene`；旧 `animation/` 入口已退出；L2 opacity/translate/scale 可用 | `NodeBuilder::group` 仅开放同节点、扁平 Tween 组合；跨节点、组内 Spring、Keyframes 作者入口与浮层自动过渡未完成，见 [节点表现层 §6.1](node_presentation.md) |
| 布局 | 排列容器及 `NanControl::layout_to` 已用在生产路径；浮层有独立锚定定位器 | 场景树 anchors 尚无实现，不能依赖草案 API 编写应用 |
| 分发 | Linux 桌面源码分发 profile 已有构建与 SDK 流程 | Modules、CMake package 与跨平台仍待重新评估 |

## 近期顺序

1. **冻结 anchors v1 设计。** 先明确容器布局模式、尺寸归属、锚点目标生命周期与可见性、重挂载、画布测量，以及裁剪/滚动边界；修改规范中的矛盾后再写公开 DSL。用 markdown 编辑器的侧边栏/编辑区案例检验“树不等于布局”。
2. **分层实现并验收。** 在显式锚定画布内计算子项矩形，统一走 `layout_to`；先做父边与同父兄弟、依赖序和环检测，再验证 z 序、命中与语义。每个新能力都要有故障注入会红的测试。之后再评估阶段 5 复合组件及浮层动效。

过渡源码入口已分批退出；这不表示兼容层清理是 anchors 求解器的技术前置。后续仍按“在扩张复杂组件前先稳定基础设施”的顺序推进。

### 兼容层退出的分批边界

第一批退出 `nandina/animation/` 的别名头：值与规格统一使用 `nandina::motion`，endpoint、宿主与组统一使用 `nandina::scene`。库与测试调用、legacy 独立头测试和 SDK 导出清单同步迁移；`animation::motion` 也不再保留。已有 `foundation/motion/spec.hpp` 公开头承担独立包含测试。

第二批已退出 `scene::LayoutConstraints`：公开 API、库内实现和测试统一写 `foundation::NanLayoutConstraints`；删除 `scene/control.hpp` 中的别名与只验证旧拼写的兼容测试，保留 foundation 数值回归。不改字段、函数签名的实际类型、约束计算或布局行为。文本与字体的旧转发头另列第三批，不与这一批机械改写混合。历史故事保留原样。

第三批已退出 `widget/primitives/text_layout.hpp`、`text_layout_backend.hpp` 与 `text/font_request.hpp` 三个纯别名头。文本布局协议及默认 backend 的唯一公开归属是 `text/text_layout*.hpp`；轻量字体请求值的唯一公开归属是 `theme/font_request.hpp`。组件公开签名、测试和示例改用 `text::` / `theme::`，不搬动字体注册器、布局算法或 StyleDocument 的引擎适配。删旧头及旧入口专用测试，保留 canonical 独立包含与行为测试；变更后的库和使用者须一同重新编译。

## Anchors 设计关口

锚点工作草案的 Q1–Q7 尚未全部定案；它描述的是目标，不是当前行为。开工前至少需要明确：

- **显式画布。** `NanControl::on_layout()` 已有默认排列行为；不能把所有未覆写的 Control 悄悄改成锚定画布。一个父容器对直接子项只采用一套系统，同层误用应确定性报错。
- **唯一尺寸来源。** 草案的 `AnchorSpec::width/height` 与现有 `ControlSizeSpec` 可能形成双重当前值。建议锚点只决定位置及双边拉伸，显式宽高仍由控件尺寸模型拥有；需先定义冲突与优先级。
- **目标身份与生命周期。** 裸 `NanNode2D*` 加名称兜底难以同时保证销毁、重挂载和重名时的确定性。建议 v1 限定父容器与同父兄弟，用可验证的弱身份；reparent 时重新校验，不静默清空。具体句柄形状待定。
- **冲突规则。** 草案 §2.8 称 `Expanded` 在画布里“什么也不做”，但 §2.2.1 要求同层无效属性报错；两者必须统一。建议报错。画布隐式尺寸、裁剪与滚动如何影响锚点也须写成可测试规则。

设计拍板后按[开发循环](development_loop.md)先更新正式规范，再自下而上实现；不要先公开会“设置成功但不生效”的 DSL。

## 验证与交付

兼容层退出会跨多个模块，需跑主套件、ASan/UBSan unit、no-RTTI unit，核对 suite 注册、SDK 导出与独立 include/消费者；构建和测试不得通过管道掩盖失败。anchors 实现还需覆盖重复布局、兄弟依赖、环、重挂载、绘制/命中同序与语义 bounds。真实窗口手感仍由作者在 playground/showcase 验收，自动测试不能替代。
