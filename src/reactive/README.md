# reactive

响应式核心层（Angular signal 风格）。纯逻辑，**仅依赖 foundation**。

## 调度图 Graph

所有响应式节点都依附于一个显式的 `Graph` 实例 —— 没有全局/线程本地状态，
多个 Graph 彼此完全隔离（单元测试天然独立）。Graph 持有依赖追踪上下文、
effect 调度队列、batch 嵌套深度，并通过侵入式链表统一管理节点内存。

```zig
var g = reactive.Graph.init(allocator);
defer g.deinit(); // 统一释放仍存活的 effect / computed
```

## API

| 概念 | 形态 | 说明 |
|------|------|------|
| `Signal(T)` | 可写状态 | `init(g, v)` / `get()` / `peek()` / `set(v)` / `update(fn)` / `asReadonly()` |
| `ReadSignal(T)` | 只读视图 | `get()` / `peek()`，不暴露写权限 |
| `computed(g, T, ctx, fn)` → `*Computed(T)` | 派生值 | 惰性求值，自动追踪依赖；`get()` / `peek()` / `dispose()` |
| `effect(g, ctx, fn)` → `*Effect` | 副作用 | 构造时立即执行一次，依赖变化重跑；`dispose()` |
| `EffectScope` | 副作用容器 | `add(ctx, fn)` / `clear()` / `deinit()` 整体清理 |
| `batch(g, ctx, fn)` | 批处理 | 块内多次 set 合并为一次 flush |

回调通过「ctx 指针 + 顶层函数」表达（Zig 无闭包惯用法）：函数签名为
`fn (@TypeOf(ctx)) T`，在其内部对 signal / computed 的 `get()` 自动建立依赖。

## 用法

```zig
const reactive = @import("NandinaUI").reactive;

var g = reactive.Graph.init(allocator);
defer g.deinit();

var count = reactive.Signal(i32).init(&g, 0);
defer count.deinit();

// 派生值：count 变化时惰性重算
const doubled = try reactive.computed(&g, i32, &count, struct {
    fn f(c: *reactive.Signal(i32)) i32 {
        return c.get() * 2;
    }
}.f);

// 副作用：依赖变化时自动重跑
const Ctx = struct { d: *reactive.Computed(i32) };
var ctx = Ctx{ .d = doubled };
_ = try reactive.effect(&g, &ctx, struct {
    fn f(c: *Ctx) void {
        std.debug.print("doubled = {}\n", .{c.d.get()});
    }
}.f);

count.set(5); // effect 重跑，doubled = 10

// 批处理：多次 set 合并为一次 flush
reactive.batch(&g, &count, struct {
    fn f(c: *reactive.Signal(i32)) void {
        c.set(6);
        c.set(7); // effect 只在 batch 退出后跑一次
    }
}.f);
```

## 设计要点

- **Push 失效 + Pull 取值**：`set` 沿依赖图传播 dirty 标记；computed 读时若 dirty 才重算
  （lazy），effect 由调度队列执行（eager）。读取顺序天然保证「先算依赖再算派生」，
  无需显式拓扑排序，菱形依赖下 effect 只跑一次、无中间态（glitch-free）。
- **动态依赖**：每次执行前清空旧依赖边并重新追踪，分支切换后旧依赖自动失效。
- **生命周期**：`Signal` 由调用方持有（栈/成员），自行 `deinit`；`effect` / `computed`
  堆分配并登记到 Graph，可 `dispose()` 提前释放，否则由 `Graph.deinit` 统一回收。
  组件内部状态用 `Signal` + `EffectScope`，卸载时 `scope.deinit()` 自动清理。
- **单线程**：UI 线程模型；追踪逻辑与调度执行分层，为未来 worker 留口。

## 状态

✅ 已落地，含单元测试（signal / computed / effect / batch / 菱形依赖 / 动态依赖 / 生命周期）。
运行 `zig build test` 验证。设计语义见 [响应式策略](../../docs/development/reactive-strategy.md)。
