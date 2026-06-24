//! nandina_abi.zig —— NandinaUI C ABI 导出层
//!
//! 本文件是 Zig Core 的发布边界。所有导出函数为 `export fn` + `extern "C"`，
//! 通过不透明句柄暴露 Zig 类型给 C（及 C 之上的任意语言绑定）。
//!
//! 设计原则：
//!   - 纯胶水层：不做业务决策，只做 Zig ↔ C 类型映射。
//!   - 不透明句柄指向堆分配的 Zig 对象，函数调用方负责销毁。
//!   - C 兼容类型用 `extern struct` 保证内存布局与 C 一致。
//!   - 固定类型展开：信号等泛型类型按具体类型（i32/f32/bool/color/insets/string）展开。

const std = @import("std");
const nandina = @import("NandinaUI");
const sdl = @import("sdl_backend");

const foundation = nandina.foundation;
const reactive = nandina.reactive;
const render = nandina.render;
const runtime = nandina.runtime;
const widgets = nandina.widgets;
const app_mod = nandina.app;

const Color = foundation.Color;
const Insets = foundation.Insets;
const Rect = foundation.Rect;
const Size = foundation.Size;

// ─────────────────────────────────────────────────────────────────────────────
// C 兼容的类型定义（与 nandina_abi.h 对齐）
// ─────────────────────────────────────────────────────────────────────────────

/// C ABI 错误码。
pub const nandina_error_t = i32;

/// C 兼容颜色：0xAARRGGBB。
pub const nandina_color_t = u32;

/// C 兼容矩形。
pub const nandina_rect_t = extern struct {
    x: f32,
    y: f32,
    w: f32,
    h: f32,

    pub fn toZig(self: nandina_rect_t) Rect {
        return Rect.fromXywh(self.x, self.y, self.w, self.h);
    }

    pub fn fromZig(r: Rect) nandina_rect_t {
        return .{ .x = r.x(), .y = r.y(), .w = r.width(), .h = r.height() };
    }
};

/// C 兼容尺寸。
pub const nandina_size_t = extern struct {
    width: f32,
    height: f32,

    pub fn toZig(self: nandina_size_t) Size {
        return .{ .width = self.width, .height = self.height };
    }

    pub fn fromZig(s: Size) nandina_size_t {
        return .{ .width = s.width, .height = s.height };
    }
};

/// C 兼容内边距。
pub const nandina_insets_t = extern struct {
    left: f32,
    top: f32,
    right: f32,
    bottom: f32,

    pub fn toZig(self: nandina_insets_t) Insets {
        return .{ .left = self.left, .top = self.top, .right = self.right, .bottom = self.bottom };
    }

    pub fn fromZig(i: Insets) nandina_insets_t {
        return .{ .left = i.left, .top = i.top, .right = i.right, .bottom = i.bottom };
    }
};

// ── 颜色转换辅助 ────────────────────────────────────────────────────────────────

fn colorFromC(c: nandina_color_t) Color {
    return Color.fromHexRgba(c);
}

fn colorToC(c: Color) nandina_color_t {
    return c.toHexRgba();
}

// ─────────────────────────────────────────────────────────────────────────────
// 辅助：线程本地的错误消息
// ─────────────────────────────────────────────────────────────────────────────

threadlocal var last_error_buf: [256]u8 = undefined;
threadlocal var last_error_set: bool = false;

fn setError(comptime fmt: []const u8, args: anytype) void {
    last_error_set = true;
    @memset(&last_error_buf, 0);
    _ = std.fmt.bufPrint(&last_error_buf, fmt, args) catch 0;
}

const Allocator = std.mem.Allocator;

// ─────────────────────────────────────────────────────────────────────────────
// § 生命周期
// ─────────────────────────────────────────────────────────────────────────────

export fn nandina_version() [*:0]const u8 {
    return "0.0.0";
}

export fn nandina_init() nandina_error_t {
    last_error_set = false;
    return 0;
}

export fn nandina_deinit() void {
    last_error_set = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// § Graph
// ─────────────────────────────────────────────────────────────────────────────

export fn nandina_graph_create(out: *?*reactive.Graph) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const g = allocator.create(reactive.Graph) catch |err| {
        setError("alloc Graph: {s}", .{@errorName(err)});
        return -1;
    };
    g.* = reactive.Graph.init(allocator);
    out.* = g;
    return 0;
}

export fn nandina_graph_destroy(g: *reactive.Graph) void {
    g.deinit();
    std.heap.c_allocator.destroy(g);
}

const BatchContext = struct {
    callback: *const fn (?*anyopaque) callconv(.c) void,
    data: ?*anyopaque,
};

fn runBatch(ctx: *BatchContext) void {
    ctx.callback(ctx.data);
}

