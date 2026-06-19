//! widgets/surface —— 基础背景容器组件（M5 响应式版本）
//!
//! `Surface` 是最基础的背景容器：负责填充背景色、圆角裁剪、边框绘制，
//! 以及基于 padding 的子节点布局。
//!
//! M5 响应式接入：所有属性通过 `ReadSignal(T)` 输入，内部使用 `EffectScope`
//! 管理副作用生命周期。属性信号变化时自动标记节点 dirty，触发重绘。
//!
//! 生命周期：
//!   - 调用方拥有所有背书 `Signal(T)`，Surface 仅持有 `ReadSignal(T)` 视图。
//!   - 通过 `node.deinitTree(allocator)` 释放（LIFO：先 scope，再 surface 本身）。
//!   - 背书 Signal 须在 Surface 释放之后再 deinit，Graph.deinit 最后执行。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const render = @import("../render/render.zig");
const layout = @import("../layout/layout.zig");
const node_mod = @import("../runtime/node.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Insets = foundation.Insets;
const Size = foundation.Size;
const Rect = foundation.Rect;
const Constraints = layout.Constraints;
const Scene = render.Scene;
const Node = node_mod.Node;

// ─────────────────────────────────────────────────────────────────────────────
// Props
// ─────────────────────────────────────────────────────────────────────────────

/// Surface 的所有响应式属性。调用方拥有背书 Signal，Surface 持有只读视图。
pub const SurfaceProps = struct {
    bg_color: reactive.ReadSignal(Color),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(Insets),
    border_color: reactive.ReadSignal(Color),
    border_width: reactive.ReadSignal(f32),
};

// ─────────────────────────────────────────────────────────────────────────────
// Surface
// ─────────────────────────────────────────────────────────────────────────────

pub const Surface = struct {
    node: Node,
    allocator: Allocator,

    // 接收的只读 Signal 视图（调用方拥有背书 Signal）
    bg_color: reactive.ReadSignal(Color),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(Insets),
    border_color: reactive.ReadSignal(Color),
    border_width: reactive.ReadSignal(f32),

    // 响应式作用域：组件销毁时整体 dispose
    scope: reactive.EffectScope,

    const vtable = node_mod.VTable{
        .measure = measureImpl,
        .layout = layoutImpl,
        .paint = paintImpl,
        .deinit = deinitImpl,
    };

    /// 创建并初始化 Surface。在 `g` 上注册一个合并 effect，读取所有输入信号，
    /// 任意信号变化时自动调用 `node.markLayoutDirty()`。
    pub fn create(allocator: Allocator, g: *reactive.Graph, props: SurfaceProps) !*Surface {
        const self = try allocator.create(Surface);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .bg_color = props.bg_color,
            .corner_radius = props.corner_radius,
            .padding = props.padding,
            .border_color = props.border_color,
            .border_width = props.border_width,
            .scope = reactive.EffectScope.init(g),
        };
        // 合并 effect：在追踪上下文中读取全部信号，建立依赖。
        // 任意信号变化时 effect 重跑，从而调用 markLayoutDirty（含 paint_dirty）。
        _ = try self.scope.add(self, struct {
            fn f(s: *Surface) void {
                _ = s.bg_color.get();
                _ = s.corner_radius.get();
                _ = s.padding.get();
                _ = s.border_color.get();
                _ = s.border_width.get();
                s.node.markLayoutDirty();
            }
        }.f);
        return self;
    }

    // ── vtable 实现 ─────────────────────────────────────────────────────────────

    /// measure：基于 padding 计算最小尺寸，若有子节点则加上子节点 preferred size。
    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *Surface = @fieldParentPtr("node", node);
        const pad = self.padding.peek();

        const h_pad = pad.horizontal();
        const v_pad = pad.vertical();

        // 内容区约束：先减去 padding
        const inner_constraints = Constraints{
            .min_width = @max(0, constraints.min_width - h_pad),
            .max_width = if (constraints.max_width == std.math.inf(f32))
                std.math.inf(f32)
            else
                @max(0, constraints.max_width - h_pad),
            .min_height = @max(0, constraints.min_height - v_pad),
            .max_height = if (constraints.max_height == std.math.inf(f32))
                std.math.inf(f32)
            else
                @max(0, constraints.max_height - v_pad),
        };

        // 测量子节点
        var child_size = Size.zero;
        for (node.children.items) |child| {
            const cs = child.measure(inner_constraints);
            child_size = .{
                .width = @max(child_size.width, cs.width),
                .height = @max(child_size.height, cs.height),
            };
        }

        const desired = Size{
            .width = child_size.width + h_pad,
            .height = child_size.height + v_pad,
        };
        return constraints.constrain(desired);
    }

    /// layout：把子节点放置在内容区（减去 padding），使用子节点自身测量尺寸。
    fn layoutImpl(node: *Node) void {
        const self: *Surface = @fieldParentPtr("node", node);
        const pad = self.padding.peek();
        for (node.children.items) |child| {
            const child_w = child.measured.width;
            const child_h = child.measured.height;
            child.setBounds(Rect.fromXywh(
                node.bounds.left + pad.left,
                node.bounds.top + pad.top,
                if (child_w > 0) child_w else node.bounds.width() - pad.horizontal(),
                if (child_h > 0) child_h else node.bounds.height() - pad.vertical(),
            ));
        }
    }

    /// paint：先绘制圆角背景，再绘制边框（若 border_width > 0）。
    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Surface = @fieldParentPtr("node", node);
        const bg = self.bg_color.peek();
        const cr = self.corner_radius.peek();
        const bw = self.border_width.peek();
        const bc = self.border_color.peek();

        // 背景
        if (cr > 0) {
            try scene.fillRoundedRect(node.bounds, cr, bg);
        } else {
            try scene.fillRect(node.bounds, bg);
        }

        // 边框（收缩半个边框宽度的矩形，模拟 stroke）
        if (bw > 0) {
            const half = bw / 2;
            const border_rect = Rect{
                .left = node.bounds.left + half,
                .top = node.bounds.top + half,
                .right = node.bounds.right - half,
                .bottom = node.bounds.bottom - half,
            };
            if (cr > 0) {
                try scene.fillRoundedRect(border_rect, @max(0, cr - half), bc);
            } else {
                try scene.fillRect(border_rect, bc);
            }
        }
    }

    /// deinit：先 dispose effect scope，再释放 Surface 自身。
    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Surface = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 辅助：构建带默认值的 props（测试用）
