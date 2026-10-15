# 跨平台后端探测（受追踪）

这是 D4「跨平台优先的窗口与渲染后端路线」P0/P1 的**探测源码**。它**不属于** Nandina 的
meson 构建，也不改动 raylib 生产路径：探测是独立的 CMake 工程，用来回答「某个后端能不能
承载 Nandina 需要的图元与 shader」这类问题。

> 结论、已验证/未验证清单、路线选择都在
> [docs/references/cross_platform_backend_probe.md](../../docs/references/cross_platform_backend_probe.md)。
> 本文只讲**怎么构建、依赖什么、固定了哪些版本**。

## 目录

| 路径 | 内容 |
| --- | --- |
| `desktop/` | SDL3 探测，两个构建方式见下：`sdl_render_device.{hpp,cpp}` 是用 SDL 2D API 实现的 `IRenderDevice`；`main.cpp`（CMake）驱动手写图元 + GPU render state（自定义 SDF shader）；`page_probe.cpp`（meson）把**真实的 showcase 页面**画到同一个设备上 |
| `web/` | SDL3 2D Renderer / WebGL2 探测（Emscripten）：`main.cpp` + CMake |
| `tools/verify_toolchain.sh` | 打印并校验所有固定版本（一条命令核对工具链） |

构建产物、SDL 源码与 emsdk **都不入库**：构建目录放在已忽略的 `playground/` 下，
依赖用下面的命令现取。

## 固定工具链

`probe/cross_platform/tools/verify_toolchain.sh` 是唯一的核对入口。2026-10-15 在本机的实测值：

| 项 | 固定/最低 | 实测 | 作用 |
| --- | --- | --- | --- |
| `glslc` | **2026.4**（配置期硬校验） | 2026.4 | HLSL → Vulkan SPIR-V |
| `spirv-val`（SPIRV-Tools） | **v2026.4**（配置期硬校验） | v2026.4 | SPIR-V 校验，**已接进构建图** |
| SDL3 | **>= 3.4.0**（`SDL_GPURenderState` 的 API floor） | 3.4.18 | 窗口、GPU/2D renderer |
| CMake / Ninja | 3.24 / 任意 | 4.4.4 / 1.13.2 | 构建器 |
| Emscripten | **6.0.10**（Web 路径） | 6.0.10 (d6c521a7) | 只在 Web 时需要 |
| `SDL_shadercross` | 提交 `1ff05bec…`（候选，**未验证**） | 未安装 | P1 的多后端产物（DXIL/MSL） |

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
**不用 `NanWindow`**（它同时持有窗口、设备、输入与帧驱动，是 P4 的待办；宿主需要的越少，P4 的契约越清楚）。

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

**Web 路径尚未完成浏览器验收**：上一次构建成功但截图停在加载壳的 "Downloading…" 上，
画面是黑的，无法据此判定成败。需要一次能读到 canvas 像素（或加载完成之后再截图）的验证。

## IRenderDevice 路径（`sdl_render_device.*`）

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

**能力差异**（不是等价实现，测到就要写下来）：

| 项 | 生产 raylib 后端 | 本设备 |
| --- | --- | --- |
| 圆角/圆的抗锯齿 | SDF 解析 + `fwidth` | 无（几何边缘是硬的；分段数按半径取 6–48） |
| `draw_circle_clipped_rounded_rect` | SDF 求解析交集 | 退化成「矩形裁剪 + 圆」，圆角处按方角裁 |
| `draw_arc` | SDF 环 | 接口默认实现（弦逼近，64 段上限 → 可见折线） |
| `draw_rounded_rect_shadow` | SDF 软阴影 | 接口默认 no-op（未实现） |
| 文本 | HarfBuzz 整形 + `GlyphAtlasTexture` | `SDL_RenderDebugText` 内置字体（仅证明文字落在同一套坐标/裁剪里） |
| alpha 图集上传 | 1 字节 A8 + raylib 扩展成 RGBA8 | A8 → RGBA8（同样的 4× 纹理字节代价），子矩形按行段上传 |
| 图像文件加载 | 有 | 接口默认 no-op |

**集成事实（P4 的输入）**：`IRenderDevice` 是 header-only 的，但它用到的 foundation 值类型
（`NanRect` / `NanPoint` / `NanSize` / `NanColor`）**有 out-of-line 实现**，所以一个独立的设备实现
必须要么链上 `libnandina`，要么把 `foundation/geometry.cpp`、`foundation/nandina_color.cpp`、
`foundation/nandina_color_space.cpp` 编进来（本探测选了后者，避免把 raylib 拖进探测）。
「设备可插拔」因此在构建层面并不成立 —— 这一点要在 P4 的 host 契约里解决。

## 已知偏差（不要拿它做框架结论）

设备路径已经经过 `IRenderDevice`，但**仍然不挂载页面**：本文件手写图元调用，而不是把
`NanSceneTree::draw()` 的结果喂给设备；字体图集也是手写的块状字形，不是 `GlyphAtlasTexture`。
所以它证明的是「设备契约在这个后端上可以落地、差异清单是什么」，**不是**「Nandina 应用已
支持该后端」。性能结论同样不成立 —— 那要跑 `playground/bench/label_bench.cpp` 那种真实页面
负载（见探测记录 §1 与 §4）。
