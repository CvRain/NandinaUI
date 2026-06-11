//! geometry —— 基础几何类型
//!
//! 对应旧 C++ foundation 中的 NanPoint / NanSize / NanRect / NanInsets。
//! 统一使用 f32 坐标（窗口像素空间）。Rect 内部以 LTRB（left/top/right/bottom）
//! 边界坐标存储，几何运算更直接，同时对外提供 x/y/width/height 访问。
const std = @import("std");

/// 二维点 / 向量。
pub const Point = struct {
    x: f32 = 0,
    y: f32 = 0,

    pub const zero: Point = .{ .x = 0, .y = 0 };

    pub fn init(x: f32, y: f32) Point {
        return .{ .x = x, .y = y };
    }

    pub fn add(self: Point, other: Point) Point {
        return .{ .x = self.x + other.x, .y = self.y + other.y };
    }

    pub fn sub(self: Point, other: Point) Point {
        return .{ .x = self.x - other.x, .y = self.y - other.y };
    }

    pub fn scale(self: Point, factor: f32) Point {
        return .{ .x = self.x * factor, .y = self.y * factor };
    }

    pub fn eql(self: Point, other: Point) bool {
        return self.x == other.x and self.y == other.y;
    }

    pub fn distanceTo(self: Point, other: Point) f32 {
        const dx = self.x - other.x;
        const dy = self.y - other.y;
        return @sqrt(dx * dx + dy * dy);
    }
};

/// 二维尺寸（宽高），保证非负语义由调用方维护。
pub const Size = struct {
    width: f32 = 0,
    height: f32 = 0,

    pub const zero: Size = .{ .width = 0, .height = 0 };

    pub fn init(width: f32, height: f32) Size {
        return .{ .width = width, .height = height };
    }

    pub fn area(self: Size) f32 {
        return self.width * self.height;
    }

    pub fn isEmpty(self: Size) bool {
        return self.width <= 0 or self.height <= 0;
    }

    pub fn eql(self: Size, other: Size) bool {
        return self.width == other.width and self.height == other.height;
    }
};

/// 二维矩形。内部以边界坐标 (left, top, right, bottom) 存储。
pub const Rect = struct {
    left: f32 = 0,
    top: f32 = 0,
    right: f32 = 0,
    bottom: f32 = 0,

    pub const empty: Rect = .{};

    /// 通过边界坐标构造。
    pub fn fromLtrb(left: f32, top: f32, right: f32, bottom: f32) Rect {
        return .{ .left = left, .top = top, .right = right, .bottom = bottom };
    }

    /// 通过位置 + 尺寸构造（UI 惯用）。
    pub fn fromXywh(px: f32, py: f32, w: f32, h: f32) Rect {
        return .{ .left = px, .top = py, .right = px + w, .bottom = py + h };
    }

    pub fn fromOriginSize(origin: Point, sz: Size) Rect {
        return fromXywh(origin.x, origin.y, sz.width, sz.height);
    }

    pub fn x(self: Rect) f32 {
        return self.left;
    }

    pub fn y(self: Rect) f32 {
        return self.top;
    }

    pub fn width(self: Rect) f32 {
        return if (self.right > self.left) self.right - self.left else 0;
    }

    pub fn height(self: Rect) f32 {
        return if (self.bottom > self.top) self.bottom - self.top else 0;
    }

    pub fn size(self: Rect) Size {
        return .{ .width = self.width(), .height = self.height() };
    }

    pub fn topLeft(self: Rect) Point {
        return .{ .x = self.left, .y = self.top };
    }

    pub fn bottomRight(self: Rect) Point {
        return .{ .x = self.right, .y = self.bottom };
    }

    pub fn center(self: Rect) Point {
        return .{
            .x = (self.left + self.right) / 2,
            .y = (self.top + self.bottom) / 2,
        };
    }

    pub fn isEmpty(self: Rect) bool {
        return self.width() <= 0 or self.height() <= 0;
    }

    pub fn isValid(self: Rect) bool {
        return self.left <= self.right and self.top <= self.bottom;
    }

    pub fn translated(self: Rect, dx: f32, dy: f32) Rect {
        return .{
            .left = self.left + dx,
            .top = self.top + dy,
            .right = self.right + dx,
            .bottom = self.bottom + dy,
        };
    }

    /// 向四周扩展 amount（负值即收缩）。
    pub fn expanded(self: Rect, amount: f32) Rect {
        return .{
            .left = self.left - amount,
            .top = self.top - amount,
            .right = self.right + amount,
            .bottom = self.bottom + amount,
        };
    }

    pub fn containsPoint(self: Rect, p: Point) bool {
        return p.x >= self.left and p.x <= self.right and
            p.y >= self.top and p.y <= self.bottom;
    }

    pub fn containsRect(self: Rect, other: Rect) bool {
        return other.left >= self.left and other.right <= self.right and
            other.top >= self.top and other.bottom <= self.bottom;
    }

    pub fn intersects(self: Rect, other: Rect) bool {
        return self.left < other.right and self.right > other.left and
            self.top < other.bottom and self.bottom > other.top;
    }

    /// 返回相交部分；若无交集则返回空矩形。
    pub fn intersected(self: Rect, other: Rect) Rect {
        const l = @max(self.left, other.left);
        const t = @max(self.top, other.top);
        const r = @min(self.right, other.right);
        const b = @min(self.bottom, other.bottom);
        if (r <= l or b <= t) return empty;
        return .{ .left = l, .top = t, .right = r, .bottom = b };
    }

    /// 返回包含两矩形的最小边界矩形。
    pub fn united(self: Rect, other: Rect) Rect {
        return .{
            .left = @min(self.left, other.left),
            .top = @min(self.top, other.top),
            .right = @max(self.right, other.right),
            .bottom = @max(self.bottom, other.bottom),
        };
    }

    /// 在 outer 中保持尺寸居中。
    pub fn centeredIn(self: Rect, outer: Rect) Rect {
        const w = self.width();
        const h = self.height();
        const cx = outer.center();
        return fromXywh(cx.x - w / 2, cx.y - h / 2, w, h);
    }

    pub fn eql(self: Rect, other: Rect) bool {
        return self.left == other.left and self.top == other.top and
            self.right == other.right and self.bottom == other.bottom;
    }
};

