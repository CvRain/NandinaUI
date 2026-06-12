//! foundation —— 基础能力层
//!
//! 提供几何（point/size/rect/insets）、颜色、约束等与具体 UI 无关的纯数据类型。
//! 本层不依赖任何其它 NandinaUI 层，是整个依赖图的根。
const std = @import("std");

pub const geometry = @import("geometry.zig");
pub const color = @import("color.zig");
pub const color_space = @import("color_space.zig");

// 常用类型的便捷再导出
pub const Point = geometry.Point;
pub const Size = geometry.Size;
pub const Rect = geometry.Rect;
pub const Insets = geometry.Insets;
pub const Color = color.Color;

// 色彩空间类型的便捷再导出
pub const Oklab = color_space.Oklab;
pub const Oklch = color_space.Oklch;
pub const Lab = color_space.Lab;
pub const Lch = color_space.Lch;
pub const Rgb = color_space.Rgb;
pub const HexRgb = color_space.HexRgb;

test {
    std.testing.refAllDecls(@This());
}
