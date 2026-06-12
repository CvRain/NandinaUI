//! layout/flow —— 流式折行布局求解器
//!
//! 对标前端的流式布局（类似 CSS `flex-wrap: wrap` 的雏形）：子节点沿主轴（水平）依次
//! 排布，当前行放不下就折到下一行。语义吸收自 archive BasicLayoutBackend 的 compute_flow，
//! 是后续 flex / grid 系统的基础。
//!
//! 这是**纯函数求解器**，与 flex.zig 一致：输入容器 bounds + 子节点规格，输出每个子节点
//! 的最终矩形。不依赖 widget 树，不分配内存（输出缓冲由调用方提供）。
//!
//! 规则：
//!   - 行内子节点按 preferred 宽度排布，超出可用宽度则换行。
//!   - 行内交叉轴（高度）按本行最高子节点对齐（顶对齐）。
//!   - `inline_gap` 为行内子节点间距，`line_gap` 为行间距。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");

const Rect = foundation.Rect;
const Size = foundation.Size;
const Insets = foundation.Insets;

/// flow 容器规格。
pub const FlowSpec = struct {
    padding: Insets = .{},
    /// 行内子节点间距。
    inline_gap: f32 = 0,
    /// 行间距。
    line_gap: f32 = 0,
};

/// flow 的子节点规格（仅需期望尺寸）。
pub const FlowChild = struct {
    preferred: Size = .{},
};

fn clampNonNeg(v: f32) f32 {
    return if (v > 0) v else 0;
}

/// 求解流式布局，写入 `out_frames`（长度须等于 children.len）。
pub fn solve(spec: FlowSpec, bounds: Rect, children: []const FlowChild, out_frames: []Rect) void {
    std.debug.assert(out_frames.len == children.len);

    const content = spec.padding.applyToRect(bounds);
    const avail_w = clampNonNeg(content.width());

    var line_start: usize = 0; // 本行第一个子节点索引
    var cursor_x: f32 = 0; // 本行已占用宽度
    var line_h: f32 = 0; // 本行最高
    var line_top: f32 = content.top;

    var i: usize = 0;
    while (i < children.len) : (i += 1) {
        const cw = clampNonNeg(children[i].preferred.width);
        const ch = clampNonNeg(children[i].preferred.height);
        const has_items = i > line_start;
        const next_x = if (has_items) cursor_x + spec.inline_gap + cw else cw;

        // 需要换行：当前行已有内容且放不下。
        if (has_items and next_x > avail_w) {
            // 定位当前已积累的这一行
            placeLine(out_frames, children, line_start, i, content.left, line_top, spec.inline_gap);
            line_top += line_h + spec.line_gap;
            line_start = i;
            cursor_x = cw;
            line_h = ch;
        } else {
            cursor_x = next_x;
            line_h = @max(line_h, ch);
        }
    }
    // 定位最后一行
    if (line_start < children.len) {
        placeLine(out_frames, children, line_start, children.len, content.left, line_top, spec.inline_gap);
    }
}

fn placeLine(
    out: []Rect,
    children: []const FlowChild,
    start: usize,
    end: usize,
    origin_x: f32,
    top: f32,
    inline_gap: f32,
) void {
    var x = origin_x;
    var idx = start;
    while (idx < end) : (idx += 1) {
        if (idx > start) x += inline_gap;
        const cw = clampNonNeg(children[idx].preferred.width);
        const ch = clampNonNeg(children[idx].preferred.height);
        out[idx] = Rect.fromXywh(x, top, cw, ch);
        x += cw;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

fn child(w: f32, h: f32) FlowChild {
    return .{ .preferred = .{ .width = w, .height = h } };
}

test "flow 单行不折行" {
    const children = [_]FlowChild{ child(30, 10), child(40, 10), child(20, 10) };
    var frames: [3]Rect = undefined;
    solve(.{ .inline_gap = 5 }, Rect.fromXywh(0, 0, 200, 100), &children, &frames);
    try std.testing.expectEqual(@as(f32, 0), frames[0].left);
    try std.testing.expectEqual(@as(f32, 35), frames[1].left); // 30 + 5
    try std.testing.expectEqual(@as(f32, 80), frames[2].left); // 35 + 40 + 5
    // 同一行
    try std.testing.expectEqual(@as(f32, 0), frames[0].top);
    try std.testing.expectEqual(@as(f32, 0), frames[2].top);
}

test "flow 超宽自动折行" {
    // 容器宽 100，三个 60 宽：第一个放第一行，第二个换行，第三个换行
    const children = [_]FlowChild{ child(60, 20), child(60, 30), child(60, 10) };
    var frames: [3]Rect = undefined;
    solve(.{ .line_gap = 5 }, Rect.fromXywh(0, 0, 100, 200), &children, &frames);

    // 第一行只有 child0
    try std.testing.expectEqual(@as(f32, 0), frames[0].left);
    try std.testing.expectEqual(@as(f32, 0), frames[0].top);
    // child1 换到第二行，top = 20 (行高) + 5 (line_gap)
    try std.testing.expectEqual(@as(f32, 0), frames[1].left);
    try std.testing.expectEqual(@as(f32, 25), frames[1].top);
    // child2 换到第三行，top = 25 + 30 + 5
    try std.testing.expectEqual(@as(f32, 60), frames[2].top);
}

test "flow 两两成行" {
    // 容器宽 100，每个 40 宽 + gap 10：两个一行（40+10+40=90 ≤ 100），第三个换行
    const children = [_]FlowChild{ child(40, 10), child(40, 10), child(40, 10) };
    var frames: [3]Rect = undefined;
    solve(.{ .inline_gap = 10, .line_gap = 4 }, Rect.fromXywh(0, 0, 100, 200), &children, &frames);
    try std.testing.expectEqual(@as(f32, 0), frames[0].left);
    try std.testing.expectEqual(@as(f32, 50), frames[1].left); // 40 + 10
    try std.testing.expectEqual(@as(f32, 0), frames[1].top); // 同第一行
    // child2 换行
    try std.testing.expectEqual(@as(f32, 0), frames[2].left);
    try std.testing.expectEqual(@as(f32, 14), frames[2].top); // 10 + 4
}

test "flow 尊重 padding" {
    const children = [_]FlowChild{child(20, 10)};
    var frames: [1]Rect = undefined;
    solve(.{ .padding = Insets.all(8) }, Rect.fromXywh(0, 0, 100, 100), &children, &frames);
    try std.testing.expectEqual(@as(f32, 8), frames[0].left);
    try std.testing.expectEqual(@as(f32, 8), frames[0].top);
}
