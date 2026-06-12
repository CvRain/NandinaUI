//! theme/tokens —— 原始设计令牌（Primitive Tokens）
//!
//! 设计系统的基础原子值（尺度），组件与语义样式都引用它们而非写死常量。
//! 设计理念（类似 shadcn / Radix primitives）：库提供一组合理默认值，开发者可覆盖任意
//! 字段定制品牌风格 —— 这就是「类似 global.css」的底座。
//!
//! token 按命名域分组：spacing / radius / border / elevation / opacity / typography。
//! 全部是带默认值的值语义 struct，覆盖时只需写要改的字段（其余取默认）。
//!
//! 依赖方向：仅依赖 foundation（如有需要）。本文件不含颜色（颜色在 palette.zig）。

const std = @import("std");

// ═════════════════════════════════════════════════════════════════════════════
// § 间距（4px 基准网格）
// ═════════════════════════════════════════════════════════════════════════════

/// 间距 token，用于 padding / gap / 内容留白。单位像素。
pub const Spacing = struct {
    none: f32 = 0,
    xxs: f32 = 2,
    xs: f32 = 4,
    sm: f32 = 8,
    md: f32 = 12,
    lg: f32 = 16,
    xl: f32 = 24,
    xxl: f32 = 32,
    xxxl: f32 = 48,
};

// ═════════════════════════════════════════════════════════════════════════════
// § 圆角
// ═════════════════════════════════════════════════════════════════════════════

/// 圆角 token。单位像素。`full` 用于 pill 形状。
pub const Radius = struct {
    none: f32 = 0,
    xs: f32 = 4,
    sm: f32 = 8,
    md: f32 = 12,
    lg: f32 = 16,
    xl: f32 = 24,
    full: f32 = 9999,
};

// ═════════════════════════════════════════════════════════════════════════════
// § 描边 / 分隔线 / focus ring
// ═════════════════════════════════════════════════════════════════════════════

/// 描边宽度 token。单位像素。
pub const Border = struct {
    none: f32 = 0,
    hairline: f32 = 1,
    thin: f32 = 1,
    medium: f32 = 2,
    thick: f32 = 3,
    divider: f32 = 1,
    focus_ring: f32 = 2,
};

// ═════════════════════════════════════════════════════════════════════════════
// § 高度 / 阴影深度
// ═════════════════════════════════════════════════════════════════════════════

/// 高度（阴影深度）token。level0 = 无阴影。
pub const Elevation = struct {
    level0: f32 = 0,
    level1: f32 = 1,
    level2: f32 = 3,
    level3: f32 = 6,
    level4: f32 = 9,
    level5: f32 = 12,
};

// ═════════════════════════════════════════════════════════════════════════════
// § 透明度语义
// ═════════════════════════════════════════════════════════════════════════════

/// 透明度 token，0..1。用于 disabled、scrim、按压反馈等。
pub const Opacity = struct {
    full: f32 = 1.0,
    high: f32 = 0.87,
    medium: f32 = 0.60,
    low: f32 = 0.38,
    disabled: f32 = 0.12,
    scrim: f32 = 0.32,
};

// ═════════════════════════════════════════════════════════════════════════════
// § 字体排印
// ═════════════════════════════════════════════════════════════════════════════

/// 字重（100..900）。
pub const FontWeight = enum(u16) {
    thin = 100,
    extra_light = 200,
    light = 300,
    regular = 400,
    medium = 500,
    semi_bold = 600,
    bold = 700,
    extra_bold = 800,
    black = 900,
};

/// 单个排版风格：字号 / 字重 / 行高 / 字间距（单位像素）。
pub const TypeStyle = struct {
    font_size: f32 = 14,
    font_weight: FontWeight = .regular,
    line_height: f32 = 20,
    letter_spacing: f32 = 0,
};

/// 排版角色（参考 Material 3 type scale）。控件优先用 role 而非裸字号。
pub const TypographyRole = enum {
    display_large,
    display_medium,
    display_small,
    headline_large,
    headline_medium,
    headline_small,
    title_large,
    title_medium,
    title_small,
    body_large,
    body_medium,
    body_small,
    label_large,
    label_medium,
    label_small,
};

