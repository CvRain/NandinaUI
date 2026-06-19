//! NandinaUI —— 库入口 / 冒烟测试
//!
//! 本文件是 NandinaUI 库的可执行入口，用于验证库基本功能。
//! GUI 展示请使用 `showcase/gui.zig`（`zig build run`）。
//! 命令行 demo 请使用 `showcase/main.zig`（`zig build run-showcase`）。

const std = @import("std");
const nandina = @import("NandinaUI");

const f = nandina.foundation;
const Color = f.Color;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    var buf: [1024]u8 = undefined;
    var wtr = std.Io.File.Writer.init(.stdout(), io, &buf);
    const out = &wtr.interface;

    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ nandina.version.major, nandina.version.minor, nandina.version.patch });
    try out.print("分层: foundation, reactive, render, layout, theme, text, runtime, widgets, app\n\n", .{});

    const rect = f.Rect.fromXywh(0, 0, 100, 80);
    try out.print("  foundation: 矩形中心 = ({d:.1}, {d:.1})\n", .{ rect.center().x, rect.center().y });

    var graph = nandina.reactive.Graph.init(init.gpa);
    defer graph.deinit();
    var count = nandina.reactive.Signal(i32).init(&graph, 42);
    defer count.deinit();
    const doubled = try nandina.reactive.computed(&graph, i32, &count, struct {
        fn f(s: *nandina.reactive.Signal(i32)) i32 {
            return s.get() * 2;
        }
    }.f);
    try out.print("  reactive:   signal({d}) -> computed -> {d}\n", .{ count.get(), doubled.get() });

    var scene = nandina.render.Scene.init(init.gpa);
    defer scene.deinit();
    try scene.fillRoundedRect(f.Rect.fromXywh(0, 0, 80, 24), 6, Color.fromHexRgb(0x89B4FA));
    try out.print("  render:     Scene 命令数 = {d}\n", .{scene.count()});

    const child_specs = [_]nandina.layout.flex.ChildSpec{ .{ .preferred = f.Size.init(100, 50) }, .{ .preferred = f.Size.init(80, 60) } };
    var frames: [2]f.Rect = undefined;
    nandina.layout.flex.solve(.{ .axis = .column, .gap = 8 }, f.Rect.fromXywh(0, 0, 200, 200), &child_specs, &frames);
    try out.print("  layout:     Column -> frames[1].y = {d:.0}\n", .{frames[1].y()});

    try out.print("\nGUI 展示: zig build run\n命令行演示: zig build run-showcase\n", .{});
    try out.flush();
}

test "main 模块占位测试" {
    try std.testing.expect(nandina.version.major == 0);
}
