//! widgets/switch —— 开关组件
//!
//! `Switch` 是布尔开关控件：圆角轨道 + 滑动圆钮。
//! 遵循 primitive 单一职责，不含标签。
//!
//! 使用示例：
//! ```zig
//! var on_sig = reactive.Signal(bool).init(&g, false);
//! defer on_sig.deinit();
//!
//! const sw = try Switch.create(allocator, &g, .{
//!     .checked = on_sig.asReadonly(),
//!     .color = readOnly(owner, Color, &g, Color.green),
//! });
//! defer sw.node.deinitTree(allocator);
//! ```

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const layout = @import("../layout/layout.zig");
const render = @import("../render/render.zig");
const runtime = @import("../runtime/runtime.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Rect = foundation.Rect;
const Size = foundation.Size;
const Constraints = layout.Constraints;
const Scene = render.Scene;
const Node = runtime.Node;
const VTable = runtime.VTable;
const Event = runtime.Event;
const EventResult = runtime.EventResult;

// ── 公共类型 ─────────────────────────────────────────────────────────────────

pub const SwitchProps = struct {
    /// 开关状态（只读信号）。
    checked: reactive.ReadSignal(bool),
    /// 开启时的主题色。
    color: reactive.ReadSignal(Color),
    /// 禁用状态。
    disabled: reactive.ReadSignal(bool),
    /// 轨道宽度（默认 40）。
    track_width: f32 = 40,
    /// 轨道高度（默认 22）。
    track_height: f32 = 22,
    /// 变化回调（context-carrying：首参为 user_data，可为 null）。
    /// 用 C 调用约定声明，与 C ABI 函数指针二进制兼容。
    on_change: ?*const fn (ctx: ?*anyopaque, checked: bool) callconv(.c) void = null,
    /// 回调上下文。
    on_change_ctx: ?*anyopaque = null,
};

// ─────────────────────────────────────────────────────────────────────────────
// Switch
// ─────────────────────────────────────────────────────────────────────────────

pub const Switch = struct {
    node: Node,
    allocator: Allocator,

    checked: reactive.ReadSignal(bool),
    color: reactive.ReadSignal(Color),
    disabled: reactive.ReadSignal(bool),
    track_width: f32,
    track_height: f32,
    on_change: ?*const fn (ctx: ?*anyopaque, checked: bool) callconv(.c) void = null,
    on_change_ctx: ?*anyopaque = null,

    hovered: bool = false,

    scope: reactive.EffectScope,

    const vtable = VTable{
        .measure = measureImpl,
        .paint = paintImpl,
        .handle_event = handleEventImpl,
        .deinit = deinitImpl,
    };

    pub fn create(
        allocator: Allocator,
        g: *reactive.Graph,
        props: SwitchProps,
    ) !*Switch {
        const self = try allocator.create(Switch);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .checked = props.checked,
            .color = props.color,
            .disabled = props.disabled,
            .track_width = props.track_width,
            .track_height = props.track_height,
            .on_change = props.on_change,
            .scope = reactive.EffectScope.init(g),
        };

        _ = try self.scope.add(self, struct {
            fn f(s: *Switch) void {
                _ = s.checked.get();
                _ = s.color.get();
                _ = s.disabled.get();
                s.node.markPaintDirty();
            }
        }.f);

        return self;
    }

    // ── vtable 实现 ──────────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *Switch = @fieldParentPtr("node", node);
        return constraints.constrain(.{ .width = self.track_width, .height = self.track_height });
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Switch = @fieldParentPtr("node", node);
        const b = node.bounds;
        const checked = self.checked.peek();
        const disabled = self.disabled.peek();
        const accent = self.color.peek();

        const track_h = b.height();
        const track_r = track_h / 2.0;
        const thumb_r = track_h * 0.38;
        const thumb_d = thumb_r * 2;

        // 轨道颜色
        const track_color = if (disabled)
            Color.fromHexRgb(0x444444)
        else if (checked)
            accent.withAlpha(0.5)
        else
            Color.fromHexRgb(0x444444);

        try scene.fillRoundedRect(b, track_r, track_color);

        // 圆钮位置
        const padding: f32 = 2.5;
        const thumb_y = b.top + padding;
        const thumb_x_on = b.right - padding - thumb_d;
        const thumb_x_off = b.left + padding;
        const thumb_x = if (checked) thumb_x_on else thumb_x_off;

        // 圆钮颜色
        const thumb_color = if (disabled)
            Color.fromHexRgb(0x666666)
        else if (checked)
            accent
        else
            Color.fromHexRgb(0xBBBBBB);

        const thumb_rect = Rect.fromXywh(thumb_x, thumb_y, thumb_d, thumb_d);
        try scene.fillRoundedRect(thumb_rect, thumb_r, thumb_color);
    }

    fn hitTest(b: Rect, x: f32, y: f32) bool {
        return x >= b.left and x <= b.right and y >= b.top and y <= b.bottom;
    }

    fn handleEventImpl(node: *Node, event: Event) EventResult {
        const self: *Switch = @fieldParentPtr("node", node);
        if (self.disabled.peek()) return .ignored;

        switch (event) {
            .pointer_move => |e| {
                const was = self.hovered;
                self.hovered = hitTest(node.bounds, e.x, e.y);
                if (was != self.hovered) self.node.markPaintDirty();
                return .ignored;
            },
            .pointer_down => |e| {
                if (hitTest(node.bounds, e.x, e.y)) {
                    if (self.on_change) |cb| cb(self.on_change_ctx, !self.checked.peek());
                    return .consumed;
                }
                return .ignored;
            },

            else => return .ignored,
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Switch = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Switch 创建与初始状态" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var checked_sig = reactive.Signal(bool).init(&g, false);
    defer checked_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0xA6E3A1));
    defer col_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, false);
    defer dis_sig.deinit();

    const sw = try Switch.create(allocator, &g, .{
        .checked = checked_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
    });
    defer sw.node.deinitTree(allocator);

    try std.testing.expect(!sw.checked.peek());
    try std.testing.expect(!sw.disabled.peek());
}

test "Switch pointer_down 不崩溃" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var checked_sig = reactive.Signal(bool).init(&g, false);
    defer checked_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0xA6E3A1));
    defer col_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, false);
    defer dis_sig.deinit();

    const sw = try Switch.create(allocator, &g, .{
        .checked = checked_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
    });
    defer sw.node.deinitTree(allocator);

    sw.node.setBounds(Rect.fromXywh(0, 0, 40, 22));
    _ = sw.node.vtable.handle_event(&sw.node, Event{ .pointer_down = .{ .button = .left, .x = 20, .y = 11 } });
    const result = sw.node.vtable.handle_event(&sw.node, Event{ .pointer_down = .{ .button = .left, .x = 200, .y = 200 } });
    try std.testing.expectEqual(EventResult.ignored, result);
}
