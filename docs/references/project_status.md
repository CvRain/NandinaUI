# 项目进度与下一步

> 2026-10-17 文档核对：本文是**跨专题实施顺序的唯一入口**；D1–D8 是设计议题编号，不是执行顺序。D4 的 Linux 默认后端切换与平台接口已落地，但仍有视觉、输入、依赖交付和目标平台验收缺口。本轮只更新文档，不修代码、不重跑历史性能或三套测试；下列未完成批次均为 planned。

## 当前能力边界

| 方向 | 当前状态 | 下一道边界 |
| --- | --- | --- |
| 作者 DSL 与文本 | builder/`configure`、Text/Label 与多种文本溢出策略可用；D6 已归档为待审阅设计 | Label 作者能力、样式绑定优先级、有限宽度内容测量与 Section 顺序均待按 [D6](authoring_dsl_and_content_measurement.md) 定形 |
| 组件与浮层 | 阶段 1–4 的基础设施与清单已经落地；阶段 5 未开始 | 复合数据组件需要稳定的布局、选择和虚拟化契约 |
| 输入与焦点 | 现有鼠标/键盘、输入焦点与作用域可用；D1 统一交互语义、焦点提示和文本会话的设计已接受 | [焦点与输入](focus_and_input.md) 为 planned：先实现鼠标/键盘提示策略，再接文本输入会话与其他设备 |
| 文本与分层 | 纯布局约束在 `foundation`，文本布局协议在 `text`，字体请求值在 `theme`；旧布局、文本与字体转发入口已退出 | `theme::StyleDocument` 对字体注册的依赖仍待收口 |
| 路由元数据 | 路由身份由页面类型 `PageKey` 确定；当前 `type` 字段既分组也作为标题 | D3 初步方案已接受、未实施：重命名 `nav_group`；空值是不分组、无标题，route 仍可见；分组按值全局合并 |
| 平台与渲染 | 编入 SDL3 时默认走 SDL3（环境变量可覆盖），未找到 SDL3 的 auto 构建仍走 raylib；`IPlatformWindow`、设备包装入口和三页 trace 对照已存在。生产 SDL 仍是 CPU 几何；SDF 只在探测中验证 | 混合/AA/图片/裁剪见 D7，键码见 D8；DXIL/MSL 加载及 Windows/Web 应用交付未验收。P3 落地不等于 P0–P4 全部验收完成 |
| 动画 | 值级 motion 在 `foundation/motion`，节点 endpoint、宿主与组调度在 `scene`；旧 `animation/` 入口已退出；L2 opacity/translate/scale 可用 | `NodeBuilder::group` 仅开放同节点、扁平 Tween 组合；跨节点、组内 Spring、Keyframes 作者入口与浮层自动过渡未完成，见 [节点表现层 §6.1](node_presentation.md) |
| 布局与文本成本 | 排列容器及 `NanControl::layout_to` 已用在生产路径；anchors 内核与作者入口已落地；**布局收敛**（Label 页 80.4 ms/帧 → 2.5 ms、稳态 0 次布局）与**两个文本 O(n²)** 已修复，见 [布局收敛](layout_convergence_and_text_measurement.md) 与 [整形成本](text_shaping_cost.md) | `fill` 画布无法占据排列容器剩余空间（见 [Anchors §5.3](anchors.md#53-剩余验收)）；`max_width` 仍会被父级紧约束压过；Label 页打开约 271 ms（构建 159 ms，[整形成本 §5.1](text_shaping_cost.md)）；一次完整布局仍约 55 ms |
| 分发 | Linux 桌面源码分发 profile 已有构建与 SDK 流程 | Modules、CMake package 与跨平台仍待重新评估 |
| 资源构建与运行时 | 四类后端、nanres 与构建 helper 已有实现；SQLite 已 vendored，但依赖无条件启用，工具仍链接完整框架；查询结果不带挂载身份 | [D5](resource_build_and_deployment.md) 已冻结、未实施：先解耦，再独立交付作用域与身份；声明式构建和部署定位随后汇合 |
| 工程门禁 | 代码格式已统一，`tools/check_format.py` 与 CI 固定 clang-format 23.1.1；SQLite 每构建配置编译一次 | D5 关闭组合与纯工具依赖门禁尚未实现；格式门禁不证明资源行为或依赖解耦 |

## D1–D8 与依赖工程的状态

| 议题 | 当前结论 | 实施状态与入口 |
| --- | --- | --- |
| D1 输入、焦点与文本会话 | 设计方向已接受；焦点事实、提示策略、文本会话分离 | 未实施；[正式契约](focus_and_input.md)。先焦点提示，随后组合输入/软键盘，再其他设备 |
| D2 主题方案与状态订阅 | 讨论已汇总；只读状态、事件和主题方案模型需定形 | 待审阅；[主题设计](theme_authoring_and_state.md)。先解决状态可观察性，完整主题作者模型另批 |
| D3 导航分组 | `type → nav_group`、空值未分组、同值全局合并已接受 | 未实施；[D3](reference_draft.md#d3-路由身份与导航分组开放到哪一层)，独立小批 |
| D4 平台与后端 | SDL3 默认路径与 host seam 已落地，raylib 保留对照 | 收尾中；[阶段证据](platform_backend_contract.md)。Windows/Web 仍不是正式应用支持面 |
| D5 资源构建与部署 | 范围与目标契约冻结 | 未实施；[正式契约](resource_build_and_deployment.md)，内部六批见下节 |
| D6 作者 DSL 与内容测量 | 方向已接受；Label 能力、绑定覆盖、测量详细契约待审阅 | 未实施；[D6](authoring_dsl_and_content_measurement.md)，测量先于 Section/语法糖 |
| D7 渲染质量与图片 | 混合修复 → SDF 高质量路径；共享解码；分层处理圆角裁剪 | 未实施；[D7](reference_draft.md#d7-渲染质量与后端视觉一致性)，每批定形接口后验收 |
| D8 键码与输入事件 | 框架拥有键身份；物理位置、逻辑含义与提交文本分开 | 未实施；[D8](reference_draft.md#d8-键码与输入码空间)，不再按纯机械 rename 估算 |
| 依赖工程 | wrap-git；默认 vendored，system/auto 显式选择；功能与来源开关分离 | 未实施；[依赖策略](dependency_policy.md)。SDL3 首批，其余逐项迁移并同步 SDK |

## 推进顺序与批次关口

以下是当前默认推进顺序，不承诺日期。每一行可以拆成多个可验收提交；进入下一批前，
先记录本批结果和剩余限制。调度顺序与技术前置分开列出，避免把所有议题串成一条硬依赖链。

| 批次 | 交付 | 技术前置 | 退出条件 |
| --- | --- | --- | --- |
| 0 文档基线（本轮） | 核对状态、修正机制判断、统一排期并提交 | 现有源码与历史证据 | 文档链接、状态、diff 核对；不将计划标为实现完成 |
| 1 D7 混合修复（下一轮首批） | 设备初始化正确设置混合；固定交互帧回归 | 无；不等依赖迁移 | 已知背景上的半透明图元像素符合独立预期；按钮按下、涟漪中间态、Dialog 遮罩通过；移除修复会红。回归 harness 自绘背景、在 present 前读回、注册为 integration 测试，golden 按[视觉回归](visual_regression.md)组织（基础正确性用独立预期，页面帧用审核过的 golden，CI 只比较不自动接受） |
| 2 SDL3 来源与交付 | 实测 CMake wrap、固定版本、三态来源和解析报告；接 CI/SDK；建立[头文件分类](sdk_surface.md)门禁 | 先确认真实 target 与构建选项 | 三态、关闭后端不获取、干净检出与预热后断网构建通过；SDL 路径被显式启用并验证；SDK 包含所需依赖。`.gitignore` 覆盖 wrap 检出目录、SDK 显式收集依赖源码与许可证、打包在 nodownload 下可重复（fixture 逐字节断言不失真） |
| 3 D7 生产 SDF | 同帧多参数实验 → 产物交付 → 逐图元实现；保留有能力说明的基础降级路径 | 批次 1；SDL3 与 shader 产物来源可重复取得，不等待所有依赖迁移 | 图元/文字/图片纹理交错时参数和状态正确；分数缩放、细描边与半透明叠加通过；质量达标且像素差异有界可解释；报告稳态与交互成本（按[性能预算](performance_budget.md)协议采样，先报告模式），**每帧状态切换次数**与内存一并记录，新路径经审核建立自己的基线；fragment uniform 预算实测；驱动级逃生舱可用 |
| 4 D7 共享图片解码 | 应用服务提供默认 CPU 解码器，两后端复用；**删除 `IPlatformWindow::make_image_decoder()`**（破坏性变更，按 [SDK 表面](sdk_surface.md) 记录）；SDL 补 `create_rgba_texture` 上传链；保持 crop/resize/tint | 解码库来源与行为定形；不依赖 SDF，可因应用需求提前 | PNG/JPEG、透明纹理、失败输入与预处理测试；**解码服务 / 设备直载 / 设备 RGBA 上传三项能力分别验收并分别报告**，不再用单个布尔值；两后端真实 Image 消费者可用；两者都不可用时降级明确并指出缺哪一项；删除后无遗留 override |
| 5 D8 输入基础 | 先修"宿主用帧级字段覆盖每事件坐标/修饰键"（已核实的缺陷），再定形物理/逻辑键、事件快照、release/repeat/unknown，迁移 mapper 与控件；保序事件另小批 | 无渲染前置；可在批次 2–4 之间独立推进 | 同帧"在 A 按下、移到 B"派发到 A；按键用事件时修饰键；US/非 US 布局、左右修饰键、按下/释放/重复、未映射策略与跨后端规范事件对照；点击切焦点与输入同帧仍按序分发 |
| 6 D7 裁剪收口 | 先涟漪专用圆角交集，再按实际用例定形图片/子树嵌套裁剪 | 高质量图元路径；通用裁剪需单独协议 | 不外溢、状态恢复、能力为 false 的降级；绘制/命中遵循既有裁剪契约，不用专用图元冒充通用能力 |
| 7 D1 第一阶段 | scene 统一焦点提示策略，widget/theme 消费结果 | 默认排在 D8 后复用事件语义；技术上不依赖完整 D8 或 D2 | 同节点模态变化、程序化恢复、窗口失活/激活、文本能力矩阵、两种 roving 模型与 paint-only 失效 |
| 8 D6 首批 | 定形 Label 作者能力与绑定覆盖；完善有限宽度自然长高及缓存契约 | 详细 API 审阅；已有布局收敛回归保持 | Label 页面与 ComponentInfoTable 验收；宽度/内容/字体变动重测，纯绘制变动不无谓整形；无界 anchors 不靠固定高度掩盖 |
| 9 D2/D3 应用入口 | D2 先落只读状态/通知；D3 单独迁移 nav_group | D2 需定形通知与生命周期；D3 无跨专题前置，可提前插入 | Shell 跟随主题且无重复事实来源；空组/合并/顺序/typed navigation 回归；不借机扩展路由历史 |
| 10 D5 工程批次 | 解耦 → 作用域 → 身份 → 声明式构建 → 部署 → 下游验收 | 见下表；解耦不等身份设计或全部依赖迁移 | 纯工具构建、关闭组合、跨挂载缓存、可重定位安装和实际资源消费者分别通过 |
| 11 后续扩展 | D1 文本会话/其他设备、D6 组合组件、阶段 5 数据组件，以及逐平台应用交付 | 按具体功能前置分批，不是一次大版本交付 | 文本协议/真实 IME/软键盘分别记状态；数据组件有布局/选择/虚拟化依据；每个平台有独立应用证据 |

### 可独立推进与不可跳过的前置

- **依赖工程贯穿批次 2 以后**：SDL3 先完成；其余依赖逐项迁移，每项同时处理离线 SDK 和 CI。
  完整依赖迁移不是批次 3、4、7、8 的硬前置；最终撤下 submodule 前必须完成分发验收。
- **D5 解耦可提前**：批次 10 表示默认排期，不表示必须等 D2/D3。若资源消费者成为近期目标，
  可在批次 2 的构建边界定形后启动；与依赖迁移修改同一构建图时按提交顺序协调。
- **D1 文本会话复用已有平台接口**：`ITextInputService` 由窗口注入，不能另造第二套 host。
  会话依赖实际平台文本能力与有序事件，不以生成 shader 或完成全部 D4 为前提。
- **D6 扩展建立在测量上**：先定覆盖/绑定语义并验收内容测量，再决定共享 box/border、子项简写、
  Section 和 anchors 与排列容器剩余空间的组合；这些分别定形，不打包成一次 DSL 重写。
- **跨平台分开记录**：SPIR-V/DXIL/MSL 已有探测产物，只有 SPIR-V 有独立 spirv-val 校验；
  DXIL/MSL 的目标设备加载待验收。Web canvas 探测不等于 Nandina 应用可交付。
- Modules、CMake package、shell/compositor、任意矢量路径仍待实际需求与对应关口，不进入当前关键路径。

### 每批的完成定义

1. 先更新专题中的归属层、API 和能力边界；需要用户判断的未定行为在实施前收敛。
2. 实现、消费者接线、针对性回归一起交付，说明“改坏哪里会红”；公共 API 变更同步相关调用。
3. 代码批按[开发流程](development_loop.md)执行主套件、ASan/UBSan unit、no-RTTI unit及对应专项。
   文档批检查链接、状态和 diff，不把历史测试结果当作本轮验证。
4. 结果写回本表所链接的专题证据，再更新状态与提交；未执行或依赖外部设备的验收明确保留。
5. 用真实应用贯穿导航、表单、图片、滚动和弹窗。Showcase 保留组件验收职责，应用/示例正文仍由
   作者主导；正式验收所需 fixture、脚本和说明不得只存在于未追踪的 playground。

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
