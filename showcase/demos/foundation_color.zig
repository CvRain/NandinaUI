//! demos/foundation_color —— foundation 颜色能力演示
//!
//! 展示 Color 的 hex 构造、alpha 调整与线性插值。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const f = nandina.foundation;

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;

    const blue = f.Color.fromHexRgb(0x3B82F6);
    try out.print("Color.fromHexRgb(0x3B82F6)\n", .{});
    try out.print("  rgba(0..1) = ({d:.3}, {d:.3}, {d:.3}, {d:.3})\n", .{
        blue.r, blue.g, blue.b, blue.a,
    });
    try out.print("  toHexRgba  = 0x{X:0>8}\n", .{blue.toHexRgba()});

    const faded = blue.withAlpha(0.5);
    try out.print("withAlpha(0.5).a = {d:.3}\n", .{faded.a});

    try out.print("black.lerp(white, t):\n", .{});
    var i: u8 = 0;
    while (i <= 4) : (i += 1) {
        const t = @as(f32, @floatFromInt(i)) / 4.0;
        const mid = f.Color.black.lerp(f.Color.white, t);
        try out.print("  t={d:.2} → gray {d:.3}\n", .{ t, mid.r });
    }
}

pub const demo = registry.Demo{
    .name = "foundation-color",
    .summary = "颜色 Color 的 hex 构造、alpha 调整与线性插值",
    .run = run,
};
