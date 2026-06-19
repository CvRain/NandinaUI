//! widgets/field —— 语义表单容器
//!
//! `Field` 组合 label + 控件 + helper/error 消息，为表单提供一致的语义容器。
//! 控件通过 `addChild` 挂入，Field 管理布局与样式状态（normal/disabled/invalid/required）。
//!
//! 使用示例：
//! ```zig
//! const field = try Field.create(allocator, &g, .{ ... });
//! const tf = try TextField.create(allocator, &g, .{ ... });
//! try field.node.addChild(allocator, &tf.node);
//! ```

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const layout = @import("../layout/layout.zig");
const render = @import("../render/render.zig");
const runtime = @import("../runtime/runtime.zig");
const text_mod = @import("../text/text.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Insets = foundation.Insets;
const Rect = foundation.Rect;
const Size = foundation.Size;
const Constraints = layout.Constraints;
const Scene = render.Scene;
const Node = runtime.Node;
const VTable = runtime.VTable;

// ── 公共类型 ─────────────────────────────────────────────────────────────────

/// Field 的构造属性。
pub const FieldProps = struct {
    /// 标签文本（只读信号）。
    label: reactive.ReadSignal([]const u8),
    /// 辅助文本（显示在控件下方，error 为空时显示）。
    helper: reactive.ReadSignal([]const u8),
    /// 错误文本（不为空时优先显示，覆盖 helper）。
    error_text: reactive.ReadSignal([]const u8),
    /// 是否必填（显示 * 指示器）。
    required: reactive.ReadSignal(bool),
    /// 是否无效状态（红色边框 / 文本）。
    invalid: reactive.ReadSignal(bool),
    /// 是否禁用。
    disabled: reactive.ReadSignal(bool),
    /// 标签颜色。
    label_color: reactive.ReadSignal(Color),
    /// 辅助文本颜色。
    helper_color: reactive.ReadSignal(Color),
    /// 错误颜色。
    error_color: reactive.ReadSignal(Color),
    /// 标签字体大小。
    label_font_size: reactive.ReadSignal(f32),
    /// 消息字体大小。
    message_font_size: reactive.ReadSignal(f32),
    /// 容器内边距（默认四周 8px）。
    padding: Insets = Insets.all(8),
    /// 标签与控件间距。
    gap: f32 = 4,
};

// ─────────────────────────────────────────────────────────────────────────────
// Field
// ─────────────────────────────────────────────────────────────────────────────

pub const Field = struct {
    node: Node,
    allocator: Allocator,

    // 响应式输入（只读视图）
    label: reactive.ReadSignal([]const u8),
    helper: reactive.ReadSignal([]const u8),
    error_text: reactive.ReadSignal([]const u8),
    required: reactive.ReadSignal(bool),
    invalid: reactive.ReadSignal(bool),
    disabled: reactive.ReadSignal(bool),
    label_color: reactive.ReadSignal(Color),
    helper_color: reactive.ReadSignal(Color),
    error_color: reactive.ReadSignal(Color),
    label_font_size: reactive.ReadSignal(f32),
    message_font_size: reactive.ReadSignal(f32),

    // 普通属性
    padding: Insets,
    gap: f32,

    // 响应式作用域
    scope: reactive.EffectScope,

    const vtable = VTable{
        .measure = measureImpl,
        .layout = layoutImpl,
        .paint = paintImpl,
        .deinit = deinitImpl,
    };

    pub fn create(
        allocator: Allocator,
        g: *reactive.Graph,
        props: FieldProps,
    ) !*Field {
        const self = try allocator.create(Field);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .label = props.label,
            .helper = props.helper,
            .error_text = props.error_text,
            .required = props.required,
            .invalid = props.invalid,
            .disabled = props.disabled,
            .label_color = props.label_color,
            .helper_color = props.helper_color,
            .error_color = props.error_color,
            .label_font_size = props.label_font_size,
            .message_font_size = props.message_font_size,
            .padding = props.padding,
            .gap = props.gap,
            .scope = reactive.EffectScope.init(g),
        };

        _ = try self.scope.add(self, struct {
            fn f(s: *Field) void {
                _ = s.label.get();
                _ = s.helper.get();
                _ = s.error_text.get();
                _ = s.required.get();
                _ = s.invalid.get();
                _ = s.disabled.get();
                _ = s.label_color.get();
                _ = s.helper_color.get();
                _ = s.error_color.get();
                _ = s.label_font_size.get();
                _ = s.message_font_size.get();
                s.node.markLayoutDirty();
            }
        }.f);

        return self;
    }

    // ── vtable 实现 ──────────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *Field = @fieldParentPtr("node", node);
        const pad = self.padding;
        const label_fs = self.label_font_size.peek();
        const msg_fs = self.message_font_size.peek();

        var total_w: f32 = pad.horizontal();
        var total_h: f32 = pad.vertical();

        // 标签行
        const label_text = self.label.peek();
        if (label_text.len > 0) {
            total_h += label_fs * 1.4;
            total_h += self.gap;
            var m = text_mod.MonospaceMetrics{};
            const fm = m.interface();
            const style = text_mod.TextStyle{ .font_size = label_fs };
            var tl = text_mod.measure(self.allocator, label_text, style, .ellipsis, .{ .max_width = 0 }, fm) catch {
                return constraints.constrain(.{ .width = total_w, .height = total_h });
            };
            defer tl.deinit();
            total_w = @max(total_w, tl.size.width + pad.horizontal());
        }

        // 必填指示器
        if (self.required.peek()) {
            total_w += label_fs * 0.6 + 4;
        }

        // 控件（第一个子节点）
        if (node.children.items.len > 0) {
            const child = node.children.items[0];
            const child_sz = child.measure(constraints);
            total_w = @max(total_w, child_sz.width + pad.horizontal());
            total_h += child_sz.height;
        } else {
            total_h += label_fs * 1.4; // 至少留一行空间
        }

        // 消息行（helper 或 error）
        const err = self.error_text.peek();
        const hlp = self.helper.peek();
        const msg = if (err.len > 0) err else hlp;
        if (msg.len > 0) {
            total_h += self.gap;
            total_h += msg_fs * 1.4;
        }

        return constraints.constrain(.{ .width = total_w, .height = total_h });
    }

    fn layoutImpl(node: *Node) void {
        const self: *Field = @fieldParentPtr("node", node);
        const pad = self.padding;
        const label_fs = self.label_font_size.peek();
        const b = node.bounds;

        var y: f32 = b.top + pad.top;

        // 标签行占位（实际绘制在 paint 中）
        if (self.label.peek().len > 0) {
            y += label_fs * 1.4 + self.gap;
        }

        // 控件布局
        if (node.children.items.len > 0) {
            const child = node.children.items[0];
            const child_w = b.width() - pad.horizontal();
            child.setBounds(Rect.fromXywh(
                b.left + pad.left,
                y,
                child_w,
                child.measured.height,
            ));
        }
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Field = @fieldParentPtr("node", node);
        const pad = self.padding;
        const b = node.bounds;
        const label_fs = self.label_font_size.peek();
        const msg_fs = self.message_font_size.peek();

        var y: f32 = b.top + pad.top;

        // 背景（invalid 时淡红色）
        if (self.invalid.peek()) {
            try scene.fillRoundedRect(b, 4, Color.fromHexRgb(0xFFF0F0));
        }

        // 标签
        const label_text = self.label.peek();
        if (label_text.len > 0) {
            // 必填指示器
            if (self.required.peek()) {
                try scene.drawText(.{
                    .text = "* ",
                    .x = b.left + pad.left,
                    .y = y,
                    .font_size = label_fs,
                    .color = self.error_color.peek(),
                });
            }

            const label_x = if (self.required.peek())
                b.left + pad.left + label_fs * 0.6 + 4
            else
                b.left + pad.left;

            try scene.drawText(.{
                .text = label_text,
                .x = label_x,
                .y = y,
                .font_size = label_fs,
                .color = if (self.invalid.peek()) self.error_color.peek() else self.label_color.peek(),
                .layout_width = @max(0, b.width() - pad.horizontal()),
            });
            y += label_fs * 1.4 + self.gap;
        }

        // 控件绘制由 Tree 递归处理，此处不重复绘制子节点

        // 消息文本（error 优先）
        const err = self.error_text.peek();
        const hlp = self.helper.peek();
        const msg = if (err.len > 0) err else hlp;
        const msg_color = if (err.len > 0) self.error_color.peek() else self.helper_color.peek();

        if (msg.len > 0) {
            // 计算消息 Y：跳过控件高度
            if (node.children.items.len > 0) {
                const child = node.children.items[0];
                y = child.bounds.bottom + self.gap;
            }

            try scene.drawText(.{
                .text = msg,
                .x = b.left + pad.left,
                .y = y,
                .font_size = msg_fs,
                .color = msg_color,
                .layout_width = @max(0, b.width() - pad.horizontal()),
            });
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Field = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Field 创建与基本属性" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "用户名");
    defer label_sig.deinit();
    var helper_sig = reactive.Signal([]const u8).init(&g, "请输入用户名");
    defer helper_sig.deinit();
    var error_sig = reactive.Signal([]const u8).init(&g, "");
    defer error_sig.deinit();
    var required_sig = reactive.Signal(bool).init(&g, true);
    defer required_sig.deinit();
    var invalid_sig = reactive.Signal(bool).init(&g, false);
    defer invalid_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();
    var label_col_sig = reactive.Signal(Color).init(&g, Color.black);
    defer label_col_sig.deinit();
    var helper_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x999999));
    defer helper_col_sig.deinit();
    var error_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0xFF0000));
    defer error_col_sig.deinit();
    var label_fs_sig = reactive.Signal(f32).init(&g, 14);
    defer label_fs_sig.deinit();
    var msg_fs_sig = reactive.Signal(f32).init(&g, 12);
    defer msg_fs_sig.deinit();

    const field = try Field.create(allocator, &g, .{
        .label = label_sig.asReadonly(),
        .helper = helper_sig.asReadonly(),
        .error_text = error_sig.asReadonly(),
        .required = required_sig.asReadonly(),
        .invalid = invalid_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
        .label_color = label_col_sig.asReadonly(),
        .helper_color = helper_col_sig.asReadonly(),
        .error_color = error_col_sig.asReadonly(),
        .label_font_size = label_fs_sig.asReadonly(),
        .message_font_size = msg_fs_sig.asReadonly(),
    });
    defer field.node.deinitTree(allocator);

    try std.testing.expectEqualStrings("用户名", field.label.peek());
    try std.testing.expect(field.required.peek());
}

