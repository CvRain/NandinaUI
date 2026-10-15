# 跨平台后端探测：SDL3 GPU 与 Web 现状

> 本文是 D4「跨平台优先的窗口与渲染后端路线」的**探测记录**，不是选型批准，也不是生产就绪声明。
> 它按 [开发流程](development_loop.md) 的语言区分「已证明」「未验证」和「只是路径选择」。

**探测源码已入库**：[probe/cross_platform/](../../probe/cross_platform/README.md)（`desktop/`、`web/`、
`tools/verify_toolchain.sh`）。只追踪源码：SDL 源码树、emsdk、构建产物仍然现取现建
（构建目录约定放在已忽略的 `playground/probe-build/` 下）。工具链的**唯一核对入口**是

```sh
probe/cross_platform/tools/verify_toolchain.sh   # 打印并校验所有固定版本，缺必需项则退出 1
```

## 1. 桌面侧：SDL3 GPU-backed Renderer（Linux / Vulkan）

**已证明的一件事**：SDL 3.4.18 的 GPU-backed Renderer 能在本机加载由 HLSL 编译出的 SPIR-V
fragment shader，并把它应用到一个被 `SDL_SetRenderClipRect` 裁剪的 `SDL_RenderTexture` 上；
圆角 SDF、软阴影和裁剪在运行截图中可见。构建把 shader 纳入依赖图，产物同时进入构建树与安装目录
（`bin/assets/sdf.frag.spv`），安装版也能启动。

固定与工具链：

| 项 | 值 |
| --- | --- |
| SDL | 系统 `sdl3>=3.4.0`，实测 3.4.18 |
| shader 编译器 | `glslc`，**配置期硬校验 2026.4** |
| 编译命令 | `glslc -x hlsl -fshader-stage=frag --target-env=vulkan1.0` |
| SPIR-V 校验器 | `spirv-val`（SPIRV-Tools），**配置期硬校验 v2026.4，且已接进构建图** |
| 运行时定位 | `SDL_GetBasePath()/assets/sdf.frag.spv`，空或非 4 字节对齐时拒绝 |
| 产物 | 构建树 `build/assets/` + 安装树 `bin/assets/`（不嵌入可执行文件） |

从入库后的受追踪位置重新构建并运行过一次（2026-10-15）：`SDL 3.4.18, renderer=gpu,
shader-formats=0x2, DPI=1`，窗口正常出帧。校验门也实测有效 —— 把 `generated/sdf.frag.spv`
替换成垃圾数据后构建在 `spirv-val` 一步失败（`error: file size should be a multiple of 4`），
而不是留到运行时。

**未验证**（不要把它读成更大范围的支持）：

- atlas 是**合成纹理**（手写像素的 64×64 块状字形），不是 Nandina 的 `GlyphAtlasTexture`
  接 `IRenderDevice`。字体图集上传路径、脏矩形上传与扩容重建都没有经过。
- SDL 键码映射只覆盖 Space / 方向键，且**没有经过实际按键验证**；Nandina 侧的规范键值
  （`widget/key_codes.hpp` 目前直接是 GLFW 码）也未接入。
- 窗口尺寸来自合成器平铺变化，不代表 DPI 或多屏验收；截图里的尺寸与请求值不同。
- 探测里仍然没有**测试目标与图像断言**。`spirv-val` 已经不在此列：它现在在构建图里，
  版本被硬校验，失败会中断构建（上一节有实测）——但「渲染出来的像素对不对」仍然只靠人看截图。
- 只覆盖了现有 SDF 图元的**一个子集**（一个 fragment shader、一种裁剪），没有对齐
  `IRenderDevice` 的 `draw_arc` / `draw_circle_clipped_rounded_rect` / 描边吸附等行为。

**探测方法本身有一个已知偏差**：探测程序不经过 Nandina 的 `IRenderDevice`，也不挂载页面。
在 [布局收敛](layout_convergence_and_text_measurement.md) 修复之前，真实页面 80 ms/帧里有 71 ms 是
CPU 布局，因此**不能**用这个探测对后端做性能结论。后续如果要比较后端，应当跑
`playground/bench/label_bench.cpp` 那种真实页面负载，而不是 SDK 示例。

## 2. Web 侧：两条不同的路径

| 路径 | 能力 | 本项目要验证的内容 |
| --- | --- | --- |
| SDL GPU-backed Renderer（桌面） | SDL 3.4+ `SDL_GPURenderState` 提供自定义 fragment shader | 见 §1；Vulkan 已试，D3D12/Metal 未试 |
| SDL 2D Renderer 的 Emscripten/WebGL 路径 | 只有 SDL 2D 绘制 API + 自定义几何；**不能**沿用桌面的自定义 fragment shader | SDF 是否退化为 CPU 细分几何/覆盖率；是否需要独立 Web device |
| 桌面 GPU renderer + Web 专用 renderer | 各平台保留各自最强的光栅化方式 | 多 `IRenderDevice` 实现的视觉回归、行为一致性与长期维护成本 |

