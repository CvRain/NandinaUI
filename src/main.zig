//! NandinaUI —— 主入口
//!
//! `zig build run` 启动一个 SDL3 窗口，展示框架的核心能力：
//! 响应式数据流、widget 组件库、布局系统、主题系统、渲染管线。
//!
//! 若 SDL3 不可用（无图形环境），自动降级为文字烟雾测试。
//! 详细的分层 demo 请运行 `zig build showcase -- <name>`。

const std = @import("std");
const nandina = @import("NandinaUI");
const sdl3 = @import("sdl_backend");

const widgets = nandina.widgets;
const runtime = nandina.runtime;
const reactive = nandina.reactive;
const render = nandina.render;
const f = nandina.foundation;
const Color = f.Color;
const Insets = f.Insets;
const Size = f.Size;

const WINDOW_W: u32 = 800;
const WINDOW_H: u32 = 600;

pub fn main(init: std.process.Init) !void {
    // 先打烟雾测试信息
    const io = init.io;
    var stdout_buf: [1024]u8 = undefined;
    var stdout_writer: std.Io.File.Writer = .init(.stdout(), io, &stdout_buf);
    const out = &stdout_writer.interface;

    const v = nandina.version;
    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ v.major, v.minor, v.patch });
    try out.print("layers: foundation, reactive, render, layout, theme, text, runtime, widgets, app\n", .{});

    // 尝试创建 SDL3 窗口；若失败（无显示环境），降级到文字输出
    var window = sdl3.Window.init(init.gpa, "NandinaUI", WINDOW_W, WINDOW_H) catch |err| {
        try out.print("\nSDL3 窗口不可用（{s}），降级为文字演示：\n\n", .{@errorName(err)});
        try runSmokeTest(init, out);
        return;
    };
    defer window.deinit();

    try out.print("SDL3 窗口已打开（{d}x{d}），正在构建界面……\n", .{ WINDOW_W, WINDOW_H });
    try out.flush();

    // 构建展示界面并进入主循环
    try runWindow(&window, init.gpa, out);
}

// ── SDL3 窗口模式 ──────────────────────────────────────────────────────────

