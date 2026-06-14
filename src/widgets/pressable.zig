//! widgets/pressable —— 交互状态机组件
//!
//! Pressable 封装 hovered / pressed / focused 三个内部交互状态（Signal）
//! 以及一个来自外部的 disabled 只读输入（ReadSignal）。
//!
//! 设计原则（M5 响应式接入）：
//!   - disabled 作为输入，由调用方持有背后的 Signal，Pressable 持有只读视图 ReadSignal。
//!   - hovered / pressed / focused 是 Pressable 自身 **拥有** 的 Signal(bool)，
//!     外部代码通过 `hoveredSignal()` 等访问器取到 ReadSignal 来订阅状态变化。
//!   - EffectScope 管理一条 effect：disabled 变化 → markPaintDirty。
//!
//! 事件处理（参考 archive/widgets/src/nan_pressable.cppm）：
//!   pointer_move  → hovered = true
//!   pointer_down  → pressed = true（left 键）
//!   pointer_up    → pressed = false；若 hovered 则触发 on_click
//!   focus_in      → focused = true
//!   focus_out     → focused / hovered / pressed = false
//!   disabled = true 时忽略所有事件

const std = @import("std");
const reactive = @import("../reactive/reactive.zig");
const runtime = @import("../runtime/runtime.zig");

const Allocator = std.mem.Allocator;
const Node = runtime.Node;
const Event = runtime.Event;
const EventResult = runtime.EventResult;

/// 创建 Pressable 时由调用方传入的属性。
pub const PressableProps = struct {
    /// disabled 由调用方控制；Pressable 持有只读视图，不拥有背后的 Signal。
    disabled: reactive.ReadSignal(bool),
};

/// 交互状态机组件：无自绘，只维护 hovered / pressed / focused 信号。
pub const Pressable = struct {
    node: Node,
    allocator: Allocator,

    // ── 输入（调用方持有背后 Signal，Pressable 只读） ──────────────────
    disabled: reactive.ReadSignal(bool),

    // ── 内部状态（Pressable 拥有；外部通过 .asReadonly() 访问器订阅） ───
    hovered: reactive.Signal(bool),
    pressed: reactive.Signal(bool),
    focused: reactive.Signal(bool),

    // ── 响应式作用域 ────────────────────────────────────────────────────
    scope: reactive.EffectScope,

    // ── 用户回调（plain fn pointer，无响应式） ──────────────────────────
    on_click: ?*const fn () void = null,
    on_press: ?*const fn () void = null,
    on_release: ?*const fn () void = null,
    on_hover_change: ?*const fn (bool) void = null,

    const vtable = runtime.VTable{
        .measure = measureImpl,
        .handle_event = handleEventImpl,
        .deinit = deinitImpl,
    };

    /// 创建并初始化一个 Pressable 节点。
    ///
    /// - `g`：响应式调度图，所有内部 Signal / Effect 挂在此图上。
    /// - `props`：包含 disabled ReadSignal（调用方所有权）。
    ///
    /// 返回值由 `allocator` 管理；通过 `node.deinitTree(allocator)` 释放。
    pub fn create(
        allocator: Allocator,
        g: *reactive.Graph,
        props: PressableProps,
    ) !*Pressable {
        const self = try allocator.create(Pressable);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .disabled = props.disabled,
            .hovered = reactive.Signal(bool).init(g, false),
            .pressed = reactive.Signal(bool).init(g, false),
            .focused = reactive.Signal(bool).init(g, false),
            .scope = reactive.EffectScope.init(g),
        };
        // Effect：disabled 变化时通知需要重绘（视觉状态随之改变）。
        _ = try self.scope.add(self, struct {
            fn f(p: *Pressable) void {
                _ = p.disabled.get(); // 建立依赖
                p.node.markPaintDirty();
            }
        }.f);
        return self;
    }

    // ── 外部访问器（只读视图） ───────────────────────────────────────────

    pub fn hoveredSignal(self: *Pressable) reactive.ReadSignal(bool) {
        return self.hovered.asReadonly();
    }

    pub fn pressedSignal(self: *Pressable) reactive.ReadSignal(bool) {
        return self.pressed.asReadonly();
    }

    pub fn focusedSignal(self: *Pressable) reactive.ReadSignal(bool) {
        return self.focused.asReadonly();
    }

    // ── vtable 实现 ──────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: @import("../layout/layout.zig").Constraints) @import("../foundation/foundation.zig").Size {
        // Pressable 无自身尺寸，返回子节点中最大尺寸，或约束最小值。
        var measured = @import("../foundation/foundation.zig").Size{};
        for (node.children.items) |child| {
            const sz = child.measure(constraints);
            if (sz.width > measured.width) measured.width = sz.width;
            if (sz.height > measured.height) measured.height = sz.height;
        }
        return constraints.constrain(measured);
    }

    fn handleEventImpl(node: *Node, ev: Event) EventResult {
        const self: *Pressable = @fieldParentPtr("node", node);

        // disabled 状态下忽略所有交互事件
        if (self.disabled.peek()) return .ignored;

        switch (ev) {
            .pointer_move => {
                const was_hovered = self.hovered.peek();
                self.hovered.set(true);
                if (!was_hovered) {
                    if (self.on_hover_change) |cb| cb(true);
                    node.markPaintDirty();
                }
                return .consumed;
            },
            .pointer_down => |e| {
                if (e.button != .left) return .ignored;
                self.pressed.set(true);
                if (self.on_press) |cb| cb();
                node.markPaintDirty();
                return .consumed;
            },
            .pointer_up => |e| {
                if (e.button != .left) return .ignored;
                const was_pressed = self.pressed.peek();
                self.pressed.set(false);
                if (was_pressed) {
                    if (self.on_release) |cb| cb();
                    // 释放时仍 hovered → click
                    if (self.hovered.peek()) {
                        if (self.on_click) |cb| cb();
                    }
                }
                node.markPaintDirty();
                return .consumed;
            },
            .focus_in => {
                self.focused.set(true);
                node.markPaintDirty();
                return .consumed;
            },
            .focus_out => {
                self.focused.set(false);
                const was_hovered = self.hovered.peek();
                self.hovered.set(false);
                self.pressed.set(false);
                if (was_hovered) {
                    if (self.on_hover_change) |cb| cb(false);
                }
                node.markPaintDirty();
                return .consumed;
            },
            else => return .ignored,
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Pressable = @fieldParentPtr("node", node);
        self.scope.deinit(); // 1. dispose effects（解绑 disabled 依赖边）
        self.hovered.deinit(); // 2. 释放内部 Signal（顺序无关）
        self.pressed.deinit();
        self.focused.deinit();
        // disabled 是 ReadSignal，不 deinit
        allocator.destroy(self); // 3. 释放节点内存
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Pressable create 和 deinit 无内存泄漏" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    p.node.deinitTree(a);
}

test "pointer_move 设置 hovered Signal" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    try std.testing.expect(!p.hoveredSignal().peek());
    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_move = .{ .x = 5, .y = 5 } });
    try std.testing.expect(p.hoveredSignal().peek());
}

