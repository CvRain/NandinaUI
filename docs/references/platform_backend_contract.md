# 平台与后端契约（D4）

> **状态：后端替换已实施，D4 收尾中。** 作者 2026-10-15 决定先完成 D4 计划内容、随后把生产
> 后端替换为 SDL3。**P3 已落地**：`IPlatformWindow` seam 成形、SDL3 宿主与 SDL 渲染设备可用，
> 且装了 SDL3 的构建默认就走 SDL3（raylib 保留为对照/离线设备）；剩余是 P1 的多后端 shader
> 产物、P2 的若干消费者面与逐图元对照。
>
> 探测证据与「已证明 / 未验证」清单见[跨平台后端探测](cross_platform_backend_probe.md)；
> 取舍过程见 [story 015](../../stories/015-the-probe-that-was-not-in-version-control.md)；
> 探测源码与构建说明见 [probe/cross_platform/](../../probe/cross_platform/README.md)。

## 1. 目标与范围

**目标平台**：Linux / Windows / Web 的应用级交付。平台层选 **SDL3**（窗口、事件、输入、
帧驱动候选），渲染在 `IRenderDevice` 之下各自实现。

**明确不在承诺内**：

- Wayland shell / compositor 集成；
- WebGL1（当前 shader 依赖 `fwidth` 导数，WebGL2 是最低要求）；
- D3D12 / Metal 的完成时间（`SDL_shadercross` 的多后端产物**已构建验证**，但**加载**需要
  对应设备，见探测记录 §3.1）；
- 「同一份 SDF shader 跑遍所有平台」：Web 走独立的 2D 实现（见 §2）。

## 2. 分层与所有权（P4 的核心）

| 职责 | 今天谁持有 | 目标 |
| --- | --- | --- |
| 窗口、事件、输入轮询、帧驱动 | ✅ `IPlatformWindow`（`app/platform_window.hpp`）+ `detail/raylib_platform_window`、`detail/sdl_platform_window`；`NanWindow` 只做协调 | 已完成。默认后端 = 装了 SDL3 就用 SDL3（`NANDINA_WINDOW_BACKEND` 可覆盖） |
| 绘制原语与帧边界 | ✅ `render/backends/sdl_device.{hpp,cpp}`（SDL 2D + CPU 几何）与 `raylib_device` | SDL 设备是默认；raylib 设备保留给对照/离线/录制 |
| 文本整形、图集、排版 | `text` 层 | **不变**：保持在设备边界之上，不随平台分叉 |
| 资源、主题、响应式、组件 | `foundation` / `theme` / `scene` / `widget` | **不变** |

两条已经实测的构建层事实（写进来是因为它们直接限制「后端可替换」的程度）：

1. **设备实现不是纯头文件插件。** `IRenderDevice` 是 header-only，但它用到的 foundation 值类型
   （`NanRect` / `NanPoint` / `NanSize` / `NanColor`）有 out-of-line 实现。独立设备要么链
   `libnandina`，要么编进 `foundation/geometry.cpp`、`nandina_color.cpp`、
   `nandina_color_space.cpp` 三个 TU（探测选了后者）。
2. **最小宿主已经证明可以绕开 `NanWindow`**：`NanApplication` + `NanRouter` + `NanSceneTree`
   + 设备即可挂载并绘制真实页面。这条路径目前只是探测代码，是 P4 拆分的现实起点。

## 3. 能力契约：差异必须写在代码里

`IRenderDevice` 已有 `supports_alpha_textures()` / `supports_rounded_rect()` 的先例。规则：

- 平台/后端的差异**表现为 capability 查询驱动的降级路径**，由消费者（组件）按能力选路；
- 不允许用「文档里记了已知差异」代替代码分支 —— 否则 SDK 示例跑通会被误读成「应用已支持」；
- 每个后端必须给出自己的能力表，并由测试或探测证据支撑。

实测到的差异（SDL 2D 设备 vs 生产 raylib 后端，见探测记录 §2.3）。**"已被查询覆盖"** 列表示
这条差异已经有代码分支、并有会红的测试；其余是待办。

