//! render/scene —— 绘制命令中间表示
//!
//! `DrawCommand` 是后端无关的绘制指令，`Scene` 是其有序缓冲。上层（runtime / widgets）
//! 只产出绘制命令，由具体 `Backend` 执行（GPU / 软件 / 录制）。命令使用 foundation 的
//! 几何与颜色类型，本层不依赖任何更高层。
//!
//! 设计要点：
//! - `Scene` 是可复用缓冲：`clear()` 保留容量，避免每帧重新分配（见编码规范第 7 节）。
//! - 文本命令以**借用切片**持有字符串，调用方须保证其生命周期覆盖到 scene 被后端消费。
//! - clip 以「push/pop 配对」表达裁剪栈，后端按栈语义处理。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");

const Rect = foundation.Rect;
const Color = foundation.Color;

/// 填充矩形。
pub const FillRect = struct {
    rect: Rect,
    color: Color,
};

/// 填充圆角矩形。`radius` 为四角统一圆角半径（像素）。
pub const FillRoundedRect = struct {
    rect: Rect,
    radius: f32,
    color: Color,
};

/// 绘制文本。当前为最小字段集；text 层落地后会扩展字体/对齐等。
/// `text` 是借用切片，调用方须保证其生命周期不短于 scene 的消费。
pub const DrawText = struct {
    text: []const u8,
    /// 文本基线起点 / 布局框左上角（像素）。
    x: f32,
    y: f32,
    font_size: f32 = 14,
    color: Color = Color.black,
    /// 可选布局框尺寸（0 表示按内容自适应），供后端做对齐 / 截断。
    layout_width: f32 = 0,
    layout_height: f32 = 0,
};

/// 压入一个裁剪区域（支持圆角）。后续绘制被限制在该区域内，直到对应的 `pop_clip`。
pub const PushClip = struct {
    rect: Rect,
    radius: f32 = 0,
};

/// 弹出最近压入的裁剪区域。
pub const PopClip = struct {};

/// 后端无关的绘制指令。
pub const DrawCommand = union(enum) {
    fill_rect: FillRect,
    fill_rounded_rect: FillRoundedRect,
    draw_text: DrawText,
    push_clip: PushClip,
    pop_clip: PopClip,
};

/// 有序绘制命令缓冲。可跨帧复用：`clear()` 保留底层容量。
pub const Scene = struct {
    allocator: std.mem.Allocator,
    commands: std.ArrayList(DrawCommand) = .empty,

    pub fn init(allocator: std.mem.Allocator) Scene {
        return .{ .allocator = allocator };
    }

    pub fn deinit(self: *Scene) void {
        self.commands.deinit(self.allocator);
        self.* = undefined;
    }

    /// 清空命令但保留已分配容量，供下一帧复用。
    pub fn clear(self: *Scene) void {
        self.commands.clearRetainingCapacity();
    }

    /// 当前命令数量。
    pub fn count(self: *const Scene) usize {
        return self.commands.items.len;
    }

    /// 追加一条原始命令。
    pub fn push(self: *Scene, command: DrawCommand) std.mem.Allocator.Error!void {
        try self.commands.append(self.allocator, command);
    }

    // ── 便捷构造方法 ──────────────────────────────────────────────────────────

    pub fn fillRect(self: *Scene, rect: Rect, color: Color) std.mem.Allocator.Error!void {
        try self.push(.{ .fill_rect = .{ .rect = rect, .color = color } });
    }

    pub fn fillRoundedRect(self: *Scene, rect: Rect, radius: f32, color: Color) std.mem.Allocator.Error!void {
        try self.push(.{ .fill_rounded_rect = .{ .rect = rect, .radius = radius, .color = color } });
    }

    pub fn drawText(self: *Scene, text: DrawText) std.mem.Allocator.Error!void {
        try self.push(.{ .draw_text = text });
    }

    pub fn pushClip(self: *Scene, rect: Rect, radius: f32) std.mem.Allocator.Error!void {
        try self.push(.{ .push_clip = .{ .rect = rect, .radius = radius } });
    }

    pub fn popClip(self: *Scene) std.mem.Allocator.Error!void {
        try self.push(.{ .pop_clip = .{} });
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Scene 追加命令并计数" {
    var scene = Scene.init(std.testing.allocator);
    defer scene.deinit();

    try scene.fillRect(Rect.fromXywh(0, 0, 100, 50), Color.white);
    try scene.fillRoundedRect(Rect.fromXywh(0, 0, 100, 50), 8, Color.black);
    try scene.drawText(.{ .text = "hi", .x = 4, .y = 16 });

    try std.testing.expectEqual(@as(usize, 3), scene.count());
    try std.testing.expect(scene.commands.items[0] == .fill_rect);
    try std.testing.expect(scene.commands.items[1] == .fill_rounded_rect);
    try std.testing.expect(scene.commands.items[2] == .draw_text);
}

test "Scene clear 保留容量" {
    var scene = Scene.init(std.testing.allocator);
    defer scene.deinit();

    try scene.fillRect(Rect.fromXywh(0, 0, 10, 10), Color.white);
    try scene.fillRect(Rect.fromXywh(0, 0, 20, 20), Color.black);
    const cap_before = scene.commands.capacity;

    scene.clear();
    try std.testing.expectEqual(@as(usize, 0), scene.count());
    try std.testing.expectEqual(cap_before, scene.commands.capacity);
}

test "Scene clip 配对" {
    var scene = Scene.init(std.testing.allocator);
    defer scene.deinit();

    try scene.pushClip(Rect.fromXywh(0, 0, 100, 100), 4);
    try scene.fillRect(Rect.fromXywh(10, 10, 20, 20), Color.white);
    try scene.popClip();

    try std.testing.expectEqual(@as(usize, 3), scene.count());
    try std.testing.expect(scene.commands.items[0] == .push_clip);
    try std.testing.expect(scene.commands.items[2] == .pop_clip);
    try std.testing.expectEqual(@as(f32, 4), scene.commands.items[0].push_clip.radius);
}
