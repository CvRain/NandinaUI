//! runtime/node —— UI 节点树
//!
//! `Node` 是 UI 运行时树的基本单元。采用**嵌入式 + vtable**模式（与 reactive.graph.Node
//! 一致）：具体节点类型把 `Node` 作为字段嵌入，通过 `@fieldParentPtr` 取回自身，用 vtable
//! 多态分发 measure / layout / paint / event。
//!
//! 职责：
//! - 持有 owning 子节点列表、父指针、几何 bounds、可见性。
//! - dirty 传播：`markLayoutDirty` / `markPaintDirty` 向上冒泡到根。
//! - 生命周期：`deinit` 递归释放子树。
//!
//! 几何计算（measure/layout）委托给具体节点的 vtable，节点内部用 layout 层求解器
//! （flex/flow/anchors），不在 runtime 写死布局算法。绘制 paint 产出 render.Scene 命令。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const render = @import("../render/render.zig");
const layout = @import("../layout/layout.zig");
const event = @import("event.zig");

const Allocator = std.mem.Allocator;
const Rect = foundation.Rect;
const Size = foundation.Size;
const Constraints = layout.Constraints;
const Scene = render.Scene;
const Event = event.Event;

/// 事件是否被消费（消费后停止冒泡）。
pub const EventResult = enum { ignored, consumed };

/// 子节点裁剪区域：节点可声明其子树被裁剪到该矩形（可带圆角）。
/// 裁剪本身由 `Tree` 的绘制遍历统一 push/pop，不在组件 paint 里重复实现。
pub const ClipRegion = struct {
    rect: Rect,
    radius: f32 = 0,
};

/// 节点 vtable：具体节点实现这些钩子。除 measure 外均可选（提供默认 no-op）。
pub const VTable = struct {
    /// 在约束下计算期望尺寸。必填。
    measure: *const fn (node: *Node, constraints: Constraints) Size,
    /// 在分配的 bounds 内放置子节点（自顶向下）。默认：把所有子节点铺满自身 bounds。
    layout: *const fn (node: *Node) void = defaultLayout,
    /// 产出自身的绘制命令（不含子节点，子节点由 Tree 递归绘制）。默认 no-op。
    paint: *const fn (node: *Node, scene: *Scene) anyerror!void = defaultPaint,
    /// 处理一个已命中本节点的事件。默认 ignored。
    handle_event: *const fn (node: *Node, ev: Event) EventResult = defaultHandleEvent,
    /// 声明本节点子树的裁剪区域。返回非 null 时，Tree 在绘制子节点前 push
    /// 该裁剪、绘完后 pop。默认不裁剪（null）。统一裁剪机制，避免每组件自造轮子。
    child_clip: *const fn (node: *Node) ?ClipRegion = defaultChildClip,
    /// 释放具体节点自有资源（不含子节点 / Node 本身缓冲，那些由框架处理）。默认 no-op。
    deinit: *const fn (node: *Node, allocator: Allocator) void = defaultDeinit,
};

fn defaultLayout(node: *Node) void {
    // 默认：子节点填满本节点内容区（bounds）。
    for (node.children.items) |child| {
        child.setBounds(node.bounds);
    }
}

fn defaultPaint(node: *Node, scene: *Scene) anyerror!void {
    _ = node;
    _ = scene;
}

fn defaultHandleEvent(node: *Node, ev: Event) EventResult {
    _ = node;
    _ = ev;
    return .ignored;
}

fn defaultChildClip(node: *Node) ?ClipRegion {
    _ = node;
    return null;
}

fn defaultDeinit(node: *Node, allocator: Allocator) void {
    _ = node;
    _ = allocator;
}

