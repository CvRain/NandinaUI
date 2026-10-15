# 项目进度与下一步

> 2026-10-15 更新：布局收敛与文本成本（两个 O(n²)、按输入键控的文本记忆化、按词断行）已实施；跨平台探测**源码**已入库（`probe/cross_platform/`），工具链固定点可一条命令核对。本文区分“实现存在”“作者可用”和“设计已接受但未实现”；不把旧头文件的存在视为永久兼容承诺。组件细目见 [组件路线图](component_roadmap.md)。D1 与 D5 仍按各自计划推进；D3/D4 仍是初步设计，D6 当前待审阅，不代表 API 或测量行为已实现。

## 当前能力边界

| 方向 | 当前状态 | 下一道边界 |
| --- | --- | --- |
| 作者 DSL 与文本 | builder/`configure`、Text/Label 与多种文本溢出策略可用；D6 已归档为待审阅设计 | Label 作者能力、样式绑定优先级、有限宽度内容测量与 Section 顺序均待按 [D6](authoring_dsl_and_content_measurement.md) 定形 |
| 组件与浮层 | 阶段 1–4 的基础设施与清单已经落地；阶段 5 未开始 | 复合数据组件需要稳定的布局、选择和虚拟化契约 |
| 输入与焦点 | 现有鼠标/键盘、输入焦点与作用域可用；D1 统一交互语义、焦点提示和文本会话的设计已接受 | [焦点与输入](focus_and_input.md) 为 planned：先实现鼠标/键盘提示策略，再接文本输入会话与其他设备 |
| 文本与分层 | 纯布局约束在 `foundation`，文本布局协议在 `text`，字体请求值在 `theme`；旧布局、文本与字体转发入口已退出 | `theme::StyleDocument` 对字体注册的依赖仍待收口 |
| 路由元数据 | 路由身份由页面类型 `PageKey` 确定；当前 `type` 字段既分组也作为标题 | D3 初步方案已接受、未实施：重命名 `nav_group`；空值是不分组、无标题，route 仍可见；分组按值全局合并 |
| 平台与渲染 | 当前生产路径为 raylib + GLFW/OpenGL；`IRenderDevice` 隔离绘制设备，但 `NanWindow` 仍负责窗口、输入和帧驱动 | D4 优先目标为 Linux/Windows/Web 应用交付；SDL 3.4+ GPU Renderer shader 已在 Vulkan 上跑通（探测源码入库于 `probe/cross_platform/`，`glslc`/`spirv-val` 版本固定且校验进构建图），WebGL 路径、`SDL_shadercross` 多后端产物与消费端耦合仍待 PoC，已证明与未验证的边界见 [跨平台后端探测](cross_platform_backend_probe.md) |
| 动画 | 值级 motion 在 `foundation/motion`，节点 endpoint、宿主与组调度在 `scene`；旧 `animation/` 入口已退出；L2 opacity/translate/scale 可用 | `NodeBuilder::group` 仅开放同节点、扁平 Tween 组合；跨节点、组内 Spring、Keyframes 作者入口与浮层自动过渡未完成，见 [节点表现层 §6.1](node_presentation.md) |
| 布局与文本成本 | 排列容器及 `NanControl::layout_to` 已用在生产路径；anchors 内核与作者入口已落地；**布局收敛**（Label 页 80.4 ms/帧 → 2.5 ms、稳态 0 次布局）与**两个文本 O(n²)** 已修复，见 [布局收敛](layout_convergence_and_text_measurement.md) 与 [整形成本](text_shaping_cost.md) | `fill` 画布无法占据排列容器剩余空间（见 [Anchors §5.3](anchors.md#53-剩余验收)）；`max_width` 仍会被父级紧约束压过；Label 页打开约 271 ms（构建 159 ms，[整形成本 §5.1](text_shaping_cost.md)）；一次完整布局仍约 55 ms |
| 分发 | Linux 桌面源码分发 profile 已有构建与 SDK 流程 | Modules、CMake package 与跨平台仍待重新评估 |
| 资源构建与运行时 | 四类后端、nanres 与构建 helper 已有实现；SQLite 已 vendored，但依赖无条件启用，工具仍链接完整框架；查询结果不带挂载身份 | [D5](resource_build_and_deployment.md) 已冻结、未实施：先解耦，再独立交付作用域与身份；声明式构建和部署定位随后汇合 |
| 工程门禁 | 代码格式已统一，`tools/check_format.py` 与 CI 固定 clang-format 23.1.1；SQLite 每构建配置编译一次 | D5 关闭组合与纯工具依赖门禁尚未实现；格式门禁不证明资源行为或依赖解耦 |

## 近期顺序

0. **布局收敛已落地，进入维护。** [布局收敛与文本测量](layout_convergence_and_text_measurement.md) 的四条修复已实施并通过主套件、ASan/UBSan、no-RTTI；`tests/layout_convergence_tests.cpp` 覆盖全部四条（含 `ScrollView` 跨轴契约，§6 的已知缺口已关闭）。文本成本见 [整形成本](text_shaping_cost.md)：两个 O(n²) 已修、`Text` 按完整排版输入记忆化（Label 页构建 858 → 159 ms）、按词断行已实施。
1. **D6 设计审阅。** 先确认 [作者 DSL、文本能力与内容测量](authoring_dsl_and_content_measurement.md) 的 Label 能力表、共享属性规则、覆盖/绑定语义和有限宽度测量前置；不把 `row(children...)`、border 或 Section 的目标形状误写成现有能力。
2. **D6 首批实现建议。** 审阅通过后先打通有限宽度下 Label 自然长高及测量缓存契约，再用 showcase Label 页面与 `ComponentInfoTable` 进行自动/人工验收；完成结果再决定组合组件、Section 和泛化 DSL 的实现范围。
3. **D1 第一阶段：焦点核心与提示策略。** API 定形以正式 reference 的文本能力矩阵、paint-only 增量失效、事件映射和程序化默认值为输入；提示查询统一为 `should_show_focus_hint()`。接现有鼠标/键盘、同节点提示变化、程序化恢复、窗口失活/激活和两种 roving 模型；Showcase 注册表单用于人工验收。具体契约和“改坏哪里会红”见 [统一输入、焦点与文本编辑](focus_and_input.md)。该优先级在 D6 审阅后再与用户确认，不表示 D6 的文档归档改变了 D1 的 API。
4. **D1 后续分阶段接入。** 第二阶段分别记录“协议/模拟测试”“指定平台组合输入”“指定平台软键盘请求/反馈/遮挡”三项状态；真实验收依赖相应平台能力接通，可来自现有后端扩展、独立适配或 D4，不要求整个 D4 完成。第三阶段接触摸、笔、手柄及混合输入，不以 composition 状态或模拟事件代替平台支持。
5. **D3 方案已接受、尚未实施；D4 实施依据已补充。** D3 采用空值未分组与无标题，迁移范围/测试见 [待决设计草案](reference_draft.md) 与 Story 011；D4 仍先应用级 PoC，并为 P1 补 shader host tool、版本与 SDF/字体/键码验收。尚未批准后端迁移。
6. **其他需求独立定案。** D2 主题订阅仍待审阅；D3/D4 文档是初步方向，不自动触发代码工作，可在阶段交付点重新排序。
7. **D5 已具备分批实施依据。** 纳入独立工程工作线；首批仅需定形模块目标与工具/后端开关，不等待全部身份、部署或 D4 选型。内部顺序与关口见下节。

D6 当前只完成设计归档，不能据此勾选上述实现批次。它提出的推荐实施路径为 Label 能力表 → 文本与布局
测量 → showcase 验收 → 组合/Section API；用户审阅后再确认它与 D1、D5 工程批次的实际调度关系。

D1 归档没有新增输入 API，也没有宣称焦点提示或虚拟键盘已经可用。
最终审阅保留“无交互历史默认显示”，将窗口活动明确为高于“始终显示”的前置门槛；第二阶段
接受 scene 的平台中立 `ITextInputService` 由窗口注入的方向，会话身份与解绑独立于同步剪贴板调用。
能力集合、每次请求结果、平台实际状态及实机验收记录分别表达；精确接口仍待阶段二定形。
取舍过程与证据边界见 [story 012](../../stories/012-from-focus-rings-to-input-sessions.md)。

## D5 资源工作线：已冻结，尚未实施

正式契约以 [资源构建、部署与运行时定位](resource_build_and_deployment.md) 为准。以下每批
独立提交与验收；“解耦先行”描述 D5 的依赖，不要求把所有后续修改塞进一次重构。

| 批次 | 交付与前置 | 验收关口 |
| --- | --- | --- |
| 1 解耦 | 保留当前 ID、包 schema、扫描行为；工具脱离完整 UI，默认工具开/后端开/vendored，增加独立关闭能力 | 单挂载/无冲突的前后逻辑等价；无 UI 依赖环境构建工具；关闭组合真实运行资源消费者；依赖闭包门禁 |
| 2 作用域 | 不改包格式或 ID 生成；管理器八入口补作用域，后端交付结果与发布代次，迁移缓存和失效 | 跨挂载/manager 冲突、ID 歧义、别名、并发发布、过期引用、已取得资源/流及缓存回收 |
| 3 身份 | 设计可与解耦并行，依赖包内 ID 契约的实现以作用域完成为前提；落实 key 映射、生成算法和作者引用 | 显式 key 跨重命名、重复内容独立记录、碰撞与重建行为；不固化新增随机 ID 的具体值 |
| 4 声明式构建 | 相应解耦/身份前置完成；resources.toml 唯一语义入口，补应用依赖、目录变化与输出发布 | 定向构建、增量、源树只读；两输出根逻辑比较之外另测真实并行/多配置隔离；生成头如启用则是真实编译输入 |
| 5 部署与定位 | 安装与定位同源，先 portable/当前平台 prefix，再逐平台扩展 | DESTDIR、安装覆盖、重定位、任意 cwd 与精确错误 |
| 6 作者验收与归档 | 独立下游应用走完整流程，按实际支持面更新教程与示例 | 干净检出、构建、安装后通过正式 ResourceManager 读取资源；平台验证分别记录 |

作用域批按 reference §13 定形六项 API 细节，身份算法另批决定；不会重新开放纯内容 ID 或
拆 blob 表作为本轮方案。CMake、Web、system SQLite 和新平台部署均须独立验证后才宣称支持。
取舍及“头文件只有路径不等于流没有文件句柄”的证据纠正见
[story 013](../../stories/013-resource-scope-before-identity.md)。本轮只同步文档，未执行上述验收。

## Anchors 阶段记录与剩余边界

1. **边界验证已完成。** 作者模型已记录在 [Anchors 布局设计](anchors.md)：显式锚定画布、`.anchor.*` 与 `parent` 关系、`NodeRef<T>` 匿名目标、`ControlSizeSpec` 尺寸归属及同画布兄弟引用。`tests/anchors_boundary_tests.cpp` 已固定有限/无界百分比、百分比 min/max、隐藏排列项、ScrollView 轴向约束与 reparent 失效规则；其中修正了 `ScrollAxis::both` 错误限制两轴的问题。剩余工作转入 anchors 内核的引用生命周期、约束冲突、依赖排序与几何一致性测试。
2. **锚定布局内核已落地。** `scene` 持有类型化弱引用、六条锚线、显式画布、尺寸冲突校验、兄弟拓扑排序与环检测；全部求解后经 `layout_to` 写入。10 个 unit 用例覆盖生命周期、模式批量切换及几何一致性，逐项故障注入记录见 anchors §5.2。不引入第二套几何或 presentation 位移。
3. **作者入口与集成回归已落地。** `ui.ref<T>()`、builder 的 `.bind(ref)` / 只读 `.anchor.*` 与 `.parent.anchor.*` / `.anchors(spec | source)`、`ui.anchor_canvas()` 与批量 `set_child_anchors()` 已实现；`tests/anchors_authoring_tests.cpp`（11 用例）覆盖前向引用、builder 复制、离开 build 栈、scope 清理、节点销毁、响应式替换、嵌套画布、滚动、z 序、页面根依赖链与初始绑定失败清理。`showcase` 的 `anchors` 页面（`showcase/pages/anchors_page.cpp`）以侧边栏停靠切换演示“树 ≠ 布局”。
4. **已知边界：无界预测量下 `fill` 和百分比画布都不能解析。** Column / Card 等容器的即时 relayout 使用 `measure_layout(loose())`；两轴显式像素尺寸能提供有限上界，`FillLength` / `PercentLength` 则都缺少有限基。`[arranged-parent]` 锁定显式尺寸通过、fill 与百分比报错三支。需要剩余区域时，可把画布放在布局根并用兄弟锚点表达；要支持排列容器剩余空间，须先设计测量阶段/约束传播或非循环回退，更新 [Anchors §5.1](anchors.md) 后再实现。
5. **人工体验与再评估。** 真实窗口的缩放、快速重复切换与明暗切换手感由作者在 showcase 验收；Anchors 边界稳定后再重新排序阶段 5 复合组件与浮层过渡动效，它们不是 anchors 求解器的前置条件。

2026-10-09 审核补充：修复非法 anchors 初始绑定残留 effect 的生命周期问题；新增直接编译
showcase 页面的小视口/零尺寸、反复换边和页面销毁回调测试。页面采用 30% 尺寸上限，并移除
跨面板固定间隔以免负跨度。百分比画布在离树 loose 测量下同样缺少有限基，不能视作确定尺寸；
上述“可解析百分比”只限父级已提供有限约束。批量关系更新提交的是描述，实际几何在下次布局更新。

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
