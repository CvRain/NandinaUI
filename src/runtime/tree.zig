//! runtime/tree —— UI 树容器与主循环
//!
//! `Tree` 拥有根 `Node`，并把 reactive / render / layout 串成一帧的调度闭环：
//!
//!   set signal → markLayoutDirty/markPaintDirty 冒泡 → frame() →
//!     relayout（measure 自底向上 + layout 自顶向下）→ repaint（产出 Scene）
//!
//! 本文件是纯逻辑：不持有平台窗口，绘制产物是 `render.Scene`，可交给任意 Backend
//! （测试用 RecordingBackend）。平台窗口 / 事件源后续作为 backend 接入。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const render = @import("../render/render.zig");
const layout = @import("../layout/layout.zig");
const node_mod = @import("node.zig");
const event = @import("event.zig");

const Allocator = std.mem.Allocator;
const Rect = foundation.Rect;
const Size = foundation.Size;
const Constraints = layout.Constraints;
const Scene = render.Scene;
const Node = node_mod.Node;
const Event = event.Event;
const EventResult = node_mod.EventResult;

/// UI 树：拥有根节点，驱动一帧的布局与绘制。
pub const Tree = struct {
    allocator: Allocator,
    root: ?*Node = null,
    /// 视口尺寸（根节点的布局约束来源）。
    viewport: Size = .{},
    /// 复用的绘制命令缓冲。
    scene: Scene,

    pub fn init(allocator: Allocator) Tree {
        return .{ .allocator = allocator, .scene = Scene.init(allocator) };
    }

    /// 释放树（递归释放根子树）与场景缓冲。
    pub fn deinit(self: *Tree) void {
        if (self.root) |r| r.deinitTree(self.allocator);
        self.scene.deinit();
        self.* = undefined;
    }

    /// 挂载根节点（接管所有权）。若已有根，先释放旧根。
    pub fn setRoot(self: *Tree, root: *Node) void {
        if (self.root) |old| old.deinitTree(self.allocator);
        self.root = root;
        root.parent = null;
        root.markLayoutDirty();
    }

    /// 设置视口尺寸；变化时触发重新布局。
    pub fn setViewport(self: *Tree, size: Size) void {
        if (self.viewport.eql(size)) return;
        self.viewport = size;
        if (self.root) |r| r.markLayoutDirty();
    }

    /// 是否有待处理的布局 / 绘制工作。
    pub fn isDirty(self: *const Tree) bool {
        const r = self.root orelse return false;
        return r.layout_dirty or r.paint_dirty;
    }

    /// 执行一帧：若布局脏则重新布局，然后（若绘制脏）重建场景命令。
    /// 返回是否产生了新的一帧（有则 `self.scene` 已更新）。
    pub fn frame(self: *Tree) anyerror!bool {
        const root = self.root orelse return false;
        if (!root.layout_dirty and !root.paint_dirty) return false;

        if (root.layout_dirty) self.relayout(root);

        // 重建场景：先清空缓冲（保留容量），再递归绘制。
        self.scene.clear();
        try paintNode(root, &self.scene);
        clearPaintDirty(root);
        return true;
    }

    /// 重新布局整棵树：根用视口尺寸做紧约束，measure 后 layout。
    fn relayout(self: *Tree, root: *Node) void {
        const c = Constraints.tight(self.viewport);
        _ = root.measure(c);
        root.setBounds(Rect.fromXywh(0, 0, self.viewport.width, self.viewport.height));
        layoutSubtree(root);
        clearLayoutDirty(root);
    }

    /// 自顶向下：父节点 layout 分配子 bounds，再递归到每个子节点。
    fn layoutSubtree(node: *Node) void {
        node.layout();
        for (node.children.items) |child| {
            layoutSubtree(child);
        }
    }

    /// 深度优先绘制：先画自身，再画子节点（子节点在上层）。
    fn paintNode(node: *Node, scene: *Scene) anyerror!void {
        if (!node.visible) return;
        try node.paint(scene);
        for (node.children.items) |child| {
            try paintNode(child, scene);
        }
    }

    fn clearLayoutDirty(node: *Node) void {
        node.layout_dirty = false;
        for (node.children.items) |child| clearLayoutDirty(child);
    }

    fn clearPaintDirty(node: *Node) void {
        node.paint_dirty = false;
        for (node.children.items) |child| clearPaintDirty(child);
    }

    // ── 事件分发 ──────────────────────────────────────────────────────────────

    /// 分发一个事件。带坐标的指针事件先命中测试找到目标节点，再从该节点向上冒泡，
    /// 直到被消费或到达根。返回事件是否被消费。
    pub fn dispatchEvent(self: *Tree, ev: Event) EventResult {
        const root = self.root orelse return .ignored;

        if (ev.pointerPos()) |pos| {
            const target = root.hitTest(pos.x, pos.y) orelse return .ignored;
            return bubble(target, ev);
        }
        // 非指针事件：直接交给根（后续可引入焦点节点路由）。
        return root.vtable.handle_event(root, ev);
    }

    /// 从 target 向上冒泡，直到某节点消费事件。
    fn bubble(target: *Node, ev: Event) EventResult {
        var cur: ?*Node = target;
        while (cur) |n| {
            if (n.vtable.handle_event(n, ev) == .consumed) return .consumed;
            cur = n.parent;
        }
        return .ignored;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试用节点
// ─────────────────────────────────────────────────────────────────────────────

const VTable = node_mod.VTable;

/// 一个用 layout.flex 排列子节点的容器节点。
const Column = struct {
    node: Node,
    gap: f32,

    const vtable = VTable{
        .measure = measure,
        .layout = layoutImpl,
        .deinit = deinitImpl,
    };

    fn create(a: Allocator, gap: f32) !*Column {
        const self = try a.create(Column);
        self.* = .{ .node = .{ .vtable = &vtable }, .gap = gap };
        return self;
    }

    fn measure(node: *Node, constraints: Constraints) Size {
        // 子节点先 measure（松约束），列容器占满约束宽、累加子高。
        var total_h: f32 = 0;
        var max_w: f32 = 0;
        for (node.children.items, 0..) |child, i| {
            const cs = child.measure(constraints.loosen());
            total_h += cs.height;
            if (i > 0) total_h += @as(*Column, @fieldParentPtr("node", node)).gap;
            if (cs.width > max_w) max_w = cs.width;
        }
        return constraints.constrain(.{ .width = max_w, .height = total_h });
    }

    fn layoutImpl(node: *Node) void {
        const self: *Column = @fieldParentPtr("node", node);
        var specs_buf: [16]layout.flex.ChildSpec = undefined;
        var frames_buf: [16]Rect = undefined;
        const n = node.children.items.len;
        for (node.children.items, 0..) |child, i| {
            specs_buf[i] = .{ .preferred = child.measured };
        }
        layout.flex.solve(
            .{ .axis = .column, .gap = self.gap, .cross_align = .stretch },
            node.bounds,
            specs_buf[0..n],
            frames_buf[0..n],
        );
        for (node.children.items, 0..) |child, i| {
            child.setBounds(frames_buf[i]);
        }
    }

    fn deinitImpl(node: *Node, a: Allocator) void {
        a.destroy(@as(*Column, @fieldParentPtr("node", node)));
    }
};

const Box = struct {
    node: Node,
    color: foundation.Color,
    size: Size,
    paints: u32 = 0,

    const vtable = VTable{
        .measure = measure,
        .paint = paint,
        .deinit = deinitImpl,
    };

    fn create(a: Allocator, color: foundation.Color, size: Size) !*Box {
        const self = try a.create(Box);
        self.* = .{ .node = .{ .vtable = &vtable }, .color = color, .size = size };
        return self;
    }

    fn measure(node: *Node, constraints: Constraints) Size {
        const self: *Box = @fieldParentPtr("node", node);
        return constraints.constrain(self.size);
    }

    fn paint(node: *Node, scene: *Scene) anyerror!void {
        const self: *Box = @fieldParentPtr("node", node);
        self.paints += 1;
        try scene.fillRect(node.bounds, self.color);
    }

    fn deinitImpl(node: *Node, a: Allocator) void {
        a.destroy(@as(*Box, @fieldParentPtr("node", node)));
    }
};

const testing = std.testing;

test "frame：布局 + 绘制一帧" {
    const a = testing.allocator;
    var tree = Tree.init(a);
    defer tree.deinit();

    const col = try Column.create(a, 10);
    const b1 = try Box.create(a, foundation.Color.white, .{ .width = 100, .height = 30 });
    const b2 = try Box.create(a, foundation.Color.black, .{ .width = 100, .height = 40 });
    try col.node.addChild(a, &b1.node);
    try col.node.addChild(a, &b2.node);

    tree.setRoot(&col.node);
    tree.setViewport(.{ .width = 200, .height = 200 });

    try testing.expect(tree.isDirty());
    const produced = try tree.frame();
    try testing.expect(produced);

    // 两个 box 各画一次 → 2 条 fill_rect
    try testing.expectEqual(@as(usize, 2), tree.scene.count());
    try testing.expectEqual(@as(u32, 1), b1.paints);

    // b1 在顶部，b2 在 b1 下方 gap=10
    try testing.expectEqual(@as(f32, 0), b1.node.bounds.top);
    try testing.expectEqual(@as(f32, 30), b1.node.bounds.height());
    try testing.expectEqual(@as(f32, 40), b2.node.bounds.top); // 30 + gap 10
    // cross stretch → 宽度撑满视口
    try testing.expectEqual(@as(f32, 200), b1.node.bounds.width());
}

test "frame：无脏不重复出帧" {
    const a = testing.allocator;
    var tree = Tree.init(a);
    defer tree.deinit();

    const box = try Box.create(a, foundation.Color.white, .{ .width = 50, .height = 50 });
    tree.setRoot(&box.node);
    tree.setViewport(.{ .width = 100, .height = 100 });

    try testing.expect(try tree.frame()); // 第一帧
    try testing.expectEqual(@as(u32, 1), box.paints);
    try testing.expect(!tree.isDirty());
    try testing.expect(!(try tree.frame())); // 无脏，不出帧
    try testing.expectEqual(@as(u32, 1), box.paints); // 未重绘

    // 标记脏后重新出帧
    box.node.markPaintDirty();
    try testing.expect(try tree.frame());
    try testing.expectEqual(@as(u32, 2), box.paints);
}

test "setViewport 变化触发重新布局" {
    const a = testing.allocator;
    var tree = Tree.init(a);
    defer tree.deinit();

    const box = try Box.create(a, foundation.Color.white, .{ .width = 1000, .height = 1000 });
    tree.setRoot(&box.node);
    tree.setViewport(.{ .width = 100, .height = 100 });
    _ = try tree.frame();
    try testing.expectEqual(@as(f32, 100), box.node.bounds.width()); // tight 到视口

    tree.setViewport(.{ .width = 300, .height = 200 });
    try testing.expect(tree.isDirty());
    _ = try tree.frame();
    try testing.expectEqual(@as(f32, 300), box.node.bounds.width());
}

test "事件命中测试 + 冒泡" {
    const a = testing.allocator;
    var tree = Tree.init(a);
    defer tree.deinit();

    const col = try Column.create(a, 0);
    const box = try Box.create(a, foundation.Color.white, .{ .width = 100, .height = 50 });
    try col.node.addChild(a, &box.node);
    tree.setRoot(&col.node);
    tree.setViewport(.{ .width = 100, .height = 100 });
    _ = try tree.frame();

    // box 不处理事件（默认 ignored），事件冒泡到 col（也 ignored）→ ignored
    const ev: Event = .{ .pointer_down = .{ .button = .left, .x = 10, .y = 10 } };
    try testing.expectEqual(EventResult.ignored, tree.dispatchEvent(ev));

    // 点击视口外 → 无命中
    const outside: Event = .{ .pointer_down = .{ .button = .left, .x = 500, .y = 500 } };
    try testing.expectEqual(EventResult.ignored, tree.dispatchEvent(outside));
}

test "setRoot 替换旧根不泄漏" {
    const a = testing.allocator;
    var tree = Tree.init(a);
    defer tree.deinit();

    const b1 = try Box.create(a, foundation.Color.white, .{ .width = 10, .height = 10 });
    tree.setRoot(&b1.node);

    const b2 = try Box.create(a, foundation.Color.black, .{ .width = 20, .height = 20 });
    tree.setRoot(&b2.node); // 旧根 b1 被释放，testing.allocator 校验
}
