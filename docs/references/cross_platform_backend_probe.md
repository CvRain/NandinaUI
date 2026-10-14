# 跨平台后端探测：SDL3 GPU 与 Web 现状

> 本文是 D4「跨平台优先的窗口与渲染后端路线」的**探测记录**，不是选型批准，也不是生产就绪声明。
> 它把此前只存在于 git 忽略目录（`playground/sdl_gpu_probe/README.md`）里的结论搬进版本管理，
> 并按 [开发流程](development_loop.md) 的语言区分「已证明」「未验证」和「只是路径选择」。
> 探测代码、SDK 与构建产物仍在 `playground/`（被 `.gitignore:120` 整目录忽略）；**本文是唯一的受管记录**。

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
| 运行时定位 | `SDL_GetBasePath()/assets/sdf.frag.spv`，空或非 4 字节对齐时拒绝 |
| 产物 | 构建树 `build/assets/` + 安装树 `bin/assets/`（不嵌入可执行文件） |

**未验证**（不要把它读成更大范围的支持）：

- atlas 是**合成纹理**（手写像素的 64×64 块状字形），不是 Nandina 的 `GlyphAtlasTexture`
  接 `IRenderDevice`。字体图集上传路径、脏矩形上传与扩容重建都没有经过。
- SDL 键码映射只覆盖 Space / 方向键，且**没有经过实际按键验证**；Nandina 侧的规范键值
  （`widget/key_codes.hpp` 目前直接是 GLFW 码）也未接入。
- 窗口尺寸来自合成器平铺变化，不代表 DPI 或多屏验收；截图里的尺寸与请求值不同。
- 探测里没有测试目标、没有图像断言：README 声称用 `spirv-val` 验证过，但 CMake 与 build.ninja
  里都没有这一步 —— 要么接进构建图，要么不要这么写。
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

- `playground/sdl_gpu_probe/web/`：用 `SDL_CreateRenderer(window, nullptr)` 的默认 WebGL2 路径，
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
| P0 SDL3 应用级探测 | **部分**：窗口/事件/帧循环与一个 SDF shader 已跑通；但没有经过 Nandina 的 `IRenderDevice`、没有真实字体图集、没有键码映射测试 |
| P1 渲染与 shader 管线门槛 | **未开始**：跨后端产物、版本契约、错误诊断、与现有 SDF 图元的等价性都还没有 |
| P2 平台能力与消费者回归 | 未开始 |
| P3 默认后端决策 | 未开始；`raylib` 仍是唯一生产路径 |
| P4 host 行为契约 | 未开始；`NanWindow` 仍同时持有窗口、设备、输入轮询与帧驱动 |

在 P1 之前，建议先补两件事：

1. 让探测走一次**真实的 `IRenderDevice` 实现**（哪怕是只支持子集的），这样测到的才是
   框架的行为，而不是另一个程序的渲染。
2. 用真实页面负载（`playground/bench/label_bench.cpp`）而不是 SDK 示例做性能与视觉对比。

## 5. 相关记录

- 视觉基线：[raylib 基线（2026-10-13）](../images/baselines/raylib-2026-10-13/README.md)
- 布局收敛（为什么不能拿旧数据做后端性能结论）：[布局收敛与文本测量](layout_convergence_and_text_measurement.md)
- D4 的待审阅问题与 host 行为契约：[待决设计草案](reference_draft.md#d4-跨平台优先的窗口与渲染后端路线)
