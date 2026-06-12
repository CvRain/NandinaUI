//! text/layout —— 文本测量与布局（约束宽度 + 溢出策略一等公民）
//!
//! `measure(allocator, text, style, constraints, metrics)` 在给定**约束宽高**内把文本排成
//! 多行，并按**溢出策略**处理超出部分，返回 `TextLayout`（多行 + 实际占用尺寸 + 是否截断）。
//!
//! 这是吸收 archive 教训的关键设计：约束宽度、换行、行数上限、溢出策略从一开始就是
//! 输入的一等公民，从根上杜绝「无界换行导致文字溢出组件」。
//!
//! ## 溢出策略（Overflow）
//! - `clip`：单行，超出可用宽度的部分直接不计入（硬截断，无省略号）。
//! - `ellipsis`（**默认**）：单行，超宽时在末尾放 "…"。开销小，是最常用策略。
//! - `wrap`：按可用宽度折行；受 `max_lines` 限制，最后一行超出仍按 ellipsis 收尾。
//! - `scale`：按比例缩小字号直到放进约束（开销较大，谨慎使用）。
//!
//! ## 与裁剪的关系（重要）
//! text 层**不做裁剪**。它只保证「布局结果不超过约束」。真正的像素级裁剪由 render 的
//! push_clip/pop_clip 负责，未来 runtime/widgets 提供统一 ClipNode 复用 —— 文本组件
//! 不必各自造裁剪轮子。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const font = @import("font.zig");

const Size = foundation.Size;
const FontMetrics = font.FontMetrics;
const TextStyle = font.TextStyle;

/// 文本溢出策略。
pub const Overflow = enum {
    clip,
    ellipsis,
    wrap,
    scale,
};

/// 文本布局约束。`max_width` <= 0 视为无界（不换行 / 不截断宽度）。
pub const Constraints = struct {
    max_width: f32 = 0,
    /// 最大行数。0 表示不限制（仅 wrap 有意义；clip/ellipsis 恒为单行）。
    max_lines: usize = 0,
};

/// 单行布局结果。`text` 是源文本的切片（借用，不拥有）。
pub const Line = struct {
    /// 该行对应的源文本字节切片。
    text: []const u8,
    width: f32,
    /// 该行是否以省略号结尾（被截断）。
    ellipsized: bool = false,
};

/// 文本布局结果。`lines` 由 allocator 分配，调用方负责 `deinit`。
pub const TextLayout = struct {
    allocator: std.mem.Allocator,
    lines: []Line,
    size: Size,
    /// 是否发生了截断（clip / ellipsis 截掉内容，或 wrap 超过 max_lines）。
    truncated: bool,
    /// scale 策略实际采用的字号（其它策略等于原字号）。
    used_font_size: f32,

    pub fn deinit(self: *TextLayout) void {
        self.allocator.free(self.lines);
        self.* = undefined;
    }
};

const ellipsis = "…"; // U+2026
const ellipsis_cp: u21 = 0x2026;

/// 测量并布局文本。返回的 `TextLayout` 需调用方 `deinit`。
pub fn measure(
    allocator: std.mem.Allocator,
    text: []const u8,
    style: TextStyle,
    overflow: Overflow,
    constraints: Constraints,
    metrics: FontMetrics,
) std.mem.Allocator.Error!TextLayout {
    const line_h = effectiveLineHeight(style, metrics);

    return switch (overflow) {
        .clip => singleLine(allocator, text, style, constraints, metrics, line_h, false),
        .ellipsis => singleLine(allocator, text, style, constraints, metrics, line_h, true),
        .wrap => wrapLines(allocator, text, style, constraints, metrics, line_h),
        .scale => scaleToFit(allocator, text, style, constraints, metrics),
    };
}

fn effectiveLineHeight(style: TextStyle, metrics: FontMetrics) f32 {
    if (style.line_height > 0) return style.line_height;
    return metrics.vmetrics(style.font_size).lineHeight();
}

/// 测量一段文本的宽度（不换行）。
fn measureRun(text: []const u8, style: TextStyle, metrics: FontMetrics) f32 {
    var w: f32 = 0;
    var view = std.unicode.Utf8View.initUnchecked(text);
    var it = view.iterator();
    var first = true;
    while (it.nextCodepoint()) |cp| {
        if (!first) w += style.letter_spacing;
        first = false;
        w += metrics.advance(cp, style.font_size);
    }
    return w;
}

