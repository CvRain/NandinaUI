//! demos/text_overflow —— 文本溢出策略演示
//!
//! 同一段长文本在固定宽度内，用四种溢出策略各布局一次，直观对比效果。最后演示
//! 「text → render」：把布局结果配 push_clip/pop_clip 转成绘制命令 —— 体现 archive 的
//! 教训：裁剪由 render 统一负责（未来 ClipNode），文本组件不各自造轮子。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const text = nandina.text;
const render = nandina.render;
const f = nandina.foundation;

fn printLayout(out: *std.Io.Writer, label: []const u8, layout: *const text.TextLayout) !void {
    try out.print("  {s}（{d} 行，{d:.1}x{d:.1}，truncated={}）\n", .{
        label, layout.lines.len, layout.size.width, layout.size.height, layout.truncated,
    });
    for (layout.lines, 0..) |line, i| {
        const tail = if (line.ellipsized) "…" else "";
        try out.print("    line[{d}] \"{s}{s}\"  w={d:.1}\n", .{ i, line.text, tail, line.width });
    }
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const a = ctx.allocator;

    var backend = text.MonospaceMetrics{};
    const metrics = backend.interface();
    const style = text.TextStyle{ .font_size = 12 }; // 每 ASCII 字符 7.2px

    const sample = "NandinaUI 是一个 Zig UI 库";
    const max_width: f32 = 120;

    try out.print("文本: \"{s}\"  约束宽度: {d}px\n\n", .{ sample, max_width });

    // 四种策略
    {
        var l = try text.measure(a, sample, style, .clip, .{ .max_width = max_width }, metrics);
        defer l.deinit();
        try printLayout(out, "clip（硬截断，无省略号）", &l);
    }
    {
        var l = try text.measure(a, sample, style, .ellipsis, .{ .max_width = max_width }, metrics);
        defer l.deinit();
        try printLayout(out, "ellipsis（默认：省略号）", &l);
    }
    {
        var l = try text.measure(a, sample, style, .wrap, .{ .max_width = max_width, .max_lines = 3 }, metrics);
        defer l.deinit();
        try printLayout(out, "wrap（折行，max_lines=3）", &l);
    }
    {
        var l = try text.measure(a, sample, style, .scale, .{ .max_width = max_width }, metrics);
        defer l.deinit();
        try printLayout(out, "scale（缩字号至放下）", &l);
    }

    // text → render：用 wrap 结果生成绘制命令，外层 push_clip 兜底（裁剪归 render）
    try out.print("\ntext → render（wrap 结果 + push_clip/pop_clip 兜底）：\n", .{});
    var l = try text.measure(a, sample, style, .wrap, .{ .max_width = max_width, .max_lines = 2 }, metrics);
    defer l.deinit();

    var scene = render.Scene.init(a);
    defer scene.deinit();

    const box = f.Rect.fromXywh(0, 0, max_width, l.size.height);
    try scene.pushClip(box, 0); // 统一裁剪：保证绘制不越界（未来由 ClipNode 复用）
    var y: f32 = 0;
    const line_h = l.size.height / @as(f32, @floatFromInt(l.lines.len));
    for (l.lines) |line| {
        try scene.drawText(.{
            .text = line.text,
            .x = 0,
            .y = y,
            .font_size = style.font_size,
            .layout_width = max_width,
            .layout_height = line_h,
        });
        y += line_h;
    }
    try scene.popClip();

    for (scene.commands.items) |cmd| {
        switch (cmd) {
            .push_clip => |c| try out.print("    PushClip  ({d:.0}x{d:.0})\n", .{ c.rect.width(), c.rect.height() }),
            .draw_text => |c| try out.print("    DrawText  \"{s}\" at y={d:.1}\n", .{ c.text, c.y }),
            .pop_clip => try out.print("    PopClip\n", .{}),
            else => {},
        }
    }
}

pub const demo = registry.Demo{
    .name = "text-overflow",
    .summary = "文本溢出策略 clip / ellipsis / wrap / scale + text→render 裁剪",
    .run = run,
};
