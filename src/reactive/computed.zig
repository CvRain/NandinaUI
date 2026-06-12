//! reactive/computed —— 惰性派生值
//!
//! `computed(g, T, ctx, f)` 创建一个派生值：`f(ctx)` 返回 `T`，执行期间读取的
//! signal / 其它 computed 自动成为依赖。它同时是：
//!   - 一个 `Reactor`：订阅上游依赖，依赖变化时把自己标 dirty 并把失效继续向下游传播；
//!   - 一个 `Source`：可被下游 computed / effect 依赖。
//!
//! 求值策略：**lazy pull**。依赖变化时只标记 dirty 并传播失效，不立即重算；
//! 下一次 `get()` 读到 dirty 才重新执行并缓存。读取顺序天然保证「先算依赖、再算派生」，
//! 无需显式拓扑排序，也避免了中间态（glitch）。
//!
//! 句柄：返回 `*Computed(T)`，`dispose()` 提前释放；未 dispose 的由 `Graph.deinit` 统一释放。

const std = @import("std");
const graph = @import("graph.zig");
const Graph = graph.Graph;

/// 惰性缓存、自动失效的派生值。
pub fn Computed(comptime T: type) type {
    return struct {
        const Self = @This();

        node: graph.Node,
        reactor: graph.Reactor,
        source: graph.Source,
        graph: *Graph,
        cached: T,
        /// 类型擦除的计算函数与其上下文。
        ctx: *anyopaque,
        compute_fn: *const fn (ctx: *anyopaque) T,

        /// 读取派生值；若依赖已变化则重算。在追踪上下文中自动注册依赖。
        pub fn get(self: *Self) T {
            self.graph.track(&self.source);
            if (self.reactor.state == .dirty) {
                self.recompute();
            }
            return self.cached;
        }

        /// 不建立依赖地读取；若 dirty 仍会重算以返回最新值。
        pub fn peek(self: *Self) T {
            if (self.reactor.state == .dirty) {
                self.recompute();
            }
            return self.cached;
        }

        /// 提前释放该 computed：解绑上下游依赖、释放内存。幂等。
        pub fn dispose(self: *Self) void {
            self.graph.disposeNode(&self.node);
        }

        fn recompute(self: *Self) void {
            const g = self.graph;
            g.clearReactorDeps(&self.reactor);
            self.reactor.state = .clean;
            const prev = g.beginRead(&self.reactor);
            defer g.endRead(prev);
            self.cached = self.compute_fn(self.ctx);
        }

        fn fromReactor(reactor: *graph.Reactor) *Self {
            return @alignCast(@fieldParentPtr("reactor", reactor));
        }

        fn fromNode(node: *graph.Node) *Self {
            return @alignCast(@fieldParentPtr("node", node));
        }

        /// Reactor.on_invalidate：上游变化时把失效继续传播给本 computed 的订阅者。
        /// 因 `invalidateReactor` 在置 dirty 前会判重，此处不会重复传播。
        fn onInvalidate(reactor: *graph.Reactor, g: *Graph) void {
            const self = fromReactor(reactor);
            var i: usize = 0;
            while (i < self.source.subs.items.len) : (i += 1) {
                g.invalidateReactor(self.source.subs.items[i]);
            }
        }

        /// computed 是 pull 模型，永不进入 pending 队列，run 不应被调用。
        fn run(reactor: *graph.Reactor, g: *Graph) void {
            _ = reactor;
            _ = g;
            unreachable;
        }

        /// Node.teardown：解绑上下游依赖并释放自身内存。
        fn teardown(node: *graph.Node, g: *Graph) void {
            const self = fromNode(node);
            self.node.disposed = true;
            self.reactor.disposed = true;
            if (!g.tearing_down) {
                g.clearReactorDeps(&self.reactor);
                g.detachSource(&self.source);
            }
            self.reactor.sources.deinit(g.allocator);
            self.source.subs.deinit(g.allocator);
            g.allocator.destroy(self);
        }
    };
}

