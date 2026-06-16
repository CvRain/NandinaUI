//! demos/sdl_window —— SDL3 窗口展示（P1 验证）
//!
//! 打开一个可调整大小的 SDL3 窗口，在其中用 NandinaUI 的 runtime.Tree 创建一棵
//! 简单的 widget 树（背景 Surface + 文本 Label），演示完整链路：
//!
//!   reactive Signal → widgets → runtime.Tree(layout+repaint→Scene)
//!     → SoftwareBackend(光栅化→ARGB像素) → SDL3 窗口呈现
//!
//! 需要图形环境（X11 / Wayland）。若运行在无显示器的终端中，此 demo 会优雅退出。
//!
//! 依赖：SDL3（通过 zon 包），在 build.zig 中已链接。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const sdl3 = @import("sdl_backend");
const widgets = nandina.widgets;
const runtime = nandina.runtime;
const reactive = nandina.reactive;
const f = nandina.foundation;

const Color = f.Color;
const Insets = f.Insets;

const WINDOW_W: u32 = 640;
const WINDOW_H: u32 = 480;

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const a = ctx.allocator;

    try out.print("正在打开 SDL3 窗口（{d}x{d}）……\n", .{ WINDOW_W, WINDOW_H });
    try out.print("（若在无图形环境的终端中运行，此 demo 会静默退出）\n", .{});

    // ── 响应式状态 ──────────────────────────────────────────────────────────
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var bg_color = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x1E1E2E));
    defer bg_color.deinit();
    var text_color = reactive.Signal(Color).init(&g, Color.fromHexRgb(0xCDD6F4));
    defer text_color.deinit();
    var corner_radius = reactive.Signal(f32).init(&g, 12);
    defer corner_radius.deinit();
    var padding = reactive.Signal(Insets).init(&g, Insets.all(24));
    defer padding.deinit();
    var border_width = reactive.Signal(f32).init(&g, 0);
    defer border_width.deinit();
    var border_color = reactive.Signal(Color).init(&g, Color.transparent);
    defer border_color.deinit();
    var greeting = reactive.Signal([]const u8).init(&g, "Hello NandinaUI!");
    defer greeting.deinit();
    var subtext = reactive.Signal([]const u8).init(&g, "Zig + SDL3 + Software Render");
    defer subtext.deinit();

    // ── Widget 树 ──────────────────────────────────────────────────────────
    var tree = runtime.Tree.init(a);
    defer tree.deinit();

    // Surface 作为根背景
    const surface = try widgets.Surface.create(a, &g, .{
        .bg_color = bg_color.asReadonly(),
        .corner_radius = corner_radius.asReadonly(),
        .padding = padding.asReadonly(),
        .border_color = border_color.asReadonly(),
        .border_width = border_width.asReadonly(),
    });

    var title_font_size = reactive.Signal(f32).init(&g, 28);
    defer title_font_size.deinit();
    var sub_font_size = reactive.Signal(f32).init(&g, 16);
    defer sub_font_size.deinit();

    // Column 容器，垂直排列文本
    const column = try widgets.Column.create(a, .{ .gap = 6 });
    try surface.node.addChild(a, &column.node);

    // 用 Label 显示文本
    const title_label = try widgets.Label.create(a, &g, .{
        .text = greeting.asReadonly(),
        .font_size = title_font_size.asReadonly(),
        .color = text_color.asReadonly(),
    });
    try column.node.addChild(a, &title_label.node);

    // 副标题
    const sub_label = try widgets.Label.create(a, &g, .{
        .text = subtext.asReadonly(),
        .font_size = sub_font_size.asReadonly(),
        .color = text_color.asReadonly(),
    });
    try column.node.addChild(a, &sub_label.node);

    tree.setRoot(&surface.node);

    // ── SDL3 窗口 ──────────────────────────────────────────────────────────
    var window = sdl3.Window.init(a, "NandinaUI Showcase", WINDOW_W, WINDOW_H) catch |err| {
        try out.print("SDL3 窗口创建失败（{s}），跳过此 demo\n", .{@errorName(err)});
        return;
    };
    defer window.deinit();

    window.setTree(&tree);

    // ── 主循环 ────────────────────────────────────────────────────────────
    var frame_count: u32 = 0;
    while (window.isRunning()) {
        if (!try window.pollEvent()) {
            // 窗口关闭请求
            break;
        }
        window.dispatchEvents();

        // 3 秒后修改响应式状态，验证实时更新
        frame_count += 1;
        if (frame_count == 1800) { // ~3 秒 @ 60fps
            greeting.set("响应式更新 ✓");
            subtext.set("Signal → dirty → relayout → repaint");
        }
        if (frame_count == 3600) { // ~6 秒
            bg_color.set(Color.fromHexRgb(0x313244));
            greeting.set("背景色也变了！");
        }
        if (frame_count == 5400) { // ~9 秒
            window.close();
            try out.print("窗口已关闭（运行了 ~9 秒，{d} 帧）\n", .{frame_count});
        }

        _ = try window.frame();
        window.present();

        // 简单帧率控制（~60fps）
        // 注意：真实应用应使用 SDL_WaitEvent 或 vsync，这里用简单 spin 延迟
        // 演示用，不精确。
        // c.SDL_Delay(16); // 取消注释以限制帧率
    }
}

pub const demo = registry.Demo{
    .name = "sdl-window",
    .summary = "SDL3 窗口 + SoftwareBackend 渲染演示",
    .run = run,
};
