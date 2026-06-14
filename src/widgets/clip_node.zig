//! widgets/clip_node —— 通用子树裁剪容器
//!
//! `ClipNode` 是一个**不可见**的通用裁剪容器：自身不绘制任何内容，只声明「我的子树被
//! 裁剪到这块区域」。任何需要裁剪子内容的组件，用 ClipNode 包裹即可 —— 不必各自重复实现
//! 裁剪逻辑（吸收 archive 教训：曾为每个组件单独设 clip，应改为统一组件）。
//!
//! 裁剪本身由 runtime 的绘制遍历统一 push/pop（见 runtime.Node.VTable.child_clip 与
//! Tree.paintNode），ClipNode 只通过 `child_clip` 钩子声明裁剪矩形与圆角。
//!
//! 属性以只读 signal 输入：
//!   - corner_radius：圆角裁剪半径。
//!   - clip_padding：裁剪区域相对自身 bounds 向内收缩（类似 padding）。
//!
//! 布局：ClipNode 把子节点铺满自身 bounds（裁剪区域可再用 clip_padding 收缩）。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const layout = @import("../layout/layout.zig");
const render = @import("../render/render.zig");
const runtime = @import("../runtime/runtime.zig");

const Allocator = std.mem.Allocator;
const Size = foundation.Size;
const Rect = foundation.Rect;
const Insets = foundation.Insets;
const Constraints = layout.Constraints;
const Node = runtime.Node;
const VTable = runtime.VTable;
const ClipRegion = runtime.ClipRegion;

/// ClipNode 的输入属性（只读信号，调用方持有背后 Signal）。
pub const ClipNodeProps = struct {
    corner_radius: reactive.ReadSignal(f32),
    clip_padding: reactive.ReadSignal(Insets),
};

/// 通用子树裁剪容器。
pub const ClipNode = struct {
    node: Node,
    allocator: Allocator,

    corner_radius: reactive.ReadSignal(f32),
    clip_padding: reactive.ReadSignal(Insets),

    scope: reactive.EffectScope,

    const vtable = VTable{
        .measure = measureImpl,
        .layout = layoutImpl,
        .child_clip = childClipImpl,
        .deinit = deinitImpl,
        // paint 用默认 no-op：ClipNode 不可见，不绘制任何内容。
    };

    pub fn create(allocator: Allocator, g: *reactive.Graph, props: ClipNodeProps) !*ClipNode {
        const self = try allocator.create(ClipNode);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .corner_radius = props.corner_radius,
            .clip_padding = props.clip_padding,
            .scope = reactive.EffectScope.init(g),
        };
        // 裁剪参数变化时需要重绘（裁剪区域随之改变）。
        _ = try self.scope.add(self, struct {
            fn f(s: *ClipNode) void {
                _ = s.corner_radius.get();
                _ = s.clip_padding.get();
                s.node.markPaintDirty();
            }
        }.f);
        return self;
    }

    // ── vtable 实现 ─────────────────────────────────────────────────────────────

    /// measure：透传约束给子节点，取子节点中最大尺寸。
    fn measureImpl(node: *Node, constraints: Constraints) Size {
        var measured = Size.zero;
        for (node.children.items) |child| {
            const cs = child.measure(constraints);
            measured = .{
                .width = @max(measured.width, cs.width),
                .height = @max(measured.height, cs.height),
            };
        }
        return constraints.constrain(measured);
    }

    /// layout：子节点铺满自身 bounds。
    fn layoutImpl(node: *Node) void {
        for (node.children.items) |child| {
            child.setBounds(node.bounds);
        }
    }

    /// child_clip：声明子树裁剪到 bounds（去掉 clip_padding），带圆角。
    fn childClipImpl(node: *Node) ?ClipRegion {
        const self: *ClipNode = @fieldParentPtr("node", node);
        const pad = self.clip_padding.peek();
        return .{
            .rect = pad.applyToRect(node.bounds),
            .radius = self.corner_radius.peek(),
        };
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *ClipNode = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const testing = std.testing;

/// 测试辅助：一个画满 bounds 的彩色方块节点。
const Box = struct {
    node: Node,
    color: foundation.Color,
    size: Size,

    const vtable = VTable{ .measure = measure, .paint = paint, .deinit = deinitImpl };

    fn create(a: Allocator, color: foundation.Color, size: Size) !*Box {
        const self = try a.create(Box);
        self.* = .{ .node = .{ .vtable = &vtable }, .color = color, .size = size };
        return self;
    }
    fn measure(node: *Node, c: Constraints) Size {
        const self: *Box = @fieldParentPtr("node", node);
        return c.constrain(self.size);
    }
    fn paint(node: *Node, scene: *render.Scene) anyerror!void {
        const self: *Box = @fieldParentPtr("node", node);
        try scene.fillRect(node.bounds, self.color);
    }
    fn deinitImpl(node: *Node, a: Allocator) void {
        a.destroy(@as(*Box, @fieldParentPtr("node", node)));
    }
};

const ClipSignals = struct {
    cr: reactive.Signal(f32),
    pad: reactive.Signal(Insets),

    fn init(g: *reactive.Graph) ClipSignals {
        return .{
            .cr = reactive.Signal(f32).init(g, 0),
            .pad = reactive.Signal(Insets).init(g, Insets.zero),
        };
    }
    fn deinit(self: *ClipSignals) void {
        self.cr.deinit();
        self.pad.deinit();
    }
    fn props(self: *ClipSignals) ClipNodeProps {
        return .{ .corner_radius = self.cr.asReadonly(), .clip_padding = self.pad.asReadonly() };
    }
};

test "ClipNode create 与 deinit 无泄漏" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var sigs = ClipSignals.init(&g);
    defer sigs.deinit();

    const clip = try ClipNode.create(a, &g, sigs.props());
    clip.node.deinitTree(a);
}

