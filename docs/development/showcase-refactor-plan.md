# Showcase 重构与「Core + 双前端」边界落地方案

> 状态：已定稿（第 11 节决策已确认，可据此进入阶段 1 实施）。本文确定 showcase 重构的
> 目标形态、分层边界、ABI 补齐清单、窗口全包入口设计与分阶段迁移步骤。
> 落地后将取代当前直连 Core 的 showcase 实现。

## 1. 背景与问题

当前仓库已经具备 Core + Bindings 架构的骨架：

- **Core**（`src/`）：foundation → reactive/render/layout/theme/text → runtime → widgets → app，
  已基本落地，并提供 `app.authoring` 声明式 DSL。
- **ABI**（`src/abi/`）：`nandina_abi.zig` + `nandina_abi.h`，编译为独立静态库 `libnandina_abi.a`。
- **showcase**：两套程序，均**直接 `@import("NandinaUI")` 调用 Core 内部 API**：
  - `showcase/main.zig` + `registry.zig` + `demos/*`：命令行文本 demo（`zig build run-showcase`）。
  - `showcase/gui.zig` + `gui_pages.zig`：SDL3 可视化画廊（`zig build run`）。

### 核心问题

1. **showcase 不走绑定边界**。作为「前端」代码，它直接消费 Core 内部 API
   （`authoring.surface`、`widgets.Surface.create`、`sdl_backend.Window` 等），
   而不是通过将来 C++ 前端必须经过的 C ABI 边界。
2. **ABI 落后于 Core，且无人消费**。`gui_pages.zig` 用到了 panel / icon / textField /
   checkbox / switch / field / PageHost / Router / 事件回调（`on_click`）等能力，
   而 ABI 层只导出了 surface / label / button / column / card / panel 的一部分，
   没有事件、没有 page/router、没有窗口入口。两套 API 并行，ABI 极易腐化。
3. **窗口与主循环耦合在 Zig 模块**。`gui.zig` 直接使用 `sdl_backend` Zig 模块，
   C++ 前端无法复用，只能各自重写窗口胶水。

## 2. 目标与非目标

### 目标

- 确立 **Core（Zig）+ C ABI + 双前端（Zig / C++）** 的清晰边界。
- showcase 成为**绑定边界的验收用例**：Zig 前端与 C++ 前端各跑一份等价页面，
  证明两条绑定能力对齐。
- ABI 以 showcase 所需能力为清单**补齐**：事件回调、page/router、缺失 widget、窗口全包入口。
- 提供「一行起窗口」的全包入口，让自用开发体验最顺滑。

### 非目标（本轮不做）

- 不实现第二个渲染后端（Vulkan/wgpu）。只保留抽象，SDL3 + ThorVG 是唯一交付实现。
- 不做 Python / Lua 等其他语言绑定。只做 Zig 与 C++ 两个前端。
- 不暴露「开发者自带窗口 / 自带渲染后端」的公开 API（架构上保留可能性，产品上不交付）。
- 不重写 Core 各层的内部实现。

## 3. 架构决策

### 决策一：Slint 模式 —— 默认全包，架构保留抽象

参照业界：

- **Qt Widgets**：全包（QPA 平台层 + 事件循环 + QPainter），方便但 core 极重。
- **Slint**：默认自带窗口（winit）+ 渲染器，但抽象出 `Platform` trait 允许高级用户替换。
  99% 用默认，1% 才自实现。
- **Dear ImGui**：完全不管窗口/渲染，每个用户写 backend 胶水，极灵活但每次都要接。

**结论**：采用 Slint 模式。

- **对外**：默认提供全包 API（一行起 SDL3 窗口 + ThorVG 渲染 + 主循环）。
- **对内**：主循环逻辑只调 `render.Backend` vtable 与 `runtime` 抽象，
  **不把 SDL3 符号渗进 ABI 契约语义**。SDL3 只是「当前唯一后端实现」。

这样既不背 Qt 式的巨型适配负担（只实现一个后端），也不像 ImGui 每次自用都写胶水；
未来需要 Vulkan / 自带窗口时，抽象层已在，再实现第二个即可（YAGNI，但留门）。

### 决策二：C++ 前端只依赖 C ABI

C++ 前端 `#include <nandina_abi.h>` + 链接 `libnandina_abi.a`，**绝不**链接或
`@import` Zig 内部符号。ABI 是唯一契约。这也是其他开发者编写任意语言绑定的范本路径。

### 决策三：两个前端「形似」，都对齐 authoring 声明式风格

