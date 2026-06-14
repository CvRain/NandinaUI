//! widgets/button —— 按钮组件（M5 响应式版）
//!
//! Button 是一个自包含的复合组件：绘制圆角背景（根据 hover/pressed/disabled 状态选色）、
//! 在内容区绘制文本标签，并处理指针事件（hover / press / release / click）。
//!
//! 设计原则：
//! - 输入属性（label/颜色/尺寸等）全部通过 `ReadSignal` 接收，由调用方持有后备 Signal。
//! - 内部状态（hovered/pressed）用 `Signal` 自持，对外通过 `hoveredSignal()`/`pressedSignal()`
//!   暴露只读视图。
//! - `EffectScope` 追踪所有输入与内部状态信号，任一变化时 `markLayoutDirty`。
//! - 回调（on_click/on_press/on_release）为普通函数指针，调用方可选填。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / render / runtime / text。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const layout = @import("../layout/layout.zig");
const render = @import("../render/render.zig");
const runtime = @import("../runtime/runtime.zig");
const text_mod = @import("../text/text.zig");

const Node = runtime.Node;
const VTable = runtime.VTable;
const EventResult = runtime.EventResult;
const Event = runtime.Event;
const PointerButton = runtime.PointerButton;
const Constraints = layout.Constraints;
const Scene = render.Scene;

// ── 公共类型 ─────────────────────────────────────────────────────────────────

/// Button 的输入属性，全部为只读信号，由调用方持有后备 Signal 的生命周期。
pub const ButtonProps = struct {
    label: reactive.ReadSignal([]const u8),
    bg_color: reactive.ReadSignal(foundation.Color),
    bg_hover_color: reactive.ReadSignal(foundation.Color),
    bg_pressed_color: reactive.ReadSignal(foundation.Color),
    text_color: reactive.ReadSignal(foundation.Color),
    font_size: reactive.ReadSignal(f32),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(foundation.Insets),
    disabled: reactive.ReadSignal(bool),
};

