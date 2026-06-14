//! widgets/focus_ring —— 焦点可视化覆盖层（最简版）
//!
//! `FocusRing` 是一个**不可命中**的覆盖层：当 `active` 为真时，在自身 bounds 外扩 `offset`
//! 处绘制一圈描边环（焦点高亮）。不可见时（active=false 或 width<=0）不绘制任何内容。
//!
//! 设计取舍（保持简单）：
//!   - 只有 `active` 是响应式输入（`ReadSignal(bool)`），变化时触发重绘。
//!   - color / width / offset 是构造时固定的普通字段（默认从约定值取，未来可接 theme token）。
//!   - render 层没有 stroke 原语，环用四条 `fillRect`（上 / 下 / 左 / 右边）拼出，不带圆角。
//!   - `hit_testable = false`：焦点环纯视觉，不参与命中测试。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const layout = @import("../layout/layout.zig");
const render = @import("../render/render.zig");
const runtime = @import("../runtime/runtime.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Size = foundation.Size;
const Rect = foundation.Rect;
const Constraints = layout.Constraints;
const Node = runtime.Node;
const VTable = runtime.VTable;
const Scene = render.Scene;

/// FocusRing 的输入属性。
pub const FocusRingProps = struct {
    /// 是否显示焦点环（响应式）。
    active: reactive.ReadSignal(bool),
    /// 环颜色（构造固定）。默认蓝色。
    color: Color = Color.fromHexRgb(0x3B82F6),
    /// 环描边宽度（构造固定）。
    width: f32 = 2,
    /// 环相对 bounds 外扩的距离（构造固定）。
    offset: f32 = 2,
};

/// 焦点可视化覆盖层。
pub const FocusRing = struct {
    node: Node,
    allocator: Allocator,

    active: reactive.ReadSignal(bool),
    color: Color,
    width: f32,
    offset: f32,

    scope: reactive.EffectScope,

    const vtable = VTable{
        .measure = measureImpl,
        .paint = paintImpl,
        .deinit = deinitImpl,
    };

    pub fn create(allocator: Allocator, g: *reactive.Graph, props: FocusRingProps) !*FocusRing {
        const self = try allocator.create(FocusRing);
        self.* = .{
            .node = .{ .vtable = &vtable, .hit_testable = false },
            .allocator = allocator,
            .active = props.active,
            .color = props.color,
            .width = props.width,
            .offset = props.offset,
            .scope = reactive.EffectScope.init(g),
        };
        // active 变化时重绘（显示 / 隐藏焦点环）。
        _ = try self.scope.add(self, struct {
            fn f(s: *FocusRing) void {
                _ = s.active.get();
                s.node.markPaintDirty();
            }
        }.f);
        return self;
    }

    // ── vtable 实现 ─────────────────────────────────────────────────────────────

    /// FocusRing 是覆盖层，不占布局空间：返回约束允许的最小尺寸。
    fn measureImpl(node: *Node, constraints: Constraints) Size {
        _ = node;
        return constraints.constrain(Size.zero);
    }

    /// active 时用四条边矩形拼出环（外扩 offset，描边宽 width）。
    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *FocusRing = @fieldParentPtr("node", node);
        if (!self.active.peek() or self.width <= 0) return;

        const b = node.bounds;
        const o = self.offset;
        const w = self.width;
        // 环的外边界
        const outer = Rect{
            .left = b.left - o,
            .top = b.top - o,
            .right = b.right + o,
            .bottom = b.bottom + o,
        };
        const ow = outer.width();
        const oh = outer.height();

        // 四条边：上、下、左、右（左右去掉上下角避免重叠绘制）
        try scene.fillRect(Rect.fromXywh(outer.left, outer.top, ow, w), self.color); // top
        try scene.fillRect(Rect.fromXywh(outer.left, outer.bottom - w, ow, w), self.color); // bottom
        try scene.fillRect(Rect.fromXywh(outer.left, outer.top + w, w, oh - 2 * w), self.color); // left
        try scene.fillRect(Rect.fromXywh(outer.right - w, outer.top + w, w, oh - 2 * w), self.color); // right
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *FocusRing = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const testing = std.testing;

test "FocusRing create 与 deinit 无泄漏" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var active = reactive.Signal(bool).init(&g, false);
    defer active.deinit();

    const ring = try FocusRing.create(a, &g, .{ .active = active.asReadonly() });
    ring.node.deinitTree(a);
}

test "FocusRing 不参与命中测试" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var active = reactive.Signal(bool).init(&g, true);
    defer active.deinit();

    const ring = try FocusRing.create(a, &g, .{ .active = active.asReadonly() });
    defer ring.node.deinitTree(a);
    ring.node.setBounds(Rect.fromXywh(0, 0, 100, 100));

    try testing.expect(!ring.node.hit_testable);
    try testing.expect(ring.node.hitTest(50, 50) == null);
}

test "inactive 时不绘制" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var active = reactive.Signal(bool).init(&g, false);
    defer active.deinit();

    const ring = try FocusRing.create(a, &g, .{ .active = active.asReadonly() });
    defer ring.node.deinitTree(a);
    ring.node.setBounds(Rect.fromXywh(0, 0, 100, 50));

    var scene = render.Scene.init(a);
    defer scene.deinit();
    try ring.node.paint(&scene);
    try testing.expectEqual(@as(usize, 0), scene.count());
}

test "active 时绘制四条边" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var active = reactive.Signal(bool).init(&g, true);
    defer active.deinit();

    const ring = try FocusRing.create(a, &g, .{ .active = active.asReadonly(), .width = 2, .offset = 2 });
    defer ring.node.deinitTree(a);
    ring.node.setBounds(Rect.fromXywh(10, 10, 100, 50));

    var scene = render.Scene.init(a);
    defer scene.deinit();
    try ring.node.paint(&scene);

    // 四条边 = 4 条 fill_rect
    try testing.expectEqual(@as(usize, 4), scene.count());
    for (scene.commands.items) |cmd| {
        try testing.expect(cmd == .fill_rect);
    }
    // top 边从 (8, 8) 开始（bounds 外扩 offset=2），宽 = 100 + 2*2 = 104
    const top = scene.commands.items[0].fill_rect.rect;
    try testing.expectEqual(@as(f32, 8), top.left);
    try testing.expectEqual(@as(f32, 8), top.top);
    try testing.expectEqual(@as(f32, 104), top.width());
    try testing.expectEqual(@as(f32, 2), top.height());
}

test "active 信号变化触发重绘" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var active = reactive.Signal(bool).init(&g, false);
    defer active.deinit();

    const ring = try FocusRing.create(a, &g, .{ .active = active.asReadonly() });
    defer ring.node.deinitTree(a);

    ring.node.paint_dirty = false;
    active.set(true);
    try testing.expect(ring.node.paint_dirty);
}
