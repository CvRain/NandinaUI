//! widgets/panel —— 面板组件（M5 响应式版）
//!
//! Panel 绘制一个带圆角、内边距和可选边框的背景矩形。
//! 所有视觉属性均通过 `ReadSignal` 输入，内部用 `EffectScope` 追踪变化并触发
//! `markLayoutDirty`，实现 M5 响应式数据流。
//!
//! 边框通过「先绘制外圆角矩形（边框色）再绘制内圆角矩形（背景色）」来模拟，
//! 因为 render.Scene 目前没有 stroke 原语。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / render / runtime。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const layout = @import("../layout/layout.zig");
const render = @import("../render/render.zig");
const runtime = @import("../runtime/runtime.zig");

const Node = runtime.Node;
const VTable = runtime.VTable;
const EventResult = runtime.EventResult;
const Event = runtime.Event;
const Constraints = layout.Constraints;
const Scene = render.Scene;

// ── 公共类型 ─────────────────────────────────────────────────────────────────

/// Panel 的输入属性，全部为只读信号，由调用方持有后备 Signal 的生命周期。
pub const PanelProps = struct {
    bg_color: reactive.ReadSignal(foundation.Color),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(foundation.Insets),
    border_color: reactive.ReadSignal(foundation.Color),
    border_width: reactive.ReadSignal(f32),
};

/// 面板组件。
pub const Panel = struct {
    node: Node,
    allocator: std.mem.Allocator,

    // 输入属性（只读信号，调用方持有后备 Signal）
    bg_color: reactive.ReadSignal(foundation.Color),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(foundation.Insets),
    border_color: reactive.ReadSignal(foundation.Color),
    border_width: reactive.ReadSignal(f32),

    // 响应式作用域：追踪所有输入信号，变化时标记脏
    scope: reactive.EffectScope,

    // ── vtable ────────────────────────────────────────────────────────────────

    const vtable = VTable{
        .measure = measureImpl,
        .paint = paintImpl,
        .deinit = deinitImpl,
    };

    // ── 构造 ──────────────────────────────────────────────────────────────────

    pub fn create(
        allocator: std.mem.Allocator,
        g: *reactive.Graph,
        props: PanelProps,
    ) !*Panel {
        const self = try allocator.create(Panel);
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
        _ = try self.scope.add(self, struct {
            fn f(s: *Panel) void {
                // 建立对所有输入信号的依赖追踪
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

    // ── vtable 实现 ───────────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: Constraints) foundation.Size {
        const self: *Panel = @fieldParentPtr("node", node);
        const pad = self.padding.peek();
        // Panel 本身没有固有内容，返回满足 padding 的最小尺寸
        const natural = foundation.Size{
            .width = pad.horizontal(),
            .height = pad.vertical(),
        };
        return constraints.constrain(natural);
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Panel = @fieldParentPtr("node", node);
        const radius = self.corner_radius.peek();
        const bw = self.border_width.peek();

        if (bw > 0) {
            // 先绘制外圆角矩形（边框色），再用内矩形覆盖（背景色）
            try scene.fillRoundedRect(node.bounds, radius, self.border_color.peek());
            const pad = foundation.Insets.all(bw);
            const inner = pad.applyToRect(node.bounds);
            const inner_radius = @max(0.0, radius - bw);
            try scene.fillRoundedRect(inner, inner_radius, self.bg_color.peek());
        } else {
            try scene.fillRoundedRect(node.bounds, radius, self.bg_color.peek());
        }
    }

    fn deinitImpl(node: *Node, allocator: std.mem.Allocator) void {
        const self: *Panel = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Panel 创建与基本绘制" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 8.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.all(4));
    defer pad_sig.deinit();
    var bc_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bc_sig.deinit();
    var bw_sig = reactive.Signal(f32).init(&g, 0.0);
    defer bw_sig.deinit();

    const panel = try Panel.create(a, &g, .{
        .bg_color = bg_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .border_color = bc_sig.asReadonly(),
        .border_width = bw_sig.asReadonly(),
    });
    defer panel.node.deinitTree(a);

    panel.node.setBounds(foundation.Rect.fromXywh(0, 0, 200, 100));

    var scene = render.Scene.init(a);
    defer scene.deinit();
    try panel.node.paint(&scene);

    // 无边框时只有一条 fill_rounded_rect 命令
    try std.testing.expectEqual(@as(usize, 1), scene.count());
    try std.testing.expect(scene.commands.items[0] == .fill_rounded_rect);
}

test "Panel 有边框时绘制两层" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 4.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.zero);
    defer pad_sig.deinit();
    var bc_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bc_sig.deinit();
    var bw_sig = reactive.Signal(f32).init(&g, 2.0);
    defer bw_sig.deinit();

    const panel = try Panel.create(a, &g, .{
        .bg_color = bg_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .border_color = bc_sig.asReadonly(),
        .border_width = bw_sig.asReadonly(),
    });
    defer panel.node.deinitTree(a);

    panel.node.setBounds(foundation.Rect.fromXywh(0, 0, 100, 50));

    var scene = render.Scene.init(a);
    defer scene.deinit();
    try panel.node.paint(&scene);

    // 有边框时：外层 + 内层 = 2 条命令
    try std.testing.expectEqual(@as(usize, 2), scene.count());
}

test "Panel bg_color 信号变化 → markLayoutDirty" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 0.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.zero);
    defer pad_sig.deinit();
    var bc_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bc_sig.deinit();
    var bw_sig = reactive.Signal(f32).init(&g, 0.0);
    defer bw_sig.deinit();

    const panel = try Panel.create(a, &g, .{
        .bg_color = bg_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .border_color = bc_sig.asReadonly(),
        .border_width = bw_sig.asReadonly(),
    });
    defer panel.node.deinitTree(a);

    // 重置 dirty 标志（create 时 effect 首次运行已标记 dirty）
    panel.node.layout_dirty = false;
    panel.node.paint_dirty = false;

    // 修改 bg_color → effect 重新运行 → markLayoutDirty
    bg_sig.set(foundation.Color.black);
    try std.testing.expect(panel.node.layout_dirty);
    try std.testing.expect(panel.node.paint_dirty);
}
