//! color —— RGBA 颜色类型
//!
//! 对应旧 C++ foundation 的 NanColor。内部以 0..1 归一化的 f32 分量存储，
//! 方便与渲染后端及插值运算对接，同时提供 8-bit / hex 互转。
const std = @import("std");

pub const Color = struct {
    r: f32 = 0,
    g: f32 = 0,
    b: f32 = 0,
    a: f32 = 1,

    pub const transparent: Color = .{ .r = 0, .g = 0, .b = 0, .a = 0 };
    pub const black: Color = .{ .r = 0, .g = 0, .b = 0, .a = 1 };
    pub const white: Color = .{ .r = 1, .g = 1, .b = 1, .a = 1 };

    /// 由 0..1 的浮点分量构造。
    pub fn rgba(r: f32, g: f32, b: f32, a: f32) Color {
        return .{ .r = r, .g = g, .b = b, .a = a };
    }

    /// 由 8-bit 整数分量构造。
    pub fn rgba8(r: u8, g: u8, b: u8, a: u8) Color {
        return .{
            .r = @as(f32, @floatFromInt(r)) / 255.0,
            .g = @as(f32, @floatFromInt(g)) / 255.0,
            .b = @as(f32, @floatFromInt(b)) / 255.0,
            .a = @as(f32, @floatFromInt(a)) / 255.0,
        };
    }

    /// 由 0xRRGGBB 构造（不透明）。
    pub fn fromHexRgb(hex: u24) Color {
        return rgba8(
            @intCast((hex >> 16) & 0xFF),
            @intCast((hex >> 8) & 0xFF),
            @intCast(hex & 0xFF),
            255,
        );
    }

    /// 由 0xRRGGBBAA 构造。
    pub fn fromHexRgba(hex: u32) Color {
        return rgba8(
            @intCast((hex >> 24) & 0xFF),
            @intCast((hex >> 16) & 0xFF),
            @intCast((hex >> 8) & 0xFF),
            @intCast(hex & 0xFF),
        );
    }

    fn to8(v: f32) u8 {
        const clamped = std.math.clamp(v, 0.0, 1.0);
        return @intFromFloat(@round(clamped * 255.0));
    }

    /// 转为 0xRRGGBBAA。
    pub fn toHexRgba(self: Color) u32 {
        return (@as(u32, to8(self.r)) << 24) |
            (@as(u32, to8(self.g)) << 16) |
            (@as(u32, to8(self.b)) << 8) |
            @as(u32, to8(self.a));
    }

    /// 返回带新 alpha 的副本。
    pub fn withAlpha(self: Color, a: f32) Color {
        return .{ .r = self.r, .g = self.g, .b = self.b, .a = a };
    }

    /// 在 self 与 other 之间线性插值，t 取 0..1。
    pub fn lerp(self: Color, other: Color, t: f32) Color {
        const k = std.math.clamp(t, 0.0, 1.0);
        return .{
            .r = self.r + (other.r - self.r) * k,
            .g = self.g + (other.g - self.g) * k,
            .b = self.b + (other.b - self.b) * k,
            .a = self.a + (other.a - self.a) * k,
        };
    }

    pub fn eql(self: Color, other: Color) bool {
        return self.r == other.r and self.g == other.g and
            self.b == other.b and self.a == other.a;
    }
};

// ---- 测试 ----

test "Color hex 往返" {
    const c = Color.fromHexRgb(0x336699);
    try std.testing.expectEqual(@as(u32, 0x336699FF), c.toHexRgba());
}

test "Color rgba8 归一化" {
    const c = Color.rgba8(255, 0, 0, 255);
    try std.testing.expect(c.eql(Color.rgba(1, 0, 0, 1)));
}

test "Color lerp 中点" {
    const mid = Color.black.lerp(Color.white, 0.5);
    try std.testing.expectApproxEqAbs(@as(f32, 0.5), mid.r, 1e-6);
    try std.testing.expectApproxEqAbs(@as(f32, 0.5), mid.g, 1e-6);
    try std.testing.expectApproxEqAbs(@as(f32, 0.5), mid.b, 1e-6);
}

test "Color withAlpha" {
    const c = Color.white.withAlpha(0.5);
    try std.testing.expectEqual(@as(f32, 0.5), c.a);
}
