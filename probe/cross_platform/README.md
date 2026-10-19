# 跨平台后端探测（受追踪）

这是 D4 的受追踪探测源码。独立 CMake 工程验证手写图元与 GPU render state；
Meson 的可选 page probe 复用框架和真实 showcase 页面。生产设备已迁入 nandina，
编入 SDL3 时正式宿主默认走 SDL3，探测不维护设备副本。

> D7 混合修复已落地；批次 3 的参数/状态最小实验已通过，生产填充/描边/圆已接入 SDF，其余图元仍走 CPU。
> 本文旧截图与性能数字保持历史记录；全局顺序见[项目进度与下一步](../../docs/references/project_status.md)。

> 结论、已验证/未验证清单、路线选择都在
> [docs/references/cross_platform_backend_probe.md](../../docs/references/cross_platform_backend_probe.md)。
> 本文只讲**怎么构建、依赖什么、固定了哪些版本**。

## 目录

| 路径 | 内容 |
| --- | --- |
| `desktop/` | SDL3 探测，两个构建方式见下：渲染设备是**库里的** `nandina/render/backends/sdl_device.{hpp,cpp}`（D4 期间从本目录移入，避免探测维护生产后端的副本）；`main.cpp`（CMake）驱动手写图元 + GPU render state（自定义 SDF shader）；`page_probe.cpp`（meson）把**真实的 showcase 页面**画到同一个设备上 |
| `web/` | SDL3 2D Renderer / WebGL2 探测（Emscripten）：`main.cpp` + CMake |
| `tools/verify_toolchain.sh` | 打印并校验所有固定版本（一条命令核对工具链） |

本探测的临时构建产物、SDL 源码与 emsdk **都不入库**：构建目录放在已忽略的 `playground/` 下，
依赖用下面的命令现取。生产 D7 的受控 shader 产物交付另按依赖策略定形。

## 固定工具链

### 批次 3：参数与状态最小实验

这条 Meson 路径与旧 CMake 单图元探针独立，使用工程解析的 SDL3 和嵌入的 SPIR-V，
普通构建不需要 glslc、shadercross 或 spirv-val：

```bash
meson setup --reconfigure buildDir -Dsdl_backend=enabled
meson compile -C buildDir sdf_state_probe && \
  meson test -C buildDir sdf-state-probe shader-artifacts --print-errorlogs
./buildDir/tests/sdf_state_probe --require-gpu
```

