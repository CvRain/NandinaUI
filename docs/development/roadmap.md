# Zig 重写路线图

> 配套阅读：[架构](architecture.md)。本文回答「应该从哪个方向开始重写」。

## 总原则

严格沿依赖方向自底向上：**先有稳定底座，再往上叠加**。
每一步都以「可编译 + 有单元测试 + `zig build test` 全绿」为完成标准，
避免重演旧版「实验性实现语义模糊」带来的返工成本。

## 推荐起点：reactive（响应式核心）

foundation 几何/颜色已经落地并有测试。下一个该动的不是 render，也不是 widgets，
而是 **reactive**。理由：

1. 它是整个数据驱动 UI 的「心脏」，runtime / widgets / app 全都依赖它。
2. 它纯逻辑、无平台依赖，最容易在纯单元测试下打磨语义，反馈快。
3. 旧版 reactive 语义已被验证过（见 `archive/docs/reactive-strategy.md`），
   重写时有清晰的语义参照，风险低、收益高。

先把内核语义钉死，后续 render/runtime/widgets 才有稳固的依赖。

## 阶段顺序

### M0 —— 骨架与 foundation ✅ 已完成

- 分层目录与 `root.zig` 聚合导出就位。
- foundation 的 Point / Size / Rect / Insets / Color 已实现并测试。
- `zig build run` / `zig build test` 可运行。

### M1 —— reactive 核心闭环 ✅ 基本完成（linkedSignal 待补）

> 命名采用 Angular signal 风格（`signal` / `computed` / `effect`），取代旧版 React 风格的 State/Effect。
>
> 实现采用显式调度图 `Graph`（无全局状态、多实例隔离），Push 失效 + Pull 取值：
> computed 惰性重算、effect 由调度队列执行，菱形依赖 glitch-free，动态依赖自动重追踪。

按以下子步骤推进，每步独立可测：

1. ✅ `Signal(T)`：init / get / peek / set / update / 版本号 / `asReadonly()`。
2. ✅ 依赖追踪上下文：`Graph` 在 effect/computed 执行期记录被读取的 source，自动建立双向边。
3. ✅ `effect(g, ctx, fn)` + `EffectScope`：依赖变化时重跑；`dispose` / `scope.deinit` 自动解绑。
4. ✅ `Computed(T)` / `computed(g, T, ctx, fn)`：惰性派生值，自动追踪依赖。
5. ✅ `batch(g, ctx, fn)`：批量 set 合并为一次 flush（支持嵌套）。
6. 🚧 只读视图 `asReadonly()` 已落地；`linkedSignal`（可写派生）待后续按需补充。

**完成定义**：能构建 signal→computed→effect 依赖图，set 触发可预测、无重复的重算，
并有覆盖依赖追踪、batch、scope 清理、菱形/动态依赖的测试。当前 `zig build test` 全绿（42 个测试）。

### M2 —— render 抽象 + layout 协议 ✅ 已完成

- ✅ `render`：`DrawCommand`（fill_rect / fill_rounded_rect / draw_text / push_clip / pop_clip）、
  `Scene` 命令缓冲（可复用）与 `Backend` 接口（vtable）已落地，并提供可断言命令序列的
  `RecordingBackend` 内存后端。showcase 新增 `render-scene` demo 展示一帧渲染产物。
- ✅ `layout`：`Constraints` 协议层 + 三套纯函数求解器 —— `flex`（盒子模型 column/row/stack，
  对标 Qt Widget）、`flow`（流式折行）、`anchors`（QML anchors 定位）。showcase 新增 `layout-box` demo
  展示三套求解器输出与「布局 → 渲染」链路。
- 两者互相独立，都只依赖 foundation。

### M3 —— theme + text ✅ 已完成

- ✅ `theme`：primitive token（spacing / radius / border / elevation / opacity / typography）、
  语义调色板（ColorRole + light/dark Scheme）、`Theme` 聚合与 resolver 已落地。默认
  主题类似「global.css」可被开发者整体/逐字段覆写。showcase 新增 `theme` demo。