test "child_clip 声明裁剪区域（含 clip_padding 收缩）" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var sigs = ClipSignals.init(&g);
    defer sigs.deinit();
    sigs.cr.set(8);
    sigs.pad.set(Insets.all(4));

    const clip = try ClipNode.create(a, &g, sigs.props());
    defer clip.node.deinitTree(a);
    clip.node.setBounds(Rect.fromXywh(0, 0, 100, 100));

    const region = clip.node.vtable.child_clip(&clip.node).?;
    try testing.expectEqual(@as(f32, 8), region.radius);
    // 100x100 去掉 all(4) padding → (4,4 92x92)
    try testing.expectEqual(@as(f32, 4), region.rect.left);
    try testing.expectEqual(@as(f32, 92), region.rect.width());
}

test "Tree 绘制时在子节点周围 push/pop clip" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var sigs = ClipSignals.init(&g);
    defer sigs.deinit();
    sigs.cr.set(6);

    var tree = runtime.Tree.init(a);
    defer tree.deinit();

    const clip = try ClipNode.create(a, &g, sigs.props());
    const box = try Box.create(a, foundation.Color.white, .{ .width = 200, .height = 200 });
    try clip.node.addChild(a, &box.node);
    tree.setRoot(&clip.node);
    tree.setViewport(.{ .width = 80, .height = 80 });

    _ = try tree.frame();

    // 期望命令序列：push_clip, fill_rect(box), pop_clip
    const cmds = tree.scene.commands.items;
    try testing.expectEqual(@as(usize, 3), cmds.len);
    try testing.expect(cmds[0] == .push_clip);
    try testing.expect(cmds[1] == .fill_rect);
    try testing.expect(cmds[2] == .pop_clip);
    // 裁剪矩形 = clip 节点 bounds（视口 80x80），圆角 6
    try testing.expectEqual(@as(f32, 6), cmds[0].push_clip.radius);
    try testing.expectEqual(@as(f32, 80), cmds[0].push_clip.rect.width());
}

test "无子节点时不产生 clip 命令" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();
    var sigs = ClipSignals.init(&g);
    defer sigs.deinit();

    var tree = runtime.Tree.init(a);
    defer tree.deinit();

    const clip = try ClipNode.create(a, &g, sigs.props());
    tree.setRoot(&clip.node);
    tree.setViewport(.{ .width = 50, .height = 50 });
    _ = try tree.frame();

    // ClipNode 不可见且无子节点 → 无任何绘制命令
    try testing.expectEqual(@as(usize, 0), tree.scene.count());
}