integration harness 自动运行正常帧、漏 uniform 更新、漏状态恢复三个进程。没有桌面显示时
报告 SKIP；需要 GPU 的专项验收必须使用 `--require-gpu`。它实际验证填充/描边、RGBA 上传纹理
及 SDL 字形纹理，不声称覆盖框架生产字体管线。具体像素预期与 uniform 边界见
[路径契约 §8](../../docs/references/render_device_path_contract.md#8-最小实验记录2026-10-18)。

维护着色器时，显式生成候选头并审阅后替换，再格式化与验证；不在构建时静默再生：

```bash
python3 tools/shader_artifacts.py --emit sdf_state > /tmp/sdf_state_spirv.hpp
python3 tools/shader_artifacts.py --verify-compiler sdf_state
```

性能实验：`sdf_state_probe --require-gpu --benchmark`。每种模式预热 300 帧、采样 1200 帧，
重复 5 次并轮换顺序；每帧 512 次几何提交。分别输出 CPU 提交、present 和等待 GPU 完成的总耗时
p50/p95，以及 state 设置与 uniform 更新次数。采样不读回像素，先前正确性测试的读回不计入采样。
这是隔离帧的微基准，不能当作 Label 页或流水化应用的帧率；普通 debug 构建也不能冒称 release 基线。

### 既有工具链记录

`probe/cross_platform/tools/verify_toolchain.sh` 核对探测工具版本；`tools/verify_shadercross.sh` 单独验证多格式生成。以下为 2026-10-15/16 的已有实测记录：

| 项 | 固定/最低 | 实测 | 作用 |
| --- | --- | --- | --- |
| `glslc` | **2026.4**（配置期硬校验） | 2026.4 | HLSL → Vulkan SPIR-V |
| `spirv-val`（SPIRV-Tools） | **v2026.4**（配置期硬校验） | v2026.4 | SPIR-V 校验，**已接进构建图** |
| SDL3 | **>= 3.4.0**（`SDL_GPURenderState` 的 API floor） | 3.4.18 | 窗口、GPU/2D renderer |
| CMake / Ninja | 3.24 / 任意 | 4.4.4 / 1.13.2 | 构建器 |
| Emscripten | **6.0.10**（Web 路径） | 6.0.10 (d6c521a7) | 只在 Web 时需要 |
| `SDL_shadercross` | 提交 `1ff05bec573988a98ef9e0260b4da44f512b8367` | 2026-10-16 已构建，三格式已生成 | 仅 SPIR-V 通过独立 spirv-val；DXIL/MSL 目标设备加载未验收 |

前四项是**必需**：缺了或版本不符时脚本退出 1、CMake 在配置期就报错。Emscripten 与
shadercross 只在对应路径上需要，缺失时脚本给警告。

## 两份构建的分工

| | CMake（独立） | meson（`-Dsdl_page_probe=true`） |
| --- | --- | --- |
| 目标 | `nandina_sdl_gpu_probe` | `nandina_sdl_page_probe` |
| 依赖 | SDL3 + Nandina **头文件** + 三个 foundation TU | `nandina_dep`（整个 libnandina）+ SDL3 |
| 画什么 | 手写图元调用 + GPU render state 的 SDF shader | **真实 showcase 页面**（`LabelPage`）：真实主题、HarfBuzz 整形、`GlyphAtlasTexture` |
| 回答 | 设备契约能不能**独立**实现（构建层面的可插拔性） | 真实页面能不能在这个后端上**正确渲染**，成本是多少 |

```sh
meson setup buildDir-probe -Dsdl_page_probe=true
meson compile -C buildDir-probe nandina_sdl_page_probe
NANDINA_PROBE_CAPTURE=page.bmp NANDINA_PROBE_FRAMES=60 ./buildDir-probe/probe/cross_platform/desktop/nandina_sdl_page_probe
```

最小宿主是手写的：`NanApplication`（资源/字体/主题/调度）+ `NanRouter` + `NanSceneTree` + 本设备，
**不用 `NanWindow`**，但必须遵循相同帧阶段、dispatcher、动画与 OverlayHost 契约。正式宿主已通过 `IPlatformWindow` 拆分平台职责。

### 真实页面跑通的结果（2026-10-15）

[截图](../../docs/images/probe/sdl-page-label.png) 与 raylib 基线在版式、字体（regular/medium/bold/italic）、
主题色、圆角卡片、裁剪上一致。同一页面的稳态成本：

```
frame cost over 91832 frames: layout=0.005 ms, draw=1.85 ms, total=1.85 ms
device stats: ... textures=234643537（每帧约 1277 次纹理绘制）alpha_uploads=65 clips=275511
```

- `layout=0.005 ms`：稳态 **0 次布局** —— 在 SDL 后端上复现了与 raylib 相同的收敛性质。
- `draw=1.85 ms`：与 raylib 基准（同页面约 2.2 ms）同量级。注意两者**不可直接比质量**：
  本设备的圆角无抗锯齿。
- `alpha_uploads=65`：4 条字体管线（400/500/700/italic）× 2 个 face，加上图集扩容时的重建。

### 同页跨后端像素对照

同一条链路可以做「同一页面、两个后端」的像素对照：raylib 侧用
`NANDINA_BENCH_SCREENSHOT=... ./buildDir/playground/label_bench label`（raylib 自己存 PNG），
SDL 侧用 `NANDINA_PROBE_CAPTURE=... nandina_sdl_page_probe`（帧缓冲读回），再逐像素比对。
2026-10-15 的实测结论与图见探测记录 §2.3.2：mean |Δ| 0.0033、最大 20/255、差异只在边缘抗锯齿。

注意**清屏色必须一致**：页面自己只画卡片与文本，最外层背景由宿主决定。第一版对照把
「宿主清成主题灰」误读成了「后端有到处都有的色差」。

**踩到并修掉的一个真实 bug**（值得记下来）：`SDL_PIXELFORMAT_RGBA8888` 是**打包**格式
（`0xRRGGBBAA`），小端机上的内存字节序是 `A,B,G,R`；而设备产出的缓冲是 `R,G,B,A`。
结果是 alpha 从错误的字节读出，**每个字形都画成一个不透明方块**。改用平台首选的
`SDL_PIXELFORMAT_RGBA32` 即可。同一轮还补上了 `SDL_SCALEMODE_NEAREST`：生产后端对字形图集
用点采样（raylib 的 `TEXTURE_FILTER_POINT`），线性过滤会造成二次软化与邻区渗色。

## 构建与运行（桌面 / Vulkan，CMake 独立版）

```sh
# 依赖（Debian/Ubuntu 系）：libsdl3-dev + shaderc 提供的 glslc + SPIRV-Tools 提供的 spirv-val
probe/cross_platform/tools/verify_toolchain.sh

cmake -S probe/cross_platform/desktop -B playground/probe-build/desktop -G Ninja
cmake --build playground/probe-build/desktop
./playground/probe-build/desktop/nandina_sdl_gpu_probe
```

shader 是构建图的一部分：改 `desktop/sdf.frag.hlsl` 会重新生成 SPIR-V、**重新过一遍
`spirv-val`**，产物落到 `generated/`、拷到可执行文件旁的 `assets/`，`cmake --install` 也会
带上。源文件、目标环境与工具版本都出现在报错信息里。

## 构建与运行（Web / WebGL2）

Web 探测需要一个 SDL 源码树（不追踪）。取固定版本放到 `web/SDL`（或传
`-DNANDINA_SDL_SOURCE_DIR=<path>`）：

```sh
# 用 emsdk 的 emcmake；探测记录里用的是 vendored SDL 3.5.0
source playground/sdl_gpu_probe/emsdk/emsdk_env.sh
git clone --depth 1 --branch release-3.5.0 https://github.com/libsdl-org/SDL probe/cross_platform/web/SDL
emcmake cmake -S probe/cross_platform/web -B playground/probe-build/web -G Ninja
cmake --build playground/probe-build/web
# 产物 html/js/wasm 需要用静态服务器打开（file:// 不行）
```

**Web 路径已完成有效验收（2026-10-15）**：`firefox --headless --screenshot` 会在 2 MB wasm
实例化**之前**截图（画面全黑 + 加载壳），所以别靠截图。探测改成自证：跑满若干帧后自己
`SDL_RenderReadPixels` 读回画布、统计像素，再 `fetch('/probe-report?...')` 回服务器，
用静态服务器的请求日志当证据。实测（Emscripten 6.0.10 / headless Firefox / 软件 WebGL）：

```
stage=renderer-created  detail=opengles2
stage=rendered  frames=92  ink=9556  teal=6220  total=307200
```

失败路径也上报（`init-failed` / `renderer-failed` 带 SDL 错误串），所以黑屏能区分原因。
完整步骤见 [探测记录 §2.1.1](../../docs/references/cross_platform_backend_probe.md#211-有效的浏览器验收让探测自己上报2026-10-15)。

## IRenderDevice 路径（库里的 `render/backends/sdl_device.*`）

这是探测记录 §4 列为 P1 前置条件的那件事：**让探测走一次真实的 `IRenderDevice`**，而不是
另写一个程序画图。设备用 SDL 的 2D API 实现，圆角与圆走 **CPU 三角形扇**（`SDL_RenderGeometry`），
不依赖自定义 shader —— 这同时验证了探测记录 §2 给 Web 定的暂定路线（CPU 几何/覆盖率 + 独立设备）。

运行后打印的统计（30 帧，`NANDINA_PROBE_CAPTURE` 触发的那次）：

```
device stats: frames=30 rects=420 outlines=30 rounded=90 circles=90 lines=600 texts=30
              textures=30 alpha_uploads=2 clips=60 cpu_vertices=9750
device capabilities: alpha_textures=1 rounded_rect=1
```

证据截图：[docs/images/probe/sdl-render-device.png](../../docs/images/probe/sdl-render-device.png)。

**已验证**（同一帧内可见）：填充/描边、圆角矩形、圆、线段、接口默认实现派生的 `draw_arc`
与 `draw_circle_clipped_rounded_rect`、`draw_text`、alpha 图集创建 + **按子矩形更新**
（`update_alpha_texture_region`）、纹理区域绘制与 tint、`set_clip` 裁剪、两个 capability 查询。

**能力差异**（不是等价实现，测到就要写下来；标 ✅ 的已经落成 capability 查询 + 会红的测试）：

| 项 | 生产 raylib 后端 | 本设备 |
| --- | --- | --- |
| 圆角/圆的抗锯齿 | SDF 解析 + `fwidth` | 无（几何边缘是硬的；分段数按半径取 6–48） |
| `draw_circle_clipped_rounded_rect` | SDF 求解析交集 | 退化成「矩形裁剪 + 圆」，圆角处按方角裁；设备返回 `supports_rounded_clip() == false`，由 `RipplePainter` 自己 `set_clip` ✅ |
| `draw_arc` | SDF 环 | 接口默认实现（弦逼近，64 段上限 → 可见折线） |
| `draw_rounded_rect_shadow` | SDF 软阴影 | 未实现；设备返回 `supports_soft_shadows() == false`，由 `ShadowPainter` 退化为硬边圆角阴影 ✅ |
| 文本 | HarfBuzz 整形 + `GlyphAtlasTexture` | `SDL_RenderDebugText` 内置字体（仅证明文字落在同一套坐标/裁剪里） |
| alpha 图集上传 | 1 字节 A8 + raylib 扩展成 RGBA8 | A8 → RGBA8（同样的 4× 纹理字节代价），子矩形按行段上传 |
| 图像文件加载 | 有 | 接口默认 no-op |

**集成事实（P4 的输入）**：`IRenderDevice` 是 header-only 的，但它用到的 foundation 值类型
（`NanRect` / `NanPoint` / `NanSize` / `NanColor`）**有 out-of-line 实现**，所以一个独立的设备实现
必须要么链上 `libnandina`，要么把 `foundation/geometry.cpp`、`foundation/nandina_color.cpp`、
`foundation/nandina_color_space.cpp` 编进来（本探测选了后者，避免把 raylib 拖进探测）。
这说明设备不是纯头文件插件；运行时注入与链接基础类型是两个边界，现有平台接口并不消除后者。

## 已知偏差（不要拿它做框架结论）

设备路径已经经过 `IRenderDevice`，但**仍然不挂载页面**：本文件手写图元调用，而不是把
`NanSceneTree::draw()` 的结果喂给设备；字体图集也是手写的块状字形，不是 `GlyphAtlasTexture`。
所以它证明的是「设备契约在这个后端上可以落地、差异清单是什么」，**不是**「Nandina 应用已
支持该后端」。性能结论同样不成立 —— 那要跑 `playground/bench/label_bench.cpp` 那种真实页面
负载（见探测记录 §1 与 §4）。