/// 创建一个 computed。`f` 的签名为 `fn (@TypeOf(ctx)) T`。
///
/// 求值是惰性的：构造时不执行 `f`，首次 `get()` 时才计算并缓存。
/// 失败仅在内存分配失败时发生。
pub fn computed(
    g: *Graph,
    comptime T: type,
    ctx: anytype,
    comptime f: fn (@TypeOf(ctx)) T,
) std.mem.Allocator.Error!*Computed(T) {
    const Ctx = @TypeOf(ctx);
    const Trampoline = struct {
        fn call(erased: *anyopaque) T {
            const typed: Ctx = @ptrCast(@alignCast(erased));
            return f(typed);
        }
    };

    const C = Computed(T);
    const self = try g.allocator.create(C);
    self.* = .{
        .node = .{ .teardown_fn = C.teardown },
        .reactor = .{
            .id = g.nextId(),
            // 初始为 dirty：尚未求值，首次 get 触发计算。
            .state = .dirty,
            .on_invalidate = C.onInvalidate,
            .run = C.run,
        },
        .source = .{ .id = g.nextId() },
        .graph = g,
        .cached = undefined,
        .ctx = @ptrCast(ctx),
        .compute_fn = Trampoline.call,
    };
    g.registerNode(&self.node);
    return self;
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const Signal = @import("signal.zig").Signal;
const effect = @import("effect.zig").effect;

test "computed 基本派生与惰性求值" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var a = Signal(i32).init(&g, 1);
    defer a.deinit();
    var b = Signal(i32).init(&g, 2);
    defer b.deinit();

    const Ctx = struct { a: *Signal(i32), b: *Signal(i32) };
    var ctx = Ctx{ .a = &a, .b = &b };

    const sum = try computed(&g, i32, &ctx, struct {
        fn f(c: *Ctx) i32 {
            return c.a.get() + c.b.get();
        }
    }.f);

    try std.testing.expectEqual(@as(i32, 3), sum.get());
    a.set(10);
    try std.testing.expectEqual(@as(i32, 12), sum.get());
    b.set(20);
    try std.testing.expectEqual(@as(i32, 30), sum.get());
}

test "computed 链式依赖（computed 依赖 computed）" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var n = Signal(i32).init(&g, 2);
    defer n.deinit();

    const doubled = try computed(&g, i32, &n, struct {
        fn f(s: *Signal(i32)) i32 {
            return s.get() * 2;
        }
    }.f);

    const Ctx2 = struct { d: *Computed(i32) };
    var ctx2 = Ctx2{ .d = doubled };
    const plus_one = try computed(&g, i32, &ctx2, struct {
        fn f(c: *Ctx2) i32 {
            return c.d.get() + 1;
        }
    }.f);

    try std.testing.expectEqual(@as(i32, 5), plus_one.get()); // 2*2+1
    n.set(10);
    try std.testing.expectEqual(@as(i32, 21), plus_one.get()); // 10*2+1
}

test "effect 依赖 computed，源变化时 effect 重跑" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var n = Signal(i32).init(&g, 1);
    defer n.deinit();

    const doubled = try computed(&g, i32, &n, struct {
        fn f(s: *Signal(i32)) i32 {
            return s.get() * 2;
        }
    }.f);

    const Ctx = struct { d: *Computed(i32), seen: i32 = 0, runs: u32 = 0 };
    var ctx = Ctx{ .d = doubled };

    _ = try effect(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            c.seen = c.d.get();
            c.runs += 1;
        }
    }.f);

    try std.testing.expectEqual(@as(u32, 1), ctx.runs);
    try std.testing.expectEqual(@as(i32, 2), ctx.seen);

    n.set(5);
    try std.testing.expectEqual(@as(u32, 2), ctx.runs);
    try std.testing.expectEqual(@as(i32, 10), ctx.seen);
}

