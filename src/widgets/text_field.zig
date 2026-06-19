//! widgets/text_field —— 单行文本输入控件
//!
//! `TextField` 处理文本编辑、焦点管理、光标渲染和基本键盘输入。
//! 内部持有文本缓冲区，通过 `text()` / `setText()` 访问，通过 `on_change` 回调通知外部。
//!
//! 使用示例：
//! ```zig
//! const tf = try TextField.create(allocator, &g, .{
//!     .font_size = readOnly(owner, f32, &g, 14),
//!     .color = readOnly(owner, Color, &g, Color.black),
//!     .placeholder = readOnly(owner, []const u8, &g, "请输入..."),
//! });
//! defer tf.node.deinitTree(allocator);
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
const Event = runtime.Event;
const EventResult = runtime.EventResult;
const KeyModifiers = runtime.KeyModifiers;

// ── SDL key code constants ────────────────────────────────────────────────────
const SDLK_BACKSPACE = 8;
const SDLK_RETURN = 13;
const SDLK_DELETE = 127;
const SDLK_LEFT = 1073741904;
const SDLK_RIGHT = 1073741903;
const SDLK_HOME = 1073741898;
const SDLK_END = 1073741901;
const SDLK_v = 118;
const SDLK_c = 99;
const SDLK_x = 120;
const SDLK_a = 97;

// ── 公共类型 ─────────────────────────────────────────────────────────────────

/// TextField 的构造属性。样式属性为只读信号。
pub const TextFieldProps = struct {
    /// 字体大小（默认 14）。
    font_size: reactive.ReadSignal(f32),
    /// 文本颜色（默认黑）。
    color: reactive.ReadSignal(Color),
    /// 占位符文本颜色。
    placeholder_color: reactive.ReadSignal(Color),
    /// 光标颜色。
    caret_color: reactive.ReadSignal(Color),
    /// 背景色。
    bg_color: reactive.ReadSignal(Color),
    /// 占位符文本（非响应式，构造时固定）。
    placeholder: []const u8 = "",
    /// 内边距（非响应式）。
    padding: Insets = Insets.symmetric(8, 4),
    /// 是否禁用。
    disabled: reactive.ReadSignal(bool),
    /// 是否只读。
    read_only: reactive.ReadSignal(bool),
    /// 最小宽度（像素），用于 measure 阶段。
    min_width: f32 = 100,
};

// ─────────────────────────────────────────────────────────────────────────────
// TextField
// ─────────────────────────────────────────────────────────────────────────────