/// 按钮组件。
pub const Button = struct {
    node: Node,
    allocator: std.mem.Allocator,

    // 输入属性（只读信号，调用方持有后备 Signal）
    label: reactive.ReadSignal([]const u8),
    bg_color: reactive.ReadSignal(foundation.Color),
    bg_hover_color: reactive.ReadSignal(foundation.Color),
    bg_pressed_color: reactive.ReadSignal(foundation.Color),
    text_color: reactive.ReadSignal(foundation.Color),
    font_size: reactive.ReadSignal(f32),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(foundation.Insets),
    disabled: reactive.ReadSignal(bool),

    // 内部状态（自持 Signal，deinit 时释放）
    hovered: reactive.Signal(bool),
    pressed: reactive.Signal(bool),

    // 响应式作用域
    scope: reactive.EffectScope,

    // 可选回调
    on_click: ?*const fn () void = null,
    on_press: ?*const fn () void = null,
    on_release: ?*const fn () void = null,

    // ── vtable ────────────────────────────────────────────────────────────────

    const vtable = VTable{
        .measure = measureImpl,
        .paint = paintImpl,
        .handle_event = handleEventImpl,
        .deinit = deinitImpl,
    };

    // ── 构造 ──────────────────────────────────────────────────────────────────

    pub fn create(
        allocator: std.mem.Allocator,
        g: *reactive.Graph,
        props: ButtonProps,
    ) !*Button {
        const self = try allocator.create(Button);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .label = props.label,
            .bg_color = props.bg_color,
            .bg_hover_color = props.bg_hover_color,
            .bg_pressed_color = props.bg_pressed_color,
            .text_color = props.text_color,
            .font_size = props.font_size,
            .corner_radius = props.corner_radius,
            .padding = props.padding,
            .disabled = props.disabled,
            .hovered = reactive.Signal(bool).init(g, false),
            .pressed = reactive.Signal(bool).init(g, false),
            .scope = reactive.EffectScope.init(g),
        };
        _ = try self.scope.add(self, struct {
            fn f(s: *Button) void {
                // 建立对所有输入信号和内部状态的依赖追踪
                _ = s.label.get();
                _ = s.bg_color.get();
                _ = s.bg_hover_color.get();
                _ = s.bg_pressed_color.get();
                _ = s.text_color.get();
                _ = s.font_size.get();
                _ = s.corner_radius.get();
                _ = s.padding.get();
                _ = s.disabled.get();
                _ = s.hovered.get();
                _ = s.pressed.get();
                s.node.markLayoutDirty();
            }
        }.f);
        return self;
    }

    // ── 只读状态访问器 ────────────────────────────────────────────────────────

    /// 返回 hovered 状态的只读信号视图。
    pub fn hoveredSignal(self: *Button) reactive.ReadSignal(bool) {
        return self.hovered.asReadonly();
    }

    /// 返回 pressed 状态的只读信号视图。
    pub fn pressedSignal(self: *Button) reactive.ReadSignal(bool) {
        return self.pressed.asReadonly();
    }

    // ── vtable 实现 ───────────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: Constraints) foundation.Size {
        const self: *Button = @fieldParentPtr("node", node);
        const pad = self.padding.peek();
        const fs = self.font_size.peek();
        const txt = self.label.peek();

        // 测量文本自然尺寸（单行，无界宽度）
        var m = text_mod.MonospaceMetrics{};
        const fm = m.interface();
        const style = text_mod.TextStyle{ .font_size = fs };
        var tl = text_mod.measure(
            self.allocator,
            txt,
            style,
            .ellipsis,
            .{ .max_width = 0 }, // 无界
            fm,
        ) catch {
            // 分配失败时退回到 padding 大小
            return constraints.constrain(.{
                .width = pad.horizontal(),
                .height = pad.vertical(),
            });
        };
        defer tl.deinit();

        const natural = foundation.Size{
            .width = tl.size.width + pad.horizontal(),
            .height = tl.size.height + pad.vertical(),
        };
        return constraints.constrain(natural);
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Button = @fieldParentPtr("node", node);

        // 根据状态选择背景色
        const bg = if (self.disabled.peek())
            self.bg_color.peek()
        else if (self.pressed.peek())
            self.bg_pressed_color.peek()
        else if (self.hovered.peek())
            self.bg_hover_color.peek()
        else
            self.bg_color.peek();

        // 绘制圆角背景
        try scene.fillRoundedRect(node.bounds, self.corner_radius.peek(), bg);

        // 绘制文本（左上角偏移 padding）
        const txt = self.label.peek();
        if (txt.len > 0) {
            const pad = self.padding.peek();
            try scene.drawText(.{
                .text = txt,
                .x = node.bounds.left + pad.left,
                .y = node.bounds.top + pad.top,
                .font_size = self.font_size.peek(),
                .color = self.text_color.peek(),
                .layout_width = @max(0, node.bounds.right - node.bounds.left - pad.horizontal()),
                .layout_height = @max(0, node.bounds.bottom - node.bounds.top - pad.vertical()),
            });
        }
    }

    fn handleEventImpl(node: *Node, ev: Event) EventResult {
        const self: *Button = @fieldParentPtr("node", node);

        if (self.disabled.peek()) return .ignored;

        switch (ev) {
            .pointer_move => {
                if (!self.hovered.peek()) {
                    self.hovered.set(true);
                }
                return .consumed;
            },
            .pointer_down => |e| {
                if (e.button == .left) {
                    self.pressed.set(true);
                    if (self.on_press) |cb| cb();
                    return .consumed;
                }
                return .ignored;
            },
            .pointer_up => |e| {
                if (e.button == .left) {
                    const was_pressed = self.pressed.peek();
                    self.pressed.set(false);
                    if (self.on_release) |cb| cb();
                    if (was_pressed and self.hovered.peek()) {
                        if (self.on_click) |cb| cb();
                    }
                    return .consumed;
                }
                return .ignored;
            },
            .focus_out => {
                self.hovered.set(false);
                self.pressed.set(false);
                return .consumed;
            },
            else => return .ignored,
        }
    }

    fn deinitImpl(node: *Node, allocator: std.mem.Allocator) void {
        const self: *Button = @fieldParentPtr("node", node);
        self.scope.deinit();
        self.hovered.deinit();
        self.pressed.deinit();
        // ReadSignal 无需 deinit（不拥有后备 Signal）
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Button 创建与基本绘制" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "Click me");
    defer label_sig.deinit();
    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var bg_hover_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bg_hover_sig.deinit();
    var bg_pressed_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bg_pressed_sig.deinit();
    var text_color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer text_color_sig.deinit();
    var font_size_sig = reactive.Signal(f32).init(&g, 14.0);
    defer font_size_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 4.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.all(8));
    defer pad_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const btn = try Button.create(a, &g, .{
        .label = label_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .bg_hover_color = bg_hover_sig.asReadonly(),
        .bg_pressed_color = bg_pressed_sig.asReadonly(),
        .text_color = text_color_sig.asReadonly(),
        .font_size = font_size_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
    });
    defer btn.node.deinitTree(a);

    btn.node.setBounds(foundation.Rect.fromXywh(0, 0, 120, 40));

    var scene = render.Scene.init(a);
    defer scene.deinit();
    try btn.node.paint(&scene);

    // 期望：1 条圆角矩形 + 1 条文本命令
    try std.testing.expectEqual(@as(usize, 2), scene.count());
    try std.testing.expect(scene.commands.items[0] == .fill_rounded_rect);
    try std.testing.expect(scene.commands.items[1] == .draw_text);
}

