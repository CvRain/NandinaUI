//! demos/foundation_geometry —— foundation 几何能力演示
//!
//! 展示 Point / Size / Rect / Insets 的构造与运算。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const f = nandina.foundation;

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;

    const rect = f.Rect.fromXywh(10, 20, 100, 80);
    try out.print("Rect.fromXywh(10, 20, 100, 80)\n", .{});
    try out.print("  width  = {d}\n", .{rect.width()});
    try out.print("  height = {d}\n", .{rect.height()});
    const c = rect.center();
    try out.print("  center = ({d}, {d})\n", .{ c.x, c.y });

    const pad = f.Insets.all(8);
    const inner = pad.applyToRect(rect);
    try out.print("Insets.all(8).applyToRect → LTRB ({d}, {d}, {d}, {d})\n", .{
        inner.left, inner.top, inner.right, inner.bottom,
    });

    const a = f.Rect.fromXywh(0, 0, 100, 100);
    const b = f.Rect.fromXywh(50, 50, 100, 100);
    const hit = a.intersected(b);
    try out.print("intersected → LTRB ({d}, {d}, {d}, {d})\n", .{
        hit.left, hit.top, hit.right, hit.bottom,
    });

    const container = f.Rect.fromXywh(0, 0, 800, 600);
    const small = f.Rect.fromXywh(0, 0, 100, 80);
    const centered = small.centeredIn(container);
    try out.print("centeredIn(800x600) → x={d} y={d}\n", .{ centered.left, centered.top });
}

pub const demo = registry.Demo{
    .name = "foundation-geometry",
    .summary = "几何类型 Point / Size / Rect / Insets 的构造与运算",
    .run = run,
};
