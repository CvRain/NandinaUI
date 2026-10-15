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
| `desktop/` | SDL3 GPU-backed Renderer 探测（Linux/Vulkan）：`main.cpp` + HLSL fragment shader + CMake |
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

## 构建与运行（桌面 / Vulkan）

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

## 已知偏差（不要拿它做框架结论）

探测程序**不经过** Nandina 的 `IRenderDevice`，也不挂载页面；字体图集是合成纹理，
不是 `GlyphAtlasTexture`。所以它证明的是「后端能加载 shader 并画出这些图元」，
**不是**「Nandina 应用已支持该后端」，也不能用来做性能结论 —— 性能对比要跑
`playground/bench/label_bench.cpp` 那种真实页面负载（见探测记录 §1 的说明）。