test "Button label 信号变化 → markLayoutDirty" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "OK");
    defer label_sig.deinit();
    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var bg_hover_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_hover_sig.deinit();
    var bg_pressed_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_pressed_sig.deinit();
    var text_color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer text_color_sig.deinit();
    var font_size_sig = reactive.Signal(f32).init(&g, 14.0);
    defer font_size_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 0.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.zero);
    defer pad_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const btn = try Button.create(a, &g, .{
        .label = label_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .bg_hover_color = bg_hover_sig.asReadonly(),
        .bg_pressed_color = bg_pressed_sig.asReadonly(),
        .text_color = text_color_sig.asReadonly(),
        .font_size = font_size_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
    });
    defer btn.node.deinitTree(a);

    // 清除 dirty（create 时 effect 首次运行已标记）
    btn.node.layout_dirty = false;
    btn.node.paint_dirty = false;

    // label 使用 update（切片不支持 == 比较，signal 总是通知；用 set 也可以）
    label_sig.update(struct {
        fn f(v: *[]const u8) void {
            v.* = "Cancel";
        }
    }.f);
    try std.testing.expect(btn.node.layout_dirty);
}

test "Button hovered 信号变化 → markLayoutDirty" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "OK");
    defer label_sig.deinit();
    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var bg_hover_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bg_hover_sig.deinit();
    var bg_pressed_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bg_pressed_sig.deinit();
    var text_color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer text_color_sig.deinit();
    var font_size_sig = reactive.Signal(f32).init(&g, 14.0);
    defer font_size_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 0.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.zero);
    defer pad_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const btn = try Button.create(a, &g, .{
        .label = label_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .bg_hover_color = bg_hover_sig.asReadonly(),
        .bg_pressed_color = bg_pressed_sig.asReadonly(),
        .text_color = text_color_sig.asReadonly(),
        .font_size = font_size_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
    });
    defer btn.node.deinitTree(a);

    btn.node.setBounds(foundation.Rect.fromXywh(0, 0, 100, 40));

    // 清除 dirty
    btn.node.layout_dirty = false;
    btn.node.paint_dirty = false;

    // 模拟 pointer_move → hovered 变为 true → effect 重跑 → markLayoutDirty
    const result = btn.node.vtable.handle_event(&btn.node, .{ .pointer_move = .{ .x = 10, .y = 10 } });
    try std.testing.expect(result == .consumed);
    try std.testing.expect(btn.hovered.peek());
    try std.testing.expect(btn.node.layout_dirty);
}

