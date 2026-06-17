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

const foundation = nandina.foundation;
const reactive = nandina.reactive;
const render = nandina.render;
const runtime = nandina.runtime;
const widgets = nandina.widgets;

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
