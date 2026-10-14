# 开发参考

本目录记录 NandinaUI 的开发约束和架构共识，主要读者是框架维护者及组件贡献者。应用开发者通常只需要阅读 `getting_started/` 与 `components/`。

## 当前文档

### 架构与约定

- [项目演进与当前形态](project_evolution.md)：四段形态的来龙去脉、迁移到正式仓库的过程，以及阅读旧文档时的注意事项。
- [模块依赖规则](module_dependency.md)：五层十二模块的分层、单向依赖约束，以及源码中仍然存在的偏离与收口方向。
- [文本布局协议](text_pipeline.md)：canonical text 类型、旧 widget 头的兼容入口、默认后端及迁移边界。
- [轻量字体请求](font_request.md)：主题描述值、text 兼容别名、字段语义与字体引擎依赖边界。
- [构建系统与 Modules 范围](build_system_scope.md)：为什么当前主线使用 Meson/include，以及 Modules 和 CMake package 的暂缓条件。
- [D5：资源构建、部署与运行时定位](resource_build_and_deployment.md)：**planned，范围与契约已冻结，未实施**。解耦/作用域/身份分批，管理器八入口、弱域令牌、后端发布与流生命周期，以及声明式构建、安装定位和故障注入；过程见 [story 013](../../stories/013-resource-scope-before-identity.md)。
- [开发流程](development_loop.md)：每轮改动的六步循环、三套验证关卡、"测试必须证明会红"，以及审核会重点看什么。
- [编码与 API 规范](coding_conventions.md)：命名与命名空间、头文件与包含、API 形态、注释、格式化与测试门槛。
- [响应式内部模型](reactive_model.md)：依赖图结构、推入失效与拉取求值、effect 调度、作用域清理顺序与已知限制。
- [设计令牌与主题系统](design_tokens.md)：三层令牌模型、配方与解析优先级、外观切换，以及控件作者的样式规则。
- [D2：主题方案、作者入口与状态订阅](theme_authoring_and_state.md)：**讨论汇总，待审阅，未实施**。独立方案与跨家族亮暗搭配、只读状态与事件、主题导入流程、自定义组件边界，以及待决项和验收建议。
- [溢出与裁剪契约](overflow_and_clip.md)：容器如何声明裁剪、render 如何执行，以及绘制与命中如何共享同一份语义。
- [节点表现层](node_presentation.md)：属性动画代价分级、已落地的 L2 `visual::Path`，以及形状家族与盒子模型规划；anchors 背景见 §2，规范见独立 reference。
- [同一节点的组合动画作者入口](animation_authoring.md)：组合规格、弱句柄的声明与触发、属性策略、生命周期和仲裁契约。
- [Anchors 布局设计](anchors.md)：作者模型、布局边界与百分比；scene 内核、作者入口、已知边界与故障注入见 §5。
- [锚点系统：历史草案](anchor_draft.md)：保留讨论脉络与旧建议；不是规范，冲突时以 [Anchors 布局设计](anchors.md) 为准。
- [配置与开发体验](authoring_configuration.md)：**作者侧规则**。回调的两类与 `guarded()` 的能力边界、handler 的 concept 化设计、组件的公开 API 表面、样式优先级。
- [D6：作者 DSL、文本能力与内容测量](authoring_dsl_and_content_measurement.md)：**讨论汇总，待审阅，未实施**。Label 能力表、共享属性归属、builder 子项简写、有限宽度自然长高，以及 border / Section 的阶段边界；过程见 [story 014](../../stories/014-the-dsl-that-kept-sending-authors-to-configure.md)。
- [待决设计草案](reference_draft.md)：**工作稿**。D2 的后续讨论已汇总到[主题方案、作者入口与状态订阅](theme_authoring_and_state.md)；D3 的 `nav_group` 空值与合并语义已收敛、未实施；D4 仍需 SDL3 应用级 PoC，含 GPU shader 工具链与未来 host 行为契约。D1 已定案为[统一输入、焦点与文本编辑](focus_and_input.md)。
- [布局收敛与文本测量](layout_convergence_and_text_measurement.md)：**已实施**。布局永不收敛的实测证据与四条根因（`Text` 写入原始尺寸、`ScrollView` 两个阶段跨轴契约不一致、`stretch` 越过子项显式尺寸、亚像素浮点噪声）、收敛契约、故障注入矩阵、未覆盖缺口与开放项（含整形成本与一次被实测否决的缓存方案）。
- [文本整形与页面打开成本](text_shaping_cost.md)：**已实施**。页面打开时间（Label 858 ms 构建 / 5000 汉字 6.4 s）的两个 O(n²)（caret 停靠点去重、逐簇推进重新分段）、空闲 CPU（`Expanded` 子项尺寸抖动导致每帧全量布局，72% → 9% 单核）与常驻内存构成（4 条字体管线 × 1024² 图集 ≈ 40 MB）、整形成本曲线、一次被实测否决的整形缓存，以及剩余目标（构建期 1.1 万次测量）。
- [跨平台后端探测：SDL3 GPU 与 Web 现状](cross_platform_backend_probe.md)：**探测记录，不是选型批准**。Vulkan GPU Renderer 与两条 Web 路径各自证明了什么、未验证清单、shader 构建链门槛，以及为什么不能用修复前的页面性能做后端结论。