test "Button 点击回调触发" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "OK");
    defer label_sig.deinit();
    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var bg_hover_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bg_hover_sig.deinit();
    var bg_pressed_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bg_pressed_sig.deinit();
    var text_color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer text_color_sig.deinit();
    var font_size_sig = reactive.Signal(f32).init(&g, 14.0);
    defer font_size_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 0.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.zero);
    defer pad_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const btn = try Button.create(a, &g, .{
        .label = label_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .bg_hover_color = bg_hover_sig.asReadonly(),
        .bg_pressed_color = bg_pressed_sig.asReadonly(),
        .text_color = text_color_sig.asReadonly(),
        .font_size = font_size_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
    });
    defer btn.node.deinitTree(a);

    btn.node.setBounds(foundation.Rect.fromXywh(0, 0, 100, 40));

    // 设置点击回调
    const Ctx = struct {
        var clicked: bool = false;
        fn onClick() void {
            clicked = true;
        }
    };
    Ctx.clicked = false;
    btn.on_click = &Ctx.onClick;

    // 模拟完整点击序列：move → down → up
    _ = btn.node.vtable.handle_event(&btn.node, .{ .pointer_move = .{ .x = 10, .y = 10 } });
    _ = btn.node.vtable.handle_event(&btn.node, .{ .pointer_down = .{ .button = .left, .x = 10, .y = 10 } });
    _ = btn.node.vtable.handle_event(&btn.node, .{ .pointer_up = .{ .button = .left, .x = 10, .y = 10 } });

    try std.testing.expect(Ctx.clicked);
}

test "Button disabled 时忽略事件" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "OK");
    defer label_sig.deinit();
    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var bg_hover_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_hover_sig.deinit();
    var bg_pressed_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_pressed_sig.deinit();
    var text_color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer text_color_sig.deinit();
    var font_size_sig = reactive.Signal(f32).init(&g, 14.0);
    defer font_size_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 0.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.zero);
    defer pad_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, true); // 禁用
    defer disabled_sig.deinit();

    const btn = try Button.create(a, &g, .{
        .label = label_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .bg_hover_color = bg_hover_sig.asReadonly(),
        .bg_pressed_color = bg_pressed_sig.asReadonly(),
        .text_color = text_color_sig.asReadonly(),
        .font_size = font_size_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
    });
    defer btn.node.deinitTree(a);

    const result = btn.node.vtable.handle_event(&btn.node, .{ .pointer_down = .{ .button = .left, .x = 10, .y = 10 } });
    try std.testing.expect(result == .ignored);
    try std.testing.expect(!btn.pressed.peek());
}

test "Button measureImpl 包含文本尺寸与 padding" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "AB");
    defer label_sig.deinit();
    var bg_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_sig.deinit();
    var bg_hover_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_hover_sig.deinit();
    var bg_pressed_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg_pressed_sig.deinit();
    var text_color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer text_color_sig.deinit();
    var font_size_sig = reactive.Signal(f32).init(&g, 14.0);
    defer font_size_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 4.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.all(8));
    defer pad_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();

    const btn = try Button.create(a, &g, .{
        .label = label_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .bg_hover_color = bg_hover_sig.asReadonly(),
        .bg_pressed_color = bg_pressed_sig.asReadonly(),
        .text_color = text_color_sig.asReadonly(),
        .font_size = font_size_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
    });
    defer btn.node.deinitTree(a);

    // loose 约束：max 500×500，min 0×0
    const size = btn.node.measure(Constraints.loose(500, 500));

    // 宽度必须 > 水平 padding（包含文本宽度）
    try std.testing.expect(size.width > 16.0);
    // 高度必须 = font_size + vertical padding = 14 + 16 = 30
    try std.testing.expectApproxEqAbs(30.0, size.height, 0.01);
}