// ─────────────────────────────────────────────────────────────────────────────

/// 在测试中构造一套 5 个 Signal 并产出 SurfaceProps。
/// 调用方拥有所有 Signal 的生命周期。
const TestSignals = struct {
    bg: reactive.Signal(Color),
    corner_radius: reactive.Signal(f32),
    padding: reactive.Signal(Insets),
    border_color: reactive.Signal(Color),
    border_width: reactive.Signal(f32),

    fn init(g: *reactive.Graph) TestSignals {
        return .{
            .bg = reactive.Signal(Color).init(g, Color.white),
            .corner_radius = reactive.Signal(f32).init(g, 0),
            .padding = reactive.Signal(Insets).init(g, Insets.zero),
            .border_color = reactive.Signal(Color).init(g, Color.transparent),
            .border_width = reactive.Signal(f32).init(g, 0),
        };
    }

    fn deinit(self: *TestSignals) void {
        self.border_width.deinit();
        self.border_color.deinit();
        self.padding.deinit();
        self.corner_radius.deinit();
        self.bg.deinit();
    }

    fn props(self: *TestSignals) SurfaceProps {
        return .{
            .bg_color = self.bg.asReadonly(),
            .corner_radius = self.corner_radius.asReadonly(),
            .padding = self.padding.asReadonly(),
            .border_color = self.border_color.asReadonly(),
            .border_width = self.border_width.asReadonly(),
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Surface create 与 deinit 无泄漏" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);

    // 创建后节点应处于 dirty 状态（effect 首次执行调用了 markLayoutDirty）
    try std.testing.expect(s.node.layout_dirty);
    try std.testing.expect(s.node.paint_dirty);
}

test "Surface scope 注册了一个 effect" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);

    try std.testing.expectEqual(@as(usize, 1), s.scope.count());
}

test "Surface bg_color signal 触发 dirty" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);

    // 清除 dirty 标记后，改变 bg_color 应重新标记 dirty
    s.node.layout_dirty = false;
    s.node.paint_dirty = false;

    sigs.bg.set(Color.black); // black != white，通知订阅者

    try std.testing.expect(s.node.paint_dirty or s.node.layout_dirty);
}

test "Surface corner_radius signal 触发 dirty" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);

    s.node.layout_dirty = false;
    s.node.paint_dirty = false;

    sigs.corner_radius.set(8.0); // 0 → 8，值不同触发 effect

    try std.testing.expect(s.node.paint_dirty or s.node.layout_dirty);
}

test "Surface padding signal 触发 dirty" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);

    s.node.layout_dirty = false;
    s.node.paint_dirty = false;

    sigs.padding.set(Insets.all(12)); // 结构体无 == 比较，始终通知

    try std.testing.expect(s.node.paint_dirty or s.node.layout_dirty);
}

test "Surface measure 无子节点返回 padding 尺寸" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    sigs.padding.set(Insets.all(10));

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);

    const sz = s.node.measure(Constraints.loose(200, 200));
    // padding 各方向 10，desired 宽高均为 20（h_pad + v_pad 各 20）
    try std.testing.expectEqual(@as(f32, 20), sz.width);
    try std.testing.expectEqual(@as(f32, 20), sz.height);
}

test "Surface paint 输出 fill_rounded_rect（有圆角时）" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    sigs.corner_radius.set(4.0);

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);
    s.node.setBounds(Rect.fromXywh(0, 0, 100, 50));

    var scene = render.Scene.init(a);
    defer scene.deinit();

    try s.node.paint(&scene);

    try std.testing.expect(scene.count() >= 1);
    try std.testing.expect(scene.commands.items[0] == .fill_rounded_rect);
    try std.testing.expectEqual(@as(f32, 4.0), scene.commands.items[0].fill_rounded_rect.radius);
}

test "Surface border_width signal 触发 dirty" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const s = try Surface.create(a, &g, sigs.props());
    defer s.node.deinitTree(a);

    s.node.layout_dirty = false;
    s.node.paint_dirty = false;

    sigs.border_width.set(2.0);

    try std.testing.expect(s.node.paint_dirty or s.node.layout_dirty);
}
