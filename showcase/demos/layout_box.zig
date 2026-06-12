//! demos/layout_box —— 布局求解器演示（flex / flow / anchors）
//!
//! 把三套布局求解器各跑一遍，打印每个子节点的最终矩形 —— 直观感受布局算法的输出。
//! 最后演示「布局 → 渲染」链路：用 flex 求出的 frame 生成 render 绘制命令。

const std = @import("std");
const nandina = @import("NandinaUI");
const registry = @import("../registry.zig");

const layout = nandina.layout;
const render = nandina.render;
const f = nandina.foundation;

fn printFrames(out: *std.Io.Writer, frames: []const f.Rect) !void {
    for (frames, 0..) |r, i| {
        try out.print("    child[{d}] = ({d},{d} {d}x{d})\n", .{ i, r.x(), r.y(), r.width(), r.height() });
    }
}

fn run(ctx: *registry.DemoContext) anyerror!void {
    const out = ctx.out;
    const container = f.Rect.fromXywh(0, 0, 300, 200);

    // ── 1) flex column：固定 header + flex 内容 + 固定 footer ──
    {
        const children = [_]layout.flex.ChildSpec{
            .{ .preferred = .{ .width = 0, .height = 40 } }, // header
            .{ .flex = 1 }, // 内容区瓜分剩余
            .{ .preferred = .{ .width = 0, .height = 30 } }, // footer
        };
        var frames: [3]f.Rect = undefined;
        layout.flex.solve(
            .{ .axis = .column, .gap = 8, .cross_align = .stretch, .padding = f.Insets.all(10) },
            container,
            &children,
            &frames,
        );
        try out.print("flex column (header / flex 内容 / footer，padding=10 gap=8)：\n", .{});
        try printFrames(out, &frames);
    }

    // ── 2) flow：标签流式折行 ──
    {
        const children = [_]layout.flow.FlowChild{
            .{ .preferred = .{ .width = 80, .height = 24 } },
            .{ .preferred = .{ .width = 120, .height = 24 } },
            .{ .preferred = .{ .width = 90, .height = 24 } },
            .{ .preferred = .{ .width = 70, .height = 24 } },
        };
        var frames: [4]f.Rect = undefined;
        layout.flow.solve(.{ .inline_gap = 8, .line_gap = 8 }, container, &children, &frames);
        try out.print("flow (4 个标签，宽 300 自动折行)：\n", .{});
        try printFrames(out, &frames);
    }

    // ── 3) anchors：顶部满宽条 + 居中块 + 右下角按钮 ──
    {
        const specs = [_]layout.anchors.AnchorSpec{
            .{ .horizontal = .{ .start = 0, .end = 0 }, .vertical = .{ .start = 0, .size = 36 } },
            .{ .horizontal = .{ .size = 120 }, .vertical = .{ .size = 60 } },
            .{ .horizontal = .{ .end = 12, .size = 80 }, .vertical = .{ .end = 12, .size = 28 } },
        };
        var frames: [3]f.Rect = undefined;
        layout.anchors.solve(container, &specs, &frames);
        try out.print("anchors (顶部条 / 居中块 / 右下角按钮)：\n", .{});
        try printFrames(out, &frames);
    }

    // ── 4) 布局 → 渲染：把 flex row 的结果转成绘制命令 ──
    {
        const palette = [_]f.Color{
            f.Color.fromHexRgb(0xE64553),
            f.Color.fromHexRgb(0x7C3AED),
            f.Color.fromHexRgb(0x3B82F6),
        };
        const children = [_]layout.flex.ChildSpec{
            .{ .flex = 1 }, .{ .flex = 2 }, .{ .flex = 1 },
        };
        var frames: [3]f.Rect = undefined;
        layout.flex.solve(
            .{ .axis = .row, .gap = 8, .cross_align = .stretch },
            f.Rect.fromXywh(0, 0, 320, 48),
            &children,
            &frames,
        );

        var scene = render.Scene.init(ctx.allocator);
        defer scene.deinit();
        for (frames, 0..) |r, i| {
            try scene.fillRoundedRect(r, 6, palette[i]);
        }

        try out.print("布局 → 渲染（flex row 1:2:1 → {d} 条圆角矩形绘制命令）：\n", .{scene.count()});
        for (scene.commands.items) |cmd| {
            const c = cmd.fill_rounded_rect;
            try out.print("    FillRoundedRect ({d},{d} {d}x{d}) color=#{X:0>8}\n", .{
                c.rect.x(), c.rect.y(), c.rect.width(), c.rect.height(), c.color.toHexRgba(),
            });
        }
    }
}

pub const demo = registry.Demo{
    .name = "layout-box",
    .summary = "布局求解器 flex / flow / anchors，并演示布局→渲染链路",
    .run = run,
};