// ── clip / ellipsis：单行 ──────────────────────────────────────────────────────

fn singleLine(
    allocator: std.mem.Allocator,
    text: []const u8,
    style: TextStyle,
    constraints: Constraints,
    metrics: FontMetrics,
    line_h: f32,
    use_ellipsis: bool,
) std.mem.Allocator.Error!TextLayout {
    // 只取第一行（遇到换行符即止）。
    const first_seg = firstSegment(text);
    const full_w = measureRun(first_seg, style, metrics);
    const has_more_segs = first_seg.len < text.len;

    const lines = try allocator.alloc(Line, 1);
    errdefer allocator.free(lines);

    const unbounded = constraints.max_width <= 0;
    if (unbounded or full_w <= constraints.max_width) {
        // 放得下整行。若原文本还有后续段（被换行符截断），标记 truncated。
        lines[0] = .{ .text = first_seg, .width = full_w };
        return .{
            .allocator = allocator,
            .lines = lines,
            .size = .{ .width = full_w, .height = line_h },
            .truncated = has_more_segs,
            .used_font_size = style.font_size,
        };
    }

    // 放不下：按字符截断，ellipsis 时为 "…" 预留宽度。
    const reserve: f32 = if (use_ellipsis) metrics.advance(ellipsis_cp, style.font_size) else 0;
    const budget = constraints.max_width - reserve;
    const cut = fitPrefix(first_seg, style, metrics, budget);

    const kept = first_seg[0..cut.byte_len];
    const kept_w = cut.width;
    const final_w = kept_w + reserve;
    lines[0] = .{ .text = kept, .width = final_w, .ellipsized = use_ellipsis };
    return .{
        .allocator = allocator,
        .lines = lines,
        .size = .{ .width = final_w, .height = line_h },
        .truncated = true,
        .used_font_size = style.font_size,
    };
}

/// 取首行（到第一个 '\n' 为止，不含换行符）。
fn firstSegment(text: []const u8) []const u8 {
    if (std.mem.indexOfScalar(u8, text, '\n')) |idx| return text[0..idx];
    return text;
}

const PrefixFit = struct { byte_len: usize, width: f32 };

/// 在 budget 宽度内尽可能多地放入 text 的前缀，返回字节长度与宽度。
fn fitPrefix(text: []const u8, style: TextStyle, metrics: FontMetrics, budget: f32) PrefixFit {
    var w: f32 = 0;
    var byte_len: usize = 0;
    var view = std.unicode.Utf8View.initUnchecked(text);
    var it = view.iterator();
    var first = true;
    while (it.nextCodepointSlice()) |slice| {
        const cp = std.unicode.utf8Decode(slice) catch 0xFFFD;
        const step = (if (first) 0 else style.letter_spacing) + metrics.advance(cp, style.font_size);
        if (w + step > budget) break;
        w += step;
        byte_len += slice.len;
        first = false;
    }
    return .{ .byte_len = byte_len, .width = w };
}

// ── wrap：按可用宽度折行 ────────────────────────────────────────────────────────

