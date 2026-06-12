//! demos/color_space —— 多色彩空间转换演示
//!
//! 展示 OKLab hub 转换：同一颜色在 RGB / OKLCH / Lab / LCH 下的表示，以及在感知均匀的
//! OKLCH 空间里生成「亮度阶梯」（最适合做 hover / dark 变体）。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const cs = nandina.foundation.color_space;

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;

    const base = cs.HexRgb.fromRgb(0x3B82F6); // 蓝色
    try out.print("基色 #3B82F6 在各色彩空间的表示：\n", .{});

    const rgb = cs.convert(cs.Rgb, base);
    try out.print("  Rgb    r={d:.3} g={d:.3} b={d:.3}\n", .{ rgb.r, rgb.g, rgb.b });

    const oklch = cs.convert(cs.Oklch, base);
    try out.print("  Oklch  l={d:.3} c={d:.3} h={d:.1}\n", .{ oklch.l, oklch.c, oklch.h });

    const lab = cs.convert(cs.Lab, base);
    try out.print("  Lab    l={d:.2} a={d:.2} b={d:.2}\n", .{ lab.l, lab.a, lab.b });

    const lch = cs.convert(cs.Lch, base);
    try out.print("  Lch    l={d:.2} c={d:.2} h={d:.1}\n", .{ lch.l, lch.c, lch.h });

    // 在 OKLCH 中生成亮度阶梯（保持色相/色度，只调亮度）—— 感知均匀
    try out.print("\nOKLCH 亮度阶梯（保持 h={d:.0} c={d:.3}，只调 l）：\n", .{ oklch.h, oklch.c });
    var i: u8 = 1;
    while (i <= 9) : (i += 1) {
        var step = oklch;
        step.l = @as(f32, @floatFromInt(i)) / 10.0;
        const hex = cs.convert(cs.HexRgb, step);
        try out.print("  l={d:.1} → #{X:0>2}{X:0>2}{X:0>2}\n", .{ step.l, hex.r, hex.g, hex.b });
    }

    // 演示自定义对接：库使用者可定义自己的颜色格式参与 convert
    try out.print("\n可扩展性：任意带 toOklab/fromOklab 的类型都能参与 convert。\n", .{});
}

pub const demo = registry.Demo{
    .name = "color-space",
    .summary = "多色彩空间转换（OKLab hub）+ OKLCH 亮度阶梯",
    .run = run,
};
