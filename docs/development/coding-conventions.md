# 编码与 API 规范（Zig）

> 状态：已定稿
> 来源：在旧 C++ `archive/docs/coding-and-api-conventions.md` 基础上，按 Zig 习惯重写。

本文约束 Zig 重写版的命名、模块组织与 API 风格。具体模块边界以 `module-dependency-rules.md` 与源码为准。

## 1. 模块组织

- 每层一个目录 `src/<layer>/`，入口文件 `<layer>.zig` 负责聚合并 `pub const` 再导出该层公共声明。
- 原子能力拆为独立文件（如 `geometry.zig`、`color.zig`），由层入口再导出。
- 库根 `src/root.zig` 聚合所有层，作为 `@import("NandinaUI")` 的入口。

```zig
// src/foundation/foundation.zig
pub const geometry = @import("geometry.zig");
pub const color = @import("color.zig");
pub const Point = geometry.Point; // 常用类型便捷再导出
```

## 2. 命名约定

| 类别 | 规则 | 示例 |
|------|------|------|
| 类型（struct/enum/union） | `PascalCase` | `Rect`、`Color`、`DrawCommand` |
| 函数 / 方法 | `camelCase` | `fromXywh`、`measureText` |
| 变量 / 字段 / 常量 | `snake_case` | `corner_radius`、`max_width` |
| 文件 / 模块 | `snake_case.zig` | `geometry.zig` |
| 编译期类型构造函数 | `PascalCase`，参数 `comptime T` | `Signal(T)`、`Computed(T)` |

不再使用旧版 `Nan` 前缀。类型直接放在其语义所属的模块命名空间下，
例如 `foundation.Rect`、`reactive.Signal(T)`，靠模块路径区分而非前缀。

## 3. 响应式命名（Angular 风格）

本版本响应式 API 采用 Angular signal 命名，取代旧版 React 风格的 `State` / `Effect`：

| 概念 | 本版本命名 | 旧版（已弃用） |
|------|-----------|---------------|
| 可写状态 | `signal(T)` / `Signal(T)` | `State` |
| 派生值 | `computed(T)` / `Computed(T)` | `Computed`（命名沿用，语义对齐 Angular） |
| 副作用 | `effect(fn)` / `Effect` | `Effect`（命名沿用） |
| 可写派生 | `linkedSignal` | — |
| 批处理 | `batch(fn)` | `batch` |

## 4. API 设计原则

### 4.1 分配器显式传入
需要分配的 API 显式接收 `std.mem.Allocator`，不依赖隐藏的全局分配器。

```zig
pub fn init(allocator: std.mem.Allocator) !Node { ... }
```

### 4.2 错误处理
用 Zig error union（`!T`）表达可恢复错误，取代旧版异常。不可恢复的契约违例用 `std.debug.assert`。

### 4.3 Authoring API 约束
面向业务开发者的组件组合 API：

- 使用者描述组件树与布局意图，而不是手写几何公式。
- 不要求使用者手工管理子节点所有权（不暴露裸指针所有权转移为主心智）。
- 挂载后的组件访问通过 `Ref` / `Handle` / `Key` 句柄机制，而不是依赖局部变量持有。

详见 [Authoring 与挂载](authoring-and-mounting.md)。

### 4.4 链式构建
组件 builder 优先返回自身以支持链式：

```zig
column(.{ label("Overview"), button("Run") }).gap(12).padding(16)
```

## 5. 文档与注释

- 公共声明用 `///` doc comment 描述用途、参数与边界。
- 模块顶部用 `//!` 描述该文件职责与依赖方向。
- 每个组件/代码文件的**使用说明**就近放在对应的 `src/<layer>/README.md`。

## 6. 测试

- 每个文件用 `test "..." { ... }` 写就近单元测试。
- 层入口与 `root.zig` 用 `std.testing.refAllDecls(@This())` 逐层引用，保证 `zig build test` 收集全部测试。
- 新增公共 API 必须同时写测试。

## 7. 性能注意

- 避免在每帧绘制路径中分配内存。
- 渲染命令缓冲优先复用 buffer，而不是每帧重建分配。
- 响应式 flush 去重，避免同一 effect 在一个 batch 内重复执行。
