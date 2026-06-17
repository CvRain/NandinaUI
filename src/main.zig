//! NandinaUI —— 主入口 / 多页面 Showcase
//!
//! `zig build run` 启动一个 SDL3 窗口，展示框架的核心能力。
//! 使用 App/Page/Router 多层页面结构，支持导航切换。
//!
//! 若 SDL3 不可用（无图形环境），自动降级为文字烟雾测试。

const std = @import("std");
const nandina = @import("NandinaUI");
const sdl3 = @import("sdl_backend");

const app = nandina.app;
const authoring = app.authoring;
const widgets = nandina.widgets;
const runtime = nandina.runtime;
const reactive = nandina.reactive;
const render = nandina.render;
const layout = nandina.layout;
const f = nandina.foundation;

const Color = f.Color;
const Insets = f.Insets;
const Size = f.Size;
const Page = app.Page;
const PageHost = app.PageHost;
const Router = app.Router;

const WINDOW_W: u32 = 800;
const WINDOW_H: u32 = 600;

// ── 全局导航器指针（供按钮回调使用） ───────────────────────────────────────
var g_router: ?*Router = null;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    var buf: [1024]u8 = undefined;
    var wtr = std.Io.File.Writer.init(.stdout(), io, &buf);
    const out = &wtr.interface;

    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ nandina.version.major, nandina.version.minor, nandina.version.patch });
    try out.print("layers: foundation, reactive, render, layout, theme, text, runtime, widgets, app\n", .{});

    var window = sdl3.Window.init(init.gpa, "NandinaUI Showcase", WINDOW_W, WINDOW_H) catch |err| {
        try out.print("\nSDL3 窗口不可用（{s}），降级为文字演示。\n", .{@errorName(err)});
        try runSmokeTest(init, out);
        return;
    };
    defer window.deinit();

    try out.print("SDL3 窗口已打开（{d}x{d}），Showcase 启动中……\n", .{ WINDOW_W, WINDOW_H });
    try out.flush();

    try runShowcase(&window, init.gpa, out);
}

// ═════════════════════════════════════════════════════════════════════════════
// Showcase
// ═════════════════════════════════════════════════════════════════════════════