/// UI 节点。具体类型嵌入本结构作为字段。
pub const Node = struct {
    vtable: *const VTable,
    parent: ?*Node = null,
    children: std.ArrayList(*Node) = .empty,

    /// 最终几何（绝对坐标，由父节点在 layout 时分配）。
    bounds: Rect = .{},
    /// measure 阶段得到的期望尺寸（缓存）。
    measured: Size = .{},

    visible: bool = true,
    /// 是否参与命中测试（不可见或禁用时为 false）。
    hit_testable: bool = true,

    /// 需要重新 measure + layout。
    layout_dirty: bool = true,
    /// 需要重新绘制。
    paint_dirty: bool = true,

    // ── 树结构 ──────────────────────────────────────────────────────────────

    /// 追加一个 owning 子节点。子节点所有权转移给本节点（由本节点 deinit 释放）。
    pub fn addChild(self: *Node, allocator: Allocator, child: *Node) Allocator.Error!void {
        child.parent = self;
        try self.children.append(allocator, child);
        self.markLayoutDirty();
    }

    /// 递归释放整棵子树：先 deinit 子节点，再调用自身 vtable.deinit，最后释放 children 缓冲。
    /// 注意：本函数不 `destroy(self)` —— 节点内存的分配方式由具体类型决定，
    /// 具体类型的 vtable.deinit 负责 `destroy`。
    pub fn deinitTree(self: *Node, allocator: Allocator) void {
        for (self.children.items) |child| {
            child.deinitTree(allocator);
        }
        self.children.deinit(allocator);
        self.vtable.deinit(self, allocator);
    }

    // ── 几何 ────────────────────────────────────────────────────────────────

    pub fn setBounds(self: *Node, bounds: Rect) void {
        self.bounds = bounds;
    }

    // ── dirty 传播 ─────────────────────────────────────────────────────────────

    /// 标记需要重新布局，并向上冒泡（同时标记 paint dirty）。
    pub fn markLayoutDirty(self: *Node) void {
        self.layout_dirty = true;
        self.paint_dirty = true;
        if (self.parent) |p| p.markLayoutDirty();
    }

    /// 仅标记需要重绘，并向上冒泡 paint dirty。
    pub fn markPaintDirty(self: *Node) void {
        self.paint_dirty = true;
        if (self.parent) |p| p.markPaintDirty();
    }

    // ── 生命周期钩子转发 ─────────────────────────────────────────────────────

    pub fn measure(self: *Node, constraints: Constraints) Size {
        self.measured = self.vtable.measure(self, constraints);
        return self.measured;
    }

    pub fn layout(self: *Node) void {
        self.vtable.layout(self);
    }

    pub fn paint(self: *Node, scene: *Scene) anyerror!void {
        return self.vtable.paint(self, scene);
    }

    // ── 命中测试 ──────────────────────────────────────────────────────────────

    /// 返回包含点 (x, y) 的最深可命中节点（深度优先，后添加的子节点优先 = 在上层）。
    pub fn hitTest(self: *Node, x: f32, y: f32) ?*Node {
        if (!self.visible or !self.hit_testable) return null;
        if (!self.bounds.containsPoint(.{ .x = x, .y = y })) return null;

        // 子节点逆序遍历：后绘制的在上层，优先命中。
        var i = self.children.items.len;
        while (i > 0) {
            i -= 1;
            if (self.children.items[i].hitTest(x, y)) |hit| return hit;
        }
        return self;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试用的简单节点：一个填充矩形 + 固定 preferred size
// ─────────────────────────────────────────────────────────────────────────────

const TestRect = struct {
    node: Node,
    color: foundation.Color,
    preferred: Size,
    paint_count: u32 = 0,
    last_event: ?Event = null,

    const vtable = VTable{
        .measure = measure,
        .paint = paint,
        .handle_event = handleEvent,
        .deinit = deinitImpl,
    };

    fn create(allocator: Allocator, color: foundation.Color, preferred: Size) !*TestRect {
        const self = try allocator.create(TestRect);
        self.* = .{ .node = .{ .vtable = &vtable }, .color = color, .preferred = preferred };
        return self;
    }

    fn measure(node: *Node, constraints: Constraints) Size {
        const self: *TestRect = @fieldParentPtr("node", node);
        return constraints.constrain(self.preferred);
    }

    fn paint(node: *Node, scene: *Scene) anyerror!void {
        const self: *TestRect = @fieldParentPtr("node", node);
        self.paint_count += 1;
        try scene.fillRect(node.bounds, self.color);
    }

    fn handleEvent(node: *Node, ev: Event) EventResult {
        const self: *TestRect = @fieldParentPtr("node", node);
        self.last_event = ev;
        return .consumed;
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *TestRect = @fieldParentPtr("node", node);
        allocator.destroy(self);
    }
};

test "Node 树结构与 deinit" {
    const a = std.testing.allocator;
    const root = try TestRect.create(a, foundation.Color.white, .{ .width = 100, .height = 100 });
    const child = try TestRect.create(a, foundation.Color.black, .{ .width = 50, .height = 50 });
    try root.node.addChild(a, &child.node);

    try std.testing.expectEqual(@as(usize, 1), root.node.children.items.len);
    try std.testing.expectEqual(&root.node, child.node.parent.?);

    root.node.deinitTree(a); // 递归释放，testing.allocator 校验无泄漏
}

test "measure 受约束限制" {
    const a = std.testing.allocator;
    const r = try TestRect.create(a, foundation.Color.white, .{ .width = 200, .height = 80 });
    defer r.node.deinitTree(a);

    const sz = r.node.measure(Constraints.loose(100, 100));
    try std.testing.expectEqual(@as(f32, 100), sz.width); // 200 被约束到 100
    try std.testing.expectEqual(@as(f32, 80), sz.height);
}

test "markLayoutDirty 向上冒泡" {
    const a = std.testing.allocator;
    const root = try TestRect.create(a, foundation.Color.white, .{ .width = 100, .height = 100 });
    const child = try TestRect.create(a, foundation.Color.black, .{ .width = 50, .height = 50 });
    try root.node.addChild(a, &child.node);
    defer root.node.deinitTree(a);

    // 清掉 dirty
    root.node.layout_dirty = false;
    root.node.paint_dirty = false;
    child.node.layout_dirty = false;
    child.node.paint_dirty = false;

    child.node.markLayoutDirty();
    try std.testing.expect(child.node.layout_dirty);
    try std.testing.expect(root.node.layout_dirty); // 冒泡到父
}

test "hitTest 返回最深的上层节点" {
    const a = std.testing.allocator;
    const root = try TestRect.create(a, foundation.Color.white, .{ .width = 100, .height = 100 });
    const child = try TestRect.create(a, foundation.Color.black, .{ .width = 50, .height = 50 });
    try root.node.addChild(a, &child.node);
    defer root.node.deinitTree(a);

    root.node.setBounds(Rect.fromXywh(0, 0, 100, 100));
    child.node.setBounds(Rect.fromXywh(10, 10, 50, 50));

    // 点在 child 内 → 命中 child
    try std.testing.expectEqual(&child.node, root.node.hitTest(20, 20).?);
    // 点在 root 内但 child 外 → 命中 root
    try std.testing.expectEqual(&root.node, root.node.hitTest(80, 80).?);
    // 点在 root 外 → 无命中
    try std.testing.expect(root.node.hitTest(200, 200) == null);
}

test "不可命中节点被跳过" {
    const a = std.testing.allocator;
    const root = try TestRect.create(a, foundation.Color.white, .{ .width = 100, .height = 100 });
    const child = try TestRect.create(a, foundation.Color.black, .{ .width = 50, .height = 50 });
    try root.node.addChild(a, &child.node);
    defer root.node.deinitTree(a);

    root.node.setBounds(Rect.fromXywh(0, 0, 100, 100));
    child.node.setBounds(Rect.fromXywh(10, 10, 50, 50));
    child.node.hit_testable = false;

    // child 不可命中 → 穿透到 root
    try std.testing.expectEqual(&root.node, root.node.hitTest(20, 20).?);
}
