//! reactive/effect —— 响应式副作用
//!
//! `effect(g, ctx, f)` 注册一个副作用：构造时立即执行一次 `f(ctx)`，
//! 执行期间读取的任意 signal / computed 都会成为它的依赖；任一依赖变化后，
//! effect 被重新调度执行（非 batch 中立即执行，batch 内合并到一次 flush）。
//!
//! 句柄：返回 `*Effect`，可调用 `dispose()` 提前停止追踪并释放；
//! 未显式 dispose 的 effect 会在 `Graph.deinit` 时统一释放。
//!
//! `EffectScope` 把一组 effect 的生命周期捆绑在一起，`deinit` 时整体 dispose —
//! 对应组件卸载时自动清理订阅的场景。
//!
//! 重入保护：effect 在自身执行期间写自己依赖的 signal 不会造成无限递归 ——
//! flush 正在进行时不会重入，重新入队的执行留待当前 flush 循环处理。

const std = @import("std");
const graph = @import("graph.zig");
const Graph = graph.Graph;

/// 副作用句柄。由 `effect()` 创建并返回，堆分配，登记于 Graph 节点链表。
pub const Effect = struct {
    node: graph.Node,
    reactor: graph.Reactor,
    graph: *Graph,
    /// 类型擦除的用户回调与其上下文。
    ctx: *anyopaque,
    call_fn: *const fn (ctx: *anyopaque) void,

    /// 提前停止该 effect：解绑依赖、释放内存。幂等。
    /// dispose 后句柄失效，不可再使用。
    pub fn dispose(self: *Effect) void {
        self.graph.disposeNode(&self.node);
    }

    fn fromReactor(reactor: *graph.Reactor) *Effect {
        return @alignCast(@fieldParentPtr("reactor", reactor));
    }

    fn fromNode(node: *graph.Node) *Effect {
        return @alignCast(@fieldParentPtr("node", node));
    }

    /// 在依赖追踪上下文中执行用户回调，重建依赖边。
    fn execute(self: *Effect) void {
        const g = self.graph;
        g.clearReactorDeps(&self.reactor);
        self.reactor.state = .clean;
        const prev = g.beginRead(&self.reactor);
        defer g.endRead(prev);
        self.call_fn(self.ctx);
    }

    /// Reactor.on_invalidate：依赖变化时把自己排进 pending 队列。
    fn onInvalidate(reactor: *graph.Reactor, g: *Graph) void {
        g.enqueue(reactor);
    }

    /// Reactor.run：被 flush 调度时重新执行。
    fn run(reactor: *graph.Reactor, g: *Graph) void {
        _ = g;
        fromReactor(reactor).execute();
    }

    /// Node.teardown：解绑依赖边并释放自身内存。
    fn teardown(node: *graph.Node, g: *Graph) void {
        const self = fromNode(node);
        self.node.disposed = true;
        self.reactor.disposed = true;
        if (!g.tearing_down) {
            g.clearReactorDeps(&self.reactor);
        }
        self.reactor.sources.deinit(g.allocator);
        g.allocator.destroy(self);
    }
};

/// 创建并立即执行一个 effect。
///
/// `ctx` 是传给回调的上下文指针（通常是 `&some_struct` 或聚合了多个 signal 的指针），
/// `f` 的签名为 `fn (@TypeOf(ctx)) void`。回调内对 signal/computed 的读取建立依赖。
///
/// 失败仅在内存分配失败时发生。
pub fn effect(
    g: *Graph,
    ctx: anytype,
    comptime f: fn (@TypeOf(ctx)) void,
) std.mem.Allocator.Error!*Effect {
    const Ctx = @TypeOf(ctx);
    const Trampoline = struct {
        fn call(erased: *anyopaque) void {
            const typed: Ctx = @ptrCast(@alignCast(erased));
            f(typed);
        }
    };

    const self = try g.allocator.create(Effect);
    self.* = .{
        .node = .{ .teardown_fn = Effect.teardown },
        .reactor = .{
            .id = g.nextId(),
            .on_invalidate = Effect.onInvalidate,
            .run = Effect.run,
        },
        .graph = g,
        .ctx = @ptrCast(ctx),
        .call_fn = Trampoline.call,
    };
    g.registerNode(&self.node);
    self.execute();
    return self;
}

