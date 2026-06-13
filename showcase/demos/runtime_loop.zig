//! demos/runtime_loop —— runtime 一帧闭环演示
//!
//! 构造一棵 Node 树（column 容器 + 两个 box），用 Tree 跑布局 + 绘制一帧，打印每个节点
//! 的最终 bounds 与产出的 Scene 命令；再演示 dirty → frame 增量出帧：改变状态后只在脏时
//! 重新布局/绘制。串起 layout → runtime → render 的完整链路。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const runtime = nandina.runtime;
const render = nandina.render;
const layout = nandina.layout;
const f = nandina.foundation;

const Node = runtime.Node;
const VTable = runtime.VTable;
const Size = f.Size;
const Rect = f.Rect;
const Constraints = layout.Constraints;
const Scene = render.Scene;

// 一个用 flex.column 排列子节点的容器。
const Column = struct {
    node: Node,
    gap: f32,
    const vtable = VTable{ .measure = measure, .layout = layoutImpl, .deinit = deinitImpl };
    fn create(a: std.mem.Allocator, gap: f32) !*Column {
        const self = try a.create(Column);
        self.* = .{ .node = .{ .vtable = &vtable }, .gap = gap };
        return self;
    }
    fn measure(node: *Node, c: Constraints) Size {
        const self: *Column = @fieldParentPtr("node", node);
        var h: f32 = 0;
        var w: f32 = 0;
        for (node.children.items, 0..) |child, i| {
            const cs = child.measure(c.loosen());
            h += cs.height;
            if (i > 0) h += self.gap;
            w = @max(w, cs.width);
        }
        return c.constrain(.{ .width = w, .height = h });
    }
    fn layoutImpl(node: *Node) void {
        const self: *Column = @fieldParentPtr("node", node);
        var specs: [8]layout.flex.ChildSpec = undefined;
        var frames: [8]Rect = undefined;
        const n = node.children.items.len;
        for (node.children.items, 0..) |child, i| specs[i] = .{ .preferred = child.measured };
        layout.flex.solve(.{ .axis = .column, .gap = self.gap, .cross_align = .stretch }, node.bounds, specs[0..n], frames[0..n]);
        for (node.children.items, 0..) |child, i| child.setBounds(frames[i]);
    }
    fn deinitImpl(node: *Node, a: std.mem.Allocator) void {
        a.destroy(@as(*Column, @fieldParentPtr("node", node)));
    }
};

const Box = struct {
    node: Node,
    color: f.Color,
    size: Size,
    label: []const u8,
    paints: u32 = 0,
    const vtable = VTable{ .measure = measure, .paint = paint, .deinit = deinitImpl };
    fn create(a: std.mem.Allocator, label: []const u8, color: f.Color, size: Size) !*Box {
        const self = try a.create(Box);
        self.* = .{ .node = .{ .vtable = &vtable }, .color = color, .size = size, .label = label };
        return self;
    }
    fn measure(node: *Node, c: Constraints) Size {
        const self: *Box = @fieldParentPtr("node", node);
        return c.constrain(self.size);
    }
    fn paint(node: *Node, scene: *Scene) anyerror!void {
        const self: *Box = @fieldParentPtr("node", node);
        self.paints += 1;
        try scene.fillRoundedRect(node.bounds, 6, self.color);
    }
    fn deinitImpl(node: *Node, a: std.mem.Allocator) void {
        a.destroy(@as(*Box, @fieldParentPtr("node", node)));
    }
};

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const a = ctx.allocator;

    var tree = runtime.Tree.init(a);
    defer tree.deinit();

    const col = try Column.create(a, 8);
    const header = try Box.create(a, "header", f.Color.fromHexRgb(0xE64553), .{ .width = 0, .height = 40 });
    const body = try Box.create(a, "body", f.Color.fromHexRgb(0x7C3AED), .{ .width = 0, .height = 80 });
    try col.node.addChild(a, &header.node);
    try col.node.addChild(a, &body.node);

    tree.setRoot(&col.node);
    tree.setViewport(.{ .width = 240, .height = 200 });

    // 第一帧
    try out.print("第一帧（视口 240x200，column gap=8, cross stretch）：\n", .{});
    _ = try tree.frame();
    try printTree(out, &col.node, 0);
    try out.print("  Scene 命令数 = {d}，header.paints={d} body.paints={d}\n", .{
        tree.scene.count(), header.paints, body.paints,
    });

    // 无脏帧
    try out.print("\n再次 frame（无脏，应跳过）：produced={}\n", .{try tree.frame()});
    try out.print("  header.paints={d}（未增加）\n", .{header.paints});

    // 视口变化 → 重新布局出帧
    try out.print("\n视口改为 320x200 → 触发重新布局：\n", .{});
    tree.setViewport(.{ .width = 320, .height = 200 });
    _ = try tree.frame();
    try out.print("  header 宽度 = {d}（cross stretch 跟随视口）\n", .{header.node.bounds.width()});

    // 命中测试 + 事件
    try out.print("\n命中测试 (10, 20) → ", .{});
    const hit = col.node.hitTest(10, 20);
    if (hit == &header.node) {
        try out.print("命中 header\n", .{});
    } else if (hit == &body.node) {
        try out.print("命中 body\n", .{});
    } else {
        try out.print("命中容器 / 无\n", .{});
    }
}

fn printTree(out: *std.Io.Writer, node: *Node, depth: usize) !void {
    try out.splatByteAll(' ', 2 + depth * 2);
    const b = node.bounds;
    try out.print("node bounds=({d:.0},{d:.0} {d:.0}x{d:.0})\n", .{ b.x(), b.y(), b.width(), b.height() });
    for (node.children.items) |child| try printTree(out, child, depth + 1);
}

pub const demo = registry.Demo{
    .name = "runtime-loop",
    .summary = "Node 树 + Tree 一帧闭环（layout→repaint）+ dirty 增量 + 命中测试",
    .run = run,
};
