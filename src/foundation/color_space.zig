//! foundation/color_space —— 多色彩空间与统一转换
//!
//! 提供 OKLab / OKLCH / Lab / LCH / Rgb / Hex 等色彩空间类型，以及任意两种空间之间的
//! 转换 `convert(To, value)`。设计沿用旧 C++ 主线：**以 OKLab 为中转枢纽（hub）**，
//! 任意 A→B 转换都走 `A.toOklab() → B.fromOklab()`，新增空间只需 O(1) 实现两个方法。
//!
//! ## 可扩展性（库使用者对接自有颜色格式）
//!
//! 任何满足「色彩空间契约」的类型都能直接参与 `convert` —— 无需修改本库。契约是：
//!   - `pub fn toOklab(self: @This()) Oklab`
//!   - `pub fn fromOklab(c: Oklab) @This()`
//!
//! ```zig
//! const MyColor = struct {
//!     // ... 自定义字段
//!     pub fn toOklab(self: @This()) color_space.Oklab { ... }
//!     pub fn fromOklab(c: color_space.Oklab) @This() { ... }
//! };
//! const v = color_space.convert(MyColor, some_rgb); // 直接可用
//! ```
//!
//! ## 与 foundation.Color 的关系
//!
//! `foundation.Color` 是渲染就绪的归一化 RGBA（render Backend 直接消费）。本模块负责
//! 调色运算（在感知均匀的 OKLab/OKLCH 空间做亮度/色相变换），算完用 `toColor` /
//! `fromColor` 在两端转换。色彩空间类型本身不参与 render，避免渲染热路径做色彩转换。

const std = @import("std");
const color = @import("color.zig");

const Color = color.Color;

// ─────────────────────────────────────────────────────────────────────────────
// § 工具
// ─────────────────────────────────────────────────────────────────────────────

fn clampUnit(v: f32) f32 {
    return std.math.clamp(v, 0.0, 1.0);
}

/// 把色相角归一化到 [0, 360)。
fn normalizeHue(hue: f32) f32 {
    var h = @mod(hue, 360.0);
    if (h < 0) h += 360.0;
    return h;
}

fn srgbToLinear(v: f32) f32 {
    const c = clampUnit(v);
    if (c <= 0.04045) return c / 12.92;
    return std.math.pow(f32, (c + 0.055) / 1.055, 2.4);
}

fn linearToSrgb(v: f32) f32 {
    const c = @max(v, 0.0);
    if (c <= 0.0031308) return 12.92 * c;
    return 1.055 * std.math.pow(f32, c, 1.0 / 2.4) - 0.055;
}

const LinearRgb = struct { r: f32, g: f32, b: f32 };
const Xyz = struct { x: f32, y: f32, z: f32 };

fn linearToXyz(c: LinearRgb) Xyz {
    return .{
        .x = 0.4124564 * c.r + 0.3575761 * c.g + 0.1804375 * c.b,
        .y = 0.2126729 * c.r + 0.7151522 * c.g + 0.0721750 * c.b,
        .z = 0.0193339 * c.r + 0.1191920 * c.g + 0.9503041 * c.b,
    };
}