fn wrapLines(
    allocator: std.mem.Allocator,
    text: []const u8,
    style: TextStyle,
    constraints: Constraints,
    metrics: FontMetrics,
    line_h: f32,
) std.mem.Allocator.Error!TextLayout {
    // 无界宽度退化为单行（按段切）。
    if (constraints.max_width <= 0) {
        return singleLine(allocator, firstSegment(text), style, constraints, metrics, line_h, false);
    }

    var lines = std.ArrayList(Line).empty;
    defer lines.deinit(allocator);

    var truncated = false;
    var max_w: f32 = 0;

    // 先按显式 '\n' 分段，每段再按宽度折行。
    var seg_it = std.mem.splitScalar(u8, text, '\n');
    outer: while (seg_it.next()) |segment| {
        var rest = segment;
        // 空段也算一行（保留空行）。
        if (rest.len == 0) {
            try appendLine(&lines, allocator, rest, 0, false, &max_w);
            if (reachedMax(lines.items.len, constraints.max_lines)) {
                truncated = hasMoreText(&seg_it, rest);
                break :outer;
            }
            continue;
        }
        while (rest.len > 0) {
            const is_last_allowed = constraints.max_lines != 0 and lines.items.len + 1 == constraints.max_lines;
            const cut = fitPrefix(rest, style, metrics, constraints.max_width);

            if (cut.byte_len == rest.len) {
                // 剩余全部放得下
                if (is_last_allowed) {
                    // 是最后允许行：检查后面是否还有内容
                    // （本段已放完，但可能还有后续段）
                }
                try appendLine(&lines, allocator, rest, cut.width, false, &max_w);
                rest = rest[rest.len..];
            } else {
                // 放不下，需要在 cut 处折行（至少放一个字符避免死循环）
                var split_len = cut.byte_len;
                if (split_len == 0) split_len = firstCodepointLen(rest);

                if (is_last_allowed) {
                    // 最后允许行：用 ellipsis 收尾，丢弃剩余
                    const reserve = metrics.advance(ellipsis_cp, style.font_size);
                    const budget = constraints.max_width - reserve;
                    const ef = fitPrefix(rest, style, metrics, budget);
                    const keep_len = if (ef.byte_len == 0) firstCodepointLen(rest) else ef.byte_len;
                    try appendLine(&lines, allocator, rest[0..keep_len], ef.width + reserve, true, &max_w);
                    truncated = true;
                    break :outer;
                }
                try appendLine(&lines, allocator, rest[0..split_len], measureRun(rest[0..split_len], style, metrics), false, &max_w);
                rest = rest[split_len..];
            }

            if (reachedMax(lines.items.len, constraints.max_lines) and rest.len > 0) {
                truncated = true;
                break :outer;
            }
        }
        if (reachedMax(lines.items.len, constraints.max_lines)) {
            if (hasMoreText(&seg_it, "")) truncated = true;
            break :outer;
        }
    }

    const owned = try lines.toOwnedSlice(allocator);
    const total_h = @as(f32, @floatFromInt(owned.len)) * line_h;
    return .{
        .allocator = allocator,
        .lines = owned,
        .size = .{ .width = max_w, .height = total_h },
        .truncated = truncated,
        .used_font_size = style.font_size,
    };
}

fn appendLine(
    lines: *std.ArrayList(Line),
    allocator: std.mem.Allocator,
    text: []const u8,
    width: f32,
    ellipsized: bool,
    max_w: *f32,
) std.mem.Allocator.Error!void {
    try lines.append(allocator, .{ .text = text, .width = width, .ellipsized = ellipsized });
    if (width > max_w.*) max_w.* = width;
}

fn reachedMax(count: usize, max_lines: usize) bool {
    return max_lines != 0 and count >= max_lines;
}

fn hasMoreText(it: *std.mem.SplitIterator(u8, .scalar), current_rest: []const u8) bool {
    if (current_rest.len > 0) return true;
    return it.peek() != null;
}

fn firstCodepointLen(text: []const u8) usize {
    if (text.len == 0) return 0;
    return std.unicode.utf8ByteSequenceLength(text[0]) catch 1;
}

// ── scale：缩小字号直到放进单行约束 ────────────────────────────────────────────

fn scaleToFit(
    allocator: std.mem.Allocator,
    text: []const u8,
    style: TextStyle,
    constraints: Constraints,
    metrics: FontMetrics,
) std.mem.Allocator.Error!TextLayout {
    const seg = firstSegment(text);
    var size = style.font_size;
    const min_size: f32 = 1.0;

    if (constraints.max_width > 0) {
        // 二分 / 线性收缩：按宽度比例估算后再校正。
        const full_w = measureRun(seg, style, metrics);
        if (full_w > constraints.max_width and full_w > 0) {
            size = @max(min_size, style.font_size * (constraints.max_width / full_w));
            // 因 advance 未必严格线性，做几步收缩校正。
            var guard: u8 = 0;
            while (guard < 8) : (guard += 1) {
                var scaled = style;
                scaled.font_size = size;
                if (measureRun(seg, scaled, metrics) <= constraints.max_width) break;
                size = @max(min_size, size * 0.95);
            }
        }
    }

    var scaled_style = style;
    scaled_style.font_size = size;
    const w = measureRun(seg, scaled_style, metrics);
    const line_h = effectiveLineHeight(scaled_style, metrics);

    const lines = try allocator.alloc(Line, 1);
    lines[0] = .{ .text = seg, .width = w };
    return .{
        .allocator = allocator,
        .lines = lines,
        .size = .{ .width = w, .height = line_h },
        .truncated = seg.len < text.len,
        .used_font_size = size,
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const testing = std.testing;

fn testMetrics() font.MonospaceMetrics {
    return .{}; // advance_ratio 0.6
}

test "clip：放得下时单行完整" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 }; // 每字符 6px
    var layout = try measure(testing.allocator, "hello", style, .clip, .{ .max_width = 100 }, fm);
    defer layout.deinit();
    try testing.expectEqual(@as(usize, 1), layout.lines.len);
    try testing.expect(!layout.truncated);
    try testing.expectApproxEqAbs(@as(f32, 30), layout.size.width, 1e-4); // 5*6
}

