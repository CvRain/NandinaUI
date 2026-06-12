//! NandinaUI showcase —— 可运行的验证载体
//!
//! 当前阶段仅打印库版本与已就位的分层模块，作为骨架烟雾测试。
//! 随着 runtime / widgets / app 落地，这里会演进为真正的展示应用。
const std = @import("std");
const nandina = @import("NandinaUI");

pub fn main(init: std.process.Init) !void {
    const io = init.io;

    var stdout_buffer: [1024]u8 = undefined;
    var stdout_file_writer: std.Io.File.Writer = .init(.stdout(), io, &stdout_buffer);
    const out = &stdout_file_writer.interface;

    const v = nandina.version;
    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ v.major, v.minor, v.patch });
    try out.print("layers: foundation, reactive, render, layout, theme, text, runtime, widgets, app\n", .{});

    // 演示 foundation 几何能力已可用。
    const rect = nandina.foundation.Rect.fromXywh(0, 0, 100, 80);
    const c = rect.center();
    try out.print("demo rect center = ({d}, {d})\n", .{ c.x, c.y });

    // 演示 reactive 最小闭环：signal → computed → effect。
    var graph = nandina.reactive.Graph.init(init.gpa);
    defer graph.deinit();

    var count = nandina.reactive.Signal(i32).init(&graph, 1);
    defer count.deinit();

    const doubled = try nandina.reactive.computed(&graph, i32, &count, struct {
        fn f(s: *nandina.reactive.Signal(i32)) i32 {
            return s.get() * 2;
        }
    }.f);

    try out.print("reactive: count={d} doubled={d}\n", .{ count.get(), doubled.get() });
    count.set(21);
    try out.print("reactive: count={d} doubled={d}\n", .{ count.get(), doubled.get() });

    try out.flush();
}

test "main 模块占位测试" {
    try std.testing.expect(nandina.version.major == 0);
}
