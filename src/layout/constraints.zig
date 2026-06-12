//! layout/constraints —— 布局协议层：尺寸约束
//!
//! `Constraints` 是布局过程中稳定的中间语义（min/max width/height），是 NandinaUI
//! 自有的核心边界 —— 无论未来求解层是否接入第三方引擎，对上提供的约束语义不变。
//!
//! 约定（见 docs/development/layout-strategy.md）：
//!   measure(constraints) -> Size   自底向上求 preferred size
//!   layout(bounds: Rect) -> void    自顶向下分配最终几何
//!
//! 无界尺寸用 `inf` 表示。求解器只对「有界可用空间」做减法 / 乘法，
//! 对无界的 max 只用 clamp（`@min` 的上界为 inf 是安全的），避免产生 NaN。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");

const Size = foundation.Size;

/// 表示「无界」的尺寸上限。
pub const inf: f32 = std.math.inf(f32);

/// 尺寸约束：宽高各自的最小 / 最大值。
pub const Constraints = struct {
    min_width: f32 = 0,
    max_width: f32 = inf,
    min_height: f32 = 0,
    max_height: f32 = inf,

    /// 完全不约束（0 → 无穷）。
    pub const unbounded: Constraints = .{};

    /// 强约束到一个确定尺寸（min == max）。
    pub fn tight(size: Size) Constraints {
        return .{
            .min_width = size.width,
            .max_width = size.width,
            .min_height = size.height,
            .max_height = size.height,
        };
    }

    /// 宽高分别强约束。
    pub fn tightFor(width: f32, height: f32) Constraints {
        return tight(.{ .width = width, .height = height });
    }

    /// 松约束：下界为 0，上界为给定最大值。
    pub fn loose(max_width: f32, max_height: f32) Constraints {
        return .{ .max_width = max_width, .max_height = max_height };
    }

    /// 是否宽度被强约束（min == max）。
    pub fn isTightWidth(self: Constraints) bool {
        return self.min_width == self.max_width;
    }

    /// 是否高度被强约束。
    pub fn isTightHeight(self: Constraints) bool {
        return self.min_height == self.max_height;
    }

    /// 宽高都被强约束。
    pub fn isTight(self: Constraints) bool {
        return self.isTightWidth() and self.isTightHeight();
    }

    /// 宽度是否有界。
    pub fn hasBoundedWidth(self: Constraints) bool {
        return self.max_width != inf;
    }

    /// 高度是否有界。
    pub fn hasBoundedHeight(self: Constraints) bool {
        return self.max_height != inf;
    }

    /// 把宽度夹到 [min_width, max_width]。
    pub fn constrainWidth(self: Constraints, width: f32) f32 {
        return std.math.clamp(width, self.min_width, self.max_width);
    }

    /// 把高度夹到 [min_height, max_height]。
    pub fn constrainHeight(self: Constraints, height: f32) f32 {
        return std.math.clamp(height, self.min_height, self.max_height);
    }

    /// 把一个尺寸夹进约束。
    pub fn constrain(self: Constraints, size: Size) Size {
        return .{
            .width = self.constrainWidth(size.width),
            .height = self.constrainHeight(size.height),
        };
    }

    /// 放松下界为 0（上界不变）。父容器以 loose 约束 measure 子节点时使用。
    pub fn loosen(self: Constraints) Constraints {
        return .{
            .min_width = 0,
            .max_width = self.max_width,
            .min_height = 0,
            .max_height = self.max_height,
        };
    }

    /// 在本约束内再收紧一层（与另一约束求交）。
    pub fn enforce(self: Constraints, other: Constraints) Constraints {
        return .{
            .min_width = std.math.clamp(self.min_width, other.min_width, other.max_width),
            .max_width = std.math.clamp(self.max_width, other.min_width, other.max_width),
            .min_height = std.math.clamp(self.min_height, other.min_height, other.max_height),
            .max_height = std.math.clamp(self.max_height, other.min_height, other.max_height),
        };
    }

    /// 满足约束的最小尺寸（下界）。
    pub fn smallest(self: Constraints) Size {
        return .{ .width = self.min_width, .height = self.min_height };
    }

    /// 满足约束的最大有界尺寸（无界轴退化为下界）。
    pub fn biggest(self: Constraints) Size {
        return .{
            .width = if (self.hasBoundedWidth()) self.max_width else self.min_width,
            .height = if (self.hasBoundedHeight()) self.max_height else self.min_height,
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "tight 强约束" {
    const c = Constraints.tightFor(100, 50);
    try std.testing.expect(c.isTight());
    try std.testing.expectEqual(@as(f32, 100), c.constrainWidth(999));
    try std.testing.expectEqual(@as(f32, 100), c.constrainWidth(0));
}

test "loose 松约束与 constrain" {
    const c = Constraints.loose(200, 100);
    try std.testing.expect(!c.isTight());
    try std.testing.expectEqual(@as(f32, 150), c.constrainWidth(150));
    try std.testing.expectEqual(@as(f32, 200), c.constrainWidth(300));
    const s = c.constrain(.{ .width = 300, .height = 40 });
    try std.testing.expectEqual(@as(f32, 200), s.width);
    try std.testing.expectEqual(@as(f32, 40), s.height);
}

test "有界性判断" {
    const c = Constraints.loose(200, inf);
    try std.testing.expect(c.hasBoundedWidth());
    try std.testing.expect(!c.hasBoundedHeight());
}

test "loosen 放松下界" {
    const c = Constraints{ .min_width = 50, .max_width = 200, .min_height = 30, .max_height = 100 };
    const l = c.loosen();
    try std.testing.expectEqual(@as(f32, 0), l.min_width);
    try std.testing.expectEqual(@as(f32, 200), l.max_width);
    try std.testing.expectEqual(@as(f32, 0), l.min_height);
}

test "biggest 无界轴退化为下界" {
    const c = Constraints{ .min_width = 10, .max_width = inf, .min_height = 5, .max_height = 80 };
    const b = c.biggest();
    try std.testing.expectEqual(@as(f32, 10), b.width);
    try std.testing.expectEqual(@as(f32, 80), b.height);
}
