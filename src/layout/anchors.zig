//! layout/anchors —— 锚点定位求解器（QML anchors 风格）
//!
//! 对标 Qt QML 的 anchors：子节点通过 left / top / right / bottom / width / height 声明
//! 相对父容器的位置，适合页面级大块内容的自由定位（组件内部布局用 flex / flow 即可）。
//! 语义吸收自 archive 的 Positioned。
//!
//! 这是**纯函数求解器**：每个子节点的锚点配置 + 父容器尺寸 → 最终矩形。每轴独立解析，
//! 优先级与 QML 一致：
//!   start + end   → pos=start, size=parent - start - end
//!   start + size  → pos=start, size=size
//!   end + size    → size=size, pos=parent - end - size
//!   start only    → pos=start, size=preferred
//!   end only      → size=preferred, pos=parent - end - size
//!   size only     → size=size, pos=(parent - size)/2   （居中）
//!   none          → pos=0, size=preferred
//!
//! 锚点值用可选 f32 表达（null = 未设置）。需要「兄弟引用」的动态锚点（如 .top = sibling.bottom + 8）
//! 由上层（widgets / 响应式 effect）在调用前算成具体数值传入，本层只做纯几何解析。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");

const Rect = foundation.Rect;
const Size = foundation.Size;

/// 单轴的锚点配置（start / end / size 三选若干）。
pub const AxisAnchors = struct {
    start: ?f32 = null,
    end: ?f32 = null,
    size: ?f32 = null,
};

/// 一个子节点的锚点配置 + 兜底 preferred 尺寸。
pub const AnchorSpec = struct {
    /// 水平轴：start=left，end=right（向内量），size=width。
    horizontal: AxisAnchors = .{},
    /// 垂直轴：start=top，end=bottom（向内量），size=height。
    vertical: AxisAnchors = .{},
    /// 未被锚点确定尺寸时的兜底期望尺寸。
    preferred: Size = .{},
};

const AxisResult = struct { pos: f32, size: f32 };

fn resolveAxis(parent: f32, a: AxisAnchors, preferred: f32) AxisResult {
    const hs = a.start != null;
    const he = a.end != null;
    const hz = a.size != null;

    if (hs and he) {
        const s = a.start.?;
        return .{ .pos = s, .size = parent - s - a.end.? };
    }
    if (hs and hz) {
        return .{ .pos = a.start.?, .size = a.size.? };
    }
    if (he and hz) {
        const w = a.size.?;
        return .{ .pos = parent - a.end.? - w, .size = w };
    }
    if (hs) {
        return .{ .pos = a.start.?, .size = if (hz) a.size.? else preferred };
    }
    if (he) {
        const w = if (hz) a.size.? else preferred;
        return .{ .pos = parent - a.end.? - w, .size = w };
    }
    if (hz) {
        const w = a.size.?;
        return .{ .pos = (parent - w) / 2, .size = w }; // 仅 size：居中
    }
    return .{ .pos = 0, .size = preferred };
}

/// 解析单个子节点的锚点 → 相对父容器原点的矩形（局部坐标）。
pub fn resolveOne(parent: Size, spec: AnchorSpec) Rect {
    const hr = resolveAxis(parent.width, spec.horizontal, spec.preferred.width);
    const vr = resolveAxis(parent.height, spec.vertical, spec.preferred.height);
    return Rect.fromXywh(hr.pos, vr.pos, hr.size, vr.size);
}

/// 批量解析，写入 `out_frames`（长度须等于 specs.len）。
/// 坐标相对父容器 `bounds` 原点偏移（即返回绝对坐标）。
pub fn solve(bounds: Rect, specs: []const AnchorSpec, out_frames: []Rect) void {
    std.debug.assert(out_frames.len == specs.len);
    const parent: Size = .{ .width = bounds.width(), .height = bounds.height() };
    for (specs, 0..) |spec, i| {
        const local = resolveOne(parent, spec);
        out_frames[i] = local.translated(bounds.left, bounds.top);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const parent_size: Size = .{ .width = 200, .height = 100 };

test "left + right 撑满减去两端" {
    const r = resolveOne(parent_size, .{ .horizontal = .{ .start = 10, .end = 20 } });
    try std.testing.expectEqual(@as(f32, 10), r.left);
    try std.testing.expectEqual(@as(f32, 170), r.width()); // 200 - 10 - 20
}

test "left + width 固定左上" {
    const r = resolveOne(parent_size, .{ .horizontal = .{ .start = 5, .size = 50 } });
    try std.testing.expectEqual(@as(f32, 5), r.left);
    try std.testing.expectEqual(@as(f32, 50), r.width());
}

test "right + width 贴右" {
    const r = resolveOne(parent_size, .{ .horizontal = .{ .end = 10, .size = 40 } });
    try std.testing.expectEqual(@as(f32, 150), r.left); // 200 - 10 - 40
    try std.testing.expectEqual(@as(f32, 40), r.width());
}

test "仅 size 居中" {
    const r = resolveOne(parent_size, .{ .horizontal = .{ .size = 80 } });
    try std.testing.expectEqual(@as(f32, 60), r.left); // (200 - 80)/2
}

test "无锚点用 preferred 且贴原点" {
    const r = resolveOne(parent_size, .{ .preferred = .{ .width = 30, .height = 20 } });
    try std.testing.expectEqual(@as(f32, 0), r.left);
    try std.testing.expectEqual(@as(f32, 30), r.width());
    try std.testing.expectEqual(@as(f32, 20), r.height());
}

test "双轴组合：贴右下角带边距" {
    const r = resolveOne(parent_size, .{
        .horizontal = .{ .end = 8, .size = 60 },
        .vertical = .{ .end = 8, .size = 24 },
    });
    try std.testing.expectEqual(@as(f32, 132), r.left); // 200 - 8 - 60
    try std.testing.expectEqual(@as(f32, 68), r.top); // 100 - 8 - 24
}

test "solve 批量并应用父容器偏移" {
    const specs = [_]AnchorSpec{
        .{ .horizontal = .{ .start = 0, .end = 0 }, .vertical = .{ .start = 0, .size = 40 } }, // 顶部满宽条
        .{ .horizontal = .{ .size = 50 }, .vertical = .{ .size = 50 } }, // 居中块
    };
    var frames: [2]Rect = undefined;
    // 父容器原点不在 (0,0)，验证偏移
    solve(Rect.fromXywh(100, 50, 200, 100), &specs, &frames);

    // 顶部条：宽 200，贴父容器左上
    try std.testing.expectEqual(@as(f32, 100), frames[0].left);
    try std.testing.expectEqual(@as(f32, 50), frames[0].top);
    try std.testing.expectEqual(@as(f32, 200), frames[0].width());
    // 居中块：(200-50)/2 + 100 = 175，(100-50)/2 + 50 = 75
    try std.testing.expectEqual(@as(f32, 175), frames[1].left);
    try std.testing.expectEqual(@as(f32, 75), frames[1].top);
}
