//! layout/flex —— 盒子模型布局求解器（column / row / stack）
//!
//! 对标 Qt Widget 的盒子布局（QVBoxLayout / QHBoxLayout / QStackedLayout），语义吸收自
//! archive 的 BasicLayoutBackend。这是**纯函数求解器**：输入容器 bounds + 子节点规格，
//! 输出每个子节点的最终矩形（frame），完全不依赖 widget 树，便于独立测试。
//!
//! 主轴（main axis）= column 取垂直、row 取水平；交叉轴（cross axis）为另一方向。
//!   - 固定子节点按 preferred 占主轴；`flex` > 0 的子节点按权重瓜分主轴剩余空间。
//!   - 交叉轴按 `cross_align` 对齐；`stretch` 时填满交叉轴。
//!   - 主轴整体按 `main_align` 分布（含 space_between / space_around）。
//!   - stack：所有子节点叠在同一区域，各自按 align 在容器内定位。
//!
//! 几何只在本层内部计算，不泄漏到 widgets / app（见 layout-strategy.md）。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const constraints = @import("constraints.zig");

const Rect = foundation.Rect;
const Size = foundation.Size;
const Insets = foundation.Insets;
const inf = constraints.inf;

/// 主轴方向。
pub const Axis = enum { row, column, stack };

/// 主轴 / 交叉轴对齐方式。
pub const Align = enum {
    start,
    center,
    end,
    /// 仅交叉轴有效：拉伸填满。
    stretch,
    /// 仅主轴有效：两端对齐，子节点间均分剩余。
    space_between,
    /// 仅主轴有效：每个子节点两侧均分剩余。
    space_around,
};

/// 一个子节点的布局规格。
pub const ChildSpec = struct {
    /// 期望尺寸（未受 flex 影响时使用）。
    preferred: Size = .{},
    min: Size = .{},
    max: Size = .{ .width = inf, .height = inf },
    /// 主轴弹性权重。0 = 固定；> 0 = 按权重瓜分剩余空间。
    flex: u16 = 0,
};

/// 容器布局规格。
pub const FlexSpec = struct {
    axis: Axis = .column,
    padding: Insets = .{},
    /// 子节点之间的间距（主轴）。
    gap: f32 = 0,
    main_align: Align = .start,
    cross_align: Align = .start,
};

fn clampNonNeg(v: f32) f32 {
    return if (v > 0) v else 0;
}

fn resolveExtent(desired: f32, lo_in: f32, hi: f32) f32 {
    const lo = clampNonNeg(lo_in);
    if (lo > hi) return @min(lo, hi);
    return std.math.clamp(clampNonNeg(desired), lo, hi);
}

fn crossPosition(origin: f32, available: f32, extent: f32, a: Align) f32 {
    return switch (a) {
        .center, .space_between, .space_around => origin + (available - extent) * 0.5,
        .end => origin + (available - extent),
        .start, .stretch => origin,
    };
}

const MainPlacement = struct { start_offset: f32 = 0, gap: f32 = 0 };

fn mainPlacement(justify: Align, child_count: usize, used: f32, available: f32, base_gap: f32) MainPlacement {
    const free = clampNonNeg(available - used);
    const n: f32 = @floatFromInt(child_count);
    return switch (justify) {
        .end => .{ .start_offset = free, .gap = base_gap },
        .center => .{ .start_offset = free * 0.5, .gap = base_gap },
        .space_between => if (child_count > 1)
            .{ .start_offset = 0, .gap = base_gap + free / (n - 1) }
        else
            .{ .start_offset = free * 0.5, .gap = base_gap },
        .space_around => if (child_count > 0) blk: {
            const extra = free / n;
            break :blk .{ .start_offset = extra * 0.5, .gap = base_gap + extra };
        } else .{ .start_offset = 0, .gap = base_gap },
        .start, .stretch => .{ .start_offset = 0, .gap = base_gap },
    };
}

/// 内容区域（容器 bounds 去掉 padding）。
fn contentRect(bounds: Rect, padding: Insets) Rect {
    return padding.applyToRect(bounds);
}

/// 求解一组子节点的最终矩形，写入 `out_frames`（长度须等于 children.len）。
///
/// 是纯函数：不分配内存，由调用方提供输出缓冲。
pub fn solve(spec: FlexSpec, bounds: Rect, children: []const ChildSpec, out_frames: []Rect) void {
    std.debug.assert(out_frames.len == children.len);
    switch (spec.axis) {
        .column => solveLinear(spec, bounds, children, out_frames, .vertical),
        .row => solveLinear(spec, bounds, children, out_frames, .horizontal),
        .stack => solveStack(spec, bounds, children, out_frames),
    }
}

const Orientation = enum { vertical, horizontal };