/// 一组 effect 的生命周期容器。`deinit` 时整体 dispose，对应组件卸载清理。
pub const EffectScope = struct {
    graph: *Graph,
    effects: std.ArrayList(*Effect) = .empty,

    pub fn init(g: *Graph) EffectScope {
        return .{ .graph = g };
    }

    /// dispose 作用域内全部 effect 并释放容器。
    pub fn deinit(self: *EffectScope) void {
        self.clear();
        self.effects.deinit(self.graph.allocator);
        self.* = undefined;
    }

    /// 在本作用域内注册一个 effect。
    pub fn add(
        self: *EffectScope,
        ctx: anytype,
        comptime f: fn (@TypeOf(ctx)) void,
    ) std.mem.Allocator.Error!*Effect {
        const e = try effect(self.graph, ctx, f);
        self.effects.append(self.graph.allocator, e) catch |err| {
            e.dispose();
            return err;
        };
        return e;
    }

    /// dispose 作用域内全部 effect（保留容器，可继续 add）。
    pub fn clear(self: *EffectScope) void {
        for (self.effects.items) |e| e.dispose();
        self.effects.clearRetainingCapacity();
    }

    /// 当前持有的 effect 数量。
    pub fn count(self: *const EffectScope) usize {
        return self.effects.items.len;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const Signal = @import("signal.zig").Signal;

test "effect 构造时立即执行一次" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var runs: u32 = 0;
    const e = try effect(&g, &runs, struct {
        fn f(r: *u32) void {
            r.* += 1;
        }
    }.f);
    _ = e;
    try std.testing.expectEqual(@as(u32, 1), runs);
}

test "effect 在依赖变化时重跑" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 0);
    defer s.deinit();

    const Ctx = struct { sig: *Signal(i32), seen: i32 = 0, runs: u32 = 0 };
    var ctx = Ctx{ .sig = &s };

    _ = try effect(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            c.seen = c.sig.get();
            c.runs += 1;
        }
    }.f);

    try std.testing.expectEqual(@as(u32, 1), ctx.runs);
    try std.testing.expectEqual(@as(i32, 0), ctx.seen);

    s.set(5);
    try std.testing.expectEqual(@as(u32, 2), ctx.runs);
    try std.testing.expectEqual(@as(i32, 5), ctx.seen);

    s.set(5); // 相等，不应重跑
    try std.testing.expectEqual(@as(u32, 2), ctx.runs);
}

test "effect dispose 后不再重跑" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 0);
    defer s.deinit();

    const Ctx = struct { sig: *Signal(i32), runs: u32 = 0 };
    var ctx = Ctx{ .sig = &s };

    const e = try effect(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            _ = c.sig.get();
            c.runs += 1;
        }
    }.f);

    try std.testing.expectEqual(@as(u32, 1), ctx.runs);
    e.dispose();
    s.set(1);
    try std.testing.expectEqual(@as(u32, 1), ctx.runs);
}

test "EffectScope 整体清理" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 0);
    defer s.deinit();

    const Ctx = struct { sig: *Signal(i32), runs: u32 = 0 };
    var a = Ctx{ .sig = &s };
    var b = Ctx{ .sig = &s };

    const runner = struct {
        fn f(c: *Ctx) void {
            _ = c.sig.get();
            c.runs += 1;
        }
    }.f;

    var scope = EffectScope.init(&g);
    defer scope.deinit();

    _ = try scope.add(&a, runner);
    _ = try scope.add(&b, runner);
    try std.testing.expectEqual(@as(usize, 2), scope.count());

    s.set(1);
    try std.testing.expectEqual(@as(u32, 2), a.runs);
    try std.testing.expectEqual(@as(u32, 2), b.runs);

    scope.clear();
    s.set(2);
    // clear 后不再重跑
    try std.testing.expectEqual(@as(u32, 2), a.runs);
    try std.testing.expectEqual(@as(u32, 2), b.runs);
}

test "未 dispose 的 effect 由 Graph.deinit 统一释放" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 0);
    defer s.deinit();

    // 故意不持有/不 dispose，验证无内存泄漏（testing.allocator 会检测）。
    _ = try effect(&g, &s, struct {
        fn f(sig: *Signal(i32)) void {
            _ = sig.get();
        }
    }.f);
    s.set(1);
}