- **Zig 前端**：直通到 `app.authoring` 层（即现 `gui_pages.zig` 用的那套 DSL），零开销。
- **C++ 前端**：在 C ABI 之上做 RAII + 链式封装，**在 C++ 里复刻 authoring 的声明式体验**，
  底层调 `nandina_*` 函数。
- 两者语言不同但心智统一（都是声明式 authoring），便于同一套 showcase 页面双语对照。

## 4. 目标目录结构

```
NandinaUI/
├── src/                      # Core（Zig）—— 仅 Core，保持现状
│   ├── foundation/ ... app/  #   分层模块（不变）
│   └── abi/                  # C ABI 导出层
│       ├── nandina_abi.zig
│       └── nandina_abi.h
├── frontend/                 # 【新增】各语言前端绑定
│   ├── zig/                  #   Zig 前端：对 app.authoring 的轻封装/再导出
│   │   └── nandina.zig
│   └── cpp/                  #   C++ 前端：C ABI 之上的 RAII + 链式封装
│       ├── include/nandina/  #     nandina.hpp 等头文件
│       └── src/              #     如需 .cpp 实现
├── showcase/                 # 展示用例（消费前端，而非 Core）
│   ├── zig/                  #   Zig 前端版 showcase
│   ├── cpp/                  #   C++ 前端版 showcase（同一组页面对照）
│   └── shared/               #   页面规格/文案等可共享的纯数据（可选）
└── docs/
```

> 迁移注意：现有 `showcase/demos/*` 命令行文本 demo **不再保留**（其「展示运行效果」的
> 职责由可视化 showcase 接管，「验证逻辑」的职责由单元测试接管）。详见第 8.1 节测试策略。

## 5. ABI 补齐清单

以 `showcase/gui.zig` + `gui_pages.zig` 实际使用的能力为基准，反推 ABI 必须导出的内容。
标注 ✅ 已有 / 🚧 待补。

### 5.1 基础与响应式（基本完备）

- ✅ 生命周期：`nandina_init` / `nandina_deinit` / `nandina_version`
- ✅ Graph：`nandina_graph_create/destroy/batch`
- ✅ Signal：i32 / f32 / bool / color / insets / string 的 create/get/set/destroy
- 🚧 Computed / Effect：当前 ABI 未导出（`nandina_effect_scope_t` 句柄已在头文件预留但无函数）。
  showcase 页面暂未直接用到，**本轮可延后**，但需在头文件标注 TODO。

### 5.2 Widget 工厂（部分缺失）

| Widget    | Core authoring | ABI 现状 | 行动                                   |
| --------- | -------------- | -------- | -------------------------------------- |
| Surface   | ✅             | ✅       | —                                      |
| Label     | ✅             | ✅       | —                                      |
| Button    | ✅             | ✅       | 补 `on_click`                          |
| Column    | ✅             | ✅       | 补 `cross_align`                       |
| Card      | ✅             | ✅       | —                                      |
| Panel     | ✅             | ✅       | —                                      |
| Icon      | ✅             | 🚧       | **新增导出**                           |
| TextField | ✅             | 🚧       | **新增导出**（含 on_change/on_submit） |
| Checkbox  | ✅             | 🚧       | **新增导出**                           |
| Switch    | ✅             | 🚧       | **新增导出**                           |
| Field     | ✅             | 🚧       | **新增导出**                           |

### 5.3 事件 / 回调（缺失，必须补）

showcase 的导航按钮依赖 `on_click`。ABI 已有 `nandina_graph_batch` 的回调范式
（函数指针 `void (*)(void*)` + `void* user_data`），照搬即可。

待补：

- `nandina_button_set_on_click(node, void(*)(void*), void* user_data)`
  （或在 `nandina_button_create` 增加回调参数）。
- TextField 的 `on_change` / `on_submit`（回调签名带 `const char* text`）。
- Checkbox / Switch 的 `on_change`（回调签名带 `bool`）。

> 回调生命周期：`user_data` 所有权归调用方；core 只持有指针，不负责释放。
> C++ 前端用它承载 `std::function` 的 trampoline。

### 5.4 Page / Router / PageHost（缺失，必须补）

showcase 的多页导航骨架。待补：

- `nandina_page_host_create(graph, ...) -> node`
- `nandina_page_host_navigate_to(host, index)`
- 页面 build 回调：`nandina_node_t* (*build)(void* user_data)`（前端在回调里构建页面树）。