| 能力 | raylib 生产后端 | SDL 2D 设备 | 已被查询覆盖 | 待办 |
| --- | --- | --- | --- | --- |
| 矩形圆角绘制 | SDF | CPU 三角形扇 | `supports_rounded_rect()`（`BoxPainter`） | 提高分段/覆盖率质量 |
| 圆角/圆抗锯齿 | SDF + `fwidth` | 硬边缘 | —（质量差异，无离散路径） | 覆盖率几何或 SDF shader。**已量化**：同页对照下差异 ≤20/255、仅 0.01% 的像素（探测记录 §2.3.2） |
| 软阴影 | SDF 软阴影 | 未实现 | **`supports_soft_shadows()`**（`ShadowPainter` 退化为硬边圆角阴影，`tests/card_tests.cpp`） | 更好的硬边近似 |
| 圆角裁剪 | SDF 解析交集 | 只能矩形裁剪 | **`supports_rounded_clip()`**（`RipplePainter` 自己 `set_clip` + 画圆，`tests/button_tests.cpp`） | 遮罩/模板或 SDF |
| 圆角**描边** | SDF | 已实现（四条直边 + 四个同心环扇） | —（不需要查询，设备必须覆写） | **接口默认会退化成方框描边**，新后端必须覆写；消费者回归在 anchors 页面扫到过（探测记录 §2.3.3） |
| `draw_arc` | SDF 环 | 接口默认（弦逼近 ≤ 64 段） | —（未实现该入口） | 提高分段或 SDF |
| 文本 | HarfBuzz + `GlyphAtlasTexture` | 已跑通（真实图集、真实页面） | `supports_alpha_textures()` | 保持 |
| 图像文件加载 | 有 | 未实现 | — | 解码器或 SDL_image |
| alpha 图集内存 | A8 → RGBA8（4×） | 同构（4×） | — | 单通道 + 专用字形着色器 |

`supports_rounded_clip()` 顺带修掉一类真实的溢出：**接口默认的
`draw_circle_clipped_rounded_rect()` 只画未裁剪的圆**，所以过去任何不覆写它的设备都会让水波纹
漏到圆角容器外面。现在消费者按能力选择「交给设备裁」还是「自己 `set_clip` 保证不溢出」。

## 4. Shader 构建链契约（P1 门槛）

| 项 | 契约 |
| --- | --- |
| canonical source | HLSL（已验证 `glslc -x hlsl -fshader-stage=frag --target-env=vulkan1.0` 路径） |
| host 工具 | 版本固定并**校验**（`glslc` 2026.4、SPIRV-Tools v2026.4 已在配置期硬校验）；跨编译时执行 host 版本，不依赖开发机「刚好装好」 |
| 产物入构建图 | 源 shader 变化触发重新生成；`spirv-val` **是构建图的一步**（已实施：坏产物会让构建失败）；安装/打包带上二进制 |
| 失败可定位 | 报源文件、目标 backend/format、工具版本与诊断输出 |
| 多后端产物 | `SDL_shadercross`（提交 `1ff05bec…`）已构建，SPIR-V/DXIL/MSL **均已实际生成**并由独立 `spirv-val` 校验（`tools/verify_shadercross.sh`，探测记录 §3.1）；**加载** DXIL/MSL 仍需 D3D12/Metal 设备 |
| API floor | `SDL_GPURenderState` 需要 SDL **3.4.0**；对外声明的最低版本不得低于它，CI/SDK 固定精确受测版本 |

## 5. 阶段交付与验收

每阶段独立提交；验收标准里的「证据」列指向可复跑的产物。

