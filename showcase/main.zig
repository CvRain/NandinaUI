//! NandinaUI Showcase —— 组件 / 能力演示运行器
//!
//! 两个用途：
//!   1. 展示库已落地的能力（类似组件库 gallery）。
//!   2. 开发时实际跑一下运行效果，直观感受实现后的行为。
//!
//! 用法：
//!   zig build run-showcase                  # 运行全部 demo
//!   zig build run-showcase -- <name>        # 只运行指定 demo
//!   zig build run-showcase -- list          # 列出全部 demo
//!
//! 现状：render / runtime / widgets 尚未落地，demo 以文本输出展示运行效果。
//! 框架（registry + DemoContext）已为可视化画廊预留扩展点，详见 registry.zig。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("registry.zig");

pub fn main(init: std.process.Init) !void {
    const io = init.io;

    var stdout_buffer: [4096]u8 = undefined;
    var stdout_file_writer: std.Io.File.Writer = .init(.stdout(), io, &stdout_buffer);
    const out = &stdout_file_writer.interface;

    // 解析参数：第一个是可执行名，其后第一个非空参数作为 demo 名 / 子命令。
    var arg_it = try std.process.Args.Iterator.initAllocator(init.minimal.args, init.gpa);
    defer arg_it.deinit();
    _ = arg_it.skip(); // 跳过程序名
    // 拷贝出来，避免后续输出期间引用迭代器内部缓冲。
    var target_buf: [256]u8 = undefined;
    var maybe_target: ?[]const u8 = null;
    if (arg_it.next()) |first| {
        const n = @min(first.len, target_buf.len);
        @memcpy(target_buf[0..n], first[0..n]);
        maybe_target = target_buf[0..n];
    }

    const v = nandina.version;
    try out.print("NandinaUI Showcase · v{d}.{d}.{d}\n", .{ v.major, v.minor, v.patch });
    try out.print("====================================\n\n", .{});

    if (maybe_target) |target| {
        if (std.mem.eql(u8, target, "list")) {
            try printList(out);
        } else if (registry.find(target)) |d| {
            try runOne(init.gpa, out, d);
        } else {
            try out.print("未找到 demo: {s}\n\n", .{target});
            try printList(out);
            try out.flush();
            return error.DemoNotFound;
        }
    } else {
        try runAll(init.gpa, out);
    }

    try out.flush();
}

fn printList(out: *std.Io.Writer) !void {
    try out.print("可用 demo（{d} 个）：\n", .{registry.demos.len});
    for (registry.demos) |d| {
        try out.print("  {s: <22} {s}\n", .{ d.name, d.summary });
    }
    try out.print("\n运行单个：zig build run-showcase -- <name>\n", .{});
}

fn runOne(gpa: std.mem.Allocator, out: *std.Io.Writer, d: registry.Demo) !void {
    var graph = nandina.reactive.Graph.init(gpa);
    defer graph.deinit();

    var ctx = registry.DemoContext{ .allocator = gpa, .out = out, .graph = &graph };

    try out.print("▶ {s} — {s}\n", .{ d.name, d.summary });
    try out.print("------------------------------------\n", .{});
    try d.run(&ctx);
    try out.print("\n", .{});
}

fn runAll(gpa: std.mem.Allocator, out: *std.Io.Writer) !void {
    for (registry.demos) |d| {
        try runOne(gpa, out, d);
    }
    try out.print("全部 {d} 个 demo 运行完毕。\n", .{registry.demos.len});
    try out.print("提示：zig build run-showcase -- <name> 可单独运行某个 demo。\n", .{});
}

test "showcase 注册表非空且名称唯一" {
    try std.testing.expect(registry.demos.len > 0);
    for (registry.demos, 0..) |a, i| {
        for (registry.demos, 0..) |b, j| {
            if (i != j) try std.testing.expect(!std.mem.eql(u8, a.name, b.name));
        }
    }
}
