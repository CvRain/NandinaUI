# NandinaUI 架构（Zig 重写）

> 本文是 Zig 重写版的架构基线。旧 C++ 主线已归档在 `archive/`，其设计文档仍作为语义参照。

## 项目定位

NandinaUI 是一个「**Core + Bindings**」架构的 UI 框架：

- **Core**（Zig）—— 框架核心，包含 foundation / reactive / runtime / layout / widgets / app 等层，
  提供完整的 UI 运行时、响应式数据流、布局系统、组件库与应用编排能力。
- **C ABI** —— Core 通过 `extern "C"` 导出稳定的二进制接口，作为多语言互操作的唯一边界。
- **Bindings** —— 在各语言（C++ / Python / Lua 等）中封装 C ABI，提供该语言惯用的 API。

目标：**Zig Core 只写一次，任何语言都能用**。开发者可以选择 Zig、C++ 或其他语言编写 UI，
渲染后端也可替换（ThorVG 为默认，SoftwareBackend 内置作为 fallback）。

项目初衷是创建一个**接近 Web App 的丝滑流畅开发体验**：上手简单、组件丰富、主题易变。
Zig 重写的目标是保留旧版已验证的核心思想（分层边界、响应式模型、design-system-first），
同时用 Zig 的显式分配器、comptime 与简洁所有权模型重建内核。

## 设计参照系

- **组件编写语法**：Flutter 式声明 + 链式组合。
- **响应式与应用结构**：Angular 的 signal / computed / effect、page、router。
- **组件与样式体系**：shadcn 的 primitives 全局源语 + 通用设计 token，功能拆成最小 node，新组件不断复用。
- **底层机制借鉴**：EUI-NED 的文字渲染、slint 的组件挂载、Qt QML 的组件设计方式。
- **多语言互操作**：Zig 作为核心实现语言，C ABI 作为发布边界，各语言绑定层提供惯用 API。

## 设计哲学

- **Design-system-first**：先 token / theme / 语义 API，再具体 widget。
- **组合优于继承**：用原语拼装，避免深继承组件树。
- **语义 API 优先**：接口表达业务语义，不暴露实现细节。
- **现代响应式**：围绕 Angular 风格的 signal / computed / effect 建设数据驱动更新。
- **显式资源管理**：遵循 Zig 习惯，分配器显式传入，所有权清晰、可测。
- **Core + Bindings**：核心用 Zig 实现一次，通过 C ABI 导出，各语言绑定层封装，不重复实现同一套逻辑。
- **可替换渲染后端**：render 层通过 `Backend` vtable 与具体后端解耦；ThorVG 为默认组件渲染后端，
  开发者可切换为软件光栅、Vulkan、wgpu 等实现。

## 分层结构

### Core 层（Zig，`src/`）

源码位于 `src/<layer>/<layer>.zig`，由 `src/root.zig` 聚合再导出。

```
foundation                         （几何 / 颜色 / 基础类型，无上游依赖）
   │
   ├── reactive   （signal / computed / effect / linkedSignal / batch）
   ├── render     （Scene / DrawCommand / Backend 接口）
   ├── layout     （Constraints / measure-layout / Row·Column·Stack）
   ├── theme      （token / palette / Theme resolver）
   ├── text       （Font / measureText / text layout）
   │
   └── runtime    （Node 树 / 事件 / 主循环；依赖 reactive·render·layout）
          │
          widgets （primitives + controls；依赖 reactive·layout·theme·text·runtime）
          │
          app     （Page / Router / 挂载入口；依赖 widgets）
```

### 依赖方向（单向，禁止反向）

`foundation → {reactive, render, layout, theme, text} → runtime → widgets → app`

- `app` 不得反向侵入 `runtime` 内核。
- 任何层都不得依赖比自己更高的层。

### ABI 层（`src/abi/`）

```
  app 及以下各层
       │
       abi       （C ABI 导出层，extern "C" 函数 + 不透明句柄）
          │
          bindings/   （各语言绑定，不在本仓库内编译，作为独立包发布）
```

**ABI 层只导出，不实现**。它是纯胶水代码，把 core 的 Zig API 包装为 `extern "C"` 函数，
不包含任何业务逻辑。C++ / Python / Lua 等语言的绑定库都基于此 ABI 构建。