pub const TextField = struct {
    node: Node,
    allocator: Allocator,

    // ── 响应式输入（样式，只读视图）───────────────────────────────────────
    font_size: reactive.ReadSignal(f32),
    color: reactive.ReadSignal(Color),
    placeholder_color: reactive.ReadSignal(Color),
    caret_color: reactive.ReadSignal(Color),
    bg_color: reactive.ReadSignal(Color),
    disabled: reactive.ReadSignal(bool),
    read_only: reactive.ReadSignal(bool),

    // ── 内部状态 ──────────────────────────────────────────────────────────
    /// 文本缓冲区（拥有所有权）。
    text_buf: std.ArrayList(u8),
    /// 光标位置（字节偏移，范围 0..text_buf.items.len）。
    caret: usize = 0,
    /// 是否有焦点。
    focused: bool = false,
    /// 光标闪烁计时器（帧计数）。
    caret_visible: bool = true,
    caret_timer: u8 = 0,

    // ── 普通属性 ──────────────────────────────────────────────────────────
    placeholder: []const u8,
    padding: Insets,
    min_width: f32,

    // ── 可选回调 ──────────────────────────────────────────────────────────
    on_change: ?*const fn (text: []const u8) void = null,
    on_submit: ?*const fn (text: []const u8) void = null,

    // ── 响应式作用域 ─────────────────────────────────────────────────────
    scope: reactive.EffectScope,

    const vtable = VTable{
        .measure = measureImpl,
        .paint = paintImpl,
        .handle_event = handleEventImpl,
        .deinit = deinitImpl,
    };

    // ── 构造 ──────────────────────────────────────────────────────────────

    pub fn create(
        allocator: Allocator,
        g: *reactive.Graph,
        props: TextFieldProps,
    ) !*TextField {
        const self = try allocator.create(TextField);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .font_size = props.font_size,
            .color = props.color,
            .placeholder_color = props.placeholder_color,
            .caret_color = props.caret_color,
            .bg_color = props.bg_color,
            .disabled = props.disabled,
            .read_only = props.read_only,
            .text_buf = undefined,
            .placeholder = props.placeholder,
            .padding = props.padding,
            .min_width = props.min_width,
            .scope = reactive.EffectScope.init(g),
        };
        self.text_buf = .empty;

        _ = try self.scope.add(self, struct {
            fn f(s: *TextField) void {
                _ = s.font_size.get();
                _ = s.color.get();
                _ = s.placeholder_color.get();
                _ = s.caret_color.get();
                _ = s.bg_color.get();
                _ = s.disabled.get();
                _ = s.read_only.get();
                s.node.markLayoutDirty();
            }
        }.f);

        return self;
    }

    // ── 公共方法 ──────────────────────────────────────────────────────────

    /// 获取当前文本内容。
    pub fn text(self: *const TextField) []const u8 {
        return self.text_buf.items;
    }

    /// 设置文本内容并移动光标到末尾。
    pub fn setText(self: *TextField, t: []const u8) !void {
        self.text_buf.clearRetainingCapacity();
        try self.text_buf.appendSlice(t);
        self.caret = t.len;
        self.node.markPaintDirty();
        if (self.on_change) |cb| cb(self.text_buf.items);
    }

    /// 清空文本。
    pub fn clear(self: *TextField) void {
        self.text_buf.clearRetainingCapacity();
        self.caret = 0;
        self.node.markPaintDirty();
        if (self.on_change) |cb| cb(self.text_buf.items);
    }

    /// 返回焦点状态的只读视图。
    pub fn focusedSignal(self: *TextField) reactive.ReadSignal(bool) {
        _ = self; // 暂不暴露为 signal，后续可扩展
        return reactive.ReadSignal(bool).init(reactive.Signal(bool).init(undefined, false).asReadonly());
    }

    // ── 内部辅助 ──────────────────────────────────────────────────────────

    /// 在光标位置插入文本。
    fn insertAtCaret(self: *TextField, t: []const u8) !void {
        try self.text_buf.insertSlice(self.allocator, self.caret, t);
        self.caret += t.len;
        self.node.markPaintDirty();
        if (self.on_change) |cb| cb(self.text_buf.items);
    }

    /// 删除光标前的字符。
    fn backspace(self: *TextField) void {
        if (self.caret == 0) return;
        // 确保在 UTF-8 边界删除
        var del_len: usize = 1;
        if (self.caret >= 2 and (@as(u8, self.text_buf.items[self.caret - 1]) & 0xC0) == 0x80) {
            // 回退到前一个 UTF-8 起始字节
            var i: usize = self.caret - 1;
            while (i > 0 and (@as(u8, self.text_buf.items[i]) & 0xC0) == 0x80) {
                i -= 1;
            }
            del_len = self.caret - i;
        }
        const start = self.caret - del_len;
        _ = self.text_buf.orderedRemove(start);
        for (1..del_len) |_| {
            _ = self.text_buf.orderedRemove(start);
        }
        self.caret = start;
        self.node.markPaintDirty();
        if (self.on_change) |cb| cb(self.text_buf.items);
    }

    /// 删除光标后的字符。
    fn deleteForward(self: *TextField) void {
        if (self.caret >= self.text_buf.items.len) return;
        // 确保在 UTF-8 边界删除
        var del_len: usize = 1;
        const byte = self.text_buf.items[self.caret];
        if (byte & 0x80 != 0) {
            // 计算 UTF-8 序列长度
            del_len = if (byte & 0xE0 == 0xC0) 2 else if (byte & 0xF0 == 0xE0) 3 else if (byte & 0xF8 == 0xF0) 4 else 1;
        }
        for (0..del_len) |_| {
            _ = self.text_buf.orderedRemove(self.caret);
        }
        self.node.markPaintDirty();
        if (self.on_change) |cb| cb(self.text_buf.items);
    }

    /// 移动光标（确保在 UTF-8 边界）。
    fn moveCaret(self: *TextField, delta: i32) void {
        if (delta < 0) {
            var i: i32 = @intCast(self.caret);
            var remaining = -delta;
            while (remaining > 0 and i > 0) : (remaining -= 1) {
                i -= 1;
                // 跳过 UTF-8 延续字节
                while (i > 0 and (@as(u8, self.text_buf.items[@intCast(i)]) & 0xC0) == 0x80) {
                    i -= 1;
                }
            }
            self.caret = @intCast(@max(0, i));
        } else if (delta > 0) {
            var i = self.caret;
            var remaining = delta;
            while (remaining > 0 and i < self.text_buf.items.len) : (remaining -= 1) {
                i += 1;
                // 跳过 UTF-8 延续字节
                while (i < self.text_buf.items.len and (@as(u8, self.text_buf.items[i]) & 0xC0) == 0x80) {
                    i += 1;
                }
            }
            self.caret = @min(i, self.text_buf.items.len);
        }
        self.caret_timer = 0;
        self.caret_visible = true;
        self.node.markPaintDirty();
    }

    fn moveCaretHome(self: *TextField) void {
        self.caret = 0;
        self.caret_timer = 0;
        self.caret_visible = true;
        self.node.markPaintDirty();
    }

    fn moveCaretEnd(self: *TextField) void {
        self.caret = self.text_buf.items.len;
        self.caret_timer = 0;
        self.caret_visible = true;
        self.node.markPaintDirty();
    }

    // ── vtable 实现 ──────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *TextField = @fieldParentPtr("node", node);
        const pad = self.padding;
        const fs = self.font_size.peek();

        // 估算文本宽度
        const display_text = if (self.text_buf.items.len > 0)
            self.text_buf.items
        else
            self.placeholder;

        var m = text_mod.MonospaceMetrics{};
        const fm = m.interface();
        const style = text_mod.TextStyle{ .font_size = fs };
        var tl = text_mod.measure(
            self.allocator,
            display_text,
            style,
            .ellipsis,
            .{ .max_width = 0 },
            fm,
        ) catch {
            return constraints.constrain(.{
                .width = pad.horizontal() + self.min_width,
                .height = pad.vertical() + fs * 1.4,
            });
        };
        defer tl.deinit();

        const natural = Size{
            .width = @max(self.min_width, tl.size.width + pad.horizontal()),
            .height = @max(fs * 1.4, tl.size.height) + pad.vertical(),
        };
        return constraints.constrain(natural);
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *TextField = @fieldParentPtr("node", node);
        const b = node.bounds;
        const pad = self.padding;
        const fs = self.font_size.peek();

        // 背景
        try scene.fillRect(b, self.bg_color.peek());

        // 文本内容区域
        const content_x = b.left + pad.left;
        const content_y = b.top + pad.top;
        const content_w = b.width() - pad.horizontal();

        const has_text = self.text_buf.items.len > 0;
        const display_text: []const u8 = if (has_text)
            self.text_buf.items
        else
            self.placeholder;
        const display_color = if (has_text)
            self.color.peek()
        else
            self.placeholder_color.peek();

        // 绘制文本
        if (display_text.len > 0) {
            try scene.drawText(.{
                .text = display_text,
                .x = content_x,
                .y = content_y,
                .font_size = fs,
                .color = display_color,
                .layout_width = @max(0, content_w),
                .layout_height = @max(0, b.height() - pad.vertical()),
            });
        }

        // 光标
        if (self.focused and self.caret_visible and !self.disabled.peek()) {
            // 计算光标 X 位置
            const caret_text = if (self.caret > 0 and self.caret <= self.text_buf.items.len)
                self.text_buf.items[0..self.caret]
            else
                "";
            var caret_x = content_x;
            if (caret_text.len > 0) {
                var m = text_mod.MonospaceMetrics{};
                const fm = m.interface();
                const style = text_mod.TextStyle{ .font_size = fs };
                var tl = text_mod.measure(
                    self.allocator,
                    caret_text,
                    style,
                    .ellipsis,
                    .{ .max_width = 0 },
                    fm,
                ) catch return;
                defer tl.deinit();
                caret_x += tl.size.width;
            }

            // 光标垂直线
            const caret_h = fs * 1.2;
            const caret_rect = Rect.fromXywh(
                caret_x,
                content_y + (fs * 1.4 - caret_h) / 2.0,
                1.5,
                caret_h,
            );
            try scene.fillRect(caret_rect, self.caret_color.peek());
        }
    }

    fn handleEventImpl(node: *Node, event: Event) EventResult {
        const self: *TextField = @fieldParentPtr("node", node);

        if (self.disabled.peek()) return .ignored;

        switch (event) {
            .focus_in => {
                self.focused = true;
                self.caret_visible = true;
                self.caret_timer = 0;
                self.node.markPaintDirty();
                return .consumed;
            },
            .focus_out => {
                self.focused = false;
                self.caret_visible = false;
                self.node.markPaintDirty();
                return .consumed;
            },
            .pointer_down => |e| {
                if (self.read_only.peek()) return .ignored;
                // 点击时移动光标到对应位置（简化：移动到末尾）
                if (self.text_buf.items.len > 0) {
                    self.moveCaretEnd();
                }
                _ = e;
                return .consumed;
            },
            .key_down => |k| {
                if (self.read_only.peek()) return .ignored;

                const mod = k.modifiers;
                const ctrl = mod.ctrl or mod.super;

                if (ctrl) {
                    switch (@as(u32, @intCast(k.code))) {
                        SDLK_a => {
                            // 全选（暂不实现选择，仅标记）
                            return .consumed;
                        },
                        SDLK_v => {
                            // 粘贴（需要剪贴板，暂跳过）
                            return .consumed;
                        },
                        SDLK_c => {
                            // 复制（暂不实现）
                            return .consumed;
                        },
                        SDLK_x => {
                            // 剪切（暂不实现）
                            return .consumed;
                        },
                        else => return .ignored,
                    }
                }

                switch (@as(u32, @intCast(k.code))) {
                    SDLK_BACKSPACE => {
                        self.backspace();
                        return .consumed;
                    },
                    SDLK_DELETE => {
                        self.deleteForward();
                        return .consumed;
                    },
                    SDLK_RETURN => {
                        if (self.on_submit) |cb| cb(self.text_buf.items);
                        return .consumed;
                    },
                    SDLK_LEFT => {
                        self.moveCaret(-1);
                        return .consumed;
                    },
                    SDLK_RIGHT => {
                        self.moveCaret(1);
                        return .consumed;
                    },
                    SDLK_HOME => {
                        self.moveCaretHome();
                        return .consumed;
                    },
                    SDLK_END => {
                        self.moveCaretEnd();
                        return .consumed;
                    },
                    else => return .ignored,
                }
            },
            .text_input => |ti| {
                if (self.read_only.peek()) return .ignored;
                self.insertAtCaret(ti.text) catch return .ignored;
                return .consumed;
            },
            else => return .ignored,
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *TextField = @fieldParentPtr("node", node);
        self.scope.deinit();
        self.text_buf.deinit(allocator);
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "TextField 创建与初始状态" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var fs_sig = reactive.Signal(f32).init(&g, 14);
    defer fs_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.black);
    defer col_sig.deinit();
    var ph_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x999999));
    defer ph_col_sig.deinit();
    var caret_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x0000FF));
    defer caret_col_sig.deinit();
    var bg_sig = reactive.Signal(Color).init(&g, Color.white);
    defer bg_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, false);
    defer dis_sig.deinit();
    var ro_sig = reactive.Signal(bool).init(&g, false);
    defer ro_sig.deinit();

    const tf = try TextField.create(allocator, &g, .{
        .font_size = fs_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .placeholder_color = ph_col_sig.asReadonly(),
        .caret_color = caret_col_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
        .read_only = ro_sig.asReadonly(),
        .placeholder = "输入文本...",
    });
    defer tf.node.deinitTree(allocator);

    try std.testing.expectEqualStrings("", tf.text());
    try std.testing.expectEqual(@as(usize, 0), tf.caret);
    try std.testing.expect(!tf.focused);
}

