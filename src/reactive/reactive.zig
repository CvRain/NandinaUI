//! reactive —— 响应式核心层（Angular signal 风格）
//!
//! 提供 signal / computed / effect / batch 等响应式原语，基于一个显式的调度图
//! `Graph`（无全局状态、多实例隔离）。设计语义见 docs/development/reactive-strategy.md。
//!
//! 依赖方向：reactive 仅依赖 foundation（如有需要），本身不引入平台依赖。
//!
//! ## 心智模型
//!
//! 所有响应式节点依附于一个 `Graph`。`Signal` 是可写状态源，`Computed` 是惰性派生值，
//! `effect` 是依赖变化时自动重跑的副作用。读取在追踪上下文中自动建立依赖边，
//! 写入沿依赖图传播失效：computed 惰性重算（pull），effect 由调度队列执行（push）。
//!
//! ```zig
//! const reactive = @import("NandinaUI").reactive;
//!
//! var g = reactive.Graph.init(allocator);
//! defer g.deinit();
//!
//! var count = reactive.Signal(i32).init(&g, 0);
//! defer count.deinit();
//!
//! const doubled = try reactive.computed(&g, i32, &count, struct {
//!     fn f(c: *reactive.Signal(i32)) i32 { return c.get() * 2; }
//! }.f);
//!
//! _ = try reactive.effect(&g, doubled, struct {
//!     fn f(d: *reactive.Computed(i32)) void { std.debug.print("{}\n", .{d.get()}); }
//! }.f);
//!
//! count.set(5); // effect 重跑，doubled = 10
//! ```

const std = @import("std");

// ── 子模块 ────────────────────────────────────────────────────────────────────
pub const graph = @import("graph.zig");
pub const signal = @import("signal.zig");
pub const computed_mod = @import("computed.zig");
pub const effect_mod = @import("effect.zig");
pub const batch_mod = @import("batch.zig");

// ── 公共 API 再导出 ─────────────────────────────────────────────────────────────

/// 响应式调度图：所有节点依附其上，析构时统一释放仍存活的节点。
pub const Graph = graph.Graph;

/// 可写响应式状态容器。
pub const Signal = signal.Signal;
/// `Signal(T)` 的只读视图。
pub const ReadSignal = signal.ReadSignal;

/// 惰性派生值类型。
pub const Computed = computed_mod.Computed;
/// 创建一个 computed。
pub const computed = computed_mod.computed;

/// 副作用句柄类型。
pub const Effect = effect_mod.Effect;
/// 一组 effect 的生命周期容器。
pub const EffectScope = effect_mod.EffectScope;
/// 创建并立即执行一个 effect。
pub const effect = effect_mod.effect;

/// 在批处理作用域内执行函数，合并失效通知。
pub const batch = batch_mod.batch;

test {
    std.testing.refAllDecls(@This());
}
