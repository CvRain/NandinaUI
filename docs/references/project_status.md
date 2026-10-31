# 项目进度与下一步

> 2026-10-29 文档同步：本文是**已完成批次与设计议题状态的唯一入口**；D1–D8 是设计议题编号，不是执行顺序。批次 1、2 已完成；批次 3 的生产接入（填充/描边/圆/线段/圆环扇区）已完成，并通过 Vulkan(SPIR-V) 与 Windows D3D12(DXIL) 设备级像素验收；Linux 与 Windows 页面 golden、release 性能基线与预算门禁已落地（路径契约 §10–§14；Windows 性能为参考数据）。**P2 已交付；P3.1a/b/c 事件快照、键身份/生命周期与跨类型保序均已交付，当前进入 D1 焦点提示**；排序与退出条件见[开发路线](development_roadmap.md)。本文沿用历史验收记录，不声称本轮新增实现或验证。

## 当前能力边界

| 方向 | 当前状态 | 下一道边界 |
| --- | --- | --- |
| 作者 DSL 与文本 | builder/`configure`、Text/Label 与多种文本溢出策略可用；D6 已归档为待审阅设计 | Label 作者能力、样式绑定优先级、有限宽度内容测量与 Section 顺序均待按 [D6](authoring_dsl_and_content_measurement.md) 定形 |
| 组件与浮层 | 阶段 1–4 的基础设施与清单已经落地；阶段 5 未开始 | 复合数据组件需要稳定的布局、选择和虚拟化契约 |
| 输入与焦点 | 现有鼠标/键盘、输入焦点与作用域可用；D1 统一交互语义、焦点提示和文本会话的设计已接受 | [焦点与输入](focus_and_input.md) 为 planned：先实现鼠标/键盘提示策略，再接文本输入会话与其他设备 |
| 文本与分层 | 纯布局约束在 `foundation`，文本布局协议在 `text`，字体请求值在 `theme`；旧布局、文本与字体转发入口已退出 | `theme::StyleDocument` 对字体注册的依赖仍待收口 |
| 路由元数据 | 路由身份由页面类型 `PageKey` 确定；当前 `type` 字段既分组也作为标题 | D3 初步方案已接受、未实施：重命名 `nav_group`；空值是不分组、无标题，route 仍可见；分组按值全局合并 |
| 平台与渲染 | 默认启用 SDL3（唯一内置后端），raylib 已删除；显式关闭 SDL3 时须注入平台才能打开窗口；`IPlatformWindow`、设备包装入口和三页 trace 对照已存在。生产 SDL 的填充/描边/圆/线段/弧已有 SDF：Linux Vulkan(SPIR-V) 与 Windows D3D12(DXIL) 均通过设备级像素验收，其余图元仍走 CPU 几何 | 混合/AA/图片/裁剪见 D7，键码见 D8；Windows 为实验性（构建/单元/doctor/DXIL 已验收，无应用交付承诺），macOS 不支持，Web 非交付路径。D4 历史 P3 落地不等于其 P0–P4 全部验收完成 |
| 动画 | 值级 motion 在 `foundation/motion`，节点 endpoint、宿主与组调度在 `scene`；旧 `animation/` 入口已退出；L2 opacity/translate/scale 可用 | `NodeBuilder::group` 仅开放同节点、扁平 Tween 组合；跨节点、组内 Spring、Keyframes 作者入口与浮层自动过渡未完成，见 [节点表现层 §6.1](node_presentation.md) |
| 布局与文本成本 | 排列容器及 `NanControl::layout_to` 已用在生产路径；anchors 内核与作者入口已落地；**布局收敛**（Label 页 80.4 ms/帧 → 2.5 ms、稳态 0 次布局）与**两个文本 O(n²)** 已修复，见 [布局收敛](layout_convergence_and_text_measurement.md) 与 [整形成本](text_shaping_cost.md) | `fill` 画布无法占据排列容器剩余空间（见 [Anchors §5.3](anchors.md#53-剩余验收)）；`max_width` 仍会被父级紧约束压过；Label 页打开约 271 ms（构建 159 ms，[整形成本 §5.1](text_shaping_cost.md)）；一次完整布局仍约 55 ms |
| 分发 | Linux 桌面源码分发 profile 已有构建与 SDK 流程；Windows 为实验性构建（无安装器承诺） | macOS 不支持；Modules、CMake package 与跨平台应用交付仍待重新评估 |
| 资源构建与运行时 | 四类后端、nanres 与构建 helper 已有实现；SQLite 已 vendored；后端结果携带发布代次与一致性类别，管理器八入口返回带域令牌/MountId/代次的作用域结果，字体与纹理缓存按作用域键控并有界淘汰/expired 域清扫，非快照内容不进入共享缓存；`resources.toml` 支持显式 key→路径映射，新 key 使用版本化派生 ID；构建 lock 写入 build 目录，旧随机 ID 可显式迁移，无变化不重写；`nanres keys --header` 生成只含逻辑 key 的 C++ 常量头，碰撞失败、无变化不重写 | [D5](resource_build_and_deployment.md) P2 已交付（P2.1a/P2.1b/P2.2a–P2.2d/P2.3a–P2.4c，含声明式构建、部署定位与下游验收）；示例应用已由 P2.4 的 `examples/lab` 改造为 `examples/notebook`（2026-10-28 起），其 SQLite 数据层与资源驱动迁移已落地；notebook 服务/UI 与 showcase 组件由维护者手工编写 |
| 工程门禁 | 代码格式统一（固定 clang-format 23.1.1）、header-surface 182/10、shader artifact 校验、golden 清单、backend budget 确定性门禁均已接入；D5 关闭组合与纯工具依赖门禁（`check_tool_dependencies`、资源选项矩阵）已实现；GPU 相关检查留在本地/发布检查 | 无 GPU 的 CI 不覆盖页面 golden 与性能采样 |

## D1–D8 与依赖工程的状态

| 议题 | 当前结论 | 实施状态与入口 |
| --- | --- | --- |
| D1 输入、焦点与文本会话 | 设计方向已接受；焦点事实、提示策略、文本会话分离 | 未实施；[正式契约](focus_and_input.md)。先焦点提示，随后组合输入/软键盘，再其他设备 |
| D2 主题方案与状态订阅 | 讨论已汇总；只读状态、事件和主题方案模型需定形 | 待审阅；[主题设计](theme_authoring_and_state.md)。先解决状态可观察性，完整主题作者模型另批 |
| D3 导航分组 | `type → nav_group`、空值未分组、同值全局合并已接受 | 未实施；[D3](reference_draft.md#d3-路由身份与导航分组开放到哪一层)，独立小批 |
| D4 平台与后端 | SDL3 默认路径与 host seam 已落地，raylib 已完整退役 | **已收尾**：生产 SDF 在 Vulkan 与 Windows D3D12 通过设备级验收；[阶段证据](platform_backend_contract.md)。Windows 为实验性支持，macOS 不支持，Web 不是交付面 |
| D5 资源构建与部署 | 范围与目标契约冻结 | **P2 已交付**：P2.1a/P2.1b/P2.2a–P2.2d/P2.3a–P2.4c；见[正式契约](resource_build_and_deployment.md) §8 与内部六批。示例应用已转入 notebook 改造（数据层先落地），服务/UI 与 showcase 组件由维护者手工编写 |
| D6 作者 DSL 与内容测量 | 方向已接受；Label 能力、绑定覆盖、测量详细契约待审阅 | 未实施；[D6](authoring_dsl_and_content_measurement.md)，测量先于 Section/语法糖 |
| D7 渲染质量与图片 | 混合修复 → SDF 高质量路径；共享解码；分层处理圆角裁剪 | 混合修复、生产 SDF（填充/描边/圆/线段/弧）与设备级/页面级验收（Linux golden）、共享图片解码已落地；阴影/裁剪已交付（SDF mode 3/4），MSL 未验收，raylib 已退役，Windows 页面 CPU 基线已批 3 条，Windows 页面基线 6 条已批准（CPU 3 + SDF 3，2026-10-22）；Windows 性能参考基线（CPU + SDF 路径）已归档；[D7](reference_draft.md#d7-渲染质量与后端视觉一致性) |
| D8 键码与输入事件 | 框架拥有键身份；物理位置、逻辑含义与提交文本分开 | **P3.1a/b/c 均已实施（2026-10-29/31）**：事件快照、类型化逻辑/物理键与键生命周期、跨类型事件保序。见[路线图](development_roadmap.md#p3-输入与焦点当前阶段) 与 [D8](reference_draft.md#d8-键码与输入码空间) |
| 依赖工程 | wrap-git；默认 vendored，system/auto 显式选择；功能与来源开关分离 | **已完成迁移**（§2.5–§2.7）：11 个依赖全部 wrap-git + 完整 commit pin（只剩字体包为 submodule、SQLite 仍为 amalgamation），SDL3 三态解析与 `deps.json`、pin/dirty 校验、SDK 依赖源码收集与裁剪、vendored 消费 fixture、CI 三条显式路径、`portable-bundle` 守卫、`header-surface` 门禁均已落地。**仍未完成**：OS 包层面的 DSO 打包（暂无应用安装路径）、内部类型进公开签名的评审项、后端拆库 |

## 推进顺序与批次关口

以下保留批次交付范围、依赖与历史验收证据；不是当前待办队列。当前执行顺序以[开发路线](development_roadmap.md)
为准：P2 已交付，先推进 P3.1a，再分批定形其余 D8/D1；历史验证不算本轮验证。

| 批次 | 交付 | 技术前置 | 退出条件 |
| --- | --- | --- | --- |
| 0 文档基线（本轮） | 核对状态、修正机制判断、统一排期并提交 | 现有源码与历史证据 | 文档链接、状态、diff 核对；不将计划标为实现完成 |
| 1 D7 混合修复（**已完成**） | 设备初始化时启用几何混合；确定性交互帧回归 | 无；不等依赖迁移 | **已交付**：`tests/sdl_visual_tests.cpp`（软件渲染器，4 用例/150 断言，解析合成预期 ±2，`unit` 的 `sdl-visual`）。本次补强真实 OverlayHost + Dialog 开关/布局/动画/绘制，在面板外核对打开前、打开后、关闭后颜色；补四角扇形内部与拼接附近采样。实际 Dialog fill alpha=0/1、扇形漏画注入均会红，恢复并重编译后全绿。历史移除混合设置的 4 用例全红记录保留。历史 GPU 证据：遮罩 mean 5.9→125.0、选中项最暗行 156.9→74.7（std 32.1→88.0）；本次未重跑 GPU，软件结果不覆盖历史 GPU 验收。页面级审核 golden 已由批次 3 补齐（6 条 approved，见[视觉回归](visual_regression.md) §5）；具体证据和边界见该文 |
| 2 SDL3 来源与交付（**已完成**） | 实测 CMake wrap、固定版本、三态来源和解析报告；接 CI/SDK；建立[头文件分类](sdk_surface.md)门禁 | **最小实验已完成**（[依赖策略 §2.4](dependency_policy.md)）：`SDL3-static`、`subproject_options()`、三态显式解析、离线与全新检出均实测；结论是不用 `[provide]` 变量名、失败必须 `error()`、随后已用 CI 固定的 Meson 1.7.2 复跑（§2.5）。正式接入已落地（[依赖策略 §2.5](dependency_policy.md)）：`sdl3_source` 三态默认 vendored、`deps.json` 报告、`.gitignore`、SDK 收集依赖源码并记录 manifest、fixture 断言依赖随包；**静态 SDL3 与 raylib 内置 GLFW 的 Wayland 协议符号冲突，vendored 改用共享库**。已交付：11 个依赖 wrap 化、三态来源与 `deps.json`、pin/dirty 校验、SDK 依赖源码收集与裁剪、两个 SDK 消费 fixture、CI 三条显式路径 + 预热/缓存、`portable-bundle` 与 `header-surface` 门禁、Meson 1.7.2 复跑。**明确不在本批**：OS 包/安装器层面的 DSO 打包（当前没有应用安装路径）、内部类型进公开签名的评审项、后端拆库（长期）。**ASan 构建目录例外**：vendored 共享 SDL3 撞 `ld.bfd` DWARF bug，该目录显式用系统 SDL3（81/81 通过）；CI 另有非阻断的 lld 实验任务 | 三态、关闭后端不获取、干净检出与预热后断网构建通过；SDL 路径被显式启用并验证；SDK 包含所需依赖。`.gitignore` 覆盖 wrap 检出目录、SDK 显式收集依赖源码与许可证、打包在 nodownload 下可重复（fixture 逐字节断言不失真） |
| 3 D7 生产 SDF（**填充/描边/圆/线段/弧已完成并验收；其余图元与未验平台在范围外**） | 同帧多参数实验 → 产物交付 → 逐图元实现；保留有能力说明的基础降级路径。实验已有 64 字节参数、4 KiB 尾部读取、三档缩放/细描边/纹理与 SDL 字形交错、两条失败注入，并已定下**不可变参数 state + 有界缓存**的提交策略（[路径契约 §9](render_device_path_contract.md#9-参数提交策略2026-10-19-决定)）。生产设备已接入 SDF 填充/描边/圆/线段/弧（`analytic_antialiasing()`、`sdf_diagnostics()`、CPU 回落），由 `sdf-device` 设备级测试与 `parameter-state-cache` 单测覆盖。**已交付**：Linux 6 条页面 golden approved、release 性能基线（8 配置）与预算门禁；Windows D3D12/DXIL 实机设备级验收（与 Linux 直线 AA 逐位一致），Windows 6 条 approved（CPU 3 + SDF 3，2026-10-22）及 CPU/SDF 各 4 组性能参考基线；线段与圆环扇区（设备级像素/AA/seam/缓存断言，PC 基准 binds 不变且性能在预算内）。**未交付**：MSL 实机验收；Windows 复验方式见[验收清单](windows_acceptance.md)，已交付基线不再列为待采集 | 批次 1、2 已完成；Vulkan 与 DXIL 已分别实机验收，MSL 未验收；Linux/Windows 页面基线平台隔离，Windows 性能不跨平台比较、不参与计时门禁 | 生产图元/文字/图片纹理交错时参数和状态正确；分数缩放、细描边与半透明叠加通过；质量达标且像素差异有界可解释；报告稳态与交互成本（按[性能预算](performance_budget.md)协议采样，先报告模式），**每帧状态设置与 uniform 更新次数**、内存一并记录，新路径经审核建立自己的基线；驱动级逃生舱可用 |
| 4 D7 共享图片解码（**已完成 2026-10-20**） | 应用服务提供默认 CPU 解码器，两后端复用；删除 `IPlatformWindow::make_image_decoder()`（破坏性变更，已按 [SDK 表面](sdk_surface.md) 记录）；SDL 补 `create_rgba_texture`；保持 crop/resize/tint | 解码库来源与行为定形；不依赖 SDF | **已交付**：vendored stb 解码（PNG/JPEG/BMP/GIF/TGA，crop/resize/tint）、`make_cpu_image_decoder()`、`NanApplication::set_image_decoder()`、三项能力（`can_decode`/`supports_direct_image_loading`/`supports_rgba_texture_upload`）、SDL 上传 + `draw_texture_region` 像素断言、`image-decoder` 8 用例（含损坏输入与后台线程）。**未做**：SDL 上完整 `Image` 页面的端到端采样（服务路径已由上传测试与能力矩阵覆盖）；WebP/QOI 不再支持（stb 无对应解码，原 raylib 路径也未启用 WebP） |
| 5 D8 输入基础 | **P3.1a/b/c 已实施**：事件坐标/修饰键快照、类型化逻辑/物理键与 press/repeat/release、跨类型事件保序；不扩展焦点策略或 IME | 无渲染前置；独立推进 | `sdl-platform-input` 11 用例/366 断言、`key-lifecycle` 5 用例通过；主套件 108/108、ASan 85 + 1 SKIP、no-RTTI 87/87，记录见 [D8](reference_draft.md#p31c跨类型事件保序2026-10-31-已实施)。后续另验真实布局与 Windows |
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
- **跨平台分开记录**：SPIR-V/DXIL/MSL 已有产物；SPIR-V 有独立 spirv-val 与设备级像素验收，
  DXIL 已在 Windows D3D12 实机通过设备级验收（直线 AA 与 Linux 逐位一致），MSL 仅有静态校验、
  目标设备未验收。Web canvas 探测不等于 Nandina 应用可交付。
- Modules、CMake package、shell/compositor、任意矢量路径仍待实际需求与对应关口，不进入当前关键路径。

### 每批的完成定义

1. 先更新专题中的归属层、API 和能力边界；需要用户判断的未定行为在实施前收敛。
2. 实现、消费者接线、针对性回归一起交付，说明“改坏哪里会红”；公共 API 变更同步相关调用。
3. 代码批按[开发流程](development_loop.md)执行主套件、ASan/UBSan unit、no-RTTI unit及对应专项。
   文档批检查链接、状态和 diff，不把历史测试结果当作本轮验证。
4. 结果写回本表所链接的专题证据，再更新状态与提交；未执行或依赖外部设备的验收明确保留。
5. 用真实应用贯穿导航、表单、图片、滚动和弹窗。Showcase 保留组件验收职责；notebook 服务/UI 与
   showcase 组件由维护者手工编写，不属于本轮框架代写范围。正式验收 fixture、脚本和说明不得只存在于未追踪的 playground。

## D5 资源工作线：P2 已实施（2026-10-22 至 2026-10-25）

正式契约以 [资源构建、部署与运行时定位](resource_build_and_deployment.md) 为准。以下每批
独立提交与验收；“解耦先行”描述 D5 的依赖，不要求把所有后续修改塞进一次重构。

| 批次 | 交付与前置 | 验收关口 |
| --- | --- | --- |
| 1 解耦 | 保留当前 ID、包 schema、扫描行为；工具脱离完整 UI，默认工具开/后端开/vendored，增加独立关闭能力 | 单挂载/无冲突的前后逻辑等价；无 UI 依赖环境构建工具；关闭组合真实运行资源消费者；依赖闭包门禁 |
| 2 作用域 | 不改包格式或 ID 生成；管理器八入口补作用域，后端交付结果与发布代次，迁移缓存和失效 | 跨挂载/manager 冲突、ID 歧义、别名、并发发布、过期引用、已取得资源/流及缓存回收 |
| 3 身份 | 设计可与解耦并行，依赖包内 ID 契约的实现以作用域完成为前提；落实 key 映射、生成算法和作者引用 | 显式 key 跨重命名、重复内容独立记录、碰撞与重建行为；新 ID 由版本化算法派生并用固定向量锁定（P2.3a 已落地），作者常量头由同一 policy 生成并作为真实编译输入（P2.3c 已落地） |
| 4 声明式构建 | 相应解耦/身份前置完成；resources.toml 唯一语义入口，补应用依赖、目录变化与输出发布 | 定向构建、增量、源树只读；两输出根逻辑比较之外另测真实并行/多配置隔离；生成头如启用则是真实编译输入 |
| 5 部署与定位 | 安装与定位同源，先 portable/当前平台 prefix，再逐平台扩展 | DESTDIR、安装覆盖、重定位、任意 cwd 与精确错误 |
| 6 作者验收与归档 | 独立下游应用走完整流程，按实际支持面更新教程与示例 | 干净检出、构建、安装后通过正式 ResourceManager 读取资源；平台验证分别记录 |

作用域批按 reference §13 定形六项 API 细节，身份算法已在 P2.3a 定形；不会重新开放纯内容 ID 或
拆 blob 表作为本轮方案。CMake、Web、system SQLite 和新平台部署均须独立验证后才宣称支持。
取舍及“头文件只有路径不等于流没有文件句柄”的证据纠正见
[story 013](../../stories/013-resource-scope-before-identity.md)。P2.1a–P2.4c 已按批次实施并验证；后续声明式构建扩展与部署定位按开发路线推进。

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

## 应用示例：从 lab 到 notebook（2026-10-28 起）

P2.4 的 `examples/lab` 已退役，示例应用改为 `examples/notebook`：目标是一个较完整的个人笔记软件，按批持续演进，而不是一次交付完整产品。

- **已落地**：SQLite 数据层（连接/statement/事务、`schema_migrations` 与 `PRAGMA user_version`、资源驱动迁移、user/note-group/note/todo 模型与仓储、`DataError` + `std::expected` 错误约定）；`notebook-data` 单元测试覆盖迁移顺序/缺口/重命名、CRUD、所有权与级联。
- **进行中**：auth/user 服务与 UI 页面；页面组件先在 `showcase/` 编写并做视觉验收，notebook 负责组合。
- **测试分工**：数据层等确定性测试留在仓库（`examples/notebook/tests/`，`notebook-data`，CI/Windows 覆盖）；UI/交互与长跑测试放未跟踪的 `playground/`，不作为 CI 证据。
- **构建前置**：notebook 需要 `build_ui`、资源工具链与 `resource_sqlite=enabled`；headless、关闭工具或关闭 SQLite 的配置用 `-Dexamples=false` 跳过它。
- **历史证据**：P2.4 的验收记录与 `docs/images/lab/` 截图保留，不随 lab 退役删除。
- **干净检出修复（2026-10-31，Windows 补验时发现）**：`resources.toml` 的扫描根改为已跟踪的 `src/data/migrations`（空占位目录不会进入 Git）；数据层测试与 README 完成 `nan_example::notebook` 命名空间迁移；启动错误改用框架 logger，避免 Windows 工具链缺少 `std::println` 的终端符号。
- **验证（2026-10-28）**：默认主套件 **106/106**（`notebook-data` 通过；lab 的 4 个测试随退役删除、notebook 数据层测试新增）；release check 8/8。干净离线配置（`--wrap-mode=nodownload -Dexamples=true -Dshowcase=false -Dbuild_tests=false`）从零构建 `nandina_examples_notebook` 通过并生成迁移包与常量头；`-Dresource_sqlite=disabled` 且 `examples` 默认开启时配置期报错 `examples/notebook needs the SQLite runtime backend`；headless 配方 `-Dsdl_backend=disabled -Dshowcase=false -Dexamples=false` 配置通过。交替矩阵：ASan **83/83（+1 SKIP）**、no-RTTI **85/85**（含 `notebook-data`）、no-SQLite **81/81（+1 SKIP）**、纯工具 8/8。Windows 设备本轮不可达（`No route to host`），未复跑；恢复后补验构建、`--suite unit` 与 `notebook-data`。

## 下一阶段路线（2026-10-20）

已验收基线与 P1–P4 的排序、退出条件、近期切入顺序和非目标见[开发路线](development_roadmap.md)。

- **P1 收口与平台补齐**：发布检查脚本、Windows 页面 golden/性能基线、图片共享解码与 raylib 退役、SDF 阴影/裁剪（已完成，Windows GUI 项待人工）；
- **P2 应用闭环**：已完成；P2.4 的 `examples/lab` 单可执行多路由贯穿登录/注册、笔记、计时器、图片、滚动与确认弹窗（2026-10-25），已于 2026-10-28 退役并由 `examples/notebook` 接替；
- **P3 输入与焦点（当前阶段）**：P3.1a/b/c 已交付，下一步 D1 焦点提示（P3.2）→ 文本会话/IME（P3.3）；Windows 已补验（构建 0 错误、unit 85/85，`key-lifecycle`/`sdl-platform-input`/`notebook-data` 通过）。
- **P4 作者与组件**：D6 Label 能力与测量 → 阶段 5 数据组件 → D2/D3。

P1.1 与 P1.6 核心实现已完成，raylib 后端、工厂、枚举项、wrap 与依赖已完整删除。
当前 vendored SDL3 默认静态、可选共享；无后端构建需关闭 showcase，并在打开窗口前注入平台。
退役验收入口为窗口生命周期、后端选择、主套件、无后端 unit 与 SDK static/shared 消费。
Windows SDF 路径 6 条 golden 与性能参考基线已交付；Image 页面端到端采样、MSL 实机验收仍待完成；Windows 后续复验方式见[验收清单](windows_acceptance.md)。
CLI 与生成模板唯一归属 NandinaCLI；框架保留最小 SDK 应用契约 fixture。
