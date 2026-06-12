//! demos/reactive_counter —— signal + effect 最小响应式闭环
//!
//! 一个计数器：count 是可写 signal，effect 订阅它并在每次变化时「重绘」。
//! 演示 effect 构造时立即执行一次、依赖变化时自动重跑、相等值不触发。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const reactive = nandina.reactive;

/// effect 的上下文：持有要读取的 signal 与输出目标。
const View = struct {
    count: *reactive.Signal(i32),
    out: *std.Io.Writer,
    renders: u32 = 0,
};

fn render(v: *View) void {
    v.renders += 1;
    // effect 内的 get() 自动建立依赖
    v.out.print("  [render #{d}] count = {d}\n", .{ v.renders, v.count.get() }) catch {};
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const g = ctx.graph;

    var count = reactive.Signal(i32).init(g, 0);
    defer count.deinit();

    var view = View{ .count = &count, .out = out };

    try out.print("注册 effect（构造时立即执行一次）：\n", .{});
    const eff = try reactive.effect(g, &view, render);
    defer eff.dispose();

    try out.print("count.set(1):\n", .{});
    count.set(1);

    try out.print("count.set(2):\n", .{});
    count.set(2);

    try out.print("count.set(2) 再次（相等，effect 不重跑）:\n", .{});
    count.set(2);

    try out.print("总渲染次数 = {d}（初始 1 + 两次有效变化 2）\n", .{view.renders});
}

pub const demo = registry.Demo{
    .name = "reactive-counter",
    .summary = "signal + effect：计数器自动「重绘」，相等值不触发",
    .run = run,
};
