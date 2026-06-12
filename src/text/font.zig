//! text/font —— 字体度量接口与占位后端
//!
//! `FontMetrics` 是字体度量的 vtable 接口：给定字号，提供单个码点的步进宽度（advance）
//! 与垂直度量（ascent / descent / line gap）。真实实现由 HarfBuzz / FreeType 后端提供
//! （后续接入），本文件给一个**等宽估算后端** `MonospaceMetrics` 作占位 —— 纯逻辑、
//! 可测、无系统依赖，让 text 布局与 layout 层现在就能跑通。
//!
//! 设计要点（吸收 archive 教训）：
//! - text 层只负责「测量与布局」，**不**自己做裁剪 —— 裁剪用 render 的 push_clip/pop_clip，
//!   未来由 runtime/widgets 的统一 ClipNode 复用，不在每个文本组件重复造轮子。
//! - 度量接口与具体字体引擎解耦，符合依赖规则第 7 条（平台库隐藏在后端实现内）。

const std = @import("std");

/// 字重（100..900）。text 层独立定义，与 theme.FontWeight 同语义但不依赖 theme
/// （text 与 theme 同为中层，保持各自独立、只依赖 foundation）。
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

/// 文本样式：字号 / 字重 / 行高 / 字间距。
pub const TextStyle = struct {
    font_size: f32 = 14,
    font_weight: FontWeight = .regular,
    /// 行高（像素）。0 表示按字体 ascent+descent+line_gap 自动推导。
    line_height: f32 = 0,
    letter_spacing: f32 = 0,
};

/// 字体的垂直度量（给定字号下，单位像素）。
pub const VMetrics = struct {
    ascent: f32,
    descent: f32,
    line_gap: f32,

    /// 默认行高 = ascent + descent + line_gap。
    pub fn lineHeight(self: VMetrics) f32 {
        return self.ascent + self.descent + self.line_gap;
    }
};

/// 字体度量接口（vtable）。具体后端持有字体资源，通过 `interface()` 暴露本接口。
pub const FontMetrics = struct {
    ptr: *anyopaque,
    vtable: *const VTable,

    pub const VTable = struct {
        /// 单个码点在给定字号下的水平步进（advance），像素。
        advance: *const fn (ptr: *anyopaque, codepoint: u21, font_size: f32) f32,
        /// 给定字号下的垂直度量。
        vmetrics: *const fn (ptr: *anyopaque, font_size: f32) VMetrics,
    };

    pub fn advance(self: FontMetrics, codepoint: u21, font_size: f32) f32 {
        return self.vtable.advance(self.ptr, codepoint, font_size);
    }

    pub fn vmetrics(self: FontMetrics, font_size: f32) VMetrics {
        return self.vtable.vmetrics(self.ptr, font_size);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § MonospaceMetrics —— 等宽估算占位后端
// ─────────────────────────────────────────────────────────────────────────────

/// 等宽估算字体度量：每个码点宽度 = font_size × advance_ratio（CJK 视为双宽）。
/// 不依赖任何字体引擎，用于测试与无字体环境下的布局估算。
pub const MonospaceMetrics = struct {
    /// 普通字符相对字号的宽度比例（典型等宽字体约 0.6）。
    advance_ratio: f32 = 0.6,
    ascent_ratio: f32 = 0.8,
    descent_ratio: f32 = 0.2,
    line_gap_ratio: f32 = 0.0,

    pub fn interface(self: *MonospaceMetrics) FontMetrics {
        return .{ .ptr = self, .vtable = &vtable };
    }

    const vtable = FontMetrics.VTable{
        .advance = advanceImpl,
        .vmetrics = vmetricsImpl,
    };

    /// 判断码点是否按双宽处理（CJK 等宽字形）。粗略覆盖常见 CJK 区段。
    fn isWide(cp: u21) bool {
        return (cp >= 0x1100 and cp <= 0x115F) or // Hangul Jamo
            (cp >= 0x2E80 and cp <= 0x303E) or // CJK 部首 / 符号
            (cp >= 0x3041 and cp <= 0x33FF) or // 假名 / CJK 符号
            (cp >= 0x3400 and cp <= 0x4DBF) or // CJK 扩展 A
            (cp >= 0x4E00 and cp <= 0x9FFF) or // CJK 统一表意
            (cp >= 0xA000 and cp <= 0xA4CF) or // 彝文
            (cp >= 0xAC00 and cp <= 0xD7A3) or // 谚文音节
            (cp >= 0xF900 and cp <= 0xFAFF) or // CJK 兼容
            (cp >= 0xFF00 and cp <= 0xFF60) or // 全角 ASCII
            (cp >= 0xFFE0 and cp <= 0xFFE6);
    }

    fn advanceImpl(ptr: *anyopaque, codepoint: u21, font_size: f32) f32 {
        const self: *MonospaceMetrics = @ptrCast(@alignCast(ptr));
        const base = font_size * self.advance_ratio;
        return if (isWide(codepoint)) base * 2 else base;
    }

    fn vmetricsImpl(ptr: *anyopaque, font_size: f32) VMetrics {
        const self: *MonospaceMetrics = @ptrCast(@alignCast(ptr));
        return .{
            .ascent = font_size * self.ascent_ratio,
            .descent = font_size * self.descent_ratio,
            .line_gap = font_size * self.line_gap_ratio,
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "MonospaceMetrics advance：ASCII 与 CJK 双宽" {
    var m = MonospaceMetrics{};
    const fm = m.interface();
    try std.testing.expectApproxEqAbs(@as(f32, 9.6), fm.advance('A', 16), 1e-4); // 16*0.6
    try std.testing.expectApproxEqAbs(@as(f32, 19.2), fm.advance('中', 16), 1e-4); // 双宽
}

test "MonospaceMetrics vmetrics 与默认行高" {
    var m = MonospaceMetrics{};
    const fm = m.interface();
    const vm = fm.vmetrics(20);
    try std.testing.expectApproxEqAbs(@as(f32, 16), vm.ascent, 1e-4); // 20*0.8
    try std.testing.expectApproxEqAbs(@as(f32, 4), vm.descent, 1e-4); // 20*0.2
    try std.testing.expectApproxEqAbs(@as(f32, 20), vm.lineHeight(), 1e-4);
}

test "TextStyle 默认值" {
    const style = TextStyle{};
    try std.testing.expectEqual(@as(f32, 14), style.font_size);
    try std.testing.expectEqual(FontWeight.regular, style.font_weight);
}
