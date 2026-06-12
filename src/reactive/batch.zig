//! reactive/batch —— 批量更新
//!
//! `batch(g, ctx, f)` 在一个批处理作用域内执行 `f(ctx)`：作用域内对 signal 的多次
//! `set` 只标记失效、不立即触发 effect；作用域退出（最外层）时统一 flush 一次，
//! 把每个受影响的 effect 合并为单次执行。
//!
//! 支持嵌套：只有最外层 batch 退出时才 flush。即使 `f` 内部触发了同步路径，
//! `endBatch` 也保证在返回前被调用（通过 `defer`），不会卡死在批量模式。

const std = @import("std");
const Graph = @import("graph.zig").Graph;

/// 在批处理作用域内执行 `f(ctx)`。`f` 的签名为 `fn (@TypeOf(ctx)) void`。
pub fn batch(
    g: *Graph,
    ctx: anytype,
    comptime f: fn (@TypeOf(ctx)) void,
) void {
    g.beginBatch();
    defer g.endBatch();
    f(ctx);
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const Signal = @import("signal.zig").Signal;
const effect = @import("effect.zig").effect;

test "batch 内多次 set 合并为一次 effect 执行" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var a = Signal(i32).init(&g, 0);
    defer a.deinit();
    var b = Signal(i32).init(&g, 0);
    defer b.deinit();

    const Ctx = struct { a: *Signal(i32), b: *Signal(i32), sum: i32 = 0, runs: u32 = 0 };
    var ctx = Ctx{ .a = &a, .b = &b };

    _ = try effect(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            c.sum = c.a.get() + c.b.get();
            c.runs += 1;
        }
    }.f);

    try std.testing.expectEqual(@as(u32, 1), ctx.runs);

    batch(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            c.a.set(10);
            c.b.set(20);
            // effect 此刻不应执行
            std.debug.assert(c.runs == 1);
        }
    }.f);

    // 退出 batch 后 effect 只执行一次
    try std.testing.expectEqual(@as(u32, 2), ctx.runs);
    try std.testing.expectEqual(@as(i32, 30), ctx.sum);
}

test "嵌套 batch 只在最外层 flush" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 0);
    defer s.deinit();

    const Ctx = struct { sig: *Signal(i32), runs: u32 = 0 };
    var ctx = Ctx{ .sig = &s };

    _ = try effect(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            _ = c.sig.get();
            c.runs += 1;
        }
    }.f);
    try std.testing.expectEqual(@as(u32, 1), ctx.runs);

    batch(&g, &ctx, struct {
        fn f(c: *Ctx) void {
            c.sig.set(1);
            batch(c.sig.graph, c, struct {
                fn inner(cc: *Ctx) void {
                    cc.sig.set(2);
                }
            }.inner);
            std.debug.assert(c.runs == 1); // 内层 batch 退出未 flush
        }
    }.f);

    try std.testing.expectEqual(@as(u32, 2), ctx.runs);
}