fn xyzToLinear(c: Xyz) LinearRgb {
    return .{
        .r = 3.2404542 * c.x - 1.5371385 * c.y - 0.4985314 * c.z,
        .g = -0.9692660 * c.x + 1.8760108 * c.y + 0.0415560 * c.z,
        .b = 0.0556434 * c.x - 0.2040259 * c.y + 1.0572252 * c.z,
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// § Oklab —— 中转枢纽
// ─────────────────────────────────────────────────────────────────────────────

/// OKLab 色彩空间：感知均匀，作为所有空间转换的枢纽。
/// l ∈ [0,1]（亮度），a/b 为色度轴，alpha ∈ [0,1]。
pub const Oklab = struct {
    l: f32 = 0,
    a: f32 = 0,
    b: f32 = 0,
    alpha: f32 = 1,

    pub fn toOklab(self: Oklab) Oklab {
        return self;
    }

    pub fn fromOklab(c: Oklab) Oklab {
        return c;
    }

    fn toLinearRgb(self: Oklab) LinearRgb {
        const l_ = self.l + 0.3963377774 * self.a + 0.2158037573 * self.b;
        const m_ = self.l - 0.1055613458 * self.a - 0.0638541728 * self.b;
        const s_ = self.l - 0.0894841775 * self.a - 1.2914855480 * self.b;
        const l3 = l_ * l_ * l_;
        const m3 = m_ * m_ * m_;
        const s3 = s_ * s_ * s_;
        return .{
            .r = 4.0767416621 * l3 - 3.3077115913 * m3 + 0.2309699292 * s3,
            .g = -1.2684380046 * l3 + 2.6097574011 * m3 - 0.3413193965 * s3,
            .b = -0.0041960863 * l3 - 0.7034186147 * m3 + 1.7076147010 * s3,
        };
    }

    fn fromLinearRgb(c: LinearRgb, alpha: f32) Oklab {
        const l_ = std.math.cbrt(0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b);
        const m_ = std.math.cbrt(0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b);
        const s_ = std.math.cbrt(0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b);
        return .{
            .l = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
            .a = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
            .b = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_,
            .alpha = alpha,
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § OKLCH —— OKLab 的极坐标形式
// ─────────────────────────────────────────────────────────────────────────────

/// OKLCH：OKLab 的极坐标表示，最适合做亮度/色相/饱和度调节。
/// l ∈ [0,1]，c ≥ 0（色度），h ∈ [0,360)（色相角，度），alpha ∈ [0,1]。
pub const Oklch = struct {
    l: f32 = 0,
    c: f32 = 0,
    h: f32 = 0,
    alpha: f32 = 1,

    pub fn toOklab(self: Oklch) Oklab {
        const hue_rad = normalizeHue(self.h) * std.math.pi / 180.0;
        return .{
            .l = self.l,
            .a = self.c * @cos(hue_rad),
            .b = self.c * @sin(hue_rad),
            .alpha = self.alpha,
        };
    }

    pub fn fromOklab(c: Oklab) Oklch {
        const chroma = std.math.hypot(c.a, c.b);
        const hue = normalizeHue(std.math.atan2(c.b, c.a) * 180.0 / std.math.pi);
        return .{ .l = c.l, .c = chroma, .h = hue, .alpha = c.alpha };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § CIELAB / LCH（D65 白点）
// ─────────────────────────────────────────────────────────────────────────────

const white_x: f32 = 0.95047;
const white_y: f32 = 1.0;
const white_z: f32 = 1.08883;

fn labF(v: f32) f32 {
    const epsilon: f32 = 216.0 / 24389.0;
    const kappa: f32 = 24389.0 / 27.0;
    if (v > epsilon) return std.math.cbrt(v);
    return (kappa * v + 16.0) / 116.0;
}

fn labFInverse(v: f32) f32 {
    const delta: f32 = 6.0 / 29.0;
    if (v > delta) return v * v * v;
    return 3.0 * delta * delta * (v - 4.0 / 29.0);
}

/// CIELAB 色彩空间（D65 白点）。l ∈ [0,100]，a/b 色度轴，alpha ∈ [0,1]。
pub const Lab = struct {
    l: f32 = 0,
    a: f32 = 0,
    b: f32 = 0,
    alpha: f32 = 1,

    pub fn toOklab(self: Lab) Oklab {
        const fy = (self.l + 16.0) / 116.0;
        const fx = fy + self.a / 500.0;
        const fz = fy - self.b / 200.0;
        const xyz = Xyz{
            .x = white_x * labFInverse(fx),
            .y = white_y * labFInverse(fy),
            .z = white_z * labFInverse(fz),
        };
        return Oklab.fromLinearRgb(xyzToLinear(xyz), self.alpha);
    }

    pub fn fromOklab(c: Oklab) Lab {
        const xyz = linearToXyz(c.toLinearRgb());
        const fx = labF(xyz.x / white_x);
        const fy = labF(xyz.y / white_y);
        const fz = labF(xyz.z / white_z);
        return .{
            .l = 116.0 * fy - 16.0,
            .a = 500.0 * (fx - fy),
            .b = 200.0 * (fy - fz),
            .alpha = c.alpha,
        };
    }
};

/// CIELCH：CIELAB 的极坐标形式。l ∈ [0,100]，c ≥ 0，h ∈ [0,360)，alpha ∈ [0,1]。
pub const Lch = struct {
    l: f32 = 0,
    c: f32 = 0,
    h: f32 = 0,
    alpha: f32 = 1,

    pub fn toOklab(self: Lch) Oklab {
        const hue_rad = normalizeHue(self.h) * std.math.pi / 180.0;
        const lab = Lab{
            .l = self.l,
            .a = self.c * @cos(hue_rad),
            .b = self.c * @sin(hue_rad),
            .alpha = self.alpha,
        };
        return lab.toOklab();
    }

    pub fn fromOklab(c: Oklab) Lch {
        const lab = Lab.fromOklab(c);
        const chroma = std.math.hypot(lab.a, lab.b);
        const hue = normalizeHue(std.math.atan2(lab.b, lab.a) * 180.0 / std.math.pi);
        return .{ .l = lab.l, .c = chroma, .h = hue, .alpha = lab.alpha };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § sRGB（0..1 浮点）与 HexRgb（8-bit 打包）
// ─────────────────────────────────────────────────────────────────────────────

/// sRGB 色彩空间，分量 0..1。与 foundation.Color 同构，但作为「色彩空间」参与 convert。
pub const Rgb = struct {
    r: f32 = 0,
    g: f32 = 0,
    b: f32 = 0,
    alpha: f32 = 1,

    pub fn toOklab(self: Rgb) Oklab {
        return Oklab.fromLinearRgb(.{
            .r = srgbToLinear(self.r),
            .g = srgbToLinear(self.g),
            .b = srgbToLinear(self.b),
        }, self.alpha);
    }

    pub fn fromOklab(c: Oklab) Rgb {
        const lin = c.toLinearRgb();
        return .{
            .r = clampUnit(linearToSrgb(lin.r)),
            .g = clampUnit(linearToSrgb(lin.g)),
            .b = clampUnit(linearToSrgb(lin.b)),
            .alpha = clampUnit(c.alpha),
        };
    }
};

/// HexRgb：8-bit 整数分量（与 CSS hex / 0xRRGGBBAA 对应）。
pub const HexRgb = struct {
    r: u8 = 0,
    g: u8 = 0,
    b: u8 = 0,
    a: u8 = 255,

    /// 由 0xRRGGBB 构造（不透明）。
    pub fn fromRgb(hex: u24) HexRgb {
        return .{
            .r = @intCast((hex >> 16) & 0xFF),
            .g = @intCast((hex >> 8) & 0xFF),
            .b = @intCast(hex & 0xFF),
            .a = 255,
        };
    }

    /// 由 0xRRGGBBAA 构造。
    pub fn fromRgba(hex: u32) HexRgb {
        return .{
            .r = @intCast((hex >> 24) & 0xFF),
            .g = @intCast((hex >> 16) & 0xFF),
            .b = @intCast((hex >> 8) & 0xFF),
            .a = @intCast(hex & 0xFF),
        };
    }

    /// 打包为 0xRRGGBBAA。
    pub fn packed_rgba(self: HexRgb) u32 {
        return (@as(u32, self.r) << 24) | (@as(u32, self.g) << 16) |
            (@as(u32, self.b) << 8) | @as(u32, self.a);
    }

    fn toRgb(self: HexRgb) Rgb {
        const inv: f32 = 1.0 / 255.0;
        return .{
            .r = @as(f32, @floatFromInt(self.r)) * inv,
            .g = @as(f32, @floatFromInt(self.g)) * inv,
            .b = @as(f32, @floatFromInt(self.b)) * inv,
            .alpha = @as(f32, @floatFromInt(self.a)) * inv,
        };
    }

    pub fn toOklab(self: HexRgb) Oklab {
        return self.toRgb().toOklab();
    }

    pub fn fromOklab(c: Oklab) HexRgb {
        const rgb = Rgb.fromOklab(c);
        const to8 = struct {
            fn f(v: f32) u8 {
                return @intFromFloat(@round(clampUnit(v) * 255.0));
            }
        }.f;
        return .{ .r = to8(rgb.r), .g = to8(rgb.g), .b = to8(rgb.b), .a = to8(rgb.alpha) };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § 统一转换
// ─────────────────────────────────────────────────────────────────────────────

/// 编译期校验：类型 `T` 是否满足色彩空间契约（有 toOklab / fromOklab）。
pub fn isColorSpace(comptime T: type) bool {
    return @hasDecl(T, "toOklab") and @hasDecl(T, "fromOklab");
}

fn assertColorSpace(comptime T: type) void {
    if (!isColorSpace(T)) {
        @compileError(@typeName(T) ++ " 不是色彩空间：需要 toOklab(self) Oklab 与 fromOklab(Oklab) Self");
    }
}

/// 在任意两个色彩空间之间转换：`convert(To, from_value)`。
/// `To` 是目标类型，`value` 是任意满足契约的源色彩空间值。
pub fn convert(comptime To: type, value: anytype) To {
    const From = @TypeOf(value);
    comptime assertColorSpace(From);
    comptime assertColorSpace(To);
    return To.fromOklab(value.toOklab());
}

/// 把任意色彩空间值转换为渲染就绪的 `foundation.Color`（归一化 RGBA）。
pub fn toColor(value: anytype) Color {
    const rgb = convert(Rgb, value);
    return .{ .r = rgb.r, .g = rgb.g, .b = rgb.b, .a = rgb.alpha };
}

/// 把 `foundation.Color` 提升为目标色彩空间（用于调色运算）。
pub fn fromColor(comptime To: type, c: Color) To {
    const rgb = Rgb{ .r = c.r, .g = c.g, .b = c.b, .alpha = c.a };
    return convert(To, rgb);
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const expectApprox = std.testing.expectApproxEqAbs;

test "Rgb ↔ Oklab 往返" {
    const rgb = Rgb{ .r = 0.2, .g = 0.5, .b = 0.9, .alpha = 1 };
    const back = convert(Rgb, rgb.toOklab());
    try expectApprox(rgb.r, back.r, 1e-4);
    try expectApprox(rgb.g, back.g, 1e-4);
    try expectApprox(rgb.b, back.b, 1e-4);
}

test "Oklch ↔ Oklab 往返" {
    const lch = Oklch{ .l = 0.6, .c = 0.12, .h = 150, .alpha = 1 };
    const back = Oklch.fromOklab(lch.toOklab());
    try expectApprox(lch.l, back.l, 1e-4);
    try expectApprox(lch.c, back.c, 1e-4);
    try expectApprox(lch.h, back.h, 1e-3);
}

test "convert Rgb → Oklch → Rgb 往返" {
    const rgb = Rgb{ .r = 0.8, .g = 0.3, .b = 0.1, .alpha = 1 };
    const oklch = convert(Oklch, rgb);
    const back = convert(Rgb, oklch);
    try expectApprox(rgb.r, back.r, 1e-3);
    try expectApprox(rgb.g, back.g, 1e-3);
    try expectApprox(rgb.b, back.b, 1e-3);
}

test "Lab ↔ Oklab 往返" {
    const lab = Lab{ .l = 50, .a = 20, .b = -30, .alpha = 1 };
    const back = Lab.fromOklab(lab.toOklab());
    try expectApprox(lab.l, back.l, 1e-2);
    try expectApprox(lab.a, back.a, 1e-2);
    try expectApprox(lab.b, back.b, 1e-2);
}

test "Lch ↔ Lab 一致性" {
    // 纯白在 Lab 中 a=b=0，转 Lch 色度应接近 0
    const white_lab = Lab{ .l = 100, .a = 0, .b = 0, .alpha = 1 };
    const lch = convert(Lch, white_lab);
    try expectApprox(@as(f32, 0), lch.c, 1e-2);
    try expectApprox(@as(f32, 100), lch.l, 1e-2);
}

test "HexRgb 构造与打包" {
    const c = HexRgb.fromRgb(0x3B82F6);
    try std.testing.expectEqual(@as(u8, 0x3B), c.r);
    try std.testing.expectEqual(@as(u8, 0x82), c.g);
    try std.testing.expectEqual(@as(u8, 0xF6), c.b);
    try std.testing.expectEqual(@as(u32, 0x3B82F6FF), c.packed_rgba());
}

test "HexRgb → Oklab → HexRgb 往返" {
    const hex = HexRgb.fromRgb(0x336699);
    const back = convert(HexRgb, hex.toOklab());
    try std.testing.expectEqual(hex.r, back.r);
    try std.testing.expectEqual(hex.g, back.g);
    try std.testing.expectEqual(hex.b, back.b);
}

test "toColor / fromColor 与 foundation.Color 互通" {
    const oklch = Oklch{ .l = 0.7, .c = 0.1, .h = 30, .alpha = 0.5 };
    const c = toColor(oklch);
    try expectApprox(@as(f32, 0.5), c.a, 1e-4);
    // 再转回 Oklch 应接近
    const back = fromColor(Oklch, c);
    try expectApprox(oklch.l, back.l, 1e-3);
    try expectApprox(oklch.alpha, back.alpha, 1e-3);
}

test "Oklch 调亮度（感知均匀变体）" {
    // 取一个颜色，在 OKLCH 中提高亮度，应得到更亮的 RGB
    const base = HexRgb.fromRgb(0x3B82F6);
    var oklch = convert(Oklch, base);
    const before = oklch.l;
    oklch.l = @min(1.0, oklch.l + 0.15);
    try std.testing.expect(oklch.l > before);
    const lighter = convert(HexRgb, oklch);
    // 亮度提升后，至少有一个通道不低于原值
    try std.testing.expect(@as(u32, lighter.r) + lighter.g + lighter.b >= @as(u32, base.r) + base.g + base.b);
}

test "isColorSpace 契约检查" {
    try std.testing.expect(isColorSpace(Oklab));
    try std.testing.expect(isColorSpace(Oklch));
    try std.testing.expect(isColorSpace(Rgb));
    try std.testing.expect(isColorSpace(HexRgb));
    try std.testing.expect(!isColorSpace(struct { x: f32 }));
}

test "自定义色彩空间可直接参与 convert（可扩展性）" {
    // 模拟库使用者定义自己的格式：灰度（只有亮度）
    const Gray = struct {
        v: f32, // 0..1 灰度
        pub fn toOklab(self: @This()) Oklab {
            const rgb = Rgb{ .r = self.v, .g = self.v, .b = self.v, .alpha = 1 };
            return rgb.toOklab();
        }
        pub fn fromOklab(c: Oklab) @This() {
            const rgb = Rgb.fromOklab(c);
            return .{ .v = (rgb.r + rgb.g + rgb.b) / 3.0 };
        }
    };
    const g = Gray{ .v = 0.5 };
    const as_hex = convert(HexRgb, g); // 自定义类型直接转标准空间
    // 灰度转换后三通道应基本相等（浮点矩阵误差允许 ±1）
    const max_ch = @max(as_hex.r, @max(as_hex.g, as_hex.b));
    const min_ch = @min(as_hex.r, @min(as_hex.g, as_hex.b));
    try std.testing.expect(max_ch - min_ch <= 1);
}