渲染后端不经过 ABI —— 它们直接在 Zig 侧实现 `render.Backend` vtable，与 Core 同进程编译。

```
render/Backend vtable（Zig 接口）
    │
    ├── render/backends/software.zig   （纯 Zig 软件光栅，内置 fallback）
    ├── render/backends/thorvg.zig     （ThorVG 矢量渲染，默认选项）
    └── render/backends/vulkan.zig     （Vulkan GPU 渲染，未来）
```

### 跨语言界面编写

开发者可通过两种方式编写界面：

1. **Zig 直接编写**：`@import("nandina")`（`frontend/zig/nandina.zig`），使用链式 builder API，零开销。
2. **C++ 编写**：通过 `frontend/cpp/include/nandina/nandina.hpp` 绑定层，调用 Core 的所有能力，
   语法风格与 Zig 版对齐（链式组合、signal/computed/effect、page/router）。

两种方式共享同一套运行时、同一个组件树、同一个渲染后端。

## 各层职责

| 层           | 职责                                              | 当前状态                            |
| ------------ | ------------------------------------------------- | ----------------------------------- |
| `foundation` | 几何（Point/Size/Rect/Insets）、颜色              | ✅ 已落地，有测试                   |
| `reactive`   | signal / computed / effect 与依赖追踪、batch 调度 | ✅ 已落地，有测试                   |
| `render`     | Scene / DrawCommand 中间层与后端接口              | ✅ 已落地，有测试                   |
| `layout`     | constraints / measure-layout 协议与基础容器       | ✅ 已落地，有测试                   |
| `theme`      | token / palette / Theme resolver                  | ✅ 已落地，有测试                   |
| `text`       | 字体、文本测量与布局（溢出策略）                  | ✅ 已落地，有测试                   |
| `runtime`    | Node 树、事件循环、调度边界                       | ✅ 纯逻辑核心已落地（平台后端待接） |
| `widgets`    | primitives 与 controls                            | ✅ 首批已落地，有测试               |
| `app`        | page / router / 挂载入口                          | ✅ 已落地                           |
| `abi`        | C ABI 导出（extern "C" 函数 + 不透明句柄）        | ✅ 已落地                           |
| `bindings`   | 各语言绑定（C++ 等在 `frontend/` 下）             | ✅ Zig/C++ 前端已落地               |

## 稳定边界（优先建立）

1. runtime 与 reactive 的调度边界。
2. render 与具体后端的抽象边界（DrawCommand / Backend 接口）。
3. theme/token 与 widgets 的接口边界。
4. app authoring 与 widgets 的语义 API 边界。
5. layout 语义层、协议层与未来求解层（Yoga 等）的边界。
6. **Core 与 ABI 的导出边界**：哪些 API 值得导出为 C ABI（核心生命周期、node 树操作、signal 读写、
   组件创建与配置），哪些保持 Zig-only（comptime 泛型、内部调度细节）。
7. **ABI 与 Bindings 的映射边界**：C ABI 仅暴露不透明句柄 + 纯函数，语意绑定层在此之上提供
   类型安全、资源安全的惯用 API。

## 关键分界原则

### Core ↔ ABI

- ABI 层不实现任何业务逻辑，只做 Zig 类型 → C 兼容类型的映射。
- 泛型（如 `Signal(T)`）在 ABI 层展开为具体类型的函数（`nandina_signal_create_i32`、`nandina_signal_create_f32` 等），
  或用 `void*` + 类型标记运行时分发。
- ABI 函数命名统一为 `nandina_<module>_<action>[_<variant>]`。

### ABI ↔ Bindings

- Bindings 是 ABI 的「语法糖」层，不绕过 ABI 直接调用 Zig API。
- C++ binding 应遵循 RAII / 智能指针等资源安全管理，隐藏不透明句柄的 alloc/dealloc。
- 其他语言（Python / Lua 等）各自遵循该语言的习惯做法。

## 构建与测试

```sh
zig build run      # 运行 Zig 前端可视化画廊（SDL3 窗口）
zig build run-cpp  # 运行 C++ 前端可视化画廊（同组页面对照）
zig build test     # 运行全部分层单元测试
```

`root.zig` 与每个层文件都用 `std.testing.refAllDecls(@This())` 逐层引用子模块，
确保 `zig build test` 能递归收集所有 `test` 块。