export fn nandina_graph_batch(
    g: *reactive.Graph,
    fn_ptr: ?*const fn (?*anyopaque) callconv(.c) void,
    user_data: ?*anyopaque,
) nandina_error_t {
    last_error_set = false;
    if (fn_ptr) |cb| {
        var ctx = BatchContext{ .callback = cb, .data = user_data };
        reactive.batch(g, &ctx, runBatch);
        return 0;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// § Signal 展开（i32 / f32 / bool / color / insets / string）
// ─────────────────────────────────────────────────────────────────────────────

// -- i32 --

export fn nandina_signal_i32_create(
    g: *reactive.Graph,
    initial: i32,
    out: *?*reactive.Signal(i32),
) nandina_error_t {
    last_error_set = false;
    const s = std.heap.c_allocator.create(reactive.Signal(i32)) catch |err| {
        setError("alloc Signal(i32): {s}", .{@errorName(err)});
        return -1;
    };
    s.* = reactive.Signal(i32).init(g, initial);
    out.* = s;
    return 0;
}

export fn nandina_signal_i32_get(s: *reactive.Signal(i32)) i32 {
    return s.get();
}

export fn nandina_signal_i32_set(s: *reactive.Signal(i32), value: i32) void {
    s.set(value);
}

export fn nandina_signal_i32_destroy(s: *reactive.Signal(i32)) void {
    s.deinit();
    std.heap.c_allocator.destroy(s);
}

// -- f32 --

export fn nandina_signal_f32_create(
    g: *reactive.Graph,
    initial: f32,
    out: *?*reactive.Signal(f32),
) nandina_error_t {
    last_error_set = false;
    const s = std.heap.c_allocator.create(reactive.Signal(f32)) catch |err| {
        setError("alloc Signal(f32): {s}", .{@errorName(err)});
        return -1;
    };
    s.* = reactive.Signal(f32).init(g, initial);
    out.* = s;
    return 0;
}

export fn nandina_signal_f32_get(s: *reactive.Signal(f32)) f32 {
    return s.get();
}

export fn nandina_signal_f32_set(s: *reactive.Signal(f32), value: f32) void {
    s.set(value);
}

export fn nandina_signal_f32_destroy(s: *reactive.Signal(f32)) void {
    s.deinit();
    std.heap.c_allocator.destroy(s);
}

// -- bool --

export fn nandina_signal_bool_create(
    g: *reactive.Graph,
    initial: bool,
    out: *?*reactive.Signal(bool),
) nandina_error_t {
    last_error_set = false;
    const s = std.heap.c_allocator.create(reactive.Signal(bool)) catch |err| {
        setError("alloc Signal(bool): {s}", .{@errorName(err)});
        return -1;
    };
    s.* = reactive.Signal(bool).init(g, initial);
    out.* = s;
    return 0;
}

export fn nandina_signal_bool_get(s: *reactive.Signal(bool)) bool {
    return s.get();
}

export fn nandina_signal_bool_set(s: *reactive.Signal(bool), value: bool) void {
    s.set(value);
}

export fn nandina_signal_bool_destroy(s: *reactive.Signal(bool)) void {
    s.deinit();
    std.heap.c_allocator.destroy(s);
}

// -- color (nandina_color_t = u32 ARGB) --

export fn nandina_signal_color_create(
    g: *reactive.Graph,
    initial: nandina_color_t,
    out: *?*reactive.Signal(Color),
) nandina_error_t {
    last_error_set = false;
    const s = std.heap.c_allocator.create(reactive.Signal(Color)) catch |err| {
        setError("alloc Signal(Color): {s}", .{@errorName(err)});
        return -1;
    };
    s.* = reactive.Signal(Color).init(g, colorFromC(initial));
    out.* = s;
    return 0;
}

export fn nandina_signal_color_get(s: *reactive.Signal(Color)) nandina_color_t {
    return colorToC(s.get());
}

export fn nandina_signal_color_set(s: *reactive.Signal(Color), value: nandina_color_t) void {
    s.set(colorFromC(value));
}

export fn nandina_signal_color_destroy(s: *reactive.Signal(Color)) void {
    s.deinit();
    std.heap.c_allocator.destroy(s);
}

// -- insets --

export fn nandina_signal_insets_create(
    g: *reactive.Graph,
    initial: nandina_insets_t,
    out: *?*reactive.Signal(Insets),
) nandina_error_t {
    last_error_set = false;
    const s = std.heap.c_allocator.create(reactive.Signal(Insets)) catch |err| {
        setError("alloc Signal(Insets): {s}", .{@errorName(err)});
        return -1;
    };
    s.* = reactive.Signal(Insets).init(g, initial.toZig());
    out.* = s;
    return 0;
}

export fn nandina_signal_insets_get(s: *reactive.Signal(Insets)) nandina_insets_t {
    return nandina_insets_t.fromZig(s.get());
}

export fn nandina_signal_insets_set(s: *reactive.Signal(Insets), value: nandina_insets_t) void {
    s.set(value.toZig());
}

export fn nandina_signal_insets_destroy(s: *reactive.Signal(Insets)) void {
    s.deinit();
    std.heap.c_allocator.destroy(s);
}

// -- string ([]const u8) --

const StringSignal = struct {
    signal: reactive.Signal([]const u8),
    owned: []u8,

    fn init(g: *reactive.Graph, initial: []const u8) !*StringSignal {
        const self = try std.heap.c_allocator.create(StringSignal);
        // 堆上分配零结尾的复制（+1 字节用于零结尾），便于 C 侧读取
        const dup = try std.heap.c_allocator.alloc(u8, initial.len + 1);
        @memcpy(dup[0..initial.len], initial);
        dup[initial.len] = 0;
        self.* = .{
            .signal = reactive.Signal([]const u8).init(g, dup[0..initial.len]),
            .owned = dup,
        };
        return self;
    }

    fn get(self: *StringSignal) []const u8 {
        return self.signal.get();
    }

    fn set(self: *StringSignal, value: []const u8) void {
        const dup = std.heap.c_allocator.alloc(u8, value.len + 1) catch return;
        @memcpy(dup[0..value.len], value);
        dup[value.len] = 0;
        std.heap.c_allocator.free(self.owned);
        self.owned = dup;
        self.signal.set(dup[0..value.len]);
    }

    fn deinit(self: *StringSignal) void {
        self.signal.deinit();
        std.heap.c_allocator.free(self.owned);
        std.heap.c_allocator.destroy(self);
    }
};

export fn nandina_signal_string_create(
    g: *reactive.Graph,
    initial: [*:0]const u8,
    out: *?*StringSignal,
) nandina_error_t {
    last_error_set = false;
    const ss = StringSignal.init(g, std.mem.sliceTo(initial, 0)) catch |err| {
        setError("alloc Signal(string): {s}", .{@errorName(err)});
        return -1;
    };
    out.* = ss;
    return 0;
}

export fn nandina_signal_string_get(s: *StringSignal) [*:0]const u8 {
    _ = s.get();
    // s.owned 已保证零结尾
    return @ptrCast(s.owned.ptr);
}

export fn nandina_signal_string_set(s: *StringSignal, value: [*:0]const u8) void {
    s.set(std.mem.sliceTo(value, 0));
}

export fn nandina_signal_string_destroy(s: *StringSignal) void {
    s.deinit();
}

// ─────────────────────────────────────────────────────────────────────────────
// § Tree
// ─────────────────────────────────────────────────────────────────────────────

export fn nandina_tree_create(out: *?*runtime.Tree) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const tree = allocator.create(runtime.Tree) catch |err| {
        setError("alloc Tree: {s}", .{@errorName(err)});
        return -1;
    };
    tree.* = runtime.Tree.init(allocator);
    out.* = tree;
    return 0;
}

export fn nandina_tree_destroy(tree: *runtime.Tree) void {
    tree.deinit();
    std.heap.c_allocator.destroy(tree);
}

export fn nandina_tree_set_root(tree: *runtime.Tree, root: *runtime.Node) void {
    tree.setRoot(root);
}

export fn nandina_tree_set_viewport(tree: *runtime.Tree, width: f32, height: f32) void {
    tree.setViewport(Size{ .width = width, .height = height });
}

export fn nandina_tree_frame(tree: *runtime.Tree) bool {
    return tree.frame() catch false;
}

// ─────────────────────────────────────────────────────────────────────────────
// § Node
// ─────────────────────────────────────────────────────────────────────────────

export fn nandina_node_add_child(parent: *runtime.Node, child: *runtime.Node) nandina_error_t {
    last_error_set = false;
    parent.addChild(std.heap.c_allocator, child) catch |err| {
        setError("addChild: {s}", .{@errorName(err)});
        return -1;
    };
    return 0;
}

export fn nandina_node_mark_layout_dirty(node: *runtime.Node) void {
    node.markLayoutDirty();
}

export fn nandina_node_mark_paint_dirty(node: *runtime.Node) void {
    node.markPaintDirty();
}

export fn nandina_node_set_visible(node: *runtime.Node, visible: bool) void {
    node.visible = visible;
}

// ─────────────────────────────────────────────────────────────────────────────
// § Widget 工厂
// ─────────────────────────────────────────────────────────────────────────────

// ── Surface ──────────────────────────────────────────────────────────────────

export fn nandina_surface_create(
    g: *reactive.Graph,
    bg_color: nandina_color_t,
    corner_radius: f32,
    padding: nandina_insets_t,
    border_color: nandina_color_t,
    border_width: f32,
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const bg_sig = allocCreateSignal(Color, g, colorFromC(bg_color));
    const cr_sig = allocCreateSignal(f32, g, corner_radius);
    const pad_sig = allocCreateSignal(Insets, g, padding.toZig());
    const bc_sig = allocCreateSignal(Color, g, colorFromC(border_color));
    const bw_sig = allocCreateSignal(f32, g, border_width);
    const surface = widgets.Surface.create(allocator, g, .{
        .bg_color = bg_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .border_color = bc_sig.asReadonly(),
        .border_width = bw_sig.asReadonly(),
    }) catch |err| {
        setError("create Surface: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &surface.node;
    return 0;
}

export fn nandina_surface_create_reactive(
    g: *reactive.Graph,
    bg_color: ?*reactive.Signal(Color),
    corner_radius: ?*reactive.Signal(f32),
    padding: ?*reactive.Signal(Insets),
    border_color: ?*reactive.Signal(Color),
    border_width: ?*reactive.Signal(f32),
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const bg = bg_color orelse allocCreateSignal(Color, g, Color.black);
    const cr = corner_radius orelse allocCreateSignal(f32, g, 0);
    const pad = padding orelse allocCreateSignal(Insets, g, Insets.all(0));
    const bc = border_color orelse allocCreateSignal(Color, g, Color.black);
    const bw = border_width orelse allocCreateSignal(f32, g, 0);
    const surface = widgets.Surface.create(allocator, g, .{
        .bg_color = bg.asReadonly(),
        .corner_radius = cr.asReadonly(),
        .padding = pad.asReadonly(),
        .border_color = bc.asReadonly(),
        .border_width = bw.asReadonly(),
    }) catch |err| {
        setError("create Surface reactive: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &surface.node;
    return 0;
}

// ── Label ────────────────────────────────────────────────────────────────────

export fn nandina_label_create(
    g: *reactive.Graph,
    text: [*:0]const u8,
    color: nandina_color_t,
    font_size: f32,
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const text_sig = allocCreateSignal([]const u8, g, std.mem.sliceTo(text, 0));
    const color_sig = allocCreateSignal(Color, g, colorFromC(color));
    const fs_sig = allocCreateSignal(f32, g, font_size);
    const label = widgets.Label.create(allocator, g, .{
        .text = text_sig.asReadonly(),
        .color = color_sig.asReadonly(),
        .font_size = fs_sig.asReadonly(),
    }) catch |err| {
        setError("create Label: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &label.node;
    return 0;
}

export fn nandina_label_create_reactive(
    g: *reactive.Graph,
    text: ?*reactive.Signal([]const u8),
    color: ?*reactive.Signal(Color),
    font_size: ?*reactive.Signal(f32),
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const t = text orelse allocCreateSignal([]const u8, g, "");
    const c = color orelse allocCreateSignal(Color, g, Color.black);
    const fs = font_size orelse allocCreateSignal(f32, g, 14);
    const label = widgets.Label.create(allocator, g, .{
        .text = t.asReadonly(),
        .color = c.asReadonly(),
        .font_size = fs.asReadonly(),
    }) catch |err| {
        setError("create Label reactive: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &label.node;
    return 0;
}

// ── Button ───────────────────────────────────────────────────────────────────

export fn nandina_button_create(
    g: *reactive.Graph,
    label_text: [*:0]const u8,
    bg_color: nandina_color_t,
    bg_hover_color: nandina_color_t,
    bg_pressed_color: nandina_color_t,
    text_color: nandina_color_t,
    font_size: f32,
    corner_radius: f32,
    padding: nandina_insets_t,
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const lb_sig = allocCreateSignal([]const u8, g, std.mem.sliceTo(label_text, 0));
    const bg_sig = allocCreateSignal(Color, g, colorFromC(bg_color));
    const bh_sig = allocCreateSignal(Color, g, colorFromC(bg_hover_color));
    const bp_sig = allocCreateSignal(Color, g, colorFromC(bg_pressed_color));
    const tc_sig = allocCreateSignal(Color, g, colorFromC(text_color));
    const fs_sig = allocCreateSignal(f32, g, font_size);
    const cr_sig = allocCreateSignal(f32, g, corner_radius);
    const pd_sig = allocCreateSignal(Insets, g, padding.toZig());
    const disabled_sig = allocCreateSignal(bool, g, false);
    const btn = widgets.Button.create(allocator, g, .{
        .label = lb_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .bg_hover_color = bh_sig.asReadonly(),
        .bg_pressed_color = bp_sig.asReadonly(),
        .text_color = tc_sig.asReadonly(),
        .font_size = fs_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pd_sig.asReadonly(),
        .disabled = disabled_sig.asReadonly(),
    }) catch |err| {
        setError("create Button: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &btn.node;
    return 0;
}

export fn nandina_button_set_on_click(
    node: *runtime.Node,
    cb: ?*const fn (?*anyopaque) callconv(.c) void,
    user_data: ?*anyopaque,
) void {
    const btn: *widgets.Button = @fieldParentPtr("node", node);
    btn.on_click = cb;
    btn.on_click_ctx = user_data;
}

// ── Row ──────────────────────────────────────────────────────────────────────

export fn nandina_row_create(gap: f32, out: *?*runtime.Node) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const r = widgets.Row.create(allocator, .{ .gap = gap }) catch |err| {
        setError("create Row: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &r.node;
    return 0;
}

// ── Stack ──────────────────────────────────────────────────────────────────────

export fn nandina_stack_create(out: *?*runtime.Node) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const s = widgets.Stack.create(allocator, .{}) catch |err| {
        setError("create Stack: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &s.node;
    return 0;
}

// ── Column ───────────────────────────────────────────────────────────────────

export fn nandina_column_create(gap: f32, out: *?*runtime.Node) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const col = widgets.Column.create(allocator, .{ .gap = gap }) catch |err| {
        setError("create Column: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &col.node;
    return 0;
}

// ── Icon ───────────────────────────────────────────────────────────────────────

/// 图标形状：0 = rect，1 = circle。
export fn nandina_icon_create(
    g: *reactive.Graph,
    color: nandina_color_t,
    size: f32,
    shape: i32,
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const color_sig = allocCreateSignal(Color, g, colorFromC(color));
    const size_sig = allocCreateSignal(f32, g, size);
    const ic = widgets.Icon.create(allocator, g, .{
        .color = color_sig.asReadonly(),
        .size = size_sig.asReadonly(),
        .shape = if (shape == 1) .circle else .rect,
    }) catch |err| {
        setError("create Icon: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &ic.node;
    return 0;
}

// ── TextField ────────────────────────────────────────────────────────────────

export fn nandina_text_field_create(
    g: *reactive.Graph,
    placeholder: [*:0]const u8,
    font_size: f32,
    color: nandina_color_t,
    bg_color: nandina_color_t,
    min_width: f32,
    out: *?*widgets.TextField,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const fs_sig = allocCreateSignal(f32, g, font_size);
    const col_sig = allocCreateSignal(Color, g, colorFromC(color));
    const ph_col_sig = allocCreateSignal(Color, g, Color.fromHexRgb(0x6C7086));
    const caret_sig = allocCreateSignal(Color, g, Color.fromHexRgb(0x89B4FA));
    const bg_sig = allocCreateSignal(Color, g, colorFromC(bg_color));
    const dis_sig = allocCreateSignal(bool, g, false);
    const ro_sig = allocCreateSignal(bool, g, false);
    const tf = widgets.TextField.create(allocator, g, .{
        .font_size = fs_sig.asReadonly(),
        .color = col_sig.asReadonly(),
        .placeholder_color = ph_col_sig.asReadonly(),
        .caret_color = caret_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
        .read_only = ro_sig.asReadonly(),
        .placeholder = std.mem.sliceTo(placeholder, 0),
        .min_width = min_width,
    }) catch |err| {
        setError("create TextField: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = tf;
    return 0;
}

export fn nandina_text_field_node(tf: *widgets.TextField) *runtime.Node {
    return &tf.node;
}

export fn nandina_text_field_text(tf: *widgets.TextField) [*:0]const u8 {
    // 注意：内部缓冲未必零结尾，复制到线程本地缓冲返回。
    const t = tf.text();
    const n = @min(t.len, last_error_buf.len - 1);
    @memcpy(last_error_buf[0..n], t[0..n]);
    last_error_buf[n] = 0;
    return @ptrCast(&last_error_buf);
}

export fn nandina_text_field_set_text(tf: *widgets.TextField, text: [*:0]const u8) void {
    tf.setText(std.mem.sliceTo(text, 0)) catch {};
}

export fn nandina_text_field_set_on_change(
    tf: *widgets.TextField,
    cb: ?*const fn (?*anyopaque, [*:0]const u8) callconv(.c) void,
    user_data: ?*anyopaque,
) void {
    _ = cb;
    _ = user_data;
    _ = tf;
    // C 侧回调签名带零结尾字符串，Zig 侧为切片，签名不兼容，暂以独立 trampoline 留待后续完善。
    setError("text_field on_change 回调暂未导出（签名待定）", .{});
}

// ── Checkbox ─────────────────────────────────────────────────────────────────

export fn nandina_checkbox_create(
    g: *reactive.Graph,
    checked: *reactive.Signal(bool),
    color: nandina_color_t,
    out: *?*widgets.Checkbox,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const col_sig = allocCreateSignal(Color, g, colorFromC(color));
    const dis_sig = allocCreateSignal(bool, g, false);
    const cb = widgets.Checkbox.create(allocator, g, .{
        .checked = checked.asReadonly(),
        .color = col_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
    }) catch |err| {
        setError("create Checkbox: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = cb;
    return 0;
}

export fn nandina_checkbox_node(cb: *widgets.Checkbox) *runtime.Node {
    return &cb.node;
}

export fn nandina_checkbox_set_on_change(
    cb: *widgets.Checkbox,
    fn_ptr: ?*const fn (?*anyopaque, bool) callconv(.c) void,
    user_data: ?*anyopaque,
) void {
    cb.on_change = fn_ptr;
    cb.on_change_ctx = user_data;
}

// ── Switch ─────────────────────────────────────────────────────────────────────

export fn nandina_switch_create(
    g: *reactive.Graph,
    checked: *reactive.Signal(bool),
    color: nandina_color_t,
    out: *?*widgets.Switch,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const col_sig = allocCreateSignal(Color, g, colorFromC(color));
    const dis_sig = allocCreateSignal(bool, g, false);
    const sw = widgets.Switch.create(allocator, g, .{
        .checked = checked.asReadonly(),
        .color = col_sig.asReadonly(),
        .disabled = dis_sig.asReadonly(),
    }) catch |err| {
        setError("create Switch: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = sw;
    return 0;
}

export fn nandina_switch_node(sw: *widgets.Switch) *runtime.Node {
    return &sw.node;
}

export fn nandina_switch_set_on_change(
    sw: *widgets.Switch,
    fn_ptr: ?*const fn (?*anyopaque, bool) callconv(.c) void,
    user_data: ?*anyopaque,
) void {
    sw.on_change = fn_ptr;
    sw.on_change_ctx = user_data;
}

// ── Field ──────────────────────────────────────────────────────────────────────

export fn nandina_field_create(
    g: *reactive.Graph,
    label_text: [*:0]const u8,
    helper: [*:0]const u8,
    required: bool,
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const fld = widgets.Field.create(allocator, g, .{
        .label = allocCreateSignal([]const u8, g, std.mem.sliceTo(label_text, 0)).asReadonly(),
        .helper = allocCreateSignal([]const u8, g, std.mem.sliceTo(helper, 0)).asReadonly(),
        .error_text = allocCreateSignal([]const u8, g, "").asReadonly(),
        .required = allocCreateSignal(bool, g, required).asReadonly(),
        .invalid = allocCreateSignal(bool, g, false).asReadonly(),
        .disabled = allocCreateSignal(bool, g, false).asReadonly(),
        .label_color = allocCreateSignal(Color, g, Color.fromHexRgb(0xCDD6F4)).asReadonly(),
        .helper_color = allocCreateSignal(Color, g, Color.fromHexRgb(0x6C7086)).asReadonly(),
        .error_color = allocCreateSignal(Color, g, Color.fromHexRgb(0xF38BA8)).asReadonly(),
        .label_font_size = allocCreateSignal(f32, g, 14).asReadonly(),
        .message_font_size = allocCreateSignal(f32, g, 12).asReadonly(),
    }) catch |err| {
        setError("create Field: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &fld.node;
    return 0;
}

// ── Card ─────────────────────────────────────────────────────────────────────

export fn nandina_card_create(
    g: *reactive.Graph,
    title: [*:0]const u8,
    description: [*:0]const u8,
    bg_color: nandina_color_t,
    corner_radius: f32,
    title_font_size: f32,
    desc_font_size: f32,
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const t_sig = allocCreateSignal([]const u8, g, std.mem.sliceTo(title, 0));
    const d_sig = allocCreateSignal([]const u8, g, std.mem.sliceTo(description, 0));
    const bg_sig = allocCreateSignal(Color, g, colorFromC(bg_color));
    const cr_sig = allocCreateSignal(f32, g, corner_radius);
    const pad_sig = allocCreateSignal(Insets, g, Insets.all(16));
    const tfs_sig = allocCreateSignal(f32, g, title_font_size);
    const dfs_sig = allocCreateSignal(f32, g, desc_font_size);
    const card = widgets.Card.create(allocator, g, .{
        .title = t_sig.asReadonly(),
        .description = d_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .title_font_size = tfs_sig.asReadonly(),
        .description_font_size = dfs_sig.asReadonly(),
    }) catch |err| {
        setError("create Card: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &card.node;
    return 0;
}

// ── Panel ────────────────────────────────────────────────────────────────────

export fn nandina_panel_create(
    g: *reactive.Graph,
    bg_color: nandina_color_t,
    corner_radius: f32,
    padding: nandina_insets_t,
    border_color: nandina_color_t,
    border_width: f32,
    out: *?*runtime.Node,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const bg_sig = allocCreateSignal(Color, g, colorFromC(bg_color));
    const cr_sig = allocCreateSignal(f32, g, corner_radius);
    const pd_sig = allocCreateSignal(Insets, g, padding.toZig());
    const bc_sig = allocCreateSignal(Color, g, colorFromC(border_color));
    const bw_sig = allocCreateSignal(f32, g, border_width);
    const panel = widgets.Panel.create(allocator, g, .{
        .bg_color = bg_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pd_sig.asReadonly(),
        .border_color = bc_sig.asReadonly(),
        .border_width = bw_sig.asReadonly(),
    }) catch |err| {
        setError("create Panel: {s}", .{@errorName(err)});
        return -1;
    };
    out.* = &panel.node;
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// § 软件渲染后端
// ─────────────────────────────────────────────────────────────────────────────

export fn nandina_software_backend_create(
    out: *?*render.SoftwareBackend,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const backend = allocator.create(render.SoftwareBackend) catch |err| {
        setError("alloc SoftwareBackend: {s}", .{@errorName(err)});
        return -1;
    };
    backend.* = render.SoftwareBackend.init();
    out.* = backend;
    return 0;
}

export fn nandina_software_backend_destroy(backend: *render.SoftwareBackend) void {
    std.heap.c_allocator.destroy(backend);
}

export fn nandina_software_backend_pixels(
    backend: *render.SoftwareBackend,
    out_data: *?*anyopaque,
    out_width: *i32,
    out_height: *i32,
) nandina_error_t {
    last_error_set = false;
    const rt = &backend.target;
    const pixels = rt.pixels orelse {
        setError("SoftwareBackend has no target", .{});
        return -1;
    };
    out_data.* = @ptrCast(pixels);
    out_width.* = @as(i32, @intCast(rt.width));
    out_height.* = @as(i32, @intCast(rt.height));
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// § PageHost — 多页导航容器
// ─────────────────────────────────────────────────────────────────────────────
//
// C 侧每个页面用一个 build 回调构建子树：`nandina_node_t* build(void* user_data)`。
// build 回调在“需要时”被调用（创建 / 导航切换）；回调内用 widget 工厂创建节点并返回根。
// 内存：PageHost 持有页面节点树，切换时释放旧树。

const AbiPage = struct {
    build: *const fn (?*anyopaque) callconv(.c) ?*runtime.Node,
    user_data: ?*anyopaque,
};

/// ABI 版 PageHost 封装：用 C 回调构建页面，桥接到 Zig 的 PageHost。
const AbiPageHost = struct {
    host: *app_mod.PageHost,
    pages: []AbiPage,
    // 为 Zig PageHost 提供的 Page 列表（build 闭包通过线程本地 current 索引取回 AbiPage）。
    zig_pages: []app_mod.Page,
};

// PageHost 的 build 签名是 fn(allocator, *Graph, *SignalOwner) -> *Node，
// 不携带 per-page 上下文。这里用一个全局注册表把「当前正在 build 的 AbiPage」传进去。
threadlocal var g_building: ?*AbiPage = null;

fn abiPageBuildTrampoline(
    a: std.mem.Allocator,
    gr: *reactive.Graph,
    owner: *app_mod.SignalOwner,
) anyerror!*runtime.Node {
    _ = a;
    _ = gr;
    _ = owner;
    const page = g_building orelse return error.NoPageContext;
    const node = page.build(page.user_data) orelse return error.PageBuildFailed;
    return node;
}

export fn nandina_page_host_create(
    g: *reactive.Graph,
    builds: [*]const ?*const fn (?*anyopaque) callconv(.c) ?*runtime.Node,
    user_datas: [*]const ?*anyopaque,
    count: usize,
    initial_index: usize,
    out: *?*AbiPageHost,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;

    const abi_pages = allocator.alloc(AbiPage, count) catch |err| {
        setError("alloc AbiPage[]: {s}", .{@errorName(err)});
        return -1;
    };
    const zig_pages = allocator.alloc(app_mod.Page, count) catch |err| {
        allocator.free(abi_pages);
        setError("alloc Page[]: {s}", .{@errorName(err)});
        return -1;
    };
    for (0..count) |i| {
        const cb = builds[i] orelse {
            allocator.free(abi_pages);
            allocator.free(zig_pages);
            setError("page[{d}] build 回调为 null", .{i});
            return -1;
        };
        abi_pages[i] = .{ .build = cb, .user_data = user_datas[i] };
        zig_pages[i] = .{ .build = abiPageBuildTrampoline };
    }

    const self = allocator.create(AbiPageHost) catch |err| {
        allocator.free(abi_pages);
        allocator.free(zig_pages);
        setError("alloc AbiPageHost: {s}", .{@errorName(err)});
        return -1;
    };
    self.* = .{ .host = undefined, .pages = abi_pages, .zig_pages = zig_pages };

    // PageHost.create 会立刻 build initial_index 页面，需先设好 g_building。
    g_building = &abi_pages[@min(initial_index, count -| 1)];
    const host = app_mod.PageHost.create(allocator, g, zig_pages, initial_index) catch |err| {
        g_building = null;
        allocator.destroy(self);
        allocator.free(abi_pages);
        allocator.free(zig_pages);
        setError("create PageHost: {s}", .{@errorName(err)});
        return -1;
    };
    g_building = null;
    self.host = host;
    out.* = self;
    return 0;
}

/// 取 PageHost 的节点指针（挂入树）。
export fn nandina_page_host_node(host: *AbiPageHost) *runtime.Node {
    return &host.host.node;
}

/// 导航到指定索引页面。
export fn nandina_page_host_navigate_to(host: *AbiPageHost, index: usize) nandina_error_t {
    last_error_set = false;
    if (index >= host.pages.len) {
        setError("navigate index {d} 越界", .{index});
        return -1;
    }
    g_building = &host.pages[index];
    defer g_building = null;
    host.host.navigateTo(index) catch |err| {
        setError("navigateTo: {s}", .{@errorName(err)});
        return -1;
    };
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// § App — 全包窗口入口（默认 SDL3 + 软件后端）
// ─────────────────────────────────────────────────────────────────────────────
//
// 「Slint 式默认全包」：core 内部开 SDL3 窗口、跑主循环、渲染。SDL3 仅作为内部
// 实现，不出现在 ABI 语义里。默认走 software 后端（当前唯一能渲染文字的路径）。

const AbiApp = struct {
    window: sdl.Window,
    tree: ?*runtime.Tree = null,
    // 字体后端（可选）。
    ft: ?nandina.text.backends.hb_ft.HarfBuzzFreeTypeMetrics = null,
};

export fn nandina_app_create(
    title: [*:0]const u8,
    width: i32,
    height: i32,
    out: *?*AbiApp,
) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    const self = allocator.create(AbiApp) catch |err| {
        setError("alloc App: {s}", .{@errorName(err)});
        return -1;
    };
    self.* = .{ .window = undefined };

    self.window = sdl.Window.init(
        allocator,
        std.mem.sliceTo(title, 0),
        @intCast(width),
        @intCast(height),
    ) catch |err| {
        allocator.destroy(self);
        setError("create Window: {s}", .{@errorName(err)});
        return -1;
    };

    // 尝试启用真实字体（失败则退回内置等宽估算，不报错）。
    if (nandina.text.backends.hb_ft.HarfBuzzFreeTypeMetrics.init(allocator)) |fb| {
        self.ft = fb;
        self.window.sw_renderer.setGlyphRenderer(self.ft.?.glyphRenderer());
    } else |_| {}

    out.* = self;
    return 0;
}

/// 挂载根节点（内部建一棵 Tree 持有该根）。
export fn nandina_app_set_root(appp: *AbiApp, root: *runtime.Node) nandina_error_t {
    last_error_set = false;
    const allocator = std.heap.c_allocator;
    if (appp.tree == null) {
        const tree = allocator.create(runtime.Tree) catch |err| {
            setError("alloc Tree: {s}", .{@errorName(err)});
            return -1;
        };
        tree.* = runtime.Tree.init(allocator);
        appp.tree = tree;
    }
    appp.tree.?.setRoot(root);
    appp.window.setTree(appp.tree.?);
    return 0;
}

/// 进入阻塞主循环：poll 事件 → dispatch → frame → present，直到窗口关闭。
export fn nandina_app_run(appp: *AbiApp) nandina_error_t {
    last_error_set = false;
    while (appp.window.isRunning()) {
        if (!(appp.window.pollEvent() catch true)) break;
        appp.window.dispatchEvents();
        _ = appp.window.frame() catch {};
        appp.window.present();
    }
    return 0;
}

export fn nandina_app_destroy(appp: *AbiApp) void {
    const allocator = std.heap.c_allocator;
    if (appp.tree) |tree| {
        tree.deinit();
        allocator.destroy(tree);
    }
    if (appp.ft) |*ft| ft.deinit();
    appp.window.deinit();
    allocator.destroy(appp);
}

// ─────────────────────────────────────────────────────────────────────────────
// § 错误消息
// ─────────────────────────────────────────────────────────────────────────────

export fn nandina_error_message(err: nandina_error_t) [*:0]const u8 {
    _ = err;
    if (last_error_set) {
        return @ptrCast(&last_error_buf);
    }
    return "no error";
}

// ─────────────────────────────────────────────────────────────────────────────
// § 辅助函数
// ─────────────────────────────────────────────────────────────────────────────

/// 在堆上创建一个初始值为 `initial` 的 Signal，挂在 `g` 上。
/// 注意：创建的 Signal 不会被自动释放；调用方须在适当时机（如 Graph.deinit 前）
/// 用对应的 `nandina_signal_*_destroy` 释放。当前 ABI 工厂函数创建的 Signal 会
/// 随宿主 Widget 一起存活，由 Widget 的 `vtable.deinit` 统一释放。
fn allocCreateSignal(comptime T: type, g: *reactive.Graph, initial: T) *reactive.Signal(T) {
    const s = std.heap.c_allocator.create(reactive.Signal(T)) catch unreachable;
    s.* = reactive.Signal(T).init(g, initial);
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试：验证 ABI 模块本身能编译
// ─────────────────────────────────────────────────────────────────────────────

test "abi module compiles" {
    _ = nandina_init;
    _ = nandina_deinit;
    _ = nandina_graph_create;
    _ = nandina_signal_i32_create;
    _ = nandina_signal_f32_create;
    _ = nandina_signal_bool_create;
    _ = nandina_signal_color_create;
    _ = nandina_signal_insets_create;
    _ = nandina_signal_string_create;
    _ = nandina_tree_create;
    _ = nandina_surface_create;
    _ = nandina_label_create;
    _ = nandina_button_create;
    _ = nandina_column_create;
    _ = nandina_card_create;
    _ = nandina_panel_create;
    _ = nandina_software_backend_create;
    _ = nandina_error_message;
}

// 回归测试：复现 C++ 前端点击导航按钮崩溃。
// 根因是 widget 回调字段曾用 Zig 默认调用约定（.auto），而 ABI 经
// nandina_button_set_on_click 存入的是 callconv(.c) 函数指针；Zig 侧按 .auto
// 调用 C 约定函数会栈/寄存器错乱导致 SEGV。本测试走与 C++ 前端完全相同的路径：
// 工厂建按钮 → set_on_click 存入 callconv(.c) 回调 → 真实派发点击 → 断言触发。
const CClickProbe = struct {
    var fired: bool = false;
    fn cb(ud: ?*anyopaque) callconv(.c) void {
        _ = ud;
        fired = true;
    }
};

test "ABI button on_click 走 C 调用约定派发不崩溃且触发" {
    // ABI 工厂内部用 std.heap.c_allocator 分配节点与信号，这里的 Tree
    // 也须用同一 allocator，否则 deinit 会用不匹配的 allocator 释放。
    const allocator = std.heap.c_allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var node: ?*runtime.Node = null;
    const pad = nandina_insets_t{ .left = 0, .top = 0, .right = 0, .bottom = 0 };
    const err = nandina_button_create(
        &g,
        "OK",
        0xFF89B4FA,
        0xFF74C7EC,
        0xFF89DCEB,
        0xFF1E1E2E,
        14.0,
        6.0,
        pad,
        &node,
    );
    try std.testing.expectEqual(@as(nandina_error_t, 0), err);
    const btn_node = node.?;

    CClickProbe.fired = false;
    nandina_button_set_on_click(btn_node, CClickProbe.cb, null);

    var tree = runtime.Tree.init(allocator);
    defer tree.deinit();
    tree.setRoot(btn_node);
    tree.setViewport(.{ .width = 200, .height = 100 });
    _ = try tree.frame();

    const b = btn_node.bounds;
    const cx = b.left + b.width() * 0.5;
    const cy = b.top + b.height() * 0.5;
    _ = tree.dispatchEvent(.{ .pointer_move = .{ .x = cx, .y = cy } });
    _ = tree.dispatchEvent(.{ .pointer_down = .{ .button = .left, .x = cx, .y = cy } });
    _ = tree.dispatchEvent(.{ .pointer_up = .{ .button = .left, .x = cx, .y = cy } });

    try std.testing.expect(CClickProbe.fired);
}

// 回归测试：复现 C++ 前端关闭窗口时的 use-after-free 崩溃。
// showcase 的拆解契约：widget 节点（含 EffectScope 与内部 Signal）由 Tree 拆解，
// 而拆解会回访后备 Graph（dispose effect、detachSource）。因此必须先拆 Tree、
// 后拆 Graph；反之则对已释放的 Graph 产生 use-after-free。本测试锁定正确顺序。
test "ABI widget 拆解顺序：Tree 先于 Graph 释放不崩溃" {
    const allocator = std.heap.c_allocator;
    var g = reactive.Graph.init(allocator);

    // 构造一棵含响应式 widget 的树（button + label + 各自的内部 Signal/EffectScope）。
    var col_node: ?*runtime.Node = null;
    try std.testing.expectEqual(@as(nandina_error_t, 0), nandina_column_create(8.0, &col_node));
    const root = col_node.?;

    var btn_node: ?*runtime.Node = null;
    const pad = nandina_insets_t{ .left = 0, .top = 0, .right = 0, .bottom = 0 };
    try std.testing.expectEqual(@as(nandina_error_t, 0), nandina_button_create(&g, "OK", 0xFF89B4FA, 0xFF74C7EC, 0xFF89DCEB, 0xFF1E1E2E, 14.0, 6.0, pad, &btn_node));
    try std.testing.expectEqual(@as(nandina_error_t, 0), nandina_node_add_child(root, btn_node.?));

    var label_node: ?*runtime.Node = null;
    try std.testing.expectEqual(@as(nandina_error_t, 0), nandina_label_create(&g, "Hi", 0xFFCDD6F4, 14.0, &label_node));
    try std.testing.expectEqual(@as(nandina_error_t, 0), nandina_node_add_child(root, label_node.?));

    var tree = runtime.Tree.init(allocator);
    tree.setRoot(root);
    tree.setViewport(.{ .width = 300, .height = 200 });
    _ = try tree.frame();

    // 正确拆解顺序：先 Tree（拆 widget 节点，此时 Graph 仍存活），再 Graph。
    tree.deinit();
    g.deinit();
}