> Page 在 Zig 侧是 `build: fn(allocator, *Graph, *SignalOwner) -> *Node`。
> ABI 版需把 allocator/graph/owner 内部化，对外只暴露 `build(user_data) -> node`，
> 内存与 SignalOwner 由 core 管理。

### 5.5 窗口与主循环（全包入口，缺失，必须补）

这是「Slint 默认全包」的落点。待补：

- `nandina_app_t`：不透明应用句柄。
- `nandina_app_create(title, width, height, nandina_app_t** out)`：开 SDL3 窗口 + ThorVG。
- `nandina_app_set_root(app, tree_or_node)`：挂载根。
- `nandina_app_run(app)`：进入阻塞主循环（poll 事件 → dispatch → frame → present）。
- `nandina_app_destroy(app)`。

内部实现：复用现有 `src/runtime/backends/sdl3.zig` 的 `Window` + 主循环逻辑
（即把 `gui.zig::runShowcase` 的循环骨架下沉为 ABI 可调用的内部函数）。
**SDL3 不出现在 `nandina_abi.h` 的语义里**，仅作为内部实现细节。

### 5.6 颜色辅助（便利项）

C++/Zig 前端都需要从 hex 构造颜色。ABI 颜色是 `uint32_t`（RRGGBBAA），
可在前端层用纯计算完成，无需 ABI 函数；但建议在 `nandina_abi.h` 注释清楚字节序约定。

## 6. Zig 前端层设计（`frontend/zig/nandina.zig`）

Zig 前端直通 Core，**不经过 C ABI**（零开销，享受 comptime 与类型安全）。
本质是对 `app.authoring` + `app.Page/Router/PageHost` + `runtime.Tree` +
`sdl_backend.Window` 的**收敛再导出**，给 showcase 一个稳定、收口的前端 API 面：

```zig
// 伪代码示意
pub const nandina = struct {
    pub const authoring = @import("NandinaUI").app.authoring;
    pub const Page = ...;
    pub const Router = ...;
    pub fn run(opts: AppOptions, build_root: fn(...) !*Node) !void { ... }
};
```

目标：showcase 的 Zig 版只 `@import("frontend/zig/nandina.zig")`，
不再散落地直接 `@import("NandinaUI")` 各子模块。

## 7. C++ 前端层设计（`frontend/cpp/include/nandina/nandina.hpp`）

仅依赖 `nandina_abi.h`。提供：

- **RAII 句柄包装**：`Graph` / `Tree` / `Node` / `Signal<T>` / `App`，析构时调对应 `*_destroy`。
- **链式/声明式 builder**：复刻 authoring 风格，例如
  ```cpp
  auto col = nandina::column({.gap = 20})
      .child(nandina::label("NandinaUI", {.color = 0xCDD6F4FF, .font_size = 30}))
      .child(nandina::button("Click", {.on_click = []{ /* ... */ }}));
  ```
- **回调桥接**：用 `void* user_data` 承载 `std::function` 的 trampoline，
  C++ lambda → C 函数指针。
- **app.run**：封装 `nandina_app_*`，一行起窗口。

构建：本仓库内的 C++ showcase 走 `build.zig`（Zig 自带 clang）编译，
链接 `libnandina_abi.a` + 其传递依赖（freetype/harfbuzz/thorvg/SDL3），
保证单一 `zig build` 入口。

> **1.0 之后**：再补一套 CMake 配置（导出 package config + 头文件 + 库），
> 使外部 C++ 项目能通过 `find_package` / `FetchContent` / vcpkg port 直接引入本库，
> 与 Slint、SDL 等库的 C++ 集成体验一致。本轮不做，仅预留。

## 8. showcase 重构

- **页面规格统一**：当前 5 个页面（概述 / Widgets / Layout / Reactive / Theme）的内容
  尽量表达为「声明式结构」，使 Zig 版与 C++ 版结构对齐、便于对照。
- **Zig 版**（`showcase/zig/`）：用 `frontend/zig/nandina.zig` 重写 `gui.zig` + `gui_pages.zig`。
- **C++ 版**（`showcase/cpp/`）：用 `frontend/cpp` 封装重写同一组页面。
- **移除命令行文本 demo**：`showcase/main.zig` + `registry.zig` + `demos/*` 整体删除。
  其原有两个职责被拆走：「展示运行效果」由可视化 showcase 承接，「验证逻辑」由单元测试承接。

构建目标调整：

- `zig build run`：Zig 前端 showcase（替换现 `gui.zig`）。
- `zig build run-cpp`（新增）：C++ 前端 showcase。
- ~~`zig build run-showcase`~~：移除（命令行文本 demo 一并删除）。

