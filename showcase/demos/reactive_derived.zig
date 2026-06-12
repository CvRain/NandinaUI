//! demos/reactive_derived —— computed 派生值与链式/菱形依赖
//!
//! 演示 computed 惰性求值、链式依赖（computed 依赖 computed）以及菱形依赖下
//! effect 只执行一次（glitch-free）。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const reactive = nandina.reactive;

const PriceCtx = struct { qty: *reactive.Signal(i32), unit: *reactive.Signal(i32) };
fn subtotal(c: *PriceCtx) i32 {
    return c.qty.get() * c.unit.get();
}

const TaxCtx = struct { sub: *reactive.Computed(i32) };
fn withTax(c: *TaxCtx) i32 {
    // 10% 税，整数演示
    return @divTrunc(c.sub.get() * 110, 100);
}

const ReportCtx = struct {
    sub: *reactive.Computed(i32),
    total: *reactive.Computed(i32),
    out: *std.Io.Writer,
    runs: u32 = 0,
};
fn report(c: *ReportCtx) void {
    c.runs += 1;
    c.out.print("  [report #{d}] subtotal={d} total(+10%)={d}\n", .{
        c.runs, c.sub.get(), c.total.get(),
    }) catch {};
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const g = ctx.graph;

    var qty = reactive.Signal(i32).init(g, 2);
    defer qty.deinit();
    var unit = reactive.Signal(i32).init(g, 50);
    defer unit.deinit();

    var price_ctx = PriceCtx{ .qty = &qty, .unit = &unit };
    const sub = try reactive.computed(g, i32, &price_ctx, subtotal);
    defer sub.dispose();

    var tax_ctx = TaxCtx{ .sub = sub };
    const total = try reactive.computed(g, i32, &tax_ctx, withTax);
    defer total.dispose();

    // report 同时依赖 sub 与 total，二者都源自 qty/unit → 菱形依赖
    var report_ctx = ReportCtx{ .sub = sub, .total = total, .out = out };
    const eff = try reactive.effect(g, &report_ctx, report);
    defer eff.dispose();

    try out.print("初始 qty=2 unit=50\n", .{});

    try out.print("unit.set(80)（菱形依赖，report 应只跑一次）:\n", .{});
    unit.set(80);

    try out.print("qty.set(3):\n", .{});
    qty.set(3);

    try out.print("report 总运行次数 = {d}（初始 1 + 两次变化 2）\n", .{report_ctx.runs});
}

pub const demo = registry.Demo{
    .name = "reactive-derived",
    .summary = "computed 链式 + 菱形依赖：派生值自动重算，effect 无重复执行",
    .run = run,
};
