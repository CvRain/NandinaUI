//! demos/widgets_gallery —— widgets 组件 + 全链路演示
//!
//! 把 Card / Button / Panel 组件挂进一棵 runtime.Tree，跑 frame 输出 Scene 命令，
//! 直观展示「reactive → widgets → runtime(layout+repaint) → render」完整链路。
//! 还演示 ClipNode 统一裁剪：把一个超大子内容包进 ClipNode，绘制时自动 push/pop clip。
//!
//! 注意：组件以 ReadSignal 输入，背后的 Signal 由本 demo 持有（生命周期长于组件）。

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

fn printScene(out: *std.Io.Writer, scene: *const render.Scene) !void {
    for (scene.commands.items) |cmd| {
        switch (cmd) {
            .fill_rect => |c| try out.print("    FillRect        ({d:.0},{d:.0} {d:.0}x{d:.0}) #{X:0>8}\n", .{ c.rect.x(), c.rect.y(), c.rect.width(), c.rect.height(), c.color.toHexRgba() }),
            .fill_rounded_rect => |c| try out.print("    FillRoundedRect ({d:.0},{d:.0} {d:.0}x{d:.0}) r={d:.0} #{X:0>8}\n", .{ c.rect.x(), c.rect.y(), c.rect.width(), c.rect.height(), c.radius, c.color.toHexRgba() }),
            .draw_text => |c| try out.print("    DrawText        \"{s}\" at=({d:.0},{d:.0}) size={d:.0}\n", .{ c.text, c.x, c.y, c.font_size }),
            .push_clip => |c| try out.print("    PushClip        ({d:.0},{d:.0} {d:.0}x{d:.0}) r={d:.0}\n", .{ c.rect.x(), c.rect.y(), c.rect.width(), c.rect.height(), c.radius }),
            .pop_clip => try out.print("    PopClip\n", .{}),
        }
    }
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const a = ctx.allocator;

    var g = reactive.Graph.init(a);
    defer g.deinit();

    // ── 1) Card：标题 + 描述 ───────────────────────────────────────
    // 背后 Signal 由 demo 持有。defer 顺序：tree(card) 先释放、再释放 signals、最后 g。
    var title = reactive.Signal([]const u8).init(&g, "NandinaUI");
    defer title.deinit();
    var desc = reactive.Signal([]const u8).init(&g, "A Zig UI library");
    defer desc.deinit();
    var card_bg = reactive.Signal(Color).init(&g, Color.white);
    defer card_bg.deinit();
    var card_cr = reactive.Signal(f32).init(&g, 12);
    defer card_cr.deinit();
    var card_pad = reactive.Signal(Insets).init(&g, Insets.all(16));
    defer card_pad.deinit();
    var tfs = reactive.Signal(f32).init(&g, 20);
    defer tfs.deinit();
    var dfs = reactive.Signal(f32).init(&g, 14);
    defer dfs.deinit();

    var tree = runtime.Tree.init(a);
    defer tree.deinit();

    const card = try widgets.Card.create(a, &g, .{
        .title = title.asReadonly(),
        .description = desc.asReadonly(),
        .bg_color = card_bg.asReadonly(),
        .corner_radius = card_cr.asReadonly(),
        .padding = card_pad.asReadonly(),
        .title_font_size = tfs.asReadonly(),
        .description_font_size = dfs.asReadonly(),
    });

    tree.setRoot(&card.node);
    tree.setViewport(.{ .width = 280, .height = 160 });

    try out.print("Card 挂进 Tree，第一帧（视口 280x160）：\n", .{});
    _ = try tree.frame();
    try printScene(out, &tree.scene);

    // ── 2) 响应式：改 title signal → 下一帧自动重绘 ───────────────────────
    try out.print("\n改 title signal = \"Hello\" → tree 变脏：{}\n", .{tree.isDirty() == false});
    title.set("Hello");
    try out.print("  set 后 isDirty = {}\n", .{tree.isDirty()});
    _ = try tree.frame();
    // 第二条命令是 title 文本
    if (tree.scene.commands.items.len >= 2 and tree.scene.commands.items[1] == .draw_text) {
        try out.print("  重绘后 title 文本 = \"{s}\"\n", .{tree.scene.commands.items[1].draw_text.text});
    }

    // ── 3) Button：交互状态色 ─────────────────────────────────────────────
    try out.print("\nButton 交互状态（hover 前后背景色变化）：\n", .{});
    try runButtonDemo(out, a, &g);

    // ── 4) ClipNode：统一裁剪超大子内容 ───────────────────────────────────
    try out.print("\nClipNode 统一裁剪（200x200 内容裁到 80x80 视口）：\n", .{});
    try runClipDemo(out, a, &g);
}

const ClickState = struct {
    var count: u32 = 0;
    fn onClick(_: ?*anyopaque) callconv(.c) void {
        count += 1;
    }
};