### 8.1 测试策略

命令行 demo 删除后，**逻辑验证完全交给单元测试**，对齐 archive 旧版「每个功能都有测试」的做法。

- **Core 单元测试**：保持现状——各模块内嵌 `test` 块，由 `root.zig` 的
  `std.testing.refAllDecls(@This())` 逐层收集，`zig build test` 统一运行。
  补测时优先覆盖本轮新增/改动的 widget 工厂与事件路径。
- **ABI 测试**：扩充 `tests/abi/`（当前仅 `test_abi_basic.c`），
  对新增的事件回调、widget 工厂、page/router、`nandina_app_*` 增加 C 用例，
  验证 ABI 契约在 C 侧可正常 link 与调用。
- **可视化回归**：Zig / C++ 两版 showcase 跑同一组页面，作为「双前端等价」的人工/冒烟验证。

> 旧版（archive）按层划分了 `tests/<layer>/` 目录结构。本仓库 Core 测试目前内嵌于源码模块，
> 暂维持内嵌方式；若后续测试规模变大，可再考虑抽出独立 `tests/<layer>/` 目录。

## 9. 分阶段迁移步骤

> 每个阶段结束都应能 `zig build` 通过并跑通已迁移部分，避免长期处于半破坏状态。

**阶段 0：方案评审**（本文）。确认边界与清单。

**阶段 1：ABI 补齐 —— 事件与缺失 widget**

- 补 `on_click` / TextField·Checkbox·Switch 回调。
- 补 Icon / TextField / Checkbox / Switch / Field 工厂导出。
- 同步更新 `nandina_abi.h` 与 `tests/abi/test_abi_basic.c`。

**阶段 2：ABI 补齐 —— Page/Router + 窗口全包入口**

- 补 PageHost / navigate + build 回调。
- 补 `nandina_app_*`（内部复用 sdl3 backend 主循环）。
- 扩展 `tests/abi` 增加一个最小 C 用例：起窗口 → 一个按钮 → 点击回调。

**阶段 3：Zig 前端层 + Zig showcase**

- 建 `frontend/zig/nandina.zig`，收敛再导出。
- 把 `showcase/gui.zig` + `gui_pages.zig` 迁到 `showcase/zig/`，改为消费前端层。
- 调整 `build.zig`：`run` 指向新路径。

**阶段 4：C++ 前端层 + C++ showcase**

- 建 `frontend/cpp`（RAII + 链式 + 回调桥）。
- 用它在 `showcase/cpp/` 重写同一组页面。
- `build.zig` 增加 `run-cpp` 目标，链接 `libnandina_abi.a`。

**阶段 5：清理与文档**

- 删除命令行文本 demo（`showcase/main.zig` + `registry.zig` + `demos/*`）及其 build 目标。
- 按需补齐新增/改动能力的单元测试（见第 8.1 节）。
- 更新 `README.md`、`docs/development/architecture.md`、`showcase/README.md`。
- 标注 ABI 中暂缓项（Computed/Effect 导出）为 TODO。

## 10. 风险与开放问题

1. **回调生命周期**：C 函数指针 + `user_data` 的所有权与释放时机需在 ABI 文档写死，
   避免 C++ 端 `std::function` 悬垂。建议：core 不持有所有权，前端保证存活到 Node 销毁。
2. **字符串生命周期**：authoring 多处接收 `[]const u8` 切片（如 label/card 文案）。
   ABI 已用零结尾复制策略（见 `StringSignal`），但非 Signal 的一次性文案
   （`nandina_label_create` 的 text）当前直接 `sliceTo` 引用——需确认 widget 是否复制，
   否则 C++ 端临时字符串会悬垂。**需要核对 widgets 的字符串持有策略**。
3. **PageHost 的 build 回调**：Zig 版 build 接收 allocator/graph/owner，ABI 版要把这些
   内部化。需设计「在回调内如何拿到用于创建子节点的上下文」——可能通过线程本地或
   回调参数传入一个 `nandina_build_ctx_t`。
4. **C++ 构建集成**：是否在 `build.zig` 内编 C++（Zig 自带 clang），还是提供 CMake 样例？
   建议先用 `build.zig` 编 C++ 目标，保证单一 `zig build` 入口。
5. **ThorVG / SDL3 静态链接传递依赖**：C++ 目标链接 `libnandina_abi.a` 时需带齐
   freetype/harfbuzz/thorvg/SDL3 及系统库，需在 build 脚本统一处理。