test "clip：超宽硬截断且不放省略号" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 }; // 6px/字符
    // 可用 20px → 放得下 3 个字符
    var layout = try measure(testing.allocator, "hello", style, .clip, .{ .max_width = 20 }, fm);
    defer layout.deinit();
    try testing.expect(layout.truncated);
    try testing.expect(!layout.lines[0].ellipsized);
    try testing.expectEqualStrings("hel", layout.lines[0].text);
}

test "ellipsis：超宽时末尾省略号（默认策略）" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 }; // 6px/字符
    // 可用 30px，"…" 占 6px → 预算 24px → 4 字符
    var layout = try measure(testing.allocator, "hello world", style, .ellipsis, .{ .max_width = 30 }, fm);
    defer layout.deinit();
    try testing.expect(layout.truncated);
    try testing.expect(layout.lines[0].ellipsized);
    try testing.expectEqualStrings("hell", layout.lines[0].text);
    // 宽度含省略号
    try testing.expectApproxEqAbs(@as(f32, 30), layout.size.width, 1e-4);
}

test "wrap：按宽度折行" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 }; // 6px/字符
    // 12 字符，可用 36px → 每行 6 字符 → 2 行
    var layout = try measure(testing.allocator, "abcdefghijkl", style, .wrap, .{ .max_width = 36 }, fm);
    defer layout.deinit();
    try testing.expectEqual(@as(usize, 2), layout.lines.len);
    try testing.expectEqualStrings("abcdef", layout.lines[0].text);
    try testing.expectEqualStrings("ghijkl", layout.lines[1].text);
    try testing.expect(!layout.truncated);
}

test "wrap：max_lines 限制 + 末行省略号" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 };
    // 18 字符，每行 6 字符，但 max_lines=2 → 第二行省略号收尾
    var layout = try measure(testing.allocator, "abcdefghijklmnopqr", style, .wrap, .{ .max_width = 36, .max_lines = 2 }, fm);
    defer layout.deinit();
    try testing.expectEqual(@as(usize, 2), layout.lines.len);
    try testing.expect(layout.truncated);
    try testing.expect(layout.lines[1].ellipsized);
}

test "wrap：尊重显式换行符" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 };
    var layout = try measure(testing.allocator, "ab\ncd", style, .wrap, .{ .max_width = 100 }, fm);
    defer layout.deinit();
    try testing.expectEqual(@as(usize, 2), layout.lines.len);
    try testing.expectEqualStrings("ab", layout.lines[0].text);
    try testing.expectEqualStrings("cd", layout.lines[1].text);
}

test "scale：缩小字号放进约束" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 20 }; // 每字符 12px，"hello" = 60px
    var layout = try measure(testing.allocator, "hello", style, .scale, .{ .max_width = 30 }, fm);
    defer layout.deinit();
    try testing.expect(layout.used_font_size < 20);
    try testing.expect(layout.size.width <= 30 + 1e-3);
    try testing.expect(!layout.truncated);
}

test "无约束宽度：单行不截断" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 };
    var layout = try measure(testing.allocator, "hello world", style, .ellipsis, .{}, fm);
    defer layout.deinit();
    try testing.expect(!layout.truncated);
    try testing.expectEqual(@as(usize, 1), layout.lines.len);
}

test "CJK 双宽测量" {
    var m = testMetrics();
    const fm = m.interface();
    const style = TextStyle{ .font_size = 10 }; // ASCII 6px，CJK 12px
    var layout = try measure(testing.allocator, "中文", style, .clip, .{ .max_width = 100 }, fm);
    defer layout.deinit();
    try testing.expectApproxEqAbs(@as(f32, 24), layout.size.width, 1e-4); // 2*12
}