test "TextField 插入文本与退格" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var fs_sig = reactive.Signal(f32).init(&g, 14);
    defer fs_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.black);
    defer col_sig.deinit();
    var ph_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x999999));
    defer ph_col_sig.deinit();
    var caret_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x0000FF));
    defer caret_col_sig.deinit();
    var bg_sig = reactive.Signal(Color).init(&g, Color.white);
    defer bg_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, false);
    defer dis_sig.deinit();
    var ro_sig = reactive.Signal(bool).init(&g, false);
    defer ro_sig.deinit();

    const tf = try TextField.create(allocator, &g, .{
        .font_size = fs_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .placeholder_color = ph_col_sig.asReadonly(),
        .caret_color = caret_col_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
        .read_only = ro_sig.asReadonly(),
    });
    defer tf.node.deinitTree(allocator);

    // 插入文本
    try tf.insertAtCaret("Hello");
    try std.testing.expectEqualStrings("Hello", tf.text());
    try std.testing.expectEqual(@as(usize, 5), tf.caret);

    // 退格
    tf.backspace();
    try std.testing.expectEqualStrings("Hell", tf.text());
    try std.testing.expectEqual(@as(usize, 4), tf.caret);

    // 再次退格删除全部
    tf.backspace();
    tf.backspace();
    tf.backspace();
    tf.backspace();
    try std.testing.expectEqualStrings("", tf.text());
    try std.testing.expectEqual(@as(usize, 0), tf.caret);

    // 空文本退格不崩溃
    tf.backspace();
    try std.testing.expectEqualStrings("", tf.text());
}

