//! demos/software_render —— 软件光栅渲染演示（ASCII 预览）
//!
//! 用 `SoftwareBackend` 把一棵 widget 树真正光栅化到内存像素缓冲，再把缓冲降采样成
//! ASCII 字符画打印出来 —— 在无图形窗口的终端里第一次「看到」NandinaUI 的渲染产物。
//!
//! 链路：reactive Signal → widgets（Card）→ runtime.Tree（layout+repaint → Scene）
//!       → SoftwareBackend（Scene → ARGB 像素）→ ASCII 降采样输出。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const widgets = nandina.widgets;
const runtime = nandina.runtime;
const reactive = nandina.reactive;
const render = nandina.render;
const f = nandina.foundation;

const Color = f.Color;
const Insets = f.Insets;

const W: u32 = 64;
const H: u32 = 32;

/// 按亮度把像素映射到 ASCII 字符（从暗到亮）。
const ramp = " .:-=+*#%@";

fn luminanceChar(px: u32) u8 {
    const a: f32 = @floatFromInt((px >> 24) & 0xFF);
    if (a < 8) return ' '; // 透明 → 空白
    const r: f32 = @floatFromInt((px >> 16) & 0xFF);
    const g: f32 = @floatFromInt((px >> 8) & 0xFF);
    const b: f32 = @floatFromInt(px & 0xFF);
    const lum = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
    const idx: usize = @intFromFloat(lum * @as(f32, @floatFromInt(ramp.len - 1)));
    return ramp[idx];
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const a = ctx.allocator;

    var g = reactive.Graph.init(a);
    defer g.deinit();

    // ── 背后 Signal（demo 持有，生命周期长于组件）──
    var title = reactive.Signal([]const u8).init(&g, "Nandina");
    defer title.deinit();
    var desc = reactive.Signal([]const u8).init(&g, "software render");
    defer desc.deinit();
    var bg = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x3B82F6));
    defer bg.deinit();
    var cr = reactive.Signal(f32).init(&g, 8);
    defer cr.deinit();
    var pad = reactive.Signal(Insets).init(&g, Insets.all(6));
    defer pad.deinit();
    var tfs = reactive.Signal(f32).init(&g, 7);
    defer tfs.deinit();
    var dfs = reactive.Signal(f32).init(&g, 5);
    defer dfs.deinit();

    var tree = runtime.Tree.init(a);
    defer tree.deinit();

    const card = try widgets.Card.create(a, &g, .{
        .title = title.asReadonly(),
        .description = desc.asReadonly(),
        .bg_color = bg.asReadonly(),
        .corner_radius = cr.asReadonly(),
        .padding = pad.asReadonly(),
        .title_font_size = tfs.asReadonly(),
        .description_font_size = dfs.asReadonly(),
    });
    tree.setRoot(&card.node);
    tree.setViewport(.{ .width = @floatFromInt(W), .height = @floatFromInt(H) });

    // 一帧：布局 + 生成 Scene
    _ = try tree.frame();
    try out.print("Scene 命令数 = {d}\n", .{tree.scene.count()});

    // ── 软件光栅：Scene → 像素缓冲 ──
    const pixels = try a.alloc(u32, W * H);
    defer a.free(pixels);
    @memset(pixels, 0xFF1E1E2E); // 不透明深色底（Catppuccin base 风格）

    var be = render.SoftwareBackend.init();
    const backend = be.interface();
    try backend.beginFrame(.{ .pixels = pixels.ptr, .width = W, .height = H, .stride = W });
    try backend.submit(&tree.scene);
    try backend.endFrame();

    // ── ASCII 预览 ──
    try out.print("\n软件光栅渲染产物（{d}x{d} 降采样为 ASCII）：\n", .{ W, H });
    try out.print("+", .{});
    try out.splatByteAll('-', W);
    try out.print("+\n", .{});
    var y: u32 = 0;
    while (y < H) : (y += 1) {
        try out.print("|", .{});
        var x: u32 = 0;
        while (x < W) : (x += 1) {
            try out.writeByte(luminanceChar(pixels[y * W + x]));
        }
        try out.print("|\n", .{});
    }
    try out.print("+", .{});
    try out.splatByteAll('-', W);
    try out.print("+\n", .{});

    try out.print("\n（圆角卡片 + 标题/描述占位条；这是 SoftwareBackend 真正光栅化的像素）\n", .{});
}

pub const demo = registry.Demo{
    .name = "software-render",
    .summary = "SoftwareBackend 把 widget 树光栅化为像素并 ASCII 预览",
    .run = run,
};