test "pointer_down / pointer_up 设置 pressed Signal" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    // 先设置 hovered（pointer_move）
    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_move = .{ .x = 5, .y = 5 } });
    // pointer_down → pressed = true
    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_down = .{ .button = .left, .x = 5, .y = 5 } });
    try std.testing.expect(p.pressedSignal().peek());
    // pointer_up → pressed = false
    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_up = .{ .button = .left, .x = 5, .y = 5 } });
    try std.testing.expect(!p.pressedSignal().peek());
}

// 全局计数器用于 on_click 回调测试（Zig fn pointer 无闭包，需要文件级变量）
var g_click_count: u32 = 0;
fn countClick() void {
    g_click_count += 1;
}

test "on_click 在 hovered+pressed 后 pointer_up 触发" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    g_click_count = 0;
    p.on_click = countClick;

    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_move = .{ .x = 5, .y = 5 } });
    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_down = .{ .button = .left, .x = 5, .y = 5 } });
    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_up = .{ .button = .left, .x = 5, .y = 5 } });
    try std.testing.expectEqual(@as(u32, 1), g_click_count);
}

test "focus_in / focus_out 设置 focused Signal" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    try std.testing.expect(!p.focusedSignal().peek());
    _ = p.node.vtable.handle_event(&p.node, .{ .focus_in = .{ .gained = true } });
    try std.testing.expect(p.focusedSignal().peek());
    _ = p.node.vtable.handle_event(&p.node, .{ .focus_out = .{ .gained = false } });
    try std.testing.expect(!p.focusedSignal().peek());
}

test "focus_out 同时清除 hovered 和 pressed" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_move = .{ .x = 1, .y = 1 } });
    _ = p.node.vtable.handle_event(&p.node, .{ .pointer_down = .{ .button = .left, .x = 1, .y = 1 } });
    try std.testing.expect(p.hoveredSignal().peek());
    try std.testing.expect(p.pressedSignal().peek());

    _ = p.node.vtable.handle_event(&p.node, .{ .focus_out = .{ .gained = false } });
    try std.testing.expect(!p.hoveredSignal().peek());
    try std.testing.expect(!p.pressedSignal().peek());
}

test "disabled=true 时事件被忽略" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, true); // 初始 disabled
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    const result = p.node.vtable.handle_event(&p.node, .{ .pointer_move = .{ .x = 5, .y = 5 } });
    try std.testing.expectEqual(EventResult.ignored, result);
    try std.testing.expect(!p.hoveredSignal().peek());
}

test "disabled Signal 变化 → paint_dirty 被设置" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    // create 时 effect 已跑一次，paint_dirty = true；手动清掉
    p.node.paint_dirty = false;

    // 改变 disabled → effect 重跑 → markPaintDirty
    disabled_sig.set(true);
    try std.testing.expect(p.node.paint_dirty);
}

test "hoveredSignal / pressedSignal / focusedSignal 返回只读视图" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const p = try Pressable.create(a, &g, .{ .disabled = disabled_sig.asReadonly() });
    defer p.node.deinitTree(a);

    // 初始均为 false
    try std.testing.expect(!p.hoveredSignal().peek());
    try std.testing.expect(!p.pressedSignal().peek());
    try std.testing.expect(!p.focusedSignal().peek());

    // 触发事件后视图反映新值
    _ = p.node.vtable.handle_event(&p.node, .{ .focus_in = .{ .gained = true } });
    try std.testing.expect(p.focusedSignal().peek());
}
