//! theme/palette —— 语义颜色调色板
//!
//! 组件按**语义角色**（ColorRole）查找颜色，从不直接写 #hex。每个角色在 light / dark
//! 两套方案下各有一个色值，切换方案即统一生效 —— 覆盖你说的「亮/暗两套」需求。
//!
//! 设计理念（对齐 Material 3 / shadcn）：
//!   - 库提供一套合理默认 palette（shadcn 中性灰）；
//!   - 开发者可逐角色 `set` 覆盖，或整体构造自有 palette，实现宽松自定义；
//!   - 颜色存为渲染就绪的 `foundation.Color`；做亮度/hover 变体时用 `color_space` 的
//!     OKLCH 在上层运算（见 color_space.zig），palette 本身只存终值。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");

const Color = foundation.Color;

/// 色彩方案（亮 / 暗）。
pub const Scheme = enum(u1) {
    light = 0,
    dark = 1,

    /// 切换到另一方案。
    pub fn toggled(self: Scheme) Scheme {
        return if (self == .light) .dark else .light;
    }
};

/// 语义颜色角色（Material 3 色彩系统子集）。组件按角色查色，而非持有具体色值。
pub const ColorRole = enum {
    // Primary
    primary,
    on_primary,
    primary_container,
    on_primary_container,
    // Secondary
    secondary,
    on_secondary,
    secondary_container,
    on_secondary_container,
    // Error
    error_,
    on_error,
    error_container,
    on_error_container,
    // Surface / Background
    surface,
    on_surface,
    surface_variant,
    on_surface_variant,
    background,
    on_background,
    // Outline
    outline,
    outline_variant,
    // Shadow / Scrim
    shadow,
    scrim,

    /// 角色总数（用于数组索引）。
    pub const count = @typeInfo(ColorRole).@"enum".fields.len;

    fn index(self: ColorRole) usize {
        return @intFromEnum(self);
    }
};

/// 语义调色板：每个角色在 light/dark 下各一个色值。
pub const Palette = struct {
    light: [ColorRole.count]Color,
    dark: [ColorRole.count]Color,

    /// 按角色 + 方案取色。
    pub fn get(self: *const Palette, role: ColorRole, scheme: Scheme) Color {
        return switch (scheme) {
            .light => self.light[role.index()],
            .dark => self.dark[role.index()],
        };
    }

    /// 覆盖单个角色的 light / dark 色值。
    pub fn set(self: *Palette, role: ColorRole, light_color: Color, dark_color: Color) void {
        self.light[role.index()] = light_color;
        self.dark[role.index()] = dark_color;
    }

    /// 库默认调色板（shadcn 中性灰风格）。
    pub fn default() Palette {
        var p: Palette = .{ .light = undefined, .dark = undefined };
        inline for (default_entries) |e| {
            p.light[@intFromEnum(e.role)] = hex(e.light);
            p.dark[@intFromEnum(e.role)] = hex(e.dark);
        }
        return p;
    }
};

fn hex(v: u32) Color {
    return Color.fromHexRgba(v);
}

const Entry = struct { role: ColorRole, light: u32, dark: u32 };

// 默认色值（0xRRGGBBAA）。来源：shadcn 默认中性灰主题（吸收自 archive）。
const default_entries = [_]Entry{
    .{ .role = .primary, .light = 0x171717FF, .dark = 0xFAFAFAFF },
    .{ .role = .on_primary, .light = 0xFAFAFAFF, .dark = 0x171717FF },
    .{ .role = .primary_container, .light = 0xF5F5F5FF, .dark = 0x262626FF },
    .{ .role = .on_primary_container, .light = 0x171717FF, .dark = 0xFAFAFAFF },

    .{ .role = .secondary, .light = 0xF5F5F5FF, .dark = 0x262626FF },
    .{ .role = .on_secondary, .light = 0x171717FF, .dark = 0xFAFAFAFF },
    .{ .role = .secondary_container, .light = 0xF5F5F5FF, .dark = 0x262626FF },
    .{ .role = .on_secondary_container, .light = 0x171717FF, .dark = 0xFAFAFAFF },

    .{ .role = .error_, .light = 0xEF4444FF, .dark = 0xEF4444FF },
    .{ .role = .on_error, .light = 0xFFFFFFFF, .dark = 0xFFFFFFFF },
    .{ .role = .error_container, .light = 0xFEE2E2FF, .dark = 0x7F1D1DFF },
    .{ .role = .on_error_container, .light = 0x991B1BFF, .dark = 0xFEE2E2FF },

    .{ .role = .surface, .light = 0xFFFFFFFF, .dark = 0x171717FF },
    .{ .role = .on_surface, .light = 0x0A0A0AFF, .dark = 0xFAFAFAFF },
    .{ .role = .surface_variant, .light = 0xF5F5F5FF, .dark = 0x262626FF },
    .{ .role = .on_surface_variant, .light = 0x737373FF, .dark = 0xA3A3A3FF },
    .{ .role = .background, .light = 0xFFFFFFFF, .dark = 0x0A0A0AFF },
    .{ .role = .on_background, .light = 0x0A0A0AFF, .dark = 0xFAFAFAFF },

    .{ .role = .outline, .light = 0xE5E5E5FF, .dark = 0xE5E5E519 },
    .{ .role = .outline_variant, .light = 0xE5E5E5FF, .dark = 0xE5E5E526 },

    .{ .role = .shadow, .light = 0x000000FF, .dark = 0x000000FF },
    .{ .role = .scrim, .light = 0x000000FF, .dark = 0x000000FF },
};

comptime {
    // 保证默认表覆盖了全部角色（漏写会在编译期暴露）。
    if (default_entries.len != ColorRole.count) {
        @compileError("default_entries 必须覆盖全部 ColorRole");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "默认 palette 覆盖全部角色" {
    const p = Palette.default();
    // surface 在 light 为白、dark 为深色
    const light_surface = p.get(.surface, .light);
    const dark_surface = p.get(.surface, .dark);
    try std.testing.expect(light_surface.r > 0.9);
    try std.testing.expect(dark_surface.r < 0.2);
}

test "scheme 切换" {
    try std.testing.expectEqual(Scheme.dark, Scheme.light.toggled());
    try std.testing.expectEqual(Scheme.light, Scheme.dark.toggled());
}

test "逐角色覆盖" {
    var p = Palette.default();
    const brand = Color.fromHexRgb(0x3B82F6);
    p.set(.primary, brand, brand);
    try std.testing.expect(p.get(.primary, .light).eql(brand));
    try std.testing.expect(p.get(.primary, .dark).eql(brand));
    // 其它角色不受影响
    try std.testing.expect(!p.get(.secondary, .light).eql(brand));
}

test "primary 在 light/dark 下相反" {
    const p = Palette.default();
    const lp = p.get(.primary, .light);
    const dp = p.get(.primary, .dark);
    // light 下 primary 是深色，dark 下是浅色
    try std.testing.expect(lp.r < 0.5);
    try std.testing.expect(dp.r > 0.5);
}

test "ColorRole.count 等于枚举字段数" {
    try std.testing.expectEqual(ColorRole.count, default_entries.len);
}