/// 全部排版风格集合，每个角色一个 TypeStyle，带 Material 3 默认值。
pub const Typography = struct {
    display_large: TypeStyle = .{ .font_size = 57, .line_height = 64, .letter_spacing = -0.25 },
    display_medium: TypeStyle = .{ .font_size = 45, .line_height = 52 },
    display_small: TypeStyle = .{ .font_size = 36, .line_height = 44 },

    headline_large: TypeStyle = .{ .font_size = 32, .line_height = 40 },
    headline_medium: TypeStyle = .{ .font_size = 28, .line_height = 36 },
    headline_small: TypeStyle = .{ .font_size = 24, .line_height = 32 },

    title_large: TypeStyle = .{ .font_size = 22, .line_height = 28 },
    title_medium: TypeStyle = .{ .font_size = 16, .font_weight = .medium, .line_height = 24, .letter_spacing = 0.15 },
    title_small: TypeStyle = .{ .font_size = 14, .font_weight = .medium, .line_height = 20, .letter_spacing = 0.1 },

    body_large: TypeStyle = .{ .font_size = 16, .line_height = 24, .letter_spacing = 0.5 },
    body_medium: TypeStyle = .{ .font_size = 14, .line_height = 20, .letter_spacing = 0.25 },
    body_small: TypeStyle = .{ .font_size = 12, .line_height = 16, .letter_spacing = 0.4 },

    label_large: TypeStyle = .{ .font_size = 14, .font_weight = .medium, .line_height = 20, .letter_spacing = 0.1 },
    label_medium: TypeStyle = .{ .font_size = 12, .font_weight = .medium, .line_height = 16, .letter_spacing = 0.5 },
    label_small: TypeStyle = .{ .font_size = 11, .font_weight = .medium, .line_height = 16, .letter_spacing = 0.5 },

    /// 按角色取出对应风格。
    pub fn resolve(self: *const Typography, role: TypographyRole) TypeStyle {
        return switch (role) {
            .display_large => self.display_large,
            .display_medium => self.display_medium,
            .display_small => self.display_small,
            .headline_large => self.headline_large,
            .headline_medium => self.headline_medium,
            .headline_small => self.headline_small,
            .title_large => self.title_large,
            .title_medium => self.title_medium,
            .title_small => self.title_small,
            .body_large => self.body_large,
            .body_medium => self.body_medium,
            .body_small => self.body_small,
            .label_large => self.label_large,
            .label_medium => self.label_medium,
            .label_small => self.label_small,
        };
    }
};

// ═════════════════════════════════════════════════════════════════════════════
// § 聚合
// ═════════════════════════════════════════════════════════════════════════════

/// 聚合全部 primitive token 的容器。每个 Theme 持有一份，可覆盖任意字段。
pub const Tokens = struct {
    spacing: Spacing = .{},
    radius: Radius = .{},
    border: Border = .{},
    elevation: Elevation = .{},
    opacity: Opacity = .{},
    typography: Typography = .{},
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Tokens 默认值" {
    const t = Tokens{};
    try std.testing.expectEqual(@as(f32, 12), t.spacing.md);
    try std.testing.expectEqual(@as(f32, 9999), t.radius.full);
    try std.testing.expectEqual(@as(f32, 2), t.border.focus_ring);
}

test "覆盖部分字段，其余取默认" {
    const t = Tokens{ .spacing = .{ .md = 16 } };
    try std.testing.expectEqual(@as(f32, 16), t.spacing.md);
    try std.testing.expectEqual(@as(f32, 8), t.spacing.sm); // 未覆盖，仍是默认
}

test "Typography role 解析" {
    const ty = Typography{};
    const title = ty.resolve(.title_medium);
    try std.testing.expectEqual(@as(f32, 16), title.font_size);
    try std.testing.expectEqual(FontWeight.medium, title.font_weight);

    const body = ty.resolve(.body_small);
    try std.testing.expectEqual(@as(f32, 12), body.font_size);
}

test "FontWeight 数值" {
    try std.testing.expectEqual(@as(u16, 700), @intFromEnum(FontWeight.bold));
}