## 11. 现状缺口盘点（字体渲染 / 渲染后端 / 组件库）

> 评审中补充。这些是 showcase 重构（尤其 C++ 前端要渲染文字、要用一致后端）会直接踩到的
> 既有缺口，需要在相应阶段一并处理或显式记为已知限制。

### 11.1 字体渲染：HarfBuzz 链接了但未启用

- 后端类型名为 `HarfBuzzFreeTypeMetrics`，`build.zig` 也通过 pkg-config 链接了 `harfbuzz`，
  但**实际只用 FreeType**：`FT_Get_Char_Index` + 自写的 `ft_glyph.c` 渲染位图。
  文件头注释亦自述「当前只用 FreeType，后续扩展 HarfBuzz shaping」。
- **没有 text shaping**：`text/layout.zig` 的 `measureRun` 逐 codepoint 累加 `advance`，
  换行按 codepoint 硬切。后果：连字、kerning、复杂文字（阿拉伯 / 印度系 / 泰文）、
  emoji、BiDi 均不支持。CJK + 拉丁基本够用。
- **结论**：HarfBuzz 当前是「链接但空转」。要么本轮接入真正的 shaping（成本较高），
  要么暂时移除 harfbuzz 链接、明确记为「FreeType-only，shaping 待办」，避免误导。
  建议：**本轮不接 shaping**，在文档与代码注释标记为已知限制，留作独立里程碑。

### 11.2 渲染后端：默认是 software，且 ThorVG 不画文字

- `src/runtime/backends/sdl3.zig` 的 `Window.backend_kind` **默认 `.software`**（纯 Zig 软件光栅），
  与 README「ThorVG 为默认」的表述**不符**。
- `src/render/backends/thorvg.zig` 的 `submit` 中 `.draw_text => {}` 是**空实现**——
  ThorVG 后端**不渲染文字**。当前唯一能渲染文字的路径是 software backend 的 `GlyphRenderer`。
- **结论**：这是「Slint 式全包默认入口」的硬前提问题。`nandina_app_*` 全包入口到底走哪个后端、
  文字如何渲染，必须先定：
  - 方案 A（稳妥）：全包入口默认 **software backend**（有文字），ThorVG 作为可选项，
    待其补齐 `draw_text`（接 GlyphRenderer 或 ThorVG 原生字体）后再切默认。
  - 方案 B：本轮就给 ThorVG 后端补 `draw_text`（复用现有 GlyphRenderer 位图，或 ThorVG text API），
    使两后端文字一致，再让全包默认走 ThorVG。
  - 同时**修正 README** 关于「默认后端」的表述，使文档与代码一致。

### 11.3 组件库：缺 Row / Stack 组件封装

- `layout.flex` 求解器已支持 column / row / stack 三种轴向，但 widgets 层**只有 `Column`**，
  `authoring` 也只暴露 `column`。横向排列（Row）与层叠（Stack）目前在 GUI 中无法直接使用。
- 现有 `gui_pages.zig` 的「行」效果其实是用嵌套 Column 模拟的，并非真正的 Row 布局。
- **结论**：showcase 要做出像样的画廊布局，建议补 `Row`（必备）与 `Stack`（可选）widget +
  对应 authoring 工厂 + ABI 导出，与 `Column` 对齐。归入阶段 1 的「缺失 widget」清单。

### 11.4 对 ABI 补齐清单与阶段的影响

- 第 5.2 节 widget 工厂清单**追加 `Row`（必备）/ `Stack`（可选）**。
- 第 5.5 节窗口全包入口需明确**默认后端选择**（建议方案 A：默认 software）。
- 字体 shaping 与 ThorVG `draw_text` 列为**独立后续里程碑**，不阻塞本轮 showcase 重构；
  但需在 README / 架构文档同步「默认后端」与「字体能力边界」的真实状态。

## 12. 已敲定决策

- [x] **目录结构**：采用第 4 节方案（`frontend/{zig,cpp}` + `showcase/{zig,cpp}`）。
- [x] **C++ 构建**：本轮走 `build.zig`（单一 `zig build` 入口）；
      1.0 之后再补 CMake 配置（`find_package` / `FetchContent` / vcpkg port）供外部项目集成。
- [x] **Computed/Effect ABI 导出**：本轮延后，基本框架搭完后再完善；头文件标 TODO。
- [x] **命令行文本 demo**：不保留，整体删除。逻辑验证全部交给单元测试（保留并加强，
      对齐 archive「每个功能都有测试」的做法，见第 8.1 节）。