| 阶段 | 交付 | 验收 | 证据/状态 |
| --- | --- | --- | --- |
| **P0** 应用级探测 | 窗口/事件/帧循环；GPU render state 自定义 shader；真实 `IRenderDevice`；真实页面 + 真实字体图集 | 真实 showcase 页面在 SDL 设备上正确渲染；稳态 0 次布局 | **基本完成**，缺键码映射验证（[§2.3.1](cross_platform_backend_probe.md)） |
| **P1** 渲染与 shader 管线门槛 | 工具链固定；SPIR-V 校验入构建图；多后端产物；同页跨后端对照；能力差异落成代码分支；Web 端有效验收 | 每个承诺格式都能生成并加载；能力差异有会红的测试；同页对照有量化结论；Web 验收能区分失败原因 | **基本完成**：工具链、校验、能力降级路径、同页像素对照（0.01% 像素、≤20/255）、Web 端验收、**多后端产物已生成并外部校验**（§3.1）均已完成；剩余：DXIL/MSL 的**加载**（需对应设备）与逐图元等价性（trace 设备未写） |
| **P2** 平台能力与消费者回归 | 能力查询落地到组件；各后端行为回归；**含外壳与交互态** | 组件在两种后端下的行为有对照记录；交互态可用框架状态核对；差异要么修复要么降级 | **进行中**：三个 showcase 页面逐像素对照（Home 相同、其余只剩角部 AA，扫出并修复圆角描边缺陷，§2.3.3）；**真实外壳 + hover/press/导航/选中态回归通过**（§2.3.4，并挖出两条 host 契约） |
| **P3** 默认后端替换 | SDL 设备成为默认；raylib 退回离线/录制设备 | 真实应用在 SDL 默认路径下通过三套验证与人工验收 | **已落地**：`NANDINA_WINDOW_BACKEND=sdl3 ./buildDir/showcase/nandina_showcase` 正常运行（renderer=gpu、~5% 单核、侧边栏 + Home 页正确渲染）；三套验证在 SDL 为默认后端下全绿（含 window-lifecycle 走 SDL 宿主）；`playground/bench` 显式钉住 raylib 以继续充当基线采集器 |
| **P4** host 行为契约收口 | 平台层接口成形；`NanWindow` 不再持有窗口/输入/帧驱动；**设备可注入**；**应用背景由谁绘制**要有定义；宿主阶段链完整（drain dispatcher、process、tree_commit、physics、advance_animations） | 同一份组件代码在 SDL 与 raylib 两个宿主下行为一致 | **基本完成**：seam + 两个实现已落地，设备由平台创建（可注入）；三条必备契约记在探测记录 §2.3.4/§2.3.5（drain UiDispatcher、完整阶段链、浮层必须以 OverlayHost 为树根）；仍缺**逐图元 trace 设备**本身 |

## 6. 迁移策略（后端替换）

1. **并行期**：两个设备都能渲染真实页面（已完成）；**同页像素对照已完成**（探测记录 §2.3.2：
   mean |Δ| 0.0033、最大 20/255、差异只出现在边缘抗锯齿）。逐图元对照受阻于设备不可注入，见 P4。
2. **补齐降级路径**：按 §3 的能力表，先把「会导致可见错误」的三项（圆角抗锯齿、圆角裁剪、
   软阴影）做成 capability 驱动的路径。
3. **消费者回归**：showcase 全页面 + `playground/bench` 在 SDL 后端下跑通；性能与内存重新测量
   （**不要**沿用布局收敛修复之前的任何后端性能数字，见[整形成本 §1](text_shaping_cost.md)）。
4. **切默认**：SDL 成为默认后端；raylib 后端保留为离线/无窗口/录制设备（测试需要它）。
5. **收口 host**：把窗口/输入/帧驱动从 `NanWindow` 拆到平台层接口。

**协调风险**：D4 P4 与 [D1 第二阶段](focus_and_input.md)（窗口注入 `ITextInputService`）都要动
`NanWindow`；两者必须排序或合并设计，不能在同一个提交里各改一半。

## 7. 未定项（不要假装已冻结）

1. Windows 与 Web 的先后顺序；
2. D3D12 / Metal 是否纳入承诺（`SDL_shadercross` 的可用性已在 Linux 上验证到"生成 +
   独立校验"这一步，见探测记录 §3.1）；
3. Web 是否复用 SDL GPU renderer，还是坚持独立的 2D 实现（当前倾向后者）；
4. 平台层接口的精确形状（`NanWindow` 拆分后的宿主 API）——属于 P4 的定形工作；
5. 规范键值的归属：`widget/key_codes.hpp` 目前直接是 GLFW 码，多后端下需要自己的规范化值。

## 8. 相关记录

- 探测证据与未验证清单：[跨平台后端探测](cross_platform_backend_probe.md)
- 探测源码、工具链核对、构建方式：[probe/cross_platform/](../../probe/cross_platform/README.md)
- 为什么不能拿旧数据做后端性能结论：[文本整形与页面打开成本](text_shaping_cost.md)
- 布局收敛（跨后端的收敛性质对照）：[布局收敛与文本测量](layout_convergence_and_text_measurement.md)
- 输入/焦点与文本会话（P4 的协调对象）：[统一输入、焦点与文本编辑](focus_and_input.md)
