//! text/backends/hb_ft —— HarfBuzz + FreeType 字体后端
//!
//! 替换 `MonospaceMetrics` 占位，使用系统 FreeType 提供真实字体度量。
//! 当前实现只使用 FreeType 做字体加载与度量；后续可扩展 HarfBuzz shaping。
//!
//! 生命周期：
//!   1. `HarfBuzzFreeTypeMetrics.init(allocator)` 加载默认系统字体。
//!   2. 通过 `interface()` 暴露 `FontMetrics` vtable 供 text 层消费。
//!   3. `deinit()` 释放字体资源。
//!
//! 依赖：系统安装 FreeType >= 2.13（pkg-config: freetype2）。
//!       可选 HarfBuzz >= 7.0（后续 shaping 用）。

const std = @import("std");
const font = @import("../font.zig");
const render = @import("../../render/render.zig");

const FontMetrics = font.FontMetrics;
const VMetrics = font.VMetrics;
const GlyphRenderer = render.GlyphRenderer;
const GlyphVMetrics = render.GlyphVMetrics;

const c = @cImport({
    @cInclude("ft2build.h");
    @cInclude("freetype/freetype.h");
});

// 注意：ft_glyph.c 通过 build.zig 的 addCSourceFile 编译，不在此处 include。

// C 包装器函数声明（实现在 ft_glyph.c，通过 build.zig 的 addCSourceFile 编译）
const NandinaFTGlyphResult = extern struct {
    width: c_int,
    rows: c_int,
    pitch: c_int,
    buffer: ?*u8,
    left: c_int,
    top: c_int,
    advance_x: c_int,
};

const NandinaFTFaceMetrics = extern struct {
    units_per_EM: c_int,
    ascender: c_int,
    descender: c_int,
    height: c_int,
};

extern "c" fn nandina_ft_render_glyph(face: *c.FT_FaceRec_, glyph_index: c_uint, pixel_size: c_int) NandinaFTGlyphResult;
extern "c" fn nandina_ft_face_metrics(face: *c.FT_FaceRec_) NandinaFTFaceMetrics;