### 2.1 已有两次 Web 构建，但都没有完成浏览器验收

- [probe/cross_platform/web/](../../probe/cross_platform/web/CMakeLists.txt)：用 `SDL_CreateRenderer(window, nullptr)` 的默认 WebGL2 路径，
  `SDL_MAIN_USE_CALLBACKS` 主回调模型，链接参数 `-sMIN/MAX_WEBGL_VERSION=2`、
  `-sALLOW_MEMORY_GROWTH=1`、`-sEXIT_RUNTIME=0`。用本地 emsdk（Emscripten 6.0.10）成功产出
  `html/js/wasm`。内容为 CPU 生成的圆角矩形三角网格 + 16×16 位图 atlas + `SDL_RenderDebugTextFormat`。
  截图 `web/firefox-webgl.png` 停在 Emscripten 加载壳的 "Downloading…" 上、画面是黑的 ——
  **无法据此判定成功或失败**，因为截图发生在资源加载完成之前。
- 另有一个独立的 SDL3 2D Renderer Web 示例（`/workspace/Cpp/playground/hello_emsdk`，不在本仓库内）：
  `SDL_CreateWindowAndRenderer` + `SDL_MAIN_USE_CALLBACKS`，随源码 vendored SDL **3.5.0**，
  用 `emcmake cmake -S . -B dist/web` 配置、`cmake --build` 构建，产物拷到 `public/js/` 由静态服务器打开。
  这条路径在本机浏览器里**实际跑起来了**（画面、输入、resize 都正常），是当前唯一被人工确认过的
  Web 运行记录。它证明的是「SDL 2D Renderer 路径可用」，不是「Nandina 在 Web 上可用」。

  注意示例自身的脚本不一致：`config-wasm.sh` 配置到 `dist/web`，而 `build-wasm.sh` 构建的是 `dist/`
  （`dist/` 里是更早一次配置留下的产物）。复用这套脚本前需要先统一构建目录。

### 2.2 暂定路线（仍未定案）

保留共享的 `IRenderDevice` 契约，Web 使用**单独的**实现接 SDL 2D Renderer / WebGL，
并接受 Web 端 SDF 光栅化与桌面存在可见差异。前置条件：

1. 需要一次**有效的浏览器运行验证**（能读到 canvas 像素或截图在加载完成之后），
   否则「黑屏」永远分不清「资源没加载完」和「没渲染」。
2. WebGL2 是最低要求（当前 shader 依赖 `fwidth` 导数）；WebGL1 不在承诺范围内。
3. 能力差异要写成**代码里的能力查询**（`IRenderDevice` 已有 `supports_alpha_textures` /
   `supports_rounded_rect` 的先例），而不是只写在文档里 —— 否则 SDK 示例跑通会被误读成
   「Nandina 应用已支持」。

## 2.3 真实 IRenderDevice 路径（新增，2026-10-15）

探测记录 §4 把「让探测走一次真实的 `IRenderDevice` 实现」列为 P1 的前置条件。现在有了：
[probe/cross_platform/desktop/sdl_render_device.hpp](../../probe/cross_platform/desktop/sdl_render_device.hpp)
用 SDL 的 2D API 实现了该契约，圆角与圆走 **CPU 三角形扇**（`SDL_RenderGeometry`），
不依赖自定义 shader —— 这同时验证了 §2.2 给 Web 定的暂定路线（CPU 几何/覆盖率 + 独立设备）。

一次 30 帧运行的统计与证据：[sdl-render-device.png](../images/probe/sdl-render-device.png)

```
device stats: frames=30 rects=420 outlines=30 rounded=90 circles=90 lines=600 texts=30
              textures=30 alpha_uploads=2 clips=60 cpu_vertices=9750
device capabilities: alpha_textures=1 rounded_rect=1
```

**已验证**：填充/描边、圆角矩形、圆、线段、由接口默认实现派生的 `draw_arc` 与
`draw_circle_clipped_rounded_rect`、`draw_text`、alpha 图集创建 + 按子矩形更新、纹理区域绘制
与 tint、`set_clip` 裁剪、两个 capability 查询。**未验证**的部分见下表。

**能力差异**（记录在案，不是等价实现）：

