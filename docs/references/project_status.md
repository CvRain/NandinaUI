# 项目进度与下一步

> 截至 2026-10-08 的源码与文档核对。本文区分“实现存在”“作者可用”和“仍是草案”；不把旧头文件的存在视为永久兼容承诺。组件细目见 [组件路线图](component_roadmap.md)。Anchors 的 scene 内核与作者入口均已落地并由 unit 覆盖；真实窗口的人工验收未完成。

## 当前能力边界

| 方向 | 当前状态 | 下一道边界 |
| --- | --- | --- |
| 组件与浮层 | 阶段 1–4 的基础设施与清单已经落地；阶段 5 未开始 | 复合数据组件需要稳定的布局、选择和虚拟化契约 |
| 文本与分层 | 纯布局约束在 `foundation`，文本布局协议在 `text`，字体请求值在 `theme`；旧布局、文本与字体转发入口已退出 | `theme::StyleDocument` 对字体注册的依赖仍待收口 |
| 动画 | 值级 motion 在 `foundation/motion`，节点 endpoint、宿主与组调度在 `scene`；旧 `animation/` 入口已退出；L2 opacity/translate/scale 可用 | `NodeBuilder::group` 仅开放同节点、扁平 Tween 组合；跨节点、组内 Spring、Keyframes 作者入口与浮层自动过渡未完成，见 [节点表现层 §6.1](node_presentation.md) |
| 布局 | 排列容器及 `NanControl::layout_to` 已用在生产路径；anchors 的 scene 内核与作者入口（`ui.ref` / `ui.anchor_canvas` / `.anchor.*` / `.anchors`）均已落地，showcase 提供侧边栏切换案例；浮层仍使用独立定位器 | `fill` 画布无法占据排列容器的剩余空间（画布需要确定尺寸），见 [Anchors §5.3](anchors.md#53-剩余验收)；真实窗口的人工验收未完成 |
| 分发 | Linux 桌面源码分发 profile 已有构建与 SDK 流程 | Modules、CMake package 与跨平台仍待重新评估 |

## 近期顺序

1. **边界验证已完成。** 作者模型已记录在 [Anchors 布局设计](anchors.md)：显式锚定画布、`.anchor.*` 与 `parent` 关系、`NodeRef<T>` 匿名目标、`ControlSizeSpec` 尺寸归属及同画布兄弟引用。`tests/anchors_boundary_tests.cpp` 已固定有限/无界百分比、百分比 min/max、隐藏排列项、ScrollView 轴向约束与 reparent 失效规则；其中修正了 `ScrollAxis::both` 错误限制两轴的问题。剩余工作转入 anchors 内核的引用生命周期、约束冲突、依赖排序与几何一致性测试。
2. **锚定布局内核已落地。** `scene` 持有类型化弱引用、六条锚线、显式画布、尺寸冲突校验、兄弟拓扑排序与环检测；全部求解后经 `layout_to` 写入。10 个 unit 用例覆盖生命周期、模式批量切换及几何一致性，逐项故障注入记录见 anchors §5.2。不引入第二套几何或 presentation 位移。
3. **作者入口与集成回归已落地。** `ui.ref<T>()`、builder 的 `.bind(ref)` / 只读 `.anchor.*` 与 `.parent.anchor.*` / `.anchors(spec | source)`、`ui.anchor_canvas()` 与批量 `set_child_anchors()` 已实现；`tests/anchors_authoring_tests.cpp`（10 用例）覆盖前向引用、builder 复制、离开 build 栈、scope 清理、节点销毁、响应式替换、嵌套画布、滚动、z 序与页面根依赖链。`showcase` 的 `anchors` 页面（`showcase/pages/anchors_page.cpp`）以侧边栏停靠切换演示“树 ≠ 布局”。
4. **已知边界：`fill` 画布无法占据排列容器的剩余空间。** 排列容器在 `add()` / `on_ready()` 时会做一次 `measure_layout(loose())`；确定尺寸（显式像素 / 可解析百分比）会把该次测量的上界收紧为有限值，所以显式尺寸的画布可以嵌入 `Column` / `Card`，而 `FillLength` 在无穷基下解析为空、上界保持无穷，构建期抛 “finite” 诊断。`[arranged-parent]` 用例锁定两支。需要"剩余区域"结构时，把画布放在布局根并用同层兄弟锚点表达，见 `showcase/pages/anchors_page.cpp`；若要支持 `fill`，须先改 [Anchors §5.1](anchors.md) 的契约再改代码。
5. **人工体验与再评估。** 真实窗口的缩放、快速重复切换与明暗切换手感由作者在 showcase 验收；Anchors 边界稳定后再重新排序阶段 5 复合组件与浮层过渡动效，它们不是 anchors 求解器的前置条件。

过渡源码入口已分批退出；这不表示兼容层清理是 anchors 求解器的技术前置。后续仍按“在扩张复杂组件前先稳定基础设施”的顺序推进。

### 兼容层退出的分批边界

第一批退出 `nandina/animation/` 的别名头：值与规格统一使用 `nandina::motion`，endpoint、宿主与组统一使用 `nandina::scene`。库与测试调用、legacy 独立头测试和 SDK 导出清单同步迁移；`animation::motion` 也不再保留。已有 `foundation/motion/spec.hpp` 公开头承担独立包含测试。

第二批已退出 `scene::LayoutConstraints`：公开 API、库内实现和测试统一写 `foundation::NanLayoutConstraints`；删除 `scene/control.hpp` 中的别名与只验证旧拼写的兼容测试，保留 foundation 数值回归。不改字段、函数签名的实际类型、约束计算或布局行为。文本与字体的旧转发头另列第三批，不与这一批机械改写混合。历史故事保留原样。

第三批已退出 `widget/primitives/text_layout.hpp`、`text_layout_backend.hpp` 与 `text/font_request.hpp` 三个纯别名头。文本布局协议及默认 backend 的唯一公开归属是 `text/text_layout*.hpp`；轻量字体请求值的唯一公开归属是 `theme/font_request.hpp`。组件公开签名、测试和示例改用 `text::` / `theme::`，不搬动字体注册器、布局算法或 StyleDocument 的引擎适配。删旧头及旧入口专用测试，保留 canonical 独立包含与行为测试；变更后的库和使用者须一同重新编译。

## Anchors 设计关口

作者模型和已达成的边界以 [Anchors 布局设计](anchors.md) 为准；[历史草案](anchor_draft.md)中的 Q1–Q7 已过期。基础边界、scene 内核与作者入口分别由 `anchors_boundary_tests` / `anchors_tests` / `anchors_authoring_tests` 验证；`scene::AnchorCanvas`、`NanControl::set_anchors`、`ui.ref` 与 `ui.anchor_canvas` 均已可用。场景树已有 z 序，anchors 复用而不再造一套。

剩余关口只有一个：画布与排列容器的组合方式（见上方第 4 条）。在它定案前，不要在应用代码里把画布塞进 `Column` / `Card`；展示案例与页面根用法已经可用。

## 验证与交付

兼容层退出会跨多个模块，需跑主套件、ASan/UBSan unit、no-RTTI unit，核对 suite 注册、SDK 导出与独立 include/消费者；构建和测试不得通过管道掩盖失败。anchors 实现还需覆盖重复布局、兄弟依赖、环、重挂载、绘制/命中同序与语义 bounds。真实窗口手感仍由作者在 playground/showcase 验收，自动测试不能替代。