fn runWindow(window: *sdl3.Window, allocator: std.mem.Allocator, _: *std.Io.Writer) !void {
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    // ── 颜色常量（Catppuccin Mocha 风格） ────────────────────────────────
    const C = struct {
        const base = Color.fromHexRgb(0x1E1E2E);
        const mantle = Color.fromHexRgb(0x181825);
        const crust = Color.fromHexRgb(0x11111B);
        const text = Color.fromHexRgb(0xCDD6F4);
        const blue = Color.fromHexRgb(0x89B4FA);
        const green = Color.fromHexRgb(0xA6E3A1);
        const red = Color.fromHexRgb(0xF38BA8);
        const peach = Color.fromHexRgb(0xFAB387);
        const mauve = Color.fromHexRgb(0xCBA6F7);
        const yellow = Color.fromHexRgb(0xF9E2AF);
        const teal = Color.fromHexRgb(0x94E2D5);
        const surface0 = Color.fromHexRgb(0x313244);
        const surface1 = Color.fromHexRgb(0x45475A);
    };

    // ── 初始化真实字体后端（若可用，否则回退 MonospaceMetrics） ──────────
    var ft_backend: nandina.text.backends.hb_ft.HarfBuzzFreeTypeMetrics = undefined;
    var using_real_font = false;
    var glyph_renderer: ?render.GlyphRenderer = null;
    if (nandina.text.backends.hb_ft.HarfBuzzFreeTypeMetrics.init(allocator)) |*fb| {
        ft_backend = fb.*;
        glyph_renderer = ft_backend.glyphRenderer();
        using_real_font = true;
    } else |_| {}

    // 将字体 glyph 渲染器注入窗口的 SoftwareBackend
    if (glyph_renderer) |gr| {
        window.renderer.setGlyphRenderer(gr);
    }

    // ── 响应式状态 ────────────────────────────────────────────────────────
    var bg = reactive.Signal(Color).init(&g, C.base);
    defer bg.deinit();
    var corner = reactive.Signal(f32).init(&g, 0);
    defer corner.deinit();
    var pad = reactive.Signal(Insets).init(&g, Insets.all(24));
    defer pad.deinit();
    var border_color = reactive.Signal(Color).init(&g, C.surface0);
    defer border_color.deinit();
    var border_w = reactive.Signal(f32).init(&g, 0);
    defer border_w.deinit();

    var title_text = reactive.Signal([]const u8).init(&g, "NandinaUI");
    defer title_text.deinit();
    var desc_text = reactive.Signal([]const u8).init(&g, "Zig · reactive · layout · theme · widgets · SDL3 · software render");
    defer desc_text.deinit();

    // Card 信号
    var card_title = reactive.Signal([]const u8).init(&g, "Card 组件");
    defer card_title.deinit();
    var card_desc = reactive.Signal([]const u8).init(&g, "组合 Surface + Label，带圆角背景的标题/描述容器");
    defer card_desc.deinit();
    var card_bg = reactive.Signal(Color).init(&g, C.mantle);
    defer card_bg.deinit();
    var card_cr = reactive.Signal(f32).init(&g, 8);
    defer card_cr.deinit();
    var card_pad = reactive.Signal(Insets).init(&g, Insets.all(16));
    defer card_pad.deinit();
    var card_tfs = reactive.Signal(f32).init(&g, 18);
    defer card_tfs.deinit();
    var card_dfs = reactive.Signal(f32).init(&g, 13);
    defer card_dfs.deinit();

    // Panel 信号
    var panel_title = reactive.Signal([]const u8).init(&g, "Panel 容器（带边框 + 圆角）");
    defer panel_title.deinit();
    var panel_bg = reactive.Signal(Color).init(&g, C.crust);
    defer panel_bg.deinit();
    var panel_cr = reactive.Signal(f32).init(&g, 6);
    defer panel_cr.deinit();
    var panel_pad = reactive.Signal(Insets).init(&g, Insets.all(12));
    defer panel_pad.deinit();
    var panel_bc = reactive.Signal(Color).init(&g, C.surface0);
    defer panel_bc.deinit();
    var panel_bw = reactive.Signal(f32).init(&g, 1);
    defer panel_bw.deinit();
    var panel_fs = reactive.Signal(f32).init(&g, 14);
    defer panel_fs.deinit();
    var panel_fg = reactive.Signal(Color).init(&g, C.text);
    defer panel_fg.deinit();

    // Button 信号
    var btn_label = reactive.Signal([]const u8).init(&g, "Button 组件");
    defer btn_label.deinit();
    var btn_bg = reactive.Signal(Color).init(&g, C.blue);
    defer btn_bg.deinit();
    var btn_hover = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x74C7EC));
    defer btn_hover.deinit();
    var btn_pressed = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x89DCEB));
    defer btn_pressed.deinit();
    var btn_fg = reactive.Signal(Color).init(&g, C.base);
    defer btn_fg.deinit();
    var btn_fs = reactive.Signal(f32).init(&g, 14);
    defer btn_fs.deinit();
    var btn_cr = reactive.Signal(f32).init(&g, 6);
    defer btn_cr.deinit();
    var btn_pad = reactive.Signal(Insets).init(&g, Insets.symmetric(20, 10));
    defer btn_pad.deinit();
    var btn_disabled = reactive.Signal(bool).init(&g, false);
    defer btn_disabled.deinit();

    // 标题/副标题信号
    var title_fs = reactive.Signal(f32).init(&g, 28);
    defer title_fs.deinit();
    var title_color = reactive.Signal(Color).init(&g, C.text);
    defer title_color.deinit();
    var sub_fs = reactive.Signal(f32).init(&g, 14);
    defer sub_fs.deinit();
    var sub_color = reactive.Signal(Color).init(&g, C.blue);
    defer sub_color.deinit();

    // ── Widget 树 ─────────────────────────────────────────────────────────
    var tree = runtime.Tree.init(allocator);
    defer tree.deinit();
    defer if (using_real_font) ft_backend.deinit();

    // 根 Surface
    const root = try widgets.Surface.create(allocator, &g, .{
        .bg_color = bg.asReadonly(),
        .corner_radius = corner.asReadonly(),
        .padding = pad.asReadonly(),
        .border_color = border_color.asReadonly(),
        .border_width = border_w.asReadonly(),
    });

    // 主 Column
    const col = try widgets.Column.create(allocator, .{ .gap = 20 });
    try root.node.addChild(allocator, &col.node);

    // ── 1. 标题 ─────────────────────────────────────────────────────────────
    {
        const label = try widgets.Label.create(allocator, &g, .{
            .text = title_text.asReadonly(),
            .font_size = title_fs.asReadonly(),
            .color = title_color.asReadonly(),
        });
        try col.node.addChild(allocator, &label.node);
    }

    // ── 2. 副标题 ──────────────────────────────────────────────────────────
    {
        const label = try widgets.Label.create(allocator, &g, .{
            .text = desc_text.asReadonly(),
            .font_size = sub_fs.asReadonly(),
            .color = sub_color.asReadonly(),
        });
        try col.node.addChild(allocator, &label.node);
    }

    // ── 3. Card ────────────────────────────────────────────────────────────
    {
        const card = try widgets.Card.create(allocator, &g, .{
            .title = card_title.asReadonly(),
            .description = card_desc.asReadonly(),
            .bg_color = card_bg.asReadonly(),
            .corner_radius = card_cr.asReadonly(),
            .padding = card_pad.asReadonly(),
            .title_font_size = card_tfs.asReadonly(),
            .description_font_size = card_dfs.asReadonly(),
        });
        try col.node.addChild(allocator, &card.node);
    }

    // ── 4. Panel ───────────────────────────────────────────────────────────
    {
        const panel = try widgets.Panel.create(allocator, &g, .{
            .bg_color = panel_bg.asReadonly(),
            .corner_radius = panel_cr.asReadonly(),
            .padding = panel_pad.asReadonly(),
            .border_color = panel_bc.asReadonly(),
            .border_width = panel_bw.asReadonly(),
        });

        // Panel 内部放一个 Label
        {
            const label = try widgets.Label.create(allocator, &g, .{
                .text = panel_title.asReadonly(),
                .font_size = panel_fs.asReadonly(),
                .color = panel_fg.asReadonly(),
            });
            try panel.node.addChild(allocator, &label.node);
        }
        try col.node.addChild(allocator, &panel.node);
    }

    // ── 5. Button 展示 ────────────────────────────────────────────────────
    {
        const btn = try widgets.Button.create(allocator, &g, .{
            .label = btn_label.asReadonly(),
            .bg_color = btn_bg.asReadonly(),
            .bg_hover_color = btn_hover.asReadonly(),
            .bg_pressed_color = btn_pressed.asReadonly(),
            .text_color = btn_fg.asReadonly(),
            .font_size = btn_fs.asReadonly(),
            .corner_radius = btn_cr.asReadonly(),
            .padding = btn_pad.asReadonly(),
            .disabled = btn_disabled.asReadonly(),
        });
        try col.node.addChild(allocator, &btn.node);
    }

    tree.setRoot(&root.node);
    window.setTree(&tree);

    // ── 主循环 ───────────────────────────────────────────────────────────
    var frame_count: u32 = 0;
    while (window.isRunning()) {
        if (!try window.pollEvent()) break;
        window.dispatchEvents();
        frame_count += 1;

        // 无显示环境时自动退出
        if (frame_count == 60) {
            const has_display = std.c.getenv("DISPLAY") != null or
                std.c.getenv("WAYLAND_DISPLAY") != null;
            if (!has_display) window.close();
        }

        _ = try window.frame();
        window.present();
    }
}