- ✅ `text`：FontMetrics 度量接口 + 等宽估算占位后端，`measure(...)` 把约束宽度 + 换行
  - 行数上限 + 溢出策略（clip / ellipsis 默认 / wrap / scale）作为一等公民，从根上防文字
    溢出组件。裁剪交给 render（未来统一 ClipNode）。showcase 新增 `text-overflow` demo。

### M4 —— runtime ✅ 纯逻辑核心已完成（平台后端待接）

- ✅ Node 树（嵌入式 + vtable，owning 子节点、dirty 两级冒泡、命中测试）、
  事件类型与分发（命中测试 + 冒泡）、`Tree` 的 `relayout → repaint` 主循环骨架已落地，
  产出 `render.Scene`。reactive / render / layout 已被串成一帧闭环。showcase 新增 `runtime-loop` demo。
- 🚧 平台窗口 / 事件源（SDL3 等）作为 backend 后续接入，不污染 runtime 接口。

### M5 —— widgets 🚧 首批已落地

- ✅ primitives：Surface（背景 / 圆角 / 描边 / padding）、Pressable（交互状态机 + 点击回调）、
  ClipNode（通用子树裁剪；裁剪由 runtime 的 child_clip 协议统一 push/pop，不重复造轮子）、
  FocusRing（焦点可视化覆盖层，最简版）。
- ✅ controls：Label（含测量缓存）、Button（状态色）、Panel（圆角 / 边框）、Card（title/description header）。
- ✅ 组件统一接收只读输入（`ReadSignal` 视图），内部状态用 `Signal` 并绑定 `EffectScope`。
- ✅ showcase 新增 `widgets-gallery` demo，演示 reactive→widgets→runtime→render 全链路。
- 🚧 待补：更多 controls（Input / Checkbox / Switch 等）。

### M6 —— app + showcase 🚧

- App / Page / Router / 统一挂载入口与 Ref/Handle/Key 访问机制。
- 把 `src/main.zig` 从烟雾程序演进为真正的多页面 showcase。

### P1 —— 真实后端：窗口 + 字体（平台就位）

> P 系列（Platform）为框架接入真实平台能力，让 UI 跑在真正的窗口中。

- **SDL3 窗口后端**（`runtime/backends/sdl3.zig`，通过 zon package `castholm/SDL`）
  - 窗口创建、resize、关闭生命周期。
  - 事件循环：指针 / 键盘 / 窗口事件 → `runtime.Event` 分发。
  - 渲染集成：`SoftwareBackend` 或 `ThorVG Backend` 的像素呈现到窗口。
- **HarfBuzz + FreeType 字体后端**（替换 `text` 层的 `MonospaceMetrics` 占位）
  - HarfBuzz shaping（纯 C 库，通过 `@cImport` 调用）。
  - FreeType 字形光栅化。
  - 系统字体发现（linux 下 fontconfig 或简单位列枚举）。
- 目标：`zig build run` 打开一个真实窗口，看到可交互的 UI。

### P2 —— C ABI 导出层（多语言互操作的基石）

> ABI 层是 NandinaUI Core 的发布边界，所有语言绑定都基于此。

- 设计 `src/abi/nandina_abi.h`：定义 Core 对外暴露的最小 API 集合。
  - 生命周期：`nandina_init` / `nandina_deinit`。
  - Graph / Signal / Computed / Effect 的基本操作。
  - Node 树的构建与挂载（`nandina_node_create` / `nandina_node_add_child` / `nandina_node_set_bounds`）。
  - 组件工厂函数（`nandina_button_create` / `nandina_surface_create` 等）。
  - 渲染后端选择与帧驱动（`nandina_backend_select` / `nandina_tree_frame`）。
- 实现 `src/abi/nandina_abi.zig`：
  - 所有导出函数为 `export fn` + `extern "C"` 调用约定。
  - 泛型展开为具体类型函数（`nandina_signal_create_i32`、`nandina_signal_create_f32`、`nandina_signal_create_bool` 等）。
  - 不透明句柄用 `*opaque` 或 `*anyopaque`。
  - 不包含任何业务逻辑，只做 Zig ↔ C 类型映射。
- 测试：写一个纯 C 的测试文件，通过 C ABI 调用 NandinaUI 创建组件树并驱动一帧。