### 应用层

- [Page / Router 合约](page_and_router.md)：三条入口路径的分工、单动作 `navigate` 路由，以及页面、参数与 Store 的状态职责边界。

### 组件与浮层

- [组件公共契约](component_contract.md)：新组件在进入推荐 API 前必须满足的状态、事件、绑定、主题、输入和无障碍要求。
- [统一输入、焦点与文本编辑](focus_and_input.md)：**planned，D1 设计已接受**。包含事件映射、paint-only 失效、无历史默认显示、窗口活动优先级，以及注入式文本服务、会话生命周期和三阶段验收；取舍过程见 [story 012](../../stories/012-from-focus-rings-to-input-sessions.md)。
- [组件开发路线图](component_roadmap.md)：当前组件盘点、基础设施依赖和推荐开发顺序。
- [浮层架构](overlay_architecture.md)：OverlayHost、portal 生命周期、分层规则与后续基础设施边界。
- [菜单族条目模型](menu_model.md)：阶段 4 菜单组件共享的 MenuItem、聚焦/激活规则与勾选语义。
- [Tooltip 迁移契约](tooltip_migration.md)：Tooltip 接入统一浮层设施时的 API 兼容和验收条件。
- [Dialog 迁移契约](dialog_migration.md)：Dialog 模态内容接入统一浮层设施时的槽位、层级、生命周期和验收条件。

### 平台

- [Linux 窗口后端与缩放](linux_window_scaling.md)：Wayland/X11 选择与多显示器 DPI 验证。

## 编写原则

开发参考应记录可复用的决策，而不是某次实现过程。内容需要回答以下问题：

- 为什么选择当前设计，替代方案有什么代价；
- 哪些规则属于公共兼容承诺，哪些仍是实验性约定；
- 新组件如何复用已有 primitive，而不是重复实现输入、主题或布局；
- 设计变化会影响哪些组件、测试和公开文档；
- 何种条件满足后，功能才可以从 experimental 进入 recommended。

如果一次变更只修复局部实现且不产生长期约束，应写在提交和测试中，不必创建参考文档。若变更建立了跨组件规则，应同步更新本目录中对应文档。

## 文档状态

参考文档使用以下术语：

- **recommended**：推荐给应用开发者，避免破坏性调整；
- **experimental**：方向基本成立，但允许在 alpha 阶段调整接口；
- **internal**：实现细节，不构成兼容承诺；
- **planned**：已经进入路线图，但尚无可用实现。

Getting Started 只应依赖 recommended API。组件文档可以介绍 experimental API，但必须明确标注状态。
