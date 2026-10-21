# 平台与后端契约（D4）

> **状态：SDL3 为唯一内置后端，raylib 已完整退役；2026-10-20 更新。**
> 默认启用 SDL3，vendored 默认静态、可选共享；关闭 SDL 后须注入平台才能打开窗口。
> 三格式探测产物与逐图元 trace 已存在；
> 剩余为生产渲染质量（D7）、规范输入（D8）、可重复依赖交付和逐平台应用验收。
> DXIL 在退役前已有 Windows 真机证据；本轮静态模式需复验，MSL 尚无目标设备验收。
> 全局批次顺序见[项目进度与下一步](project_status.md)，本文历史测试与测量本轮未重跑。
>
> 探测证据与「已证明 / 未验证」清单见[跨平台后端探测](cross_platform_backend_probe.md)；
> 取舍过程见 [story 015](../../stories/015-the-probe-that-was-not-in-version-control.md)；
> 探测源码与构建说明见 [probe/cross_platform/](../../probe/cross_platform/README.md)。

## 1. 目标与范围

**目标平台**：Linux / Windows / Web 的应用级交付。平台层选 **SDL3**（窗口、事件、输入、
帧驱动），渲染在 `IRenderDevice` 之下各自实现。

**明确不在承诺内**：

- Wayland shell / compositor 集成；
- WebGL1（当前 shader 依赖 `fwidth` 导数，WebGL2 是最低要求）；
- D3D12 / Metal 的完成时间（`SDL_shadercross` 的多后端产物**已构建验证**，但**加载**需要
  对应设备，见探测记录 §3.1）；
- 「同一份 SDF shader 跑遍所有平台」：Web 走独立的 2D 实现（见 §2）。

## 2. 分层与所有权（P4 的核心）

