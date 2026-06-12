//! demos/render_scene —— render 场景与录制后端演示
//!
//! 构造一帧 UI 的绘制命令（卡片背景 + 圆角 + 裁剪 + 文本），提交给 RecordingBackend，
//! 然后把录制到的命令序列以可读文本「渲染」出来 —— 在没有图形窗口的阶段，
//! 这就是直观感受一帧渲染产物的方式。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const render = nandina.render;
const f = nandina.foundation;

fn printCommand(out: *std.Io.Writer, cmd: render.DrawCommand, indent: usize) !void {
    try out.splatByteAll(' ', indent);
    switch (cmd) {
        .fill_rect => |c| try out.print("FillRect        rect=({d},{d} {d}x{d}) color=#{X:0>8}\n", .{
            c.rect.x(), c.rect.y(), c.rect.width(), c.rect.height(), c.color.toHexRgba(),
        }),
        .fill_rounded_rect => |c| try out.print("FillRoundedRect rect=({d},{d} {d}x{d}) r={d} color=#{X:0>8}\n", .{
            c.rect.x(), c.rect.y(), c.rect.width(), c.rect.height(), c.radius, c.color.toHexRgba(),
        }),
        .draw_text => |c| try out.print("DrawText        \"{s}\" at=({d},{d}) size={d} color=#{X:0>8}\n", .{
            c.text, c.x, c.y, c.font_size, c.color.toHexRgba(),
        }),
        .push_clip => |c| try out.print("PushClip        rect=({d},{d} {d}x{d}) r={d}\n", .{
            c.rect.x(), c.rect.y(), c.rect.width(), c.rect.height(), c.radius,
        }),
        .pop_clip => try out.print("PopClip\n", .{}),
    }
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const a = ctx.allocator;

    // 1) 构造一帧的绘制命令：一张带圆角和文字的卡片。
    var scene = render.Scene.init(a);
    defer scene.deinit();

    const surface = f.Color.fromHexRgb(0xEFF1F5);
    const card = f.Color.white;
    const accent = f.Color.fromHexRgb(0xE64553);
    const ink = f.Color.fromHexRgb(0x4C4F69);

    const card_rect = f.Rect.fromXywh(24, 24, 320, 160);

    try scene.fillRect(f.Rect.fromXywh(0, 0, 800, 480), surface); // 背景
    try scene.fillRoundedRect(card_rect, 14, card); // 卡片
    try scene.pushClip(card_rect, 14); // 限制文字在卡片内
    try scene.drawText(.{ .text = "Hello NandinaUI", .x = 44, .y = 64, .font_size = 26, .color = accent });
    try scene.drawText(.{ .text = "render 层：后端无关的绘制命令", .x = 44, .y = 104, .font_size = 15, .color = ink });
    try scene.popClip();

    try out.print("构造场景：{d} 条绘制命令\n", .{scene.count()});

    // 2) 提交给录制后端。
    var rec = render.RecordingBackend.init(a);
    defer rec.deinit();
    const backend = rec.interface();

    try backend.beginFrame(.{ .width = 800, .height = 480 });
    try backend.submit(&scene);
    try backend.endFrame();

    // 3) 把录制到的「渲染产物」打印出来。
    try out.print("RecordingBackend 录制结果（帧 #{d}，目标 {d}x{d}）：\n", .{
        rec.frame_count, rec.target.width, rec.target.height,
    });
    for (rec.commands.items) |cmd| {
        try printCommand(out, cmd, 2);
    }
}

pub const demo = registry.Demo{
    .name = "render-scene",
    .summary = "Scene + RecordingBackend：构造一帧绘制命令并打印渲染产物",
    .run = run,
};