// ── 文字烟雾测试（降级 / 无显示环境） ──────────────────────────────────────

fn runSmokeTest(init: std.process.Init, out: *std.Io.Writer) !void {
    const gpa = init.gpa;

    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ nandina.version.major, nandina.version.minor, nandina.version.patch });
    try out.print("分层模块: foundation, reactive, render, layout, theme, text, runtime, widgets, app\n", .{});

    // foundation 验证
    const rect = f.Rect.fromXywh(0, 0, 100, 80);
    try out.print("  foundation: 矩形中心 = ({d:.1}, {d:.1})\n", .{ rect.center().x, rect.center().y });

    // reactive 验证
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

    // render 验证
    var scene = render.Scene.init(gpa);
    defer scene.deinit();
    try scene.fillRoundedRect(f.Rect.fromXywh(0, 0, 80, 24), 6, Color.fromHexRgb(0x89B4FA));
    try out.print("  render:     Scene 命令数 = {d}\n", .{scene.count()});

    // layout 验证
    const child_specs = [_]nandina.layout.flex.ChildSpec{
        .{ .preferred = Size.init(100, 50) },
        .{ .preferred = Size.init(80, 60) },
    };
    var frames: [2]f.Rect = undefined;
    nandina.layout.flex.solve(.{ .axis = .column, .gap = 8 }, f.Rect.fromXywh(0, 0, 200, 200), &child_specs, &frames);
    try out.print("  layout:     Column 布局 → frames[1].y = {d:.0}\n", .{frames[1].y()});

    try out.print("\n要查看图形界面，请在有显示器的环境中运行。\n", .{});
    try out.print("分层演示请运行: zig build showcase -- <name>\n", .{});
    try out.print("  demo 列表:  zig build showcase -- list\n", .{});
    try out.flush();
}

// ── tests ──────────────────────────────────────────────────────────────────

test "main 模块占位测试" {
    try std.testing.expect(nandina.version.major == 0);
}

// 引用 layout 模块让测试收集到
const layout = nandina.layout;