| 项 | 生产 raylib 后端 | SDL 2D 设备 | 影响 |
| --- | --- | --- | --- |
| 圆角/圆抗锯齿 | SDF + `fwidth` | 无（硬边缘，分段 6–48） | 视觉差异；Web 路线要接受或用覆盖率 |
| `draw_circle_clipped_rounded_rect` | SDF 解析交集 | 矩形裁剪 + 圆（圆角处方角） | 图标/头像的裁剪会露方角 |
| `draw_arc` | SDF 环 | 接口默认（弦逼近，≤64 段） | 可见折线 |
| `draw_rounded_rect_shadow` | SDF 软阴影 | no-op | 弹层/卡片阴影缺失 |
| 文本 | HarfBuzz + `GlyphAtlasTexture` | `SDL_RenderDebugText` 内置字体 | 只证明文字落在同一套坐标/裁剪里 |
| alpha 图集 | A8，raylib 扩成 RGBA8 | A8 → RGBA8（同样 4×），按行段上传子矩形 | 与生产后端同构的内存代价 |
| 图像文件加载 | 有 | no-op | 需要解码器或 SDL_image |

**构建层的事实（P4 的输入）**：`IRenderDevice` 头文件是 header-only，但它用到的 foundation
值类型有 out-of-line 实现，所以独立设备要么链 `libnandina`、要么编进三个 foundation TU
（探测选了后者）。「设备实现可插拔」在构建层面并不成立，这条要进入 P4 的 host 行为契约。

## 3. Shader 构建链（P1 门槛）

现状：Nandina 的 SDF shader 是**内嵌在 `render/backends/raylib_device.cpp` 里的 GLSL 330 字符串**，
一个 `uMode` 管 6 种图元（fill / outline / segment / clipped_circle / shadow / arc）。
探测里换成了一份 HLSL 源 + glslc 预编译 SPIR-V。P1 需要把下面几条定下来：

- **canonical shader source 与转换链**：选 HLSL 还是 glslc 可编译的 GLSL 子集；
  若要覆盖 D3D12/Metal，需要 SPIR-V/DXIL/MSL 各自的产物与来源前端。
- **host tool 是构建机依赖**：固定版本、固定获取方式，跨编译时执行 host 版本；
  不依赖开发机上"刚好装好"的工具。
- **产物进构建图与最终包**：源 shader 变化必须触发重新生成；安装/打包带上所需二进制；
  运行时不得偷偷要求构建机工具。
- **失败可定位**：报源文件、目标 backend/format、工具版本与诊断输出；CI 对每个承诺的格式
  实际生成并加载。`SDL_shadercross` 的候选提交 `1ff05bec573988a98ef9e0260b4da44f512b8367`
  目前**多后端产物尚未构建验证**。

`SDL_GPURenderState` 的 API floor 是 SDL 3.4.0：对外声明的最低版本不得低于它，
CI/SDK 再固定精确受测版本并在升级时复检。P1 的交付物是「目标 driver 上可用 + 已知限制清单」，
不等同生产就绪。

## 4. 与 D4 交付顺序的对应

| D4 阶段 | 当前状态 |
| --- | --- |
| P0 SDL3 应用级探测 | **大部分**：窗口/事件/帧循环、GPU render state 的自定义 SDF shader、以及一个真实的 `IRenderDevice`（SDL 2D + CPU 几何）都已跑通；仍缺真实字体图集（`GlyphAtlasTexture` 接设备）与键码映射验证 |
| P1 渲染与 shader 管线门槛 | **进行中**：工具链已固定并可一条命令核对（`glslc`/`spirv-val`/SDL/emsdk），SPIR-V 校验进构建图；仍缺 `SDL_shadercross` 的 DXIL/MSL 产物、多格式 CI 生成与加载、以及与现有 SDF 图元的逐项等价性 |
| P2 平台能力与消费者回归 | 未开始 |
| P3 默认后端决策 | 未开始；`raylib` 仍是唯一生产路径 |
| P4 host 行为契约 | 未开始；`NanWindow` 仍同时持有窗口、设备、输入轮询与帧驱动 |

在 P1 之前建议的两件事，第 1 件已完成（§2.3）；第 2 件仍未做：

1. ~~让探测走一次**真实的 `IRenderDevice` 实现**~~ —— 已完成，见 §2.3。
2. 用真实页面负载（`playground/bench/label_bench.cpp`）而不是 SDK 示例做性能与视觉对比。
   下一步是把 `NanSceneTree::draw(device)` 的输出（真实的字体图集与页面）接到这个设备上，
   而不是继续手写图元调用。

## 5. 相关记录

- 探测源码与构建说明：[probe/cross_platform/README.md](../../probe/cross_platform/README.md)
- 工具链核对：`probe/cross_platform/tools/verify_toolchain.sh`
- 探测证据截图：[docs/images/probe/](../images/probe/)（`sdl-gpu-vulkan.png`、`sdl-gpu-vulkan-top.png`）
- 视觉基线：[raylib 基线（2026-10-13）](../images/baselines/raylib-2026-10-13/README.md)
- 布局收敛（为什么不能拿旧数据做后端性能结论）：[布局收敛与文本测量](layout_convergence_and_text_measurement.md)
- D4 的待审阅问题与 host 行为契约：[待决设计草案](reference_draft.md#d4-跨平台优先的窗口与渲染后端路线)
