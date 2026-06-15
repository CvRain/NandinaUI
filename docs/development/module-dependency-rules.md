# 模块依赖规则

> 状态：已定稿
> 来源：改写自旧 C++ 主线 `archive/docs/module-dependency-rules.md`，依赖方向在 Zig 重写中继续有效。

为了保持架构清晰、避免循环依赖、确保层级职责明确，本项目严格遵守以下依赖规则。

## 模块分层与职责

| 模块         | 职责                                                                   |
| ------------ | ---------------------------------------------------------------------- |
| `foundation` | 基础公用类型（几何、颜色），无上游依赖                                 |
| `reactive`   | 响应式核心：signal / computed / effect / batch，纯逻辑                 |
| `render`     | 渲染抽象与后端适配边界（Scene / DrawCommand / Backend）                |
| `layout`     | 约束、容器协议与基础布局原语（Row / Column / Stack）                   |
| `theme`      | 主题与设计令牌（token / palette / resolver）                           |
| `text`       | 字体、文本 shaping、测量与绘制                                         |
| `runtime`    | 平台抽象：窗口、事件、Node 树、主循环                                  |
| `widgets`    | 组件库：primitives 与 controls                                         |
| `app`        | 应用开发层：page、router、authoring、挂载入口                          |
| `abi`        | C ABI 导出层：extern "C" 函数 + 不透明句柄，纯胶水不包含业务逻辑       |
| `bindings`   | 各语言绑定（C++ / Python / Lua 等），在 ABI 之上构建，不在本仓库内编译 |

## 依赖方向规则（单向）

1. **禁止向上依赖**：底层模块严禁依赖高层模块（如 `runtime` 不得引用 `widgets` / `app` 类型）。
2. **禁止循环依赖**：若 A 依赖 B，则 B 及其所有子依赖都不得依赖 A。
3. **reactive 独立性**：`reactive` 是纯逻辑层，不依赖 `render` / `layout`。
4. **runtime 边界**：`runtime` 依赖 `foundation / reactive / render / layout`，但不感知具体业务组件。
5. **text / theme / layout 为中层能力**：可向下依赖 `foundation` 必要子集，不得反向依赖 `widgets` / `app`。
6. **app 为汇聚层**：可依赖 `widgets` 及其下层，用于编排完整应用。
7. **平台库隔离**：第三方平台/渲染库（如窗口、绘制后端）应隐藏在 render / runtime 的实现内部，不污染上层接口。
8. **abi 只导出不实现**：`abi` 层可依赖 `app` 及其下层（因它需要访问整个 Core 来导出），
   但 `abi` 的任何上层（bindings / 应用）不得反向依赖 Zig Core 的内部模块 —— 只能通过 C ABI 调用。
9. **bindings 不在本仓库编译**：各语言绑定是独立项目，只依赖 `nandina_abi.h` 和 `libnandina` 的
   动态/静态库。仓库内只保留绑定源码作为参考实现和测试。
10. **渲染后端不经过 ABI**：`render/backends/*` 直接在 Zig 侧实现 `Backend` vtable，
    与 Core 同进程编译。后端切换在 Zig 侧完成，C ABI 层只暴露 `nandina_backend_select(id)`。

## 物理实现约束（Zig）

- 每层入口为 `src/<layer>/<layer>.zig`，通过 `pub const` 再导出公共声明。
- 跨层引用只通过 `@import("NandinaUI")` 聚合根或显式相对路径引入对方入口文件，不深入对方内部子文件。
- 平台相关代码集中在各自层的 backend 子模块，用接口（vtable / 比较函数指针）解耦。
- ABI 层 `src/abi/nandina_abi.zig` 不纳入 `root.zig` 的聚合导出（它不是面向 Zig 使用者的公共 API），
  而是由 `build.zig` 单独编译为共享库或静态库，同时产出 `nandina_abi.h` 头文件。
- ABI 导出函数必须加上 `export` 关键字，并使用 `extern "C"` 调用约定。
- 渲染后端放在 `render/backends/<name>.zig`，通过编译时选择或运行时注册加入 `Backend` 接口。

## 违反约束的后果

任何违反依赖方向的提交都被视为技术债，必须在进入下一里程碑前重构。
