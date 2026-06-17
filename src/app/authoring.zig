//! app/authoring —— Authoring DSL 工厂函数
//!
//! 提供 Flutter 风格的声明式工厂函数，隐藏 allocator.create / Signal.init / addChild
//! 等样板代码。每个工厂函数返回 `*Node`，可直接挂到父节点上。
//!
//! 设计目标：
//!   1. 业务作者只需描述「有什么」「怎么排」，不用手工管理所有权。
//!   2. 工厂内部自动创建 Signal 和 EffectScope，组件销毁时自动清理。
//!   3. 未来可扩展为完整的 DSL（如 column(.{gap:8}, { label(...), button(...) })）。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const runtime = @import("../runtime/runtime.zig");
const widgets = @import("../widgets/widgets.zig");
const page_mod = @import("page.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Insets = foundation.Insets;
const Node = runtime.Node;
const Ref = page_mod.Ref;
const Graph = reactive.Graph;

// ── 内部辅助：创建单个 Signal 并返回 ReadSignal ──────────────────────────────

fn allocSignal(comptime T: type, g: *Graph, value: T) *reactive.Signal(T) {
    const s = std.heap.c_allocator.create(reactive.Signal(T)) catch unreachable;
    s.* = reactive.Signal(T).init(g, value);
    return s;
}

/// 创建一个值为 `value` 的 ReadSignal，用于非响应式（静态）属性输入。
/// 创建的 Signal 由调用方负责释放（当前 leak，适合静态 demo 场景）。
pub fn readOnly(comptime T: type, g: *Graph, value: T) reactive.ReadSignal(T) {
    return allocSignal(T, g, value).asReadonly();
}

// ── 默认信号工厂（用于非响应式属性） ──────────────────────────────────────────

const defaultColors = struct {
    pub const text = Color.fromHexRgb(0xCDD6F4);
    pub const bg = Color.fromHexRgb(0x1E1E2E);
    pub const blue = Color.fromHexRgb(0x89B4FA);
    pub const blue_hover = Color.fromHexRgb(0x74C7EC);
    pub const blue_pressed = Color.fromHexRgb(0x89DCEB);
    pub const surface0 = Color.fromHexRgb(0x313244);
    pub const mantle = Color.fromHexRgb(0x181825);
    pub const crust = Color.fromHexRgb(0x11111B);
};

// ═════════════════════════════════════════════════════════════════════════════
// 工厂函数
// ═════════════════════════════════════════════════════════════════════════════

/// Surface 工厂。
pub fn surface(
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
        .bg_color = readOnly(Color, g, config.bg_color),
        .corner_radius = readOnly(f32, g, config.corner_radius),
        .padding = readOnly(Insets, g, config.padding),
        .border_color = readOnly(Color, g, config.border_color),
        .border_width = readOnly(f32, g, config.border_width),
    });
    return &s.node;
}

/// Column 容器工厂。
pub fn column(
    allocator: Allocator,
    config: struct {
        gap: f32 = 0,
    },
) !*Node {
    const c = try widgets.Column.create(allocator, .{ .gap = config.gap });
    return &c.node;
}

/// Label 文本工厂。
pub fn label(
    allocator: Allocator,
    g: *Graph,
    text: []const u8,
    config: struct {
        color: Color = defaultColors.text,
        font_size: f32 = 14,
    },
) !*Node {
    const l = try widgets.Label.create(allocator, g, .{
        .text = readOnly([]const u8, g, text),
        .color = readOnly(Color, g, config.color),
        .font_size = readOnly(f32, g, config.font_size),
    });
    return &l.node;
}

/// Button 按钮工厂。
pub fn button(
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
        on_click: ?*const fn () void = null,
    },
) !*Node {
    const b = try widgets.Button.create(allocator, g, .{
        .label = readOnly([]const u8, g, label_text),
        .bg_color = readOnly(Color, g, config.bg_color),
        .bg_hover_color = readOnly(Color, g, config.bg_hover_color),
        .bg_pressed_color = readOnly(Color, g, config.bg_pressed_color),
        .text_color = readOnly(Color, g, config.text_color),
        .font_size = readOnly(f32, g, config.font_size),
        .corner_radius = readOnly(f32, g, config.corner_radius),
        .padding = readOnly(Insets, g, config.padding),
        .disabled = readOnly(bool, g, false),
    });
    if (config.on_click) |cb| b.on_click = cb;
    return &b.node;
}

/// Card 卡片工厂。
pub fn card(
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
        .title = readOnly([]const u8, g, title_text),
        .description = readOnly([]const u8, g, description_text),
        .bg_color = readOnly(Color, g, config.bg_color),
        .corner_radius = readOnly(f32, g, config.corner_radius),
        .padding = readOnly(Insets, g, Insets.all(16)),
        .title_font_size = readOnly(f32, g, config.title_font_size),
        .description_font_size = readOnly(f32, g, config.desc_font_size),
    });
    return &c.node;
}

/// Panel 面板工厂。
pub fn panel(
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
        .bg_color = readOnly(Color, g, config.bg_color),
        .corner_radius = readOnly(f32, g, config.corner_radius),
        .padding = readOnly(Insets, g, config.padding),
        .border_color = readOnly(Color, g, config.border_color),
        .border_width = readOnly(f32, g, config.border_width),
    });
    return &p.node;
}