fn runShowcase(window: *sdl3.Window, allocator: std.mem.Allocator, _: *std.Io.Writer) !void {
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    // ── 字体后端 ──────────────────────────────────────────────────────────
    var ft_backend: nandina.text.backends.hb_ft.HarfBuzzFreeTypeMetrics = undefined;
    var using_real_font = false;
    var glyph_renderer: ?render.GlyphRenderer = null;
    if (nandina.text.backends.hb_ft.HarfBuzzFreeTypeMetrics.init(allocator)) |*fb| {
        ft_backend = fb.*;
        glyph_renderer = ft_backend.glyphRenderer();
        using_real_font = true;
    } else |_| {}
    if (glyph_renderer) |gr| {
        window.sw_renderer.setGlyphRenderer(gr);
    }

    // ── 页面列表 ──────────────────────────────────────────────────────────
    const pages = [_]Page{
        .{ .title = "概述", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph) !*runtime.Node {
                return buildOverview(a, gr);
            }
        }.f },
        .{ .title = "Widgets", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph) !*runtime.Node {
                return buildWidgets(a, gr);
            }
        }.f },
        .{ .title = "Layout", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph) !*runtime.Node {
                return buildLayout(a, gr);
            }
        }.f },
        .{ .title = "Reactive", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph) !*runtime.Node {
                return buildReactive(a, gr);
            }
        }.f },
        .{ .title = "Theme", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph) !*runtime.Node {
                return buildTheme(a, gr);
            }
        }.f },
    };

    // ── PageHost + Router ─────────────────────────────────────────────────
    var page_host = try PageHost.create(allocator, &g, &pages, 0);
    errdefer page_host.node.deinitTree(allocator);

    var router = Router.init(allocator, &.{}, page_host);
    g_router = &router;

    // ── 构建根 Widget 树 ──────────────────────────────────────────────────
    var tree = runtime.Tree.init(allocator);
    defer tree.deinit();
    defer if (using_real_font) ft_backend.deinit();

    // 根 Surface
    const root = try authoring.surface(allocator, &g, .{ .bg_color = C.base });

    // 主 Column（垂直布局）
    const main_col = try authoring.column(allocator, .{ .gap = 0 });
    try root.addChild(allocator, main_col);

    // ── 顶栏 ──────────────────────────────────────────────────────────────
    {
        const header = try widgets.Surface.create(allocator, &g, .{
            .bg_color = authoring.readOnly(Color, &g, C.mantle),
            .corner_radius = authoring.readOnly(f32, &g, 0),
            .padding = authoring.readOnly(Insets, &g, Insets.symmetric(16, 12)),
            .border_color = authoring.readOnly(Color, &g, C.surface0),
            .border_width = authoring.readOnly(f32, &g, 0),
        });
        try main_col.addChild(allocator, &header.node);

        const nav_col = try authoring.column(allocator, .{ .gap = 6 });
        // 标题
        const title_label = try authoring.label(allocator, &g, "NandinaUI Showcase", .{
            .color = C.text,
            .font_size = 20,
        });
        try nav_col.addChild(allocator, title_label);

        // 导航按钮
        {
            inline for (.{ "概述", "Widgets", "Layout", "Reactive", "Theme" }, 0..) |name, i| {
                const btn = try authoring.button(allocator, &g, name, .{
                    .bg_color = C.surface0,
                    .bg_hover_color = C.blue,
                    .bg_pressed_color = C.teal,
                    .text_color = C.text,
                    .font_size = 13,
                    .corner_radius = 4,
                    .padding = Insets.symmetric(12, 6),
                    .on_click = struct {
                        fn cb() void {
                            if (g_router) |r| _ = r.page_host.navigateTo(i) catch {};
                        }
                    }.cb,
                });
                try nav_col.addChild(allocator, btn);
            }
        }
        try header.node.addChild(allocator, nav_col);
    }

    // ── 页面内容区 ────────────────────────────────────────────────────────
    {
        const content_area = try widgets.Surface.create(allocator, &g, .{
            .bg_color = authoring.readOnly(Color, &g, C.base),
            .corner_radius = authoring.readOnly(f32, &g, 0),
            .padding = authoring.readOnly(Insets, &g, Insets.all(24)),
            .border_color = authoring.readOnly(Color, &g, C.base),
            .border_width = authoring.readOnly(f32, &g, 0),
        });
        try main_col.addChild(allocator, &content_area.node);
        try content_area.node.addChild(allocator, &page_host.node);
    }

    tree.setRoot(root);
    window.setTree(&tree);

    // ── 主循环 ───────────────────────────────────────────────────────────
    var frame_count: u32 = 0;
    while (window.isRunning()) {
        if (!try window.pollEvent()) break;
        window.dispatchEvents();
        frame_count += 1;
        if (frame_count == 60) {
            const has_display = std.c.getenv("DISPLAY") != null or std.c.getenv("WAYLAND_DISPLAY") != null;
            if (!has_display) window.close();
        }
        _ = try window.frame();
        window.present();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 页面构建函数
// ═════════════════════════════════════════════════════════════════════════════

const C = Colors{
    .base = Color.fromHexRgb(0x1E1E2E),
    .mantle = Color.fromHexRgb(0x181825),
    .crust = Color.fromHexRgb(0x11111B),
    .text = Color.fromHexRgb(0xCDD6F4),
    .blue = Color.fromHexRgb(0x89B4FA),
    .green = Color.fromHexRgb(0xA6E3A1),
    .red = Color.fromHexRgb(0xF38BA8),
    .peach = Color.fromHexRgb(0xFAB387),
    .mauve = Color.fromHexRgb(0xCBA6F7),
    .yellow = Color.fromHexRgb(0xF9E2AF),
    .teal = Color.fromHexRgb(0x94E2D5),
    .surface0 = Color.fromHexRgb(0x313244),
    .surface1 = Color.fromHexRgb(0x45475A),
    .surface2 = Color.fromHexRgb(0x585B70),
};

const Colors = struct {
    base: Color,
    mantle: Color,
    crust: Color,
    text: Color,
    blue: Color,
    green: Color,
    red: Color,
    peach: Color,
    mauve: Color,
    yellow: Color,
    teal: Color,
    surface0: Color,
    surface1: Color,
    surface2: Color,
};

/// 概述页
fn buildOverview(a: std.mem.Allocator, g: *reactive.Graph) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 16 });
    try col.addChild(a, try authoring.label(a, g, "NandinaUI", .{ .color = C.text, .font_size = 28 }));
    try col.addChild(a, try authoring.label(a, g, "Zig · reactive · layout · theme · widgets · SDL3 · software render", .{ .color = C.blue, .font_size = 14 }));
    try col.addChild(a, try authoring.card(a, g, "分层架构", "foundation → reactive → render → layout → theme → text → runtime → widgets → app", .{ .bg_color = C.mantle, .corner_radius = 8 }));
    try col.addChild(a, try authoring.card(a, g, "当前状态", "M0-M5: ✅ Core 完整 · P1: ✅ SDL3 + 字体 · P2: ✅ C ABI · M6: 🚧 Showcase", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 16 }));
    return col;
}

