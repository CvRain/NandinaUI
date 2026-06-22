//! app/authoring —— Authoring DSL 工厂函数
//!
//! 提供 Flutter 风格的声明式工厂函数，隐藏 allocator.create / Signal.init / addChild
//! 等样板代码。每个工厂函数返回 `*Node`，可直接挂到父节点上。
//!
//! ## 所有权策略
//! 工厂函数内部创建的所有 Signal 使用 `SignalOwner` 统一管理生命周期：
//!   1. 每个页面构建时分配一个 `SignalOwner`
//!   2. 所有通过 `SignalOwner.readOnly()` 创建的 Signal 由 owner 持有
//!   3. 页面销毁时调用 `SignalOwner.deinit()` 统一释放所有 Signal

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const runtime = @import("../runtime/runtime.zig");
const layout = @import("../layout/layout.zig");
const widgets = @import("../widgets/widgets.zig");
const page_mod = @import("page.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Insets = foundation.Insets;
const Node = runtime.Node;
const Ref = page_mod.Ref;
const Graph = reactive.Graph;

// ═════════════════════════════════════════════════════════════════════════════
// SignalOwner —— 管理工厂函数创建的 Signal 生命周期
// ═════════════════════════════════════════════════════════════════════════════

const OwnedSignal = struct {
    ptr: *anyopaque,
    deinit_fn: *const fn (*anyopaque) void,
};

/// 持有工厂函数内部创建的 Signal 指针，统一释放。
pub const SignalOwner = struct {
    allocator: Allocator,
    signals: std.ArrayList(OwnedSignal) = .empty,

    pub fn init(allocator: Allocator) SignalOwner {
        return .{ .allocator = allocator };
    }

    /// 释放所有托管的 Signal（调用 deinit_fn 释放 Graph 内资源，
    /// 再释放 Signal 自身内存）。
    pub fn deinit(self: *SignalOwner) void {
        for (self.signals.items) |owned| {
            owned.deinit_fn(owned.ptr);
        }
        self.signals.deinit(self.allocator);
    }
};

// ── 内部辅助 ─────────────────────────────────────────────────────────────────

fn allocSignal(owner: *SignalOwner, comptime T: type, g: *Graph, value: T) *reactive.Signal(T) {
    const c_alloc = std.heap.c_allocator;
    const s = c_alloc.create(reactive.Signal(T)) catch unreachable;
    s.* = reactive.Signal(T).init(g, value);

    const deleter = struct {
        fn del(ptr: *anyopaque) void {
            const sig: *reactive.Signal(T) = @ptrCast(@alignCast(ptr));
            sig.deinit();
            std.heap.c_allocator.destroy(sig);
        }
    }.del;

    owner.signals.append(owner.allocator, .{ .ptr = @ptrCast(s), .deinit_fn = deleter }) catch unreachable;
    return s;
}

/// 创建一个值为 `value` 的 ReadSignal，所有权由 owner 管理。
/// 返回的 ReadSignal 在 owner.deinit 之前有效。
pub fn readOnly(owner: *SignalOwner, comptime T: type, g: *Graph, value: T) reactive.ReadSignal(T) {
    return allocSignal(owner, T, g, value).asReadonly();
}

// ── 默认信号工厂（用于非响应式属性） ──────────────────────────────────────────

const defaultColors = struct {
    pub const text = Color.fromHexRgb(0xCDD6F4);
    pub const bg = Color.fromHexRgb(0x1E1E2E);
    pub const blue = Color.fromHexRgb(0x89B4FA);
    pub const blue_hover = Color.fromHexRgb(0x74C7EC);
    pub const blue_pressed = Color.fromHexRgb(0x89DCEB);
    pub const green = Color.fromHexRgb(0xA6E3A1);
    pub const surface0 = Color.fromHexRgb(0x313244);

    pub const mantle = Color.fromHexRgb(0x181825);
    pub const crust = Color.fromHexRgb(0x11111B);
};

// ═════════════════════════════════════════════════════════════════════════════
// 工厂函数（均接收 owner: *SignalOwner）
// ═════════════════════════════════════════════════════════════════════════════

/// Surface 工厂。
pub fn surface(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    config: struct {
        bg_color: Color = defaultColors.bg,
        corner_radius: f32 = 0,
        padding: Insets = Insets.all(0),
        border_color: Color = Color.black,
        border_width: f32 = 0,
    },
) !*Node {
    const s = try widgets.Surface.create(allocator, g, .{
        .bg_color = readOnly(owner, Color, g, config.bg_color),
        .corner_radius = readOnly(owner, f32, g, config.corner_radius),
        .padding = readOnly(owner, Insets, g, config.padding),
        .border_color = readOnly(owner, Color, g, config.border_color),
        .border_width = readOnly(owner, f32, g, config.border_width),
    });
    return &s.node;
}

/// Column 容器工厂。
pub fn column(
    allocator: Allocator,
    config: struct {
        gap: f32 = 0,
        main_align: layout.Align = .start,
        cross_align: layout.Align = .stretch,
    },
) !*Node {
    const c = try widgets.Column.create(allocator, .{
        .gap = config.gap,
        .main_align = config.main_align,
        .cross_align = config.cross_align,
    });
    return &c.node;
}

/// Row 容器工厂。
pub fn row(
    allocator: Allocator,
    config: struct {
        gap: f32 = 0,
        main_align: layout.Align = .start,
        cross_align: layout.Align = .start,
    },
) !*Node {
    const r = try widgets.Row.create(allocator, .{
        .gap = config.gap,
        .main_align = config.main_align,
        .cross_align = config.cross_align,
    });
    return &r.node;
}

/// Stack 容器工厂。
pub fn stack(
    allocator: Allocator,
    config: struct {
        main_align: layout.Align = .start,
        cross_align: layout.Align = .start,
    },
) !*Node {
    const s = try widgets.Stack.create(allocator, .{
        .main_align = config.main_align,
        .cross_align = config.cross_align,
    });
    return &s.node;
}

/// Label 文本工厂。
pub fn label(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    text: []const u8,
    config: struct {
        color: Color = defaultColors.text,
        font_size: f32 = 14,
    },
) !*Node {
    const l = try widgets.Label.create(allocator, g, .{
        .text = readOnly(owner, []const u8, g, text),
        .color = readOnly(owner, Color, g, config.color),
        .font_size = readOnly(owner, f32, g, config.font_size),
    });
    return &l.node;
}

/// Button 按钮工厂。
pub fn button(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    label_text: []const u8,
    config: struct {
        bg_color: Color = defaultColors.blue,
        bg_hover_color: Color = defaultColors.blue_hover,
        bg_pressed_color: Color = defaultColors.blue_pressed,
        text_color: Color = defaultColors.bg,
        font_size: f32 = 14,
        corner_radius: f32 = 6,
        padding: Insets = Insets.symmetric(20, 10),
        /// 点击回调（context-carrying：首参为 on_click_ctx，可忽略）。
        on_click: ?*const fn (ctx: ?*anyopaque) callconv(.c) void = null,
        /// 点击回调上下文。
        on_click_ctx: ?*anyopaque = null,
    },
) !*Node {
    const b = try widgets.Button.create(allocator, g, .{
        .label = readOnly(owner, []const u8, g, label_text),
        .bg_color = readOnly(owner, Color, g, config.bg_color),
        .bg_hover_color = readOnly(owner, Color, g, config.bg_hover_color),
        .bg_pressed_color = readOnly(owner, Color, g, config.bg_pressed_color),
        .text_color = readOnly(owner, Color, g, config.text_color),
        .font_size = readOnly(owner, f32, g, config.font_size),
        .corner_radius = readOnly(owner, f32, g, config.corner_radius),
        .padding = readOnly(owner, Insets, g, config.padding),
        .disabled = readOnly(owner, bool, g, false),
    });
    if (config.on_click) |cb| {
        b.on_click = cb;
        b.on_click_ctx = config.on_click_ctx;
    }
    return &b.node;
}

/// Card 卡片工厂。
pub fn card(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    title_text: []const u8,
    description_text: []const u8,
    config: struct {
        bg_color: Color = defaultColors.mantle,
        corner_radius: f32 = 8,
        title_font_size: f32 = 18,
        desc_font_size: f32 = 13,
    },
) !*Node {
    const c = try widgets.Card.create(allocator, g, .{
        .title = readOnly(owner, []const u8, g, title_text),
        .description = readOnly(owner, []const u8, g, description_text),
        .bg_color = readOnly(owner, Color, g, config.bg_color),
        .corner_radius = readOnly(owner, f32, g, config.corner_radius),
        .padding = readOnly(owner, Insets, g, Insets.all(16)),
        .title_font_size = readOnly(owner, f32, g, config.title_font_size),
        .description_font_size = readOnly(owner, f32, g, config.desc_font_size),
    });
    return &c.node;
}

/// Panel 面板工厂。
pub fn panel(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    config: struct {
        bg_color: Color = defaultColors.crust,
        corner_radius: f32 = 6,
        padding: Insets = Insets.all(12),
        border_color: Color = defaultColors.surface0,
        border_width: f32 = 1,
    },
) !*Node {
    const p = try widgets.Panel.create(allocator, g, .{
        .bg_color = readOnly(owner, Color, g, config.bg_color),
        .corner_radius = readOnly(owner, f32, g, config.corner_radius),
        .padding = readOnly(owner, Insets, g, config.padding),
        .border_color = readOnly(owner, Color, g, config.border_color),
        .border_width = readOnly(owner, f32, g, config.border_width),
    });
    return &p.node;
}

/// Icon 图标工厂。
pub fn icon(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    config: struct {
        color: Color = defaultColors.blue,
        size: f32 = 16,
        shape: widgets.IconShape = .rect,
    },
) !*Node {
    const i = try widgets.Icon.create(allocator, g, .{
        .color = readOnly(owner, Color, g, config.color),
        .size = readOnly(owner, f32, g, config.size),
        .shape = config.shape,
    });
    return &i.node;
}

/// TextField 文本输入工厂。
pub fn textField(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    config: struct {
        placeholder: []const u8 = "",
        font_size: f32 = 14,
        color: Color = defaultColors.text,
        placeholder_color: Color = Color.fromHexRgb(0x6C7086),
        caret_color: Color = defaultColors.blue,
        bg_color: Color = defaultColors.surface0,
        min_width: f32 = 150,
        padding: Insets = Insets.symmetric(10, 6),
        on_change: ?*const fn (ctx: ?*anyopaque, text: []const u8) void = null,
        on_change_ctx: ?*anyopaque = null,
        on_submit: ?*const fn (ctx: ?*anyopaque, text: []const u8) void = null,
        on_submit_ctx: ?*anyopaque = null,
    },
) !*widgets.TextField {
    const tf = try widgets.TextField.create(allocator, g, .{
        .font_size = readOnly(owner, f32, g, config.font_size),
        .color = readOnly(owner, Color, g, config.color),
        .placeholder_color = readOnly(owner, Color, g, config.placeholder_color),
        .caret_color = readOnly(owner, Color, g, config.caret_color),
        .bg_color = readOnly(owner, Color, g, config.bg_color),
        .disabled = readOnly(owner, bool, g, false),
        .read_only = readOnly(owner, bool, g, false),
        .placeholder = config.placeholder,
        .min_width = config.min_width,
        .padding = config.padding,
    });
    if (config.on_change) |cb| {
        tf.on_change = cb;
        tf.on_change_ctx = config.on_change_ctx;
    }
    if (config.on_submit) |cb| {
        tf.on_submit = cb;
        tf.on_submit_ctx = config.on_submit_ctx;
    }
    return tf;
}

/// Field 语义表单容器工厂。
pub fn field(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    config: struct {
        label: []const u8 = "",
        helper: []const u8 = "",
        required: bool = false,
    },
) !*widgets.Field {
    const fld = try widgets.Field.create(allocator, g, .{
        .label = readOnly(owner, []const u8, g, config.label),
        .helper = readOnly(owner, []const u8, g, config.helper),
        .error_text = readOnly(owner, []const u8, g, ""),
        .required = readOnly(owner, bool, g, config.required),
        .invalid = readOnly(owner, bool, g, false),
        .disabled = readOnly(owner, bool, g, false),
        .label_color = readOnly(owner, Color, g, defaultColors.text),
        .helper_color = readOnly(owner, Color, g, Color.fromHexRgb(0x6C7086)),
        .error_color = readOnly(owner, Color, g, Color.fromHexRgb(0xF38BA8)),
        .label_font_size = readOnly(owner, f32, g, 14),
        .message_font_size = readOnly(owner, f32, g, 12),
    });
    return fld;
}

/// Checkbox 复选框工厂。
pub fn checkbox(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    checked_sig: *reactive.Signal(bool),
    config: struct {
        color: Color = defaultColors.blue,
        on_change: ?*const fn (ctx: ?*anyopaque, checked: bool) callconv(.c) void = null,
        on_change_ctx: ?*anyopaque = null,
    },
) !*widgets.Checkbox {
    const cb = try widgets.Checkbox.create(allocator, g, .{
        .checked = checked_sig.asReadonly(),
        .color = readOnly(owner, Color, g, config.color),
        .disabled = readOnly(owner, bool, g, false),
    });
    if (config.on_change) |cb_fn| {
        cb.on_change = cb_fn;
        cb.on_change_ctx = config.on_change_ctx;
    }
    return cb;
}

/// Switch 开关工厂。
pub fn switch_(
    owner: *SignalOwner,
    allocator: Allocator,
    g: *Graph,
    checked_sig: *reactive.Signal(bool),
    config: struct {
        color: Color = defaultColors.green,
        on_change: ?*const fn (ctx: ?*anyopaque, checked: bool) callconv(.c) void = null,
        on_change_ctx: ?*anyopaque = null,
    },
) !*widgets.Switch {
    const sw = try widgets.Switch.create(allocator, g, .{
        .checked = checked_sig.asReadonly(),
        .color = readOnly(owner, Color, g, config.color),
        .disabled = readOnly(owner, bool, g, false),
    });
    if (config.on_change) |sw_fn| {
        sw.on_change = sw_fn;
        sw.on_change_ctx = config.on_change_ctx;
    }
    return sw;
}
