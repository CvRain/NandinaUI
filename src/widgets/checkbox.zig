//! widgets/checkbox —— 复选框组件
//!
//! `Checkbox` 是布尔勾选控件：正方形选框 + 内部勾选标记。
//! 标签由外部通过 `Row + Label` 组合，保持 primitive 单一职责。
//!
//! 使用示例：
//! ```zig
//! var checked_sig = reactive.Signal(bool).init(&g, false);
//! defer checked_sig.deinit();
//!
//! const cb = try Checkbox.create(allocator, &g, .{
//!     .checked = checked_sig.asReadonly(),
//!     .color = readOnly(owner, Color, &g, Color.blue),
//! });
//! defer cb.node.deinitTree(allocator);
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

pub const CheckboxProps = struct {
    /// 勾选状态（只读信号，由调用方持有 Signal 所有权）。
    checked: reactive.ReadSignal(bool),
    /// 主题色（边框/填充色）。
    color: reactive.ReadSignal(Color),
    /// 禁用状态。
    disabled: reactive.ReadSignal(bool),
    /// 尺寸（默认 18x18）。
    size: f32 = 18,
    /// 勾选变化回调。
    on_change: ?*const fn (checked: bool) void = null,
};

// ─────────────────────────────────────────────────────────────────────────────
// Checkbox
// ─────────────────────────────────────────────────────────────────────────────

pub const Checkbox = struct {
    node: Node,
    allocator: Allocator,

    checked: reactive.ReadSignal(bool),
    color: reactive.ReadSignal(Color),
    disabled: reactive.ReadSignal(bool),
    size: f32,
    on_change: ?*const fn (checked: bool) void = null,

    // 内部交互状态
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
        props: CheckboxProps,
    ) !*Checkbox {
        const self = try allocator.create(Checkbox);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .checked = props.checked,
            .color = props.color,
            .disabled = props.disabled,
            .size = props.size,
            .on_change = props.on_change,
            .scope = reactive.EffectScope.init(g),
        };

        _ = try self.scope.add(self, struct {
            fn f(s: *Checkbox) void {
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
        const self: *Checkbox = @fieldParentPtr("node", node);
        return constraints.constrain(.{ .width = self.size, .height = self.size });
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Checkbox = @fieldParentPtr("node", node);
        const b = node.bounds;
        const sz = @min(b.width(), b.height());
        const r = 3.0; // 圆角
        const checked = self.checked.peek();
        const disabled = self.disabled.peek();

        const accent = self.color.peek();
        const border_c = if (disabled)
            Color.fromHexRgb(0x666666)
        else if (self.hovered)
            accent.withAlpha(0.8)
        else
            accent;
        const fill_c = if (checked) accent else Color.transparent;

        // 选框背景
        try scene.fillRoundedRect(b, r, if (checked) fill_c else Color.fromHexRgb(0x1A1A2E));

        // 选框描边
        const stroke_w: f32 = 1.5;
        try scene.fillRoundedRect(b, r, border_c); // 用 fill 模拟描边...

        // 实际：绘制两个重叠的圆角矩形模拟空心描边
        // 简化：先画填色背景，再画内部镂空
        if (!checked) {
            const inner = Rect.fromXywh(
                b.left + stroke_w,
                b.top + stroke_w,
                b.width() - stroke_w * 2,
                b.height() - stroke_w * 2,
            );
            try scene.fillRoundedRect(inner, r - 0.5, Color.fromHexRgb(0x1E1E2E));
        }

        // 勾选标记 ✓（使用 drawText 绘制）
        if (checked) {
            try scene.drawText(.{
                .text = "✓",
                .x = b.left + sz * 0.15,
                .y = b.top + sz * 0.02,
                .font_size = sz * 0.85,
                .color = Color.white,
            });
        }
    }

    fn hitTest(b: Rect, x: f32, y: f32) bool {
        return x >= b.left and x <= b.right and y >= b.top and y <= b.bottom;
    }

    fn handleEventImpl(node: *Node, event: Event) EventResult {
        const self: *Checkbox = @fieldParentPtr("node", node);
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
                    if (self.on_change) |cb| cb(!self.checked.peek());
                    return .consumed;
                }
                return .ignored;
            },
            else => return .ignored,
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Checkbox = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Checkbox 创建与初始状态" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var checked_sig = reactive.Signal(bool).init(&g, false);
    defer checked_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x89B4FA));
    defer col_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, false);
    defer dis_sig.deinit();

    const cb = try Checkbox.create(allocator, &g, .{
        .checked = checked_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
    });
    defer cb.node.deinitTree(allocator);

    try std.testing.expect(!cb.checked.peek());
    try std.testing.expect(!cb.disabled.peek());
}

test "Checkbox pointer_down 不崩溃" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var checked_sig = reactive.Signal(bool).init(&g, false);
    defer checked_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x89B4FA));
    defer col_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, false);
    defer dis_sig.deinit();

    const cb = try Checkbox.create(allocator, &g, .{
        .checked = checked_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
    });
    defer cb.node.deinitTree(allocator);

    cb.node.setBounds(Rect.fromXywh(0, 0, 18, 18));
    // 点击在选框范围内 — 不应崩溃
    _ = cb.node.vtable.handle_event(&cb.node, Event{ .pointer_down = .{ .button = .left, .x = 9, .y = 9 } });
    // 点击在选框范围外 — 应忽略
    const result = cb.node.vtable.handle_event(&cb.node, Event{ .pointer_down = .{ .button = .left, .x = 200, .y = 200 } });
    try std.testing.expectEqual(EventResult.ignored, result);
}

test "Checkbox disabled 时不响应" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var checked_sig = reactive.Signal(bool).init(&g, false);
    defer checked_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x89B4FA));
    defer col_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, true);
    defer dis_sig.deinit();

    const cb = try Checkbox.create(allocator, &g, .{
        .checked = checked_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
    });
    defer cb.node.deinitTree(allocator);

    cb.node.setBounds(Rect.fromXywh(0, 0, 18, 18));
    // disabled 时不处理任何事件
    const r1 = cb.node.vtable.handle_event(&cb.node, Event{ .pointer_down = .{ .button = .left, .x = 9, .y = 9 } });
    try std.testing.expectEqual(EventResult.ignored, r1);
    const r2 = cb.node.vtable.handle_event(&cb.node, Event{ .pointer_move = .{ .x = 9, .y = 9 } });
    try std.testing.expectEqual(EventResult.ignored, r2);
}