test "TextField 焦点事件" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var fs_sig = reactive.Signal(f32).init(&g, 14);
    defer fs_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.black);
    defer col_sig.deinit();
    var ph_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x999999));
    defer ph_col_sig.deinit();
    var caret_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x0000FF));
    defer caret_col_sig.deinit();
    var bg_sig = reactive.Signal(Color).init(&g, Color.white);
    defer bg_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, false);
    defer dis_sig.deinit();
    var ro_sig = reactive.Signal(bool).init(&g, false);
    defer ro_sig.deinit();

    const tf = try TextField.create(allocator, &g, .{
        .font_size = fs_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .placeholder_color = ph_col_sig.asReadonly(),
        .caret_color = caret_col_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
        .read_only = ro_sig.asReadonly(),
    });
    defer tf.node.deinitTree(allocator);

    // focus_in
    _ = tf.node.vtable.handle_event(&tf.node, Event{ .focus_in = .{ .gained = true } });
    try std.testing.expect(tf.focused);

    // focus_out
    _ = tf.node.vtable.handle_event(&tf.node, Event{ .focus_out = .{ .gained = false } });
    try std.testing.expect(!tf.focused);
}

test "TextField disabled 时不处理事件" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var fs_sig = reactive.Signal(f32).init(&g, 14);
    defer fs_sig.deinit();
    var col_sig = reactive.Signal(Color).init(&g, Color.black);
    defer col_sig.deinit();
    var ph_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x999999));
    defer ph_col_sig.deinit();
    var caret_col_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x0000FF));
    defer caret_col_sig.deinit();
    var bg_sig = reactive.Signal(Color).init(&g, Color.white);
    defer bg_sig.deinit();
    var dis_sig = reactive.Signal(bool).init(&g, true);
    defer dis_sig.deinit();
    var ro_sig = reactive.Signal(bool).init(&g, false);
    defer ro_sig.deinit();

    const tf = try TextField.create(allocator, &g, .{
        .font_size = fs_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .placeholder_color = ph_col_sig.asReadonly(),
        .caret_color = caret_col_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
        .read_only = ro_sig.asReadonly(),
    });
    defer tf.node.deinitTree(allocator);

    // disabled 时 text_input 被忽略
    const result = tf.node.vtable.handle_event(&tf.node, Event{ .text_input = .{ .text = "x" } });
    try std.testing.expectEqual(EventResult.ignored, result);
    try std.testing.expectEqualStrings("", tf.text());
}