/// Widgets 页
fn buildWidgets(a: std.mem.Allocator, g: *reactive.Graph) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 16 });
    try col.addChild(a, try authoring.label(a, g, "组件展示", .{ .color = C.text, .font_size = 22 }));

    // Surface demo
    const surf = try authoring.surface(a, g, .{ .bg_color = C.mantle, .corner_radius = 8, .padding = Insets.all(16), .border_color = C.surface0, .border_width = 1 });
    try surf.addChild(a, try authoring.label(a, g, "Surface — 基础背景容器（圆角/描边/padding）", .{ .color = C.text, .font_size = 13 }));
    try col.addChild(a, surf);

    // Button demo
    try col.addChild(a, try authoring.button(a, g, "可点击按钮", .{
        .bg_color = C.blue,
        .bg_hover_color = Color.fromHexRgb(0x74C7EC),
        .bg_pressed_color = Color.fromHexRgb(0x89DCEB),
        .text_color = C.base,
        .font_size = 14,
        .corner_radius = 6,
        .padding = Insets.symmetric(20, 10),
    }));

    // Card demo
    try col.addChild(a, try authoring.card(a, g, "Card 组件", "组合 Surface + Label，带圆角背景的标题/描述容器", .{ .bg_color = C.mantle, .corner_radius = 8 }));

    // Panel demo
    const pnl = try authoring.panel(a, g, .{ .bg_color = C.crust, .corner_radius = 6, .padding = Insets.all(12), .border_color = C.surface0, .border_width = 1 });
    try pnl.addChild(a, try authoring.label(a, g, "Panel — 带边框/圆角的内容面板", .{ .color = C.text, .font_size = 13 }));
    try col.addChild(a, pnl);

    return col;
}

/// Layout 页
fn buildLayout(a: std.mem.Allocator, g: *reactive.Graph) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 16 });
    try col.addChild(a, try authoring.label(a, g, "布局系统", .{ .color = C.text, .font_size = 22 }));
    try col.addChild(a, try authoring.label(a, g, "Constraints + Flex/Flow/Anchors 三套纯函数求解器", .{ .color = C.blue, .font_size = 13 }));

    const demo_box = try authoring.surface(a, g, .{ .bg_color = C.mantle, .corner_radius = 8, .padding = Insets.all(16) });
    {
        const inner = try authoring.column(a, .{ .gap = 8 });
        try demo_box.addChild(a, inner);
        const items = [_]struct { label: []const u8, color: Color }{
            .{ .label = "Item 1 (flex: 1)", .color = C.blue },
            .{ .label = "Item 2 (flex: 2)", .color = C.green },
            .{ .label = "Item 3 (fixed)", .color = C.peach },
        };
        for (items) |item| {
            const it = try authoring.surface(a, g, .{ .bg_color = item.color.withAlpha(0.3), .corner_radius = 4, .padding = Insets.all(8) });
            try it.addChild(a, try authoring.label(a, g, item.label, .{ .color = C.text, .font_size = 12 }));
            try inner.addChild(a, it);
        }
    }
    try col.addChild(a, demo_box);
    return col;
}