fn runButtonDemo(out: *std.Io.Writer, a: std.mem.Allocator, g: *reactive.Graph) !void {
    var label = reactive.Signal([]const u8).init(g, "Run");
    defer label.deinit();
    var bg = reactive.Signal(Color).init(g, Color.fromHexRgb(0x3B82F6));
    defer bg.deinit();
    var bg_hover = reactive.Signal(Color).init(g, Color.fromHexRgb(0x2563EB));
    defer bg_hover.deinit();
    var bg_pressed = reactive.Signal(Color).init(g, Color.fromHexRgb(0x1D4ED8));
    defer bg_pressed.deinit();
    var text_color = reactive.Signal(Color).init(g, Color.white);
    defer text_color.deinit();
    var fs = reactive.Signal(f32).init(g, 14);
    defer fs.deinit();
    var cr = reactive.Signal(f32).init(g, 6);
    defer cr.deinit();
    var pad = reactive.Signal(Insets).init(g, Insets.symmetric(12, 8));
    defer pad.deinit();
    var disabled = reactive.Signal(bool).init(g, false);
    defer disabled.deinit();

    const btn = try widgets.Button.create(a, g, .{
        .label = label.asReadonly(),
        .bg_color = bg.asReadonly(),
        .bg_hover_color = bg_hover.asReadonly(),
        .bg_pressed_color = bg_pressed.asReadonly(),
        .text_color = text_color.asReadonly(),
        .font_size = fs.asReadonly(),
        .corner_radius = cr.asReadonly(),
        .padding = pad.asReadonly(),
        .disabled = disabled.asReadonly(),
    });
    defer btn.node.deinitTree(a);

    ClickState.count = 0;
    btn.on_click = ClickState.onClick;
    btn.node.setBounds(f.Rect.fromXywh(0, 0, 80, 32));

    // 普通态绘制
    var scene = render.Scene.init(a);
    defer scene.deinit();
    try btn.node.paint(&scene);
    try out.print("    普通态背景 = #{X:0>8}\n", .{scene.commands.items[0].fill_rounded_rect.color.toHexRgba()});

    // 模拟 hover → 重绘
    _ = btn.node.vtable.handle_event(&btn.node, .{ .pointer_move = .{ .x = 10, .y = 10 } });
    scene.clear();
    try btn.node.paint(&scene);
    try out.print("    hover 态背景 = #{X:0>8}\n", .{scene.commands.items[0].fill_rounded_rect.color.toHexRgba()});

    // 模拟点击：down + up
    _ = btn.node.vtable.handle_event(&btn.node, .{ .pointer_down = .{ .button = .left, .x = 10, .y = 10 } });
    _ = btn.node.vtable.handle_event(&btn.node, .{ .pointer_up = .{ .button = .left, .x = 10, .y = 10 } });
    try out.print("    点击一次 → on_click 触发次数 = {d}\n", .{ClickState.count});
}

fn runClipDemo(out: *std.Io.Writer, a: std.mem.Allocator, g: *reactive.Graph) !void {
    var cr = reactive.Signal(f32).init(g, 8);
    defer cr.deinit();
    var pad = reactive.Signal(Insets).init(g, Insets.zero);
    defer pad.deinit();

    // ClipNode 包一个 Panel（背景比裁剪区大）
    var panel_bg = reactive.Signal(Color).init(g, Color.fromHexRgb(0xE64553));
    defer panel_bg.deinit();
    var panel_cr = reactive.Signal(f32).init(g, 0);
    defer panel_cr.deinit();
    var panel_pad = reactive.Signal(Insets).init(g, Insets.zero);
    defer panel_pad.deinit();
    var panel_bc = reactive.Signal(Color).init(g, Color.transparent);
    defer panel_bc.deinit();
    var panel_bw = reactive.Signal(f32).init(g, 0);
    defer panel_bw.deinit();

    var tree = runtime.Tree.init(a);
    defer tree.deinit();

    const clip = try widgets.ClipNode.create(a, g, .{
        .corner_radius = cr.asReadonly(),
        .clip_padding = pad.asReadonly(),
    });
    const panel = try widgets.Panel.create(a, g, .{
        .bg_color = panel_bg.asReadonly(),
        .corner_radius = panel_cr.asReadonly(),
        .padding = panel_pad.asReadonly(),
        .border_color = panel_bc.asReadonly(),
        .border_width = panel_bw.asReadonly(),
    });
    try clip.node.addChild(a, &panel.node);
    tree.setRoot(&clip.node);
    tree.setViewport(.{ .width = 80, .height = 80 });

    _ = try tree.frame();
    try printScene(out, &tree.scene);
    try out.print("    ↑ 注意：Panel 绘制被 PushClip/PopClip 包裹，裁剪由 runtime 统一处理\n", .{});
}

pub const demo = registry.Demo{
    .name = "widgets-gallery",
    .summary = "Card / Button / Panel / ClipNode 组件 + reactive→runtime→render 全链路",
    .run = run,
};