fn solveLinear(
    spec: FlexSpec,
    bounds: Rect,
    children: []const ChildSpec,
    out: []Rect,
    orient: Orientation,
) void {
    const content = contentRect(bounds, spec.padding);
    const avail_main = clampNonNeg(if (orient == .vertical) content.height() else content.width());
    const avail_cross = clampNonNeg(if (orient == .vertical) content.width() else content.height());

    // 主轴 / 交叉轴的尺寸取值器
    const mainPref = struct {
        fn f(o: Orientation, c: ChildSpec) f32 {
            return if (o == .vertical) c.preferred.height else c.preferred.width;
        }
    }.f;
    const mainMin = struct {
        fn f(o: Orientation, c: ChildSpec) f32 {
            return if (o == .vertical) c.min.height else c.min.width;
        }
    }.f;
    const mainMax = struct {
        fn f(o: Orientation, c: ChildSpec) f32 {
            return if (o == .vertical) c.max.height else c.max.width;
        }
    }.f;

    // 1) 统计固定主轴尺寸与 flex 总权重
    var fixed_total: f32 = 0;
    var flex_total: u32 = 0;
    for (children) |c| {
        if (c.flex > 0) {
            flex_total += c.flex;
        } else {
            fixed_total += resolveExtent(mainPref(orient, c), mainMin(orient, c), mainMax(orient, c));
        }
    }

    const child_count = children.len;
    const gap_total: f32 = if (child_count > 1) spec.gap * @as(f32, @floatFromInt(child_count - 1)) else 0;
    const remaining = avail_main - fixed_total - gap_total;
    const flex_inv: f32 = if (flex_total > 0) 1.0 / @as(f32, @floatFromInt(flex_total)) else 0;

    // 2) 计算每个子节点的主轴尺寸
    var used_main: f32 = 0;
    for (children, 0..) |c, i| {
        const desired = if (c.flex > 0 and flex_total > 0)
            (if (remaining > 0) remaining * (@as(f32, @floatFromInt(c.flex)) * flex_inv) else 0)
        else
            clampNonNeg(mainPref(orient, c));
        const extent = resolveExtent(desired, mainMin(orient, c), mainMax(orient, c));
        // 暂存主轴尺寸到 out（用 width 字段中转，稍后覆盖）
        if (orient == .vertical) {
            out[i] = Rect.fromXywh(0, 0, 0, extent);
        } else {
            out[i] = Rect.fromXywh(0, 0, extent, 0);
        }
        used_main += extent;
    }

    const used = used_main + gap_total;
    const place = mainPlacement(spec.main_align, child_count, used, avail_main, spec.gap);

    // 3) 依次定位
    const main_origin = (if (orient == .vertical) content.top else content.left) + place.start_offset;
    const cross_origin = if (orient == .vertical) content.left else content.top;
    var cursor = main_origin;
    for (children, 0..) |c, i| {
        if (i > 0) cursor += place.gap;

        const main_extent = if (orient == .vertical) out[i].height() else out[i].width();

        const cross_pref = clampNonNeg(if (orient == .vertical) c.preferred.width else c.preferred.height);
        const cross_min = if (orient == .vertical) c.min.width else c.min.height;
        const cross_max = if (orient == .vertical) c.max.width else c.max.height;
        const cross_desired = if (spec.cross_align == .stretch) avail_cross else @min(cross_pref, avail_cross);
        const cross_extent = resolveExtent(cross_desired, cross_min, cross_max);
        const cross_pos = crossPosition(cross_origin, avail_cross, cross_extent, spec.cross_align);

        if (orient == .vertical) {
            out[i] = Rect.fromXywh(cross_pos, cursor, cross_extent, main_extent);
        } else {
            out[i] = Rect.fromXywh(cursor, cross_pos, main_extent, cross_extent);
        }
        cursor += main_extent;
    }
}