| 职责 | 今天谁持有 | 目标 |
| --- | --- | --- |
| 窗口、事件、输入轮询、帧驱动 | ✅ `IPlatformWindow` + `detail/sdl_platform_window`；`NanWindow` 只做协调 | SDL3 为唯一内置后端；自定义平台仍可注入。无后端时打开窗口明确报错 |
| 绘制原语与帧边界 | ✅ `render/backends/sdl_device.{hpp,cpp}`（SDF + CPU 几何回落） | raylib 实现已删除；保留 `IRenderDevice`、录制装饰器与测试替身 |
| 文本整形、图集、排版 | `text` 层 | **不变**：保持在设备边界之上，不随平台分叉 |
| 图片解码 | 应用服务（默认或注入的 `IImageDecoder`） | **已落地（2026-10-20）**：`make_image_decoder()` 已删除；`NanApplication` 默认注入 CPU 解码器（stb），平台只保留设备上传入口（[D7](reference_draft.md#图片解码器删除平台钩子改为应用服务2026-10-17-定)） |
| 资源、主题、响应式、组件 | `resource` / `theme` / `reactive` / `widget` | 保持各自职责，平台不持有其业务状态 |

两条已有探测记录支持的构建层事实（写进来是因为它们直接限制「后端可替换」的程度）：

1. **设备实现不是纯头文件插件。** `IRenderDevice` 是 header-only，但它用到的 foundation 值类型
   （`NanRect` / `NanPoint` / `NanSize` / `NanColor`）有 out-of-line 实现。独立设备要么链
   `libnandina`，要么编进 `foundation/geometry.cpp`、`nandina_color.cpp`、
   `nandina_color_space.cpp` 三个 TU（探测选了后者）。
2. **最小宿主已经证明可以绕开 `NanWindow`**：`NanApplication` + `NanRouter` + `NanSceneTree`
   + 设备即可挂载并绘制真实页面。这条路径是探测代码；正式宿主接口现已拆出。探测仍需遵守相同帧阶段和浮层根契约。

## 3. 能力契约：差异必须写在代码里

`IRenderDevice` 已有 `supports_alpha_textures()` / `supports_rounded_rect()` 的先例。规则：

- 平台/后端的差异**表现为 capability 查询驱动的降级路径**，由消费者（组件）按能力选路；
- 不允许用「文档里记了已知差异」代替代码分支 —— 否则 SDK 示例跑通会被误读成「应用已支持」；
- 每个后端必须给出自己的能力表，并由测试或探测证据支撑。

下面保留退役前的差异取证（SDL 2D 设备 vs 当时的 raylib 后端，见探测记录 §2.3）。
raylib 不再可构建；当前 SDF 已覆盖填充/描边/圆/线段/弧/软阴影/涟漪圆角交集（2026-10-21）。通用子树圆角裁剪不在本轮范围。
**"已被查询覆盖"** 列表示
这条差异已经有代码分支、并有会红的测试；其余是待办。

| 能力 | raylib 生产后端 | SDL 2D 设备 | 已被查询覆盖 | 待办 |
| --- | --- | --- | --- | --- |
| 矩形圆角绘制 | SDF | CPU 三角形扇 | `supports_rounded_rect()`（`BoxPainter`） | D7 生产 SDF 路径 |
| 圆角/圆抗锯齿 | SDF + `fwidth` | 硬边缘 | —（质量差异，无离散路径） | D7 SDF。历史单页静止帧差异见探测记录 §2.3.2，不代表其他页面或交互态 |
| 几何**半透明混合** | 正常 | **已修复（2026-10-17）**：设备构造启用 `SDL_BLENDMODE_BLEND`，`check_sdl` 处理失败 | — | 回归已落地：`tests/sdl_visual_tests.cpp` 用软件渲染器 + 解析合成预期覆盖半透明圆角矩形、按下态、涟漪中间态、Dialog 遮罩；删除设置 4 用例全红（见[视觉回归](visual_regression.md) §5） |
| 软阴影 | SDF 软阴影 | 未实现（硬边圆角降级） | **`supports_soft_shadows()`**（`ShadowPainter` 按能力选路，`tests/card_tests.cpp`）；SDF 路径已实现（mode 4，`sdf-device` 覆盖内部/软边衰减/缓存） | 已完成（2026-10-21）；无 SDF 设备继续明确降级 |
| 涟漪专用圆角交集 | SDF 解析交集 | 只能矩形裁剪 | **`supports_rounded_clip()`**（`RipplePainter` 按能力选路，`tests/button_tests.cpp`）；SDF 路径实现 mode 3 交集（`sdf-device` 覆盖矩形/圆/圆角三种裁剪方向） | 已完成（2026-10-21）；图片/子树通用裁剪另批定形 |
| 圆角**描边** | SDF | 已实现（四条直边 + 四个同心环扇） | —（不需要查询，设备必须覆写） | **接口默认会退化成方框描边**，新后端必须覆写；消费者回归在 anchors 页面扫到过（探测记录 §2.3.3） |
| `draw_arc` | SDF 环 | **已实现**（同心环扇，按张角取 6–96 段） | —（不需要查询，设备必须覆写） | 当前 showcase 没有页面用到 Spinner/ProgressBar，因此只有设备级证据、没有页面级证据 |
| 文本 | HarfBuzz + `GlyphAtlasTexture` | 已跑通（真实图集、真实页面） | `supports_alpha_textures()` | 保持 |
| 图像文件加载 | 直载与 `create_rgba_texture` 都有 | **已落地**：SDL 直载 false / RGBA 上传 true；raylib 两者皆 true | **三项能力分别查询**：`can_decode()` / `supports_direct_image_loading()` / `supports_rgba_texture_upload()` | 标准路径 = 解码服务可用 **且** 设备支持 RGBA 上传；SDL 的 `create_rgba_texture` 已补齐并通过 `sdl-visual` 上传/绘制像素断言；见 [D7](reference_draft.md#三项能力分开表达2026-10-17-定) |
| alpha 图集内存 | A8 → RGBA8（4×） | 同构（4×） | — | 单通道 + 专用字形着色器 |

`supports_rounded_clip()` 顺带修掉一类真实的溢出：**接口默认的
`draw_circle_clipped_rounded_rect()` 只画未裁剪的圆**，所以过去任何不覆写它的设备都会让水波纹
漏到圆角容器外面。现在消费者按能力选择「交给设备裁」还是「自己 `set_clip` 限制到矩形边界」。这不保证圆角区域不外溢，也不证明通用子树裁剪已可用。

## 4. Shader 构建链契约（P1 门槛）

| 项 | 契约 |
| --- | --- |
| canonical source | HLSL（已验证 `glslc -x hlsl -fshader-stage=frag --target-env=vulkan1.0` 路径） |
| host 工具 | 版本固定并**校验**（`glslc` 2026.4、SPIRV-Tools v2026.4 已在配置期硬校验）；跨编译时执行 host 版本，不依赖开发机「刚好装好」 |
| 产物交付 | 探测已把生成与 spirv-val 接进构建图；生产 D7 推荐随源码包交付受控产物，维护关卡核对源哈希/工具/生成结果，普通消费者不依赖 shadercross；嵌入/安装布局待定 |
| 失败可定位 | 报源文件、目标 backend/format、工具版本与诊断输出 |
| 多后端产物 | `SDL_shadercross`（提交 `1ff05bec…`）已构建，SPIR-V/DXIL/MSL **均已实际生成**；仅 SPIR-V 由独立 `spirv-val` 校验（`tools/verify_shadercross.sh`，探测记录 §3.1）；**加载** DXIL/MSL 仍需 D3D12/Metal 设备 |
| API floor | `SDL_GPURenderState` 需要 SDL **3.4.0**；对外声明的最低版本不得低于它，CI/SDK 固定精确受测版本 |

## 5. 阶段交付与验收

阶段中的已完成部分与剩余验收分别记录；下面引用的是历史证据，不是本轮运行结果。

| 阶段 | 交付 | 验收 | 证据/状态 |
| --- | --- | --- | --- |
| **P0** 应用级探测 | 窗口/事件/帧循环；GPU render state 自定义 shader；真实 `IRenderDevice`；真实页面 + 真实字体图集 | 真实 showcase 页面在 SDL 设备上正确渲染；稳态 0 次布局 | **基本完成**，缺键码映射验证（[§2.3.1](cross_platform_backend_probe.md)） |
| **P1** 渲染与 shader 管线门槛 | 工具链固定；SPIR-V 校验入构建图；多后端产物；同页跨后端对照；能力差异落成代码分支；Web 端有效验收 | 每个承诺格式都能生成并加载；能力差异有会红的测试；同页对照有量化结论；Web 验收能区分失败原因 | **基本完成**：工具链、校验、能力降级路径、同页像素对照（0.01% 像素、≤20/255）、Web 端验收、**多后端产物已生成并外部校验**（§3.1）、**逐图元对照**（`render/recording_device.hpp` + `tools/compare_render_traces.py`，三个页面的图元构成逐项相同，§2.3.6）均已完成；剩余：DXIL/MSL 的**加载**（需 D3D12/Metal 设备） |
| **P2** 平台能力与消费者回归 | 能力查询落地到组件；各后端行为回归；**含外壳、交互态与浮层** | 组件在两种后端下的行为有对照记录；交互态可用框架状态核对；差异要么修复要么降级 | **基本完成**：三个页面像素对照（Home 相同、其余只剩角部 AA，扫出并修复圆角描边退化，§2.3.3）；**真实外壳 + hover/press/导航/选中态**回归通过并挖出三条 host 契约（§2.3.4）；**浮层回归**（`Dialog` 经 `OverlayHost` present，`overlay_count=1`，§2.3.5）；`draw_arc` 已实现；剩余观感差异归 [D7](reference_draft.md#d7-渲染质量与后端视觉一致性) |
| **P3** 默认后端替换 | SDL 设备成为默认；raylib 退回离线/录制设备 | 真实应用在 SDL 默认路径下通过三套验证与人工验收 | **机械验收通过；当时人工观感验收未通过（混合缺陷），该缺陷已于 2026-10-17 修复、待人工复验**（含性能/内存复测：SDL 64 MB / 6% 单核 vs raylib 150 MB / 10%，探测记录 §2.3.7）：`NANDINA_WINDOW_BACKEND=sdl3 ./buildDir/showcase/nandina_showcase` 可运行（renderer=gpu、~5% 单核、侧边栏 + Home 页渲染），三套验证在 SDL 为默认后端下全绿（含 window-lifecycle 走 SDL 宿主），`playground/bench` 显式钉住 raylib 作基线；但作者实测出现按钮按下白底白字、涟漪失真（几何混合缺失，见 [D7](reference_draft.md#d7-渲染质量与后端视觉一致性)），因此**不能标记为通过人工验收**，该项转入 D7 批次 1 |
| **P4** host 行为契约收口 | 平台层接口成形；`NanWindow` 不再持有窗口/输入/帧驱动；**设备可注入**；**应用背景由谁绘制**要有定义；宿主阶段链完整（drain dispatcher、process、tree_commit、physics、advance_animations） | 同一份组件代码在 SDL 与 raylib 两个宿主下行为一致 | **基本完成**：seam + 两个实现已落地，设备由平台创建（可注入）；三条必备契约记在探测记录 §2.3.4/§2.3.5（drain UiDispatcher、完整阶段链、浮层必须以 OverlayHost 为树根）；逐图元 trace 设备（`render/recording_device.hpp`）与对照工具已落地（探测记录 §2.3.6） |

## 6. D4 收尾与 D7/D8 的接口

- 默认切换和平台接口不重做；复用 `IPlatformWindow`、设备包装及既有 trace 工具。
- D7 首先修混合，再以同帧多参数图元、文字/纹理交错验证生产 SDF。已有探测 shader
  无 uniform buffer，不能替代这一步；基础 SDL Renderer 回退必须明确能力和质量。
- 图片解码归共享 CPU 服务；纹理上传归设备。通用圆角裁剪与涟漪专用交集分别定形。
- D8 定形物理/逻辑键、事件快照与输入保序；D1 第二阶段的 `ITextInputService`
  复用现有 seam，由窗口注入，不能另写一套宿主。
- 历史 trace、静止帧像素、交互状态、交互像素是不同证据；通过其中一类不自动覆盖其他类。
  视觉验收细目以 [D7](reference_draft.md#d7-渲染质量与后端视觉一致性) 为准。

## 7. 尚未完成的验收与决策

1. Windows 与 Web 的应用级交付顺序、构建工具链和真实应用验收等级；
2. DXIL/MSL 在 D3D12/Metal 设备上的创建、绑定、绘制与读回；生成/静态校验不替代实际加载；
3. Web 应用的主循环、输入、字体和资源交付；现有 Web 探测只证明指定探测路径；
4. D7 的 shader 参数/产物布局、基础回退和通用裁剪协议；
5. D8 的框架输入值类型及各平台能提供的字段；不再把 GLFW 编号作为公开契约；
6. 依赖策略的 SDL3 固定源码、明确启用的 CI 与离线 SDK；当前 CI 可能在无 SDL3 时只覆盖 raylib。

政策已定、机制未实施的三份配套文档：[SDK 表面](sdk_surface.md)（头文件等级与门禁）、
[视觉回归](visual_regression.md)（golden 组织与审批）、[性能预算](performance_budget.md)
（采样协议与候选阈值）。

## 8. 相关记录

- 探测证据与未验证清单：[跨平台后端探测](cross_platform_backend_probe.md)
- 探测源码、工具链核对、构建方式：[probe/cross_platform/](../../probe/cross_platform/README.md)
- 为什么不能拿旧数据做后端性能结论：[文本整形与页面打开成本](text_shaping_cost.md)
- 布局收敛（跨后端的收敛性质对照）：[布局收敛与文本测量](layout_convergence_and_text_measurement.md)
- 输入/焦点与文本会话（P4 的协调对象）：[统一输入、焦点与文本编辑](focus_and_input.md)