/// 四边缩进（padding / margin / border）。
pub const Insets = struct {
    left: f32 = 0,
    top: f32 = 0,
    right: f32 = 0,
    bottom: f32 = 0,

    pub const zero: Insets = .{};

    pub fn all(value: f32) Insets {
        return .{ .left = value, .top = value, .right = value, .bottom = value };
    }

    pub fn symmetric(h: f32, v: f32) Insets {
        return .{ .left = h, .top = v, .right = h, .bottom = v };
    }

    pub fn init(left: f32, top: f32, right: f32, bottom: f32) Insets {
        return .{ .left = left, .top = top, .right = right, .bottom = bottom };
    }

    pub fn horizontal(self: Insets) f32 {
        return self.left + self.right;
    }

    pub fn vertical(self: Insets) f32 {
        return self.top + self.bottom;
    }

    pub fn isZero(self: Insets) bool {
        return self.left == 0 and self.top == 0 and self.right == 0 and self.bottom == 0;
    }

    /// 向内应用到矩形（正值缩小）。
    pub fn applyToRect(self: Insets, rect: Rect) Rect {
        return .{
            .left = rect.left + self.left,
            .top = rect.top + self.top,
            .right = rect.right - self.right,
            .bottom = rect.bottom - self.bottom,
        };
    }

    /// 向外扩张矩形（正值扩大）。
    pub fn inflateRect(self: Insets, rect: Rect) Rect {
        return .{
            .left = rect.left - self.left,
            .top = rect.top - self.top,
            .right = rect.right + self.right,
            .bottom = rect.bottom + self.bottom,
        };
    }

    pub fn add(self: Insets, other: Insets) Insets {
        return .{
            .left = self.left + other.left,
            .top = self.top + other.top,
            .right = self.right + other.right,
            .bottom = self.bottom + other.bottom,
        };
    }

    pub fn eql(self: Insets, other: Insets) bool {
        return self.left == other.left and self.top == other.top and
            self.right == other.right and self.bottom == other.bottom;
    }
};

// ---- 测试 ----

test "Point 基本运算" {
    const a = Point.init(1, 2);
    const b = Point.init(3, 4);
    try std.testing.expect(a.add(b).eql(Point.init(4, 6)));
    try std.testing.expect(b.sub(a).eql(Point.init(2, 2)));
    try std.testing.expectApproxEqAbs(@as(f32, 5), Point.zero.distanceTo(Point.init(3, 4)), 1e-6);
}

test "Size 面积与空判断" {
    const s = Size.init(4, 5);
    try std.testing.expectEqual(@as(f32, 20), s.area());
    try std.testing.expect(!s.isEmpty());
    try std.testing.expect(Size.init(0, 10).isEmpty());
}

test "Rect 构造与访问" {
    const r = Rect.fromXywh(10, 20, 100, 80);
    try std.testing.expectEqual(@as(f32, 100), r.width());
    try std.testing.expectEqual(@as(f32, 80), r.height());
    try std.testing.expect(r.center().eql(Point.init(60, 60)));
    try std.testing.expect(r.topLeft().eql(Point.init(10, 20)));
}

test "Rect 相交与包含" {
    const a = Rect.fromXywh(0, 0, 100, 100);
    const b = Rect.fromXywh(50, 50, 100, 100);
    try std.testing.expect(a.intersects(b));
    try std.testing.expect(a.intersected(b).eql(Rect.fromLtrb(50, 50, 100, 100)));
    try std.testing.expect(a.containsPoint(Point.init(10, 10)));
    try std.testing.expect(!a.containsPoint(Point.init(150, 10)));

    const c = Rect.fromXywh(200, 200, 10, 10);
    try std.testing.expect(!a.intersects(c));
    try std.testing.expect(a.intersected(c).isEmpty());
}

test "Rect 居中" {
    const container = Rect.fromXywh(0, 0, 800, 600);
    const r = Rect.fromXywh(0, 0, 100, 80);
    const centered = r.centeredIn(container);
    try std.testing.expect(centered.eql(Rect.fromXywh(350, 260, 100, 80)));
}

test "Insets 应用到矩形" {
    const padding = Insets.all(10);
    const rect = Rect.fromXywh(0, 0, 100, 100);
    try std.testing.expect(padding.applyToRect(rect).eql(Rect.fromLtrb(10, 10, 90, 90)));
    try std.testing.expect(padding.inflateRect(rect).eql(Rect.fromLtrb(-10, -10, 110, 110)));
    try std.testing.expectEqual(@as(f32, 20), padding.horizontal());
}