test "菱形依赖：effect 只执行一次，结果一致（无 glitch）" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    // a → b, a → c, (b,c) → effect。a 变化应只让 effect 跑一次。
    var a = Signal(i32).init(&g, 1);
    defer a.deinit();

    const b = try computed(&g, i32, &a, struct {
        fn f(s: *Signal(i32)) i32 {
            return s.get() + 1;
        }
    }.f);
    const c = try computed(&g, i32, &a, struct {
        fn f(s: *Signal(i32)) i32 {
            return s.get() * 10;
        }
    }.f);

    const Ctx = struct { b: *Computed(i32), c: *Computed(i32), sum: i32 = 0, runs: u32 = 0 };
    var ctx = Ctx{ .b = b, .c = c };

    _ = try effect(&g, &ctx, struct {
        fn f(cc: *Ctx) void {
            cc.sum = cc.b.get() + cc.c.get();
            cc.runs += 1;
        }
    }.f);

    try std.testing.expectEqual(@as(u32, 1), ctx.runs);
    try std.testing.expectEqual(@as(i32, 12), ctx.sum); // (1+1) + (1*10)

    a.set(2);
    try std.testing.expectEqual(@as(u32, 2), ctx.runs); // 只跑一次
    try std.testing.expectEqual(@as(i32, 23), ctx.sum); // (2+1) + (2*10)
}

test "动态依赖：分支切换后旧依赖不再触发" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var cond = Signal(bool).init(&g, true);
    defer cond.deinit();
    var a = Signal(i32).init(&g, 1);
    defer a.deinit();
    var b = Signal(i32).init(&g, 100);
    defer b.deinit();

    const Ctx = struct {
        cond: *Signal(bool),
        a: *Signal(i32),
        b: *Signal(i32),
        seen: i32 = 0,
        runs: u32 = 0,
    };
    var ctx = Ctx{ .cond = &cond, .a = &a, .b = &b };

    _ = try effect(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            c.seen = if (c.cond.get()) c.a.get() else c.b.get();
            c.runs += 1;
        }
    }.f);

    try std.testing.expectEqual(@as(i32, 1), ctx.seen);

    // 当前依赖 a，改 b 不应触发
    b.set(200);
    try std.testing.expectEqual(@as(u32, 1), ctx.runs);

    // 切到 b 分支
    cond.set(false);
    try std.testing.expectEqual(@as(u32, 2), ctx.runs);
    try std.testing.expectEqual(@as(i32, 200), ctx.seen);

    // 现在依赖 b，改 a 不应触发
    a.set(5);
    try std.testing.expectEqual(@as(u32, 2), ctx.runs);

    // 改 b 应触发
    b.set(300);
    try std.testing.expectEqual(@as(u32, 3), ctx.runs);
    try std.testing.expectEqual(@as(i32, 300), ctx.seen);
}

test "computed dispose 后由 Graph.deinit 安全清理" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var n = Signal(i32).init(&g, 1);
    defer n.deinit();

    const c = try computed(&g, i32, &n, struct {
        fn f(s: *Signal(i32)) i32 {
            return s.get() + 1;
        }
    }.f);
    _ = c.get();
    c.dispose();
    n.set(2); // dispose 后不应触及已释放的 computed
}

test "computed 只在被读取时计算（惰性）" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var n = Signal(i32).init(&g, 0);
    defer n.deinit();

    const Ctx = struct { sig: *Signal(i32), computes: u32 = 0 };
    var ctx = Ctx{ .sig = &n };

    const c = try computed(&g, i32, &ctx, struct {
        fn f(cc: *Ctx) i32 {
            cc.computes += 1;
            return cc.sig.get();
        }
    }.f);

    try std.testing.expectEqual(@as(u32, 0), ctx.computes); // 还没读，没算
    _ = c.get();
    try std.testing.expectEqual(@as(u32, 1), ctx.computes);
    _ = c.get(); // clean，复用缓存
    try std.testing.expectEqual(@as(u32, 1), ctx.computes);

    n.set(1); // 失效但不立即重算
    try std.testing.expectEqual(@as(u32, 1), ctx.computes);
    _ = c.get(); // 重算
    try std.testing.expectEqual(@as(u32, 2), ctx.computes);
}