/// Reactive 页
fn buildReactive(a: std.mem.Allocator, g: *reactive.Graph) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 16 });
    try col.addChild(a, try authoring.label(a, g, "响应式核心", .{ .color = C.text, .font_size = 22 }));
    try col.addChild(a, try authoring.label(a, g, "Signal → Computed → Effect，Angular 风格响应式数据流", .{ .color = C.blue, .font_size = 13 }));
    try col.addChild(a, try authoring.card(a, g, "核心原语", "Graph（调度图）· Signal（可写状态）· Computed（惰性派生）· Effect（副作用）· batch（批量更新）", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 16 }));
    try col.addChild(a, try authoring.card(a, g, "设计要点", "无全局状态（多 Graph 隔离）· 菱形依赖 glitch-free · 动态依赖自动重追踪 · EffectScope 生命周期管理", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 16 }));
    return col;
}

/// Theme 页
fn buildTheme(a: std.mem.Allocator, g: *reactive.Graph) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 16 });
    try col.addChild(a, try authoring.label(a, g, "主题系统", .{ .color = C.text, .font_size = 22 }));
    try col.addChild(a, try authoring.label(a, g, "Design Tokens + Semantic Palette + Theme Resolver", .{ .color = C.blue, .font_size = 13 }));

    const swatch_box = try authoring.surface(a, g, .{ .bg_color = C.mantle, .corner_radius = 8, .padding = Insets.all(16) });
    {
        const inner = try authoring.column(a, .{ .gap = 8 });
        try swatch_box.addChild(a, inner);
        const swatches = [_]Color{ C.red, C.peach, C.yellow, C.green, C.teal, C.blue, C.mauve };
        for (swatches) |sw| {
            try inner.addChild(a, try authoring.surface(a, g, .{ .bg_color = sw, .corner_radius = 4, .padding = Insets.all(6) }));
        }
    }
    try col.addChild(a, swatch_box);
    return col;
}

// ── 文字烟雾测试 ──────────────────────────────────────────────────────────

fn runSmokeTest(init: std.process.Init, out: *std.Io.Writer) !void {
    const gpa = init.gpa;
    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ nandina.version.major, nandina.version.minor, nandina.version.patch });
    try out.print("分层模块: foundation, reactive, render, layout, theme, text, runtime, widgets, app\n", .{});

    const rect = f.Rect.fromXywh(0, 0, 100, 80);
    try out.print("  foundation: 矩形中心 = ({d:.1}, {d:.1})\n", .{ rect.center().x, rect.center().y });

    var graph = reactive.Graph.init(gpa);
    defer graph.deinit();
    var count = reactive.Signal(i32).init(&graph, 42);
    defer count.deinit();
    const doubled = try reactive.computed(&graph, i32, &count, struct {
        fn f(s: *reactive.Signal(i32)) i32 {
            return s.get() * 2;
        }
    }.f);
    try out.print("  reactive:   signal({d}) → computed → {d}\n", .{ count.get(), doubled.get() });

    var scene = render.Scene.init(gpa);
    defer scene.deinit();
    try scene.fillRoundedRect(f.Rect.fromXywh(0, 0, 80, 24), 6, Color.fromHexRgb(0x89B4FA));
    try out.print("  render:     Scene 命令数 = {d}\n", .{scene.count()});

    const child_specs = [_]layout.flex.ChildSpec{ .{ .preferred = Size.init(100, 50) }, .{ .preferred = Size.init(80, 60) } };
    var frames: [2]f.Rect = undefined;
    layout.flex.solve(.{ .axis = .column, .gap = 8 }, f.Rect.fromXywh(0, 0, 200, 200), &child_specs, &frames);
    try out.print("  layout:     Column 布局 → frames[1].y = {d:.0}\n", .{frames[1].y()});

    try out.print("\n要查看图形界面，请在有显示器的环境中运行。\n", .{});
    try out.flush();
}

test "main 模块占位测试" {
    try std.testing.expect(nandina.version.major == 0);
}
