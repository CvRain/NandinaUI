//! demos/theme —— 主题与设计令牌演示
//!
//! 展示：按语义角色取色、light/dark 切换、token 解析，以及「类似 global.css」的
//! 自定义主题（覆盖 token + palette）。最后演示 theme → render：用主题色生成一帧卡片。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const theme = nandina.theme;
const render = nandina.render;
const f = nandina.foundation;

fn printColor(out: *std.Io.Writer, label: []const u8, c: f.Color) !void {
    try out.print("    {s: <22} #{X:0>8}\n", .{ label, c.toHexRgba() });
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;

    // 1) 默认主题：light / dark 下的语义角色取色
    var t = theme.Theme.default();
    try out.print("默认主题 light scheme：\n", .{});
    try printColor(out, "surface", t.color(.surface));
    try printColor(out, "on_surface", t.color(.on_surface));
    try printColor(out, "primary", t.color(.primary));

    t.setScheme(.dark);
    try out.print("切换到 dark scheme：\n", .{});
    try printColor(out, "surface", t.color(.surface));
    try printColor(out, "on_surface", t.color(.on_surface));
    try printColor(out, "primary", t.color(.primary));

    // 2) token 解析
    try out.print("\n令牌解析：\n", .{});
    try out.print("    spacing.md   = {d}\n", .{t.tokens.spacing.md});
    try out.print("    radius.lg    = {d}\n", .{t.tokens.radius.lg});
    const headline = t.typeStyle(.headline_large);
    try out.print("    headline_large = {d}px / weight {d}\n", .{
        headline.font_size, @intFromEnum(headline.font_weight),
    });

    // 3) 「类似 global.css」：自定义主题覆盖品牌色与圆角
    try out.print("\n自定义主题（覆盖 primary 品牌色 + radius.md）：\n", .{});
    var brand_theme = theme.Theme{
        .name = "brand",
        .palette = theme.Palette.default(),
        .tokens = .{ .radius = .{ .md = 6 } },
    };
    const brand = f.Color.fromHexRgb(0x3B82F6);
    brand_theme.palette.set(.primary, brand, brand);
    try printColor(out, "primary (light)", brand_theme.colorIn(.primary, .light));
    try out.print("    radius.md    = {d}（默认 12 → 覆盖为 6）\n", .{brand_theme.tokens.radius.md});

    // 4) theme → render：用主题语义色生成一帧卡片绘制命令
    try out.print("\ntheme → render（用语义色构造卡片）：\n", .{});
    var scene = render.Scene.init(ctx.allocator);
    defer scene.deinit();
    const card_rect = f.Rect.fromXywh(0, 0, 240, 120);
    try scene.fillRect(card_rect, brand_theme.colorIn(.background, .light));
    try scene.fillRoundedRect(
        f.Rect.fromXywh(16, 16, 208, 88),
        brand_theme.tokens.radius.md,
        brand_theme.colorIn(.surface, .light),
    );
    try scene.fillRoundedRect(
        f.Rect.fromXywh(28, 64, 96, 28),
        brand_theme.tokens.radius.sm,
        brand_theme.colorIn(.primary, .light),
    );
    for (scene.commands.items) |cmd| {
        switch (cmd) {
            .fill_rect => |c| try out.print("    FillRect        color=#{X:0>8}\n", .{c.color.toHexRgba()}),
            .fill_rounded_rect => |c| try out.print("    FillRoundedRect r={d} color=#{X:0>8}\n", .{ c.radius, c.color.toHexRgba() }),
            else => {},
        }
    }
}

pub const demo = registry.Demo{
    .name = "theme",
    .summary = "设计令牌 + 语义调色板 + light/dark 切换 + 自定义主题",
    .run = run,
};