test "Field error 优先于 helper" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var label_sig = reactive.Signal([]const u8).init(&g, "邮箱");
    defer label_sig.deinit();
    var helper_sig = reactive.Signal([]const u8).init(&g, "例如 user@example.com");
    defer helper_sig.deinit();
    var error_sig = reactive.Signal([]const u8).init(&g, "格式不正确");
    defer error_sig.deinit();
    var required_sig = reactive.Signal(bool).init(&g, false);
    defer required_sig.deinit();
    var invalid_sig = reactive.Signal(bool).init(&g, true);
    defer invalid_sig.deinit();
    var disabled_sig = reactive.Signal(bool).init(&g, false);
    defer disabled_sig.deinit();
    var label_col_sig = reactive.Signal(Color).init(&g, Color.black);
    defer label_col_sig.deinit();
    var helper_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x999999));
    defer helper_col_sig.deinit();
    var error_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0xFF0000));
    defer error_col_sig.deinit();
    var label_fs_sig = reactive.Signal(f32).init(&g, 14);
    defer label_fs_sig.deinit();
    var msg_fs_sig = reactive.Signal(f32).init(&g, 12);
    defer msg_fs_sig.deinit();

    const field = try Field.create(allocator, &g, .{
        .label = label_sig.asReadonly(),
        .helper = helper_sig.asReadonly(),
        .error_text = error_sig.asReadonly(),
        .required = required_sig.asReadonly(),
        .invalid = invalid_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
        .label_color = label_col_sig.asReadonly(),
        .helper_color = helper_col_sig.asReadonly(),
        .error_color = error_col_sig.asReadonly(),
        .label_font_size = label_fs_sig.asReadonly(),
        .message_font_size = msg_fs_sig.asReadonly(),
    });
    defer field.node.deinitTree(allocator);

    try std.testing.expect(field.invalid.peek());
    try std.testing.expectEqualStrings("格式不正确", field.error_text.peek());
    try std.testing.expectEqualStrings("例如 user@example.com", field.helper.peek());
}
