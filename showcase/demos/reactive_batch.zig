//! demos/reactive_batch —— batch 批量更新合并通知
//!
//! 对比「逐个 set」与「batch 内 set」对 effect 执行次数的影响：
//! batch 内多次写入只触发一次 flush。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const reactive = nandina.reactive;

const Ctx = struct {
    first: *reactive.Signal(i32),
    last: *reactive.Signal(i32),
    out: *std.Io.Writer,
    runs: u32 = 0,
};

fn render(c: *Ctx) void {
    c.runs += 1;
    c.out.print("  [render #{d}] sum = {d}\n", .{ c.runs, c.first.get() + c.last.get() }) catch {};
}

const Mutator = struct {
    first: *reactive.Signal(i32),
    last: *reactive.Signal(i32),
};

fn mutate(m: *Mutator) void {
    m.first.set(100);
    m.last.set(200);
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const g = ctx.graph;

    var first = reactive.Signal(i32).init(g, 0);
    defer first.deinit();
    var last = reactive.Signal(i32).init(g, 0);
    defer last.deinit();

    var view = Ctx{ .first = &first, .last = &last, .out = out };
    const eff = try reactive.effect(g, &view, render);
    defer eff.dispose();

    try out.print("不使用 batch，逐个 set（触发两次 render）:\n", .{});
    first.set(1);
    last.set(2);
    const without_batch = view.runs;

    try out.print("使用 batch，两次 set 合并（只触发一次 render）:\n", .{});
    var mut = Mutator{ .first = &first, .last = &last };
    reactive.batch(g, &mut, mutate);
    const with_batch = view.runs - without_batch;

    try out.print("无 batch 期间 render 次数 = {d}，batch 期间新增 = {d}\n", .{
        without_batch - 1, // 减去初始那次
        with_batch,
    });
}

pub const demo = registry.Demo{
    .name = "reactive-batch",
    .summary = "batch：多次 set 合并为一次 flush，避免重复重算",
    .run = run,
};