fn solveStack(spec: FlexSpec, bounds: Rect, children: []const ChildSpec, out: []Rect) void {
    const content = contentRect(bounds, spec.padding);
    const avail_w = clampNonNeg(content.width());
    const avail_h = clampNonNeg(content.height());

    for (children, 0..) |c, i| {
        const w_desired = if (spec.cross_align == .stretch) avail_w else @min(clampNonNeg(c.preferred.width), avail_w);
        const h_desired = if (spec.main_align == .stretch) avail_h else @min(clampNonNeg(c.preferred.height), avail_h);
        const w = resolveExtent(w_desired, c.min.width, c.max.width);
        const h = resolveExtent(h_desired, c.min.height, c.max.height);
        const x = crossPosition(content.left, avail_w, w, spec.cross_align);
        const y = crossPosition(content.top, avail_h, h, spec.main_align);
        out[i] = Rect.fromXywh(x, y, w, h);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

fn child(w: f32, h: f32) ChildSpec {
    return .{ .preferred = .{ .width = w, .height = h } };
}

test "column 固定子节点垂直堆叠 + gap" {
    const children = [_]ChildSpec{ child(100, 30), child(100, 40), child(100, 20) };
    var frames: [3]Rect = undefined;
    solve(
        .{ .axis = .column, .gap = 10 },
        Rect.fromXywh(0, 0, 200, 200),
        &children,
        &frames,
    );
    try std.testing.expectEqual(@as(f32, 0), frames[0].top);
    try std.testing.expectEqual(@as(f32, 30), frames[0].height());
    try std.testing.expectEqual(@as(f32, 40), frames[1].top); // 30 + gap 10
    try std.testing.expectEqual(@as(f32, 90), frames[2].top); // 40 + 40 + 10
}

test "column flex 瓜分剩余空间" {
    // 容器高 100，固定 20，剩 80 给两个 flex（1:3）
    const children = [_]ChildSpec{
        child(50, 20),
        .{ .preferred = .{ .width = 50, .height = 0 }, .flex = 1 },
        .{ .preferred = .{ .width = 50, .height = 0 }, .flex = 3 },
    };
    var frames: [3]Rect = undefined;
    solve(.{ .axis = .column }, Rect.fromXywh(0, 0, 50, 100), &children, &frames);
    try std.testing.expectEqual(@as(f32, 20), frames[0].height());
    try std.testing.expectApproxEqAbs(@as(f32, 20), frames[1].height(), 1e-4); // 80 * 1/4
    try std.testing.expectApproxEqAbs(@as(f32, 60), frames[2].height(), 1e-4); // 80 * 3/4
}

test "row 水平排布 + cross stretch" {
    const children = [_]ChildSpec{ child(40, 10), child(60, 10) };
    var frames: [2]Rect = undefined;
    solve(
        .{ .axis = .row, .gap = 10, .cross_align = .stretch },
        Rect.fromXywh(0, 0, 200, 80),
        &children,
        &frames,
    );
    try std.testing.expectEqual(@as(f32, 0), frames[0].left);
    try std.testing.expectEqual(@as(f32, 40), frames[0].width());
    try std.testing.expectEqual(@as(f32, 50), frames[1].left); // 40 + gap 10
    // stretch 填满交叉轴（高度）
    try std.testing.expectEqual(@as(f32, 80), frames[0].height());
    try std.testing.expectEqual(@as(f32, 80), frames[1].height());
}

test "column main_align center" {
    const children = [_]ChildSpec{ child(50, 20), child(50, 20) };
    var frames: [2]Rect = undefined;
    // 容器高 100，内容高 40，居中 → 起点 30
    solve(.{ .axis = .column, .main_align = .center }, Rect.fromXywh(0, 0, 50, 100), &children, &frames);
    try std.testing.expectEqual(@as(f32, 30), frames[0].top);
    try std.testing.expectEqual(@as(f32, 50), frames[1].top);
}

test "row space_between 两端对齐" {
    const children = [_]ChildSpec{ child(20, 10), child(20, 10), child(20, 10) };
    var frames: [3]Rect = undefined;
    // 容器宽 200，三个 20 共 60，剩 140 分到 2 个间隙各 70
    solve(.{ .axis = .row, .main_align = .space_between }, Rect.fromXywh(0, 0, 200, 50), &children, &frames);
    try std.testing.expectEqual(@as(f32, 0), frames[0].left);
    try std.testing.expectApproxEqAbs(@as(f32, 90), frames[1].left, 1e-4); // 20 + 70
    try std.testing.expectApproxEqAbs(@as(f32, 180), frames[2].left, 1e-4); // 200 - 20
}

test "padding 收缩内容区" {
    const children = [_]ChildSpec{child(0, 0)};
    var frames: [1]Rect = undefined;
    solve(
        .{ .axis = .column, .padding = Insets.all(10), .cross_align = .stretch },
        Rect.fromXywh(0, 0, 100, 100),
        &children,
        &frames,
    );
    try std.testing.expectEqual(@as(f32, 10), frames[0].left);
    try std.testing.expectEqual(@as(f32, 10), frames[0].top);
    try std.testing.expectEqual(@as(f32, 80), frames[0].width());
}

test "stack 叠放 + 居中" {
    const children = [_]ChildSpec{ child(40, 40), child(20, 20) };
    var frames: [2]Rect = undefined;
    solve(
        .{ .axis = .stack, .main_align = .center, .cross_align = .center },
        Rect.fromXywh(0, 0, 100, 100),
        &children,
        &frames,
    );
    // 两个都居中
    try std.testing.expectEqual(@as(f32, 30), frames[0].left); // (100-40)/2
    try std.testing.expectEqual(@as(f32, 40), frames[1].left); // (100-20)/2
    try std.testing.expectEqual(@as(f32, 30), frames[0].top);
}