### P3 —— C++ Binding 与 ThorVG 后端

> C++ 是第一个也是最重要的绑定目标。同时接入 ThorVG 作为默认渲染后端。

- **C++ Binding**（`src/bindings/cpp/nandina.hpp`）
  - 在 C ABI 之上封装 RAII 类型：`nandina::Graph`、`nandina::Signal<T>`、`nandina::Button` 等。
  - 链式 builder API，与 Zig 版风格对齐。
  - RAII 资源管理：构造时创建、析构时释放，隐藏不透明句柄。
  - 回调支持：`onClick` / `onHover` 等 C++ lambda → C ABI 函数指针的桥接。
- **ThorVG 渲染后端**（`render/backends/thorvg.zig`）
  - C wrapper：`ext/thorvg_bridge.hpp` + `ext/thorvg_bridge.cpp`，把 ThorVG 的基本绘制原语
    （fill rect / fill rounded rect / fill path / stroke / push/pop transform / clip）包装为
    `extern "C"` 函数。
  - Zig 适配：在 `render/Backend` vtable 中调用 bridge 函数，把 `Scene` 命令翻译为 ThorVG 调用。
  - build 集成：`build.zig` 通过 `addCSourceFile` 编译 ThorVG 源码 + C wrapper。
  - 默认启用：`runtime.Tree` 可配置选择 Software / ThorVG 后端。
  - showcase 支持运行时切换后端对比渲染效果。

### P4 —— 应用成熟化

> Core 功能补全 + 多语言互操作 + 文档化。

- **app 层完善**
  - Page 生命周期（create / activate / deactivate / destroy）。
  - Router（路由注册、导航、历史、参数传递）。
  - PageHost（延迟换页、过渡动画预留）。
  - Ref / Handle / Key 组件访问机制。
- **showcase 演进**
  - 多页面 showcase（Overview / Widgets / Layout / Reactive / Theme / Text / About）。
  - 主题切换演示。
  - 后端切换演示（Software ↔ ThorVG）。
- **C ABI 测试覆盖**
  - 完整的 C 端到端测试套件。
  - CI 中同时运行 `zig build test` 与 C 测试。

### P5 —— 更多语言绑定与生态

> 在 C ABI 稳定后，为更多语言提供绑定。

- **Python binding**（`src/bindings/python/`，通过 `ctypes` 或 `ziggy-pydust`）。
- **Lua binding**（`src/bindings/lua/`，通过 Lua C API）。
- **文档化**：
  - 《如何编写一个语言绑定》。
  - 《如何编写一个自定义渲染后端》。
  - 《从 C++ 开始使用 NandinaUI》。
- **包管理**：为各 binding 提供独立的包发布（PyPI / LuaRocks / vcpkg 等）。

## 完成定义

| 里程碑 | 完成标准                                                             |
| ------ | -------------------------------------------------------------------- |
| M0-M5  | `zig build test` 全绿，showcase demo 可运行                          |
| P1     | `zig build run` 打开真实 SDL3 窗口，显示可交互 UI                    |
| P2     | C 测试文件通过 C ABI 调用 Core，`zig build test` 包含 C 测试         |
| P3     | C++ showcase 与 Zig showcase 功能等价，ThorVG 渲染与 Software 可切换 |
| P4     | app 层完整，多页面 showcase 可导航可切换主题                         |
| P5     | 至少 3 种语言绑定可用，有发布包                                      |

## 关键决策（重写时先定）

- **分配器策略**：内核 API 显式接收 `std.mem.Allocator`，不藏全局分配器。
- **Computed 求值**：建议初期 eager（flush 时统一重算，实现简单、易测），
  性能需要时再切 lazy-on-get。
- **线程模型**：初期单线程（UI 线程）；追踪逻辑与调度执行分层，为未来 worker offload 留口。
- **错误处理**：用 Zig error union 取代旧版异常；effect 出错要恢复追踪上下文、不破坏内核一致性。

## 每步的纪律

- 一次只推进一层 / 一个原语，保持 `zig build test` 全绿。
- 新增公共 API 必须同时写 test。
- 不把几何/坐标计算泄漏到 widgets/app —— 由 layout 统一负责。
