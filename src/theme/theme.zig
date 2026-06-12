//! theme —— 主题与设计令牌层
//!
//! 把 primitive token、semantic palette、typography 聚合为一个 `Theme`，并提供按语义
//! 角色解析具体值的 resolver。设计遵循 design-system-first：先 token / palette / 语义
//! API，再具体 widget（见 docs/development/design-tokens.md）。
//!
//! 依赖方向：theme 仅依赖 foundation（颜色 / 色彩空间）。
//!
//! ## 「类似 global.css」的自定义模型
//!
//! 库提供一套完整默认主题 `Theme.default()`。开发者可：
//!   - 覆盖 token 的任意字段（`Tokens{ .radius = .{ .md = 10 } }`）；
//!   - 逐角色覆盖 palette（`theme.palette.set(.primary, light, dark)`）；
//!   - 或整体构造自己的 `Theme`。
//! 这相当于前端项目里换一套 global.css —— 宽松地重写整个 UI 的样式与配色。
//!
//! ## 亮 / 暗与「多主题」
//!
//! `Theme` 自带 light/dark 双方案，`setScheme` / `toggleScheme` 即可切换，覆盖最常见
//! 需求。更花哨的「多套命名主题快速切换」属于上层编排：用 reactive 的 `Signal(Theme)`
//! 持有当前主题、`effect` 在变化时重建即可，theme 层只需保证 `Theme` 是值语义、可整体
//! 替换。本层因此只保留 `name` 字段作为扩展位，不内建全局 manager（依赖规则：theme 只
//! 依赖 foundation，不感知响应式 / 运行时）。

const std = @import("std");

pub const tokens = @import("tokens.zig");
pub const palette = @import("palette.zig");

// ── 公共 API 再导出 ─────────────────────────────────────────────────────────────

pub const Tokens = tokens.Tokens;
pub const TypographyRole = tokens.TypographyRole;
pub const TypeStyle = tokens.TypeStyle;
pub const FontWeight = tokens.FontWeight;

pub const Palette = palette.Palette;
pub const ColorRole = palette.ColorRole;
pub const Scheme = palette.Scheme;

const foundation = @import("../foundation/foundation.zig");
const Color = foundation.Color;

/// 一个完整主题：名称 + 调色板 + 令牌 + 当前色彩方案。值语义，可整体拷贝 / 替换。
pub const Theme = struct {
    /// 主题名（扩展位：未来多主题注册 / 切换时用）。
    name: []const u8 = "default",
    palette: Palette,
    tokens: Tokens = .{},
    scheme: Scheme = .light,

    /// 库默认主题（shadcn 中性灰 + Material 3 token 尺度）。
    pub fn default() Theme {
        return .{ .palette = Palette.default() };
    }

    /// 切换当前色彩方案（light ↔ dark）。
    pub fn toggleScheme(self: *Theme) void {
        self.scheme = self.scheme.toggled();
    }

    /// 设置色彩方案。
    pub fn setScheme(self: *Theme, scheme: Scheme) void {
        self.scheme = scheme;
    }

    // ── resolver：按语义角色解析具体值（widgets 的统一入口）──

    /// 按角色解析颜色（用当前 scheme）。
    pub fn color(self: *const Theme, role: ColorRole) Color {
        return self.palette.get(role, self.scheme);
    }

    /// 按角色 + 指定 scheme 解析颜色。
    pub fn colorIn(self: *const Theme, role: ColorRole, scheme: Scheme) Color {
        return self.palette.get(role, scheme);
    }

    /// 按排版角色解析文本风格。
    pub fn typeStyle(self: *const Theme, role: TypographyRole) TypeStyle {
        return self.tokens.typography.resolve(role);
    }
};

test {
    std.testing.refAllDecls(@This());
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Theme.default 可用并按角色解析颜色" {
    const t = Theme.default();
    try std.testing.expectEqualStrings("default", t.name);
    try std.testing.expectEqual(Scheme.light, t.scheme);
    // 默认 light scheme 下 surface 接近白
    try std.testing.expect(t.color(.surface).r > 0.9);
}

test "切换 scheme 影响 color() 结果" {
    var t = Theme.default();
    const light_surface = t.color(.surface);
    t.toggleScheme();
    const dark_surface = t.color(.surface);
    try std.testing.expectEqual(Scheme.dark, t.scheme);
    try std.testing.expect(dark_surface.r < light_surface.r);
}

test "colorIn 显式指定 scheme 不依赖当前态" {
    const t = Theme.default();
    const dark_bg = t.colorIn(.background, .dark);
    try std.testing.expect(dark_bg.r < 0.2);
}

test "typeStyle 按角色解析" {
    const t = Theme.default();
    const headline = t.typeStyle(.headline_large);
    try std.testing.expectEqual(@as(f32, 32), headline.font_size);
}

test "自定义主题：覆盖 token 与 palette（类似 global.css）" {
    var t = Theme{
        .name = "brand",
        .palette = Palette.default(),
        .tokens = .{ .radius = .{ .md = 10 } }, // 覆盖一个 token
    };
    const brand = Color.fromHexRgb(0x3B82F6);
    t.palette.set(.primary, brand, brand);

    try std.testing.expectEqualStrings("brand", t.name);
    try std.testing.expectEqual(@as(f32, 10), t.tokens.radius.md);
    try std.testing.expect(t.color(.primary).eql(brand));
    // 未覆盖的 token 仍是默认
    try std.testing.expectEqual(@as(f32, 8), t.tokens.radius.sm);
}
