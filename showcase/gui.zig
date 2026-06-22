//! showcase/gui —— NandinaUI SDL3 可视化展示应用
//!
//! `zig build run` 启动本程序：打开 SDL3 窗口，展示多页面组件画廊。
//! 命令行 demo 请运行 `zig build run-showcase`。
//!
//! 本文件仅包含 main 入口与主循环；页面内容由 `gui_pages.zig` 提供。

const std = @import("std");
const nandina = @import("NandinaUI");
const sdl3 = @import("sdl_backend");
const gui_pages = @import("gui_pages.zig");

const app = nandina.app;
const authoring = app.authoring;
const widgets = nandina.widgets;
const runtime = nandina.runtime;
const reactive = nandina.reactive;
const render = nandina.render;
const f = nandina.foundation;

const Color = f.Color;
const Insets = f.Insets;
const Page = app.Page;
const PageHost = app.PageHost;
const Router = app.Router;

const WINDOW_W: u32 = 800;
const WINDOW_H: u32 = 600;

const C = gui_pages.C;

// ── 全局导航器（供按钮回调使用）──────────────────────────────────────────
var g_router: ?*Router = null;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    var buf: [1024]u8 = undefined;
    var wtr = std.Io.File.Writer.init(.stdout(), io, &buf);
    const out = &wtr.interface;

    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ nandina.version.major, nandina.version.minor, nandina.version.patch });
    try out.print("layers: foundation, reactive, render, layout, theme, text, runtime, widgets, app\n", .{});

    var window = sdl3.Window.init(init.gpa, "NandinaUI Showcase", WINDOW_W, WINDOW_H) catch |err| {
        try out.print("\nSDL3 窗口不可用（{s}），无法启动图形界面。\n", .{@errorName(err)});
        // 无 GUI 环境时，提示用户运行命令行版
        try out.print("提示：运行 `zig build run-showcase` 可查看命令行演示。\n", .{});
        return;
    };
    defer window.deinit();

    try out.print("SDL3 窗口已打开（{d}x{d}），Showcase 启动中……\n", .{ WINDOW_W, WINDOW_H });
    try out.flush();

    try runShowcase(&window, init.gpa, out);
}

// ═════════════════════════════════════════════════════════════════════════════
// Showcase 主逻辑
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
            fn f(a: std.mem.Allocator, gr: *reactive.Graph, o: *app.SignalOwner) !*runtime.Node {
                return gui_pages.buildOverview(a, gr, o);
            }
        }.f },
        .{ .title = "Widgets", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph, o: *app.SignalOwner) !*runtime.Node {
                return gui_pages.buildWidgets(a, gr, o);
            }
        }.f },
        .{ .title = "Layout", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph, o: *app.SignalOwner) !*runtime.Node {
                return gui_pages.buildLayout(a, gr, o);
            }
        }.f },
        .{ .title = "Reactive", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph, o: *app.SignalOwner) !*runtime.Node {
                return gui_pages.buildReactive(a, gr, o);
            }
        }.f },
        .{ .title = "Theme", .build = struct {
            fn f(a: std.mem.Allocator, gr: *reactive.Graph, o: *app.SignalOwner) !*runtime.Node {
                return gui_pages.buildTheme(a, gr, o);
            }
        }.f },
    };

    // ── PageHost + Router ─────────────────────────────────────────────────
    var page_host = try PageHost.create(allocator, &g, &pages, 0);
    errdefer page_host.node.deinitTree(allocator);

    var router = Router.init(allocator, &.{}, page_host);
    g_router = &router;

    // ── 根级 SignalOwner ─────────────────────────────────────────────────
    var root_owner = app.SignalOwner.init(allocator);
    defer root_owner.deinit();

    // ── 构建根 Widget 树 ──────────────────────────────────────────────────
    var tree = runtime.Tree.init(allocator);
    defer tree.deinit();
    defer if (using_real_font) ft_backend.deinit();

    const root = try authoring.surface(&root_owner, allocator, &g, .{ .bg_color = C.base });
    const main_col = try authoring.column(allocator, .{ .gap = 0, .cross_align = .stretch });
    try root.addChild(allocator, main_col);

    // ── 顶栏 ──────────────────────────────────────────────────────────────
    {
        const header = try widgets.Surface.create(allocator, &g, .{
            .bg_color = authoring.readOnly(&root_owner, Color, &g, C.mantle),
            .corner_radius = authoring.readOnly(&root_owner, f32, &g, 0),
            .padding = authoring.readOnly(&root_owner, Insets, &g, Insets.symmetric(16, 12)),
            .border_color = authoring.readOnly(&root_owner, Color, &g, C.surface0),
            .border_width = authoring.readOnly(&root_owner, f32, &g, 0),
        });
        try main_col.addChild(allocator, &header.node);

        const nav_col = try authoring.column(allocator, .{ .gap = 6 });
        try nav_col.addChild(allocator, try authoring.label(&root_owner, allocator, &g, "NandinaUI Showcase", .{ .color = C.text, .font_size = 20 }));

        inline for (.{ "概述", "Widgets", "Layout", "Reactive", "Theme" }, 0..) |name, i| {
            const btn = try authoring.button(&root_owner, allocator, &g, name, .{
                .bg_color = C.surface0,
                .bg_hover_color = C.blue,
                .bg_pressed_color = C.teal,
                .text_color = C.text,
                .font_size = 13,
                .corner_radius = 4,
                .padding = Insets.symmetric(12, 6),
                .on_click = struct {
                    fn cb(_: ?*anyopaque) callconv(.c) void {
                        if (g_router) |r| _ = r.page_host.navigateTo(i) catch {};
                    }
                }.cb,
            });
            try nav_col.addChild(allocator, btn);
        }
        try header.node.addChild(allocator, nav_col);
    }

    // ── 页面内容区 ────────────────────────────────────────────────────────
    {
        const content_area = try widgets.Surface.create(allocator, &g, .{
            .bg_color = authoring.readOnly(&root_owner, Color, &g, C.base),
            .corner_radius = authoring.readOnly(&root_owner, f32, &g, 0),
            .padding = authoring.readOnly(&root_owner, Insets, &g, Insets.all(24)),
            .border_color = authoring.readOnly(&root_owner, Color, &g, C.base),
            .border_width = authoring.readOnly(&root_owner, f32, &g, 0),
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