/// FreeType 字体度量后端。
pub const HarfBuzzFreeTypeMetrics = struct {
    allocator: std.mem.Allocator,
    library: c.FT_Library,
    face: c.FT_Face,

    pub fn init(allocator: std.mem.Allocator) !HarfBuzzFreeTypeMetrics {
        var library: c.FT_Library = undefined;
        if (c.FT_Init_FreeType(&library) != 0) {
            return error.FreeTypeInitFailed;
        }

        // 查找系统默认字体
        const font_path = findSystemFont(allocator) catch |err| {
            _ = c.FT_Done_FreeType(library);
            return err;
        };
        defer allocator.free(font_path);

        var face: c.FT_Face = undefined;
        if (c.FT_New_Face(library, font_path.ptr, 0, &face) != 0) {
            _ = c.FT_Done_FreeType(library);
            return error.FreeTypeLoadFontFailed;
        }

        return HarfBuzzFreeTypeMetrics{
            .allocator = allocator,
            .library = library,
            .face = face,
        };
    }

    pub fn deinit(self: *HarfBuzzFreeTypeMetrics) void {
        _ = c.FT_Done_Face(self.face);
        _ = c.FT_Done_FreeType(self.library);
        self.* = undefined;
    }

    /// 暴露字体度量接口。
    pub fn interface(self: *HarfBuzzFreeTypeMetrics) FontMetrics {
        return .{ .ptr = self, .vtable = &vtable };
    }

    /// 创建字形渲染器，供 SoftwareBackend 等渲染后端消费。
    /// 返回的 GlyphRenderer 引用 self，调用方须保证 self 生命周期覆盖渲染器。
    pub fn glyphRenderer(self: *HarfBuzzFreeTypeMetrics) render.GlyphRenderer {
        return .{ .ptr = self, .vtable = &glyph_vtable };
    }

    const vtable = FontMetrics.VTable{
        .advance = advanceImpl,
        .vmetrics = vmetricsImpl,
    };

    fn advanceImpl(ptr: *anyopaque, codepoint: u21, font_size: f32) f32 {
        const self: *HarfBuzzFreeTypeMetrics = @ptrCast(@alignCast(ptr));
        const glyph_idx = c.FT_Get_Char_Index(self.face, codepoint);
        if (glyph_idx == 0) return font_size * 0.5;
        const g = nandina_ft_render_glyph(self.face, glyph_idx, @intFromFloat(font_size));
        if (g.advance_x > 0) return @floatFromInt(g.advance_x);
        return font_size * 0.5;
    }

    fn vmetricsImpl(ptr: *anyopaque, font_size: f32) VMetrics {
        const self: *HarfBuzzFreeTypeMetrics = @ptrCast(@alignCast(ptr));
        const m = nandina_ft_face_metrics(self.face);
        const upem = @as(f32, @floatFromInt(m.units_per_EM));
        if (upem == 0) return .{ .ascent = font_size * 0.8, .descent = font_size * 0.2, .line_gap = 0 };
        return .{
            .ascent = @as(f32, @floatFromInt(m.ascender)) * font_size / upem,
            .descent = -@as(f32, @floatFromInt(m.descender)) * font_size / upem,
            .line_gap = if (m.height > 0 and m.ascender > 0)
                @max(0, @as(f32, @floatFromInt(m.height - m.ascender + m.descender)) * font_size / upem)
            else
                0,
        };
    }

    // ── Glyph 渲染（供 SoftwareBackend 的真实文字绘制） ─────────────────────

    fn renderCodepointImpl(ptr: *anyopaque, codepoint: u21, font_size: f32, color: u32, x: i32, y: i32, pixels: [*]u32, width: u32, height: u32, stride: u32) i32 {
        const self: *HarfBuzzFreeTypeMetrics = @ptrCast(@alignCast(ptr));

        // 查找字形索引
        const glyph_idx = c.FT_Get_Char_Index(self.face, codepoint);
        if (glyph_idx == 0) return @intFromFloat(font_size * 0.5);

        // 用 C 包装函数渲染 glyph 并获取 bitmap 数据
        const g = nandina_ft_render_glyph(self.face, glyph_idx, @intFromFloat(font_size));

        if (g.width <= 0 or g.rows <= 0 or g.buffer == null) return @intFromFloat(font_size * 0.5);

        const dst_x = x + g.left;
        const dst_y = y - g.top;

        const col_r = @as(u32, (color >> 16) & 0xFF);
        const col_g = @as(u32, (color >> 8) & 0xFF);
        const col_b = @as(u32, color & 0xFF);
        const col_a = @as(u32, (color >> 24) & 0xFF);

        var row: i32 = 0;
        while (row < g.rows) : (row += 1) {
            const py = dst_y + row;
            if (py < 0 or py >= @as(i32, @intCast(height))) continue;

            const src_row = @as([*]u8, @ptrCast(g.buffer.?)) + @as(usize, @intCast(row * g.pitch));
            var col: i32 = 0;
            while (col < g.width) : (col += 1) {
                const px = dst_x + col;
                if (px < 0 or px >= @as(i32, @intCast(width))) continue;

                // 灰度值 [0, 255]
                const gray = src_row[@intCast(col)];
                if (gray == 0) continue;

                // Alpha 混合：source-over
                const src_alpha = col_a * @as(u32, gray) / 255;
                if (src_alpha == 0) continue;

                const idx = @as(usize, @intCast(py)) * stride + @as(usize, @intCast(px));
                const dst_val = pixels[idx];
                const dst_a = (dst_val >> 24) & 0xFF;
                const dst_r = (dst_val >> 16) & 0xFF;
                const dst_g = (dst_val >> 8) & 0xFF;
                const dst_b = dst_val & 0xFF;

                const out_a = src_alpha + dst_a * (255 - src_alpha) / 255;
                const out_r = if (out_a > 0)
                    (col_r * src_alpha + dst_r * dst_a * (255 - src_alpha) / 255) / out_a
                else
                    0;
                const out_g = if (out_a > 0)
                    (col_g * src_alpha + dst_g * dst_a * (255 - src_alpha) / 255) / out_a
                else
                    0;
                const out_b = if (out_a > 0)
                    (col_b * src_alpha + dst_b * dst_a * (255 - src_alpha) / 255) / out_a
                else
                    0;

                pixels[idx] = (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
            }
        }

        return g.advance_x;
    }

    fn glyphAdvanceImpl(ptr: *anyopaque, codepoint: u21, font_size: f32) f32 {
        const self: *HarfBuzzFreeTypeMetrics = @ptrCast(@alignCast(ptr));
        const glyph_idx = c.FT_Get_Char_Index(self.face, codepoint);
        if (glyph_idx == 0) return font_size * 0.5;
        const g = nandina_ft_render_glyph(self.face, glyph_idx, @intFromFloat(font_size));
        if (g.advance_x > 0) return @floatFromInt(g.advance_x);
        return font_size * 0.5;
    }

    fn glyphVMetricsImpl(ptr: *anyopaque, font_size: f32) render.GlyphVMetrics {
        const self: *HarfBuzzFreeTypeMetrics = @ptrCast(@alignCast(ptr));
        const m = nandina_ft_face_metrics(self.face);
        const upem = @as(f32, @floatFromInt(m.units_per_EM));
        if (upem == 0) return .{ .ascent = font_size * 0.8, .descent = font_size * 0.2, .line_gap = 0 };
        return .{
            .ascent = @as(f32, @floatFromInt(m.ascender)) * font_size / upem,
            .descent = -@as(f32, @floatFromInt(m.descender)) * font_size / upem,
            .line_gap = @max(0, @as(f32, @floatFromInt(m.height - m.ascender + m.descender)) * font_size / upem),
        };
    }

    const glyph_vtable = render.GlyphRenderer.VTable{
        .render_codepoint = renderCodepointImpl,
        .advance = glyphAdvanceImpl,
        .vmetrics = glyphVMetricsImpl,
    };
};

/// 查找系统可用字体文件路径。
/// 搜索顺序：环境变量 NANDINA_FONT → 常用系统字体路径。
/// 搜索顺序：环境变量 NANDINA_FONT → 常用系统字体路径。
fn findSystemFont(allocator: std.mem.Allocator) ![]u8 {
    // 1. 环境变量
    if (std.c.getenv("NANDINA_FONT")) |path| {
        const len = std.mem.len(path);
        return try allocator.dupe(u8, path[0..len]);
    }

    // 2. 常用系统字体路径
    const candidates = [_][]const u8{
        "/usr/share/fonts/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        // 优先级最低：NixOS / 通用
        "/run/current-system/sw/share/fonts/noto/NotoSans-Regular.ttf",
    };
    for (candidates) |path| {
        // 尝试用 C fopen 检测文件存在（绕过 Zig fs API 差异）
        const c_path = try allocator.dupe(u8, path);
        defer allocator.free(c_path);
        if (std.c.fopen(@ptrCast(c_path.ptr), "r")) |f| {
            _ = std.c.fclose(f);
            return try allocator.dupe(u8, path);
        }
    }

    // 3. 用 fontconfig 查询（如果可用）
    if (try findFontViaFontconfig(allocator)) |path| return path;

    return error.NoSystemFontFound;
}

/// 尝试用 fontconfig 查找字体（通过运行时 dlopen，避免编译期依赖）。
fn findFontViaFontconfig(allocator: std.mem.Allocator) !?[]u8 {
    // 当前阶段暂不依赖 fontconfig 编译期头文件，回退 null
    _ = allocator;
    return null;
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "HarfBuzzFreeTypeMetrics 初始化与基本度量" {
    var metrics = HarfBuzzFreeTypeMetrics.init(std.testing.allocator) catch |err| {
        // 若无系统字体，跳过测试
        if (err == error.NoSystemFontFound) return error.SkipZigTest;
        return err;
    };
    defer metrics.deinit();

    const fm = metrics.interface();
    const advance_a = fm.advance('A', 16);
    try std.testing.expect(advance_a > 0);
    try std.testing.expect(advance_a < 32); // 合理的 advance 范围

    const vm = fm.vmetrics(16);
    try std.testing.expect(vm.ascent > 0);
    try std.testing.expect(vm.descent >= 0);
    try std.testing.expect(vm.lineHeight() > 0);
}
