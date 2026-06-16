//! render/backends/software —— 纯 Zig 软件光栅后端
//!
//! `SoftwareBackend` 实现 `render.Backend` 接口，把 `Scene` 的绘制命令直接光栅化到
//! `RenderTarget` 的 ARGB8888 像素缓冲。**无任何外部依赖**，纯 Zig 实现，可独立单元测试。
//!
//! 它与未来的 GPU 后端（Vulkan / wgpu）是平级的 `Backend` 实现 —— 上层只产出 DrawCommand，
//! 切换后端不改一行业务代码。软件光栅器可长期留作 fallback 与测试基准。
//!
//! 能力：
//!   - fill_rect / fill_rounded_rect：实心填充，圆角带简单抗锯齿（按覆盖率混合）。
//!   - push_clip / pop_clip：矩形裁剪栈（求交）。圆角裁剪 v1 先按外接矩形近似。
//!   - draw_text：v1 占位 —— 在布局框内画一条淡色基线条，等真实字体后端接入后替换。
//!   - alpha 混合：源 over 目标（source-over）。
//!
//! 像素格式 ARGB8888：高位到低位为 A,R,G,B（u32 = 0xAARRGGBB）。

const std = @import("std");
const foundation = @import("../../foundation/foundation.zig");
const scene_mod = @import("../scene.zig");
const backend_mod = @import("../backend.zig");

const Color = foundation.Color;
const Rect = foundation.Rect;
const Scene = scene_mod.Scene;
const Backend = backend_mod.Backend;
const GlyphRenderer = backend_mod.GlyphRenderer;
const RenderTarget = backend_mod.RenderTarget;
const BackendError = backend_mod.BackendError;

/// 整数像素矩形（半开区间 [x0, x1) × [y0, y1)），用于裁剪与遍历。
const IRect = struct {
    x0: i32,
    y0: i32,
    x1: i32,
    y1: i32,

    fn intersect(a: IRect, b: IRect) IRect {
        return .{
            .x0 = @max(a.x0, b.x0),
            .y0 = @max(a.y0, b.y0),
            .x1 = @min(a.x1, b.x1),
            .y1 = @min(a.y1, b.y1),
        };
    }

    fn isEmpty(self: IRect) bool {
        return self.x1 <= self.x0 or self.y1 <= self.y0;
    }
};

/// 软件光栅后端。clip 栈深度有上限（足够 UI 嵌套），溢出时退化为不再细化裁剪。
pub const SoftwareBackend = struct {
    target: RenderTarget = .{},
    in_frame: bool = false,
    frame_count: u32 = 0,

    /// 裁剪栈：每层是当前有效裁剪矩形（已与父层求交）。
    clip_stack: [32]IRect = undefined,
    clip_depth: usize = 0,

    /// 可选的字形渲染器。设置后可渲染真实文字 glyph 而非占位符。
    glyph_renderer: ?backend_mod.GlyphRenderer = null,

    pub fn init() SoftwareBackend {
        return .{};
    }

    /// 设置字形渲染器，使文字渲染从占位符升级为真实 glyph。
    pub fn setGlyphRenderer(self: *SoftwareBackend, renderer: backend_mod.GlyphRenderer) void {
        self.glyph_renderer = renderer;
    }

    pub fn interface(self: *SoftwareBackend) Backend {
        return .{ .ptr = self, .vtable = &vtable };
    }

    const vtable = Backend.VTable{
        .begin_frame = beginFrame,
        .submit = submit,
        .end_frame = endFrame,
    };

    // ── 帧生命周期 ──────────────────────────────────────────────────────────────

    fn beginFrame(ptr: *anyopaque, target: RenderTarget) BackendError!void {
        const self: *SoftwareBackend = @ptrCast(@alignCast(ptr));
        self.target = target;
        self.in_frame = true;
        self.clip_depth = 0;
    }

    fn submit(ptr: *anyopaque, scene: *const Scene) BackendError!void {
        const self: *SoftwareBackend = @ptrCast(@alignCast(ptr));
        if (!self.in_frame) return BackendError.NotInFrame;
        for (scene.commands.items) |cmd| {
            switch (cmd) {
                .fill_rect => |c| self.fillRect(c.rect, c.color, 0),
                .fill_rounded_rect => |c| self.fillRect(c.rect, c.color, c.radius),
                .draw_text => |c| self.drawTextPlaceholder(c),
                .push_clip => |c| self.pushClip(c.rect),
                .pop_clip => self.popClip(),
            }
        }
    }

    fn endFrame(ptr: *anyopaque) BackendError!void {
        const self: *SoftwareBackend = @ptrCast(@alignCast(ptr));
        if (!self.in_frame) return BackendError.NotInFrame;
        self.in_frame = false;
        self.frame_count += 1;
    }

    // ── 裁剪栈 ──────────────────────────────────────────────────────────────────

    /// 当前有效裁剪矩形（栈顶；空栈时为整个目标）。
    fn currentClip(self: *const SoftwareBackend) IRect {
        if (self.clip_depth == 0) {
            return .{ .x0 = 0, .y0 = 0, .x1 = @intCast(self.target.width), .y1 = @intCast(self.target.height) };
        }
        return self.clip_stack[self.clip_depth - 1];
    }

    fn pushClip(self: *SoftwareBackend, rect: Rect) void {
        const r = roundRect(rect).intersect(self.currentClip());
        if (self.clip_depth >= self.clip_stack.len) return; // 溢出：忽略更深的裁剪
        self.clip_stack[self.clip_depth] = r;
        self.clip_depth += 1;
    }

    fn popClip(self: *SoftwareBackend) void {
        if (self.clip_depth > 0) self.clip_depth -= 1;
    }

    // ── 光栅化 ──────────────────────────────────────────────────────────────────

    /// 填充矩形（radius>0 时四角做抗锯齿圆角）。
    fn fillRect(self: *SoftwareBackend, rect: Rect, color: Color, radius: f32) void {
        const pixels = self.target.pixels orelse return;
        const stride: usize = if (self.target.stride != 0) self.target.stride else self.target.width;

        // 与当前裁剪求交，得到要遍历的整数像素范围。
        const area = roundRect(rect).intersect(self.currentClip());
        if (area.isEmpty()) return;

        const r = @max(0, @min(radius, @min(rect.width(), rect.height()) / 2));

        var y: i32 = area.y0;
        while (y < area.y1) : (y += 1) {
            var x: i32 = area.x0;
            const row: usize = @as(usize, @intCast(y)) * stride;
            while (x < area.x1) : (x += 1) {
                // 像素中心
                const px = @as(f32, @floatFromInt(x)) + 0.5;
                const py = @as(f32, @floatFromInt(y)) + 0.5;
                const cov = if (r > 0) roundedCoverage(rect, r, px, py) else 1.0;
                if (cov <= 0) continue;
                const idx = row + @as(usize, @intCast(x));
                blend(&pixels[idx], color, cov);
            }
        }
    }

    /// 文本占位：在布局框内画一条淡色基线条（真实字体后端接入后替换）。
    fn drawTextPlaceholder(self: *SoftwareBackend, c: scene_mod.DrawText) void {
        if (c.text.len == 0) return;

        // 若有 glyph renderer，渲染真实文字
        if (self.glyph_renderer) |gr| {
            self.drawTextReal(gr, c);
            return;
        }

        // 降级：占位符绘制
        const text_w = if (c.layout_width > 0) c.layout_width else @as(f32, @floatFromInt(c.text.len)) * c.font_size * 0.6;
        const text_h = if (c.layout_height > 0) c.layout_height else c.font_size * 1.2;

        var bg = c.color;
        bg.a *= 0.08;
        self.fillRect(Rect.fromXywh(c.x, c.y, text_w, text_h), bg, 0);

        const baseline_y = c.y + text_h * 0.7;
        const line_h = c.font_size * 0.15;
        const gap = line_h * 1.2;
        const num_lines = @min(3, @as(u32, @intCast(@max(1, c.text.len / 3))));
        var faded = c.color;
        faded.a = @min(255, faded.a * 2);

        var line_i: u32 = 0;
        while (line_i < num_lines) : (line_i += 1) {
            const line_w = text_w * (1.0 - @as(f32, @floatFromInt(line_i)) * 0.15);
            const y = baseline_y + @as(f32, @floatFromInt(line_i)) * gap;
            self.fillRect(Rect.fromXywh(c.x, y, line_w, line_h), faded, 0);
        }
    }

    /// 用 glyph renderer 渲染真实文字。
    fn drawTextReal(self: *SoftwareBackend, gr: GlyphRenderer, c: scene_mod.DrawText) void {
        const vm = gr.vmetrics(c.font_size);
        const baseline_y = c.y + vm.ascent;

        // 将颜色转为 ARGB8888 u32
        const col = c.color.toHexRgba();

        // 逐码点渲染
        var view = std.unicode.Utf8View.initUnchecked(c.text);
        var it = view.iterator();
        var cursor_x: f32 = c.x;

        while (it.nextCodepoint()) |cp| {
            if (cp == '\n') {
                // 换行（简单处理）
                cursor_x = c.x;
                continue;
            }

            const adv = gr.advance(cp, c.font_size);
            const ix: i32 = @intFromFloat(cursor_x);
            const iy: i32 = @intFromFloat(baseline_y);

            _ = gr.renderCodepoint(cp, c.font_size, col, ix, iy, self.target.pixels orelse return, self.target.width, self.target.height, if (self.target.stride > 0) self.target.stride else self.target.width);

            cursor_x += adv;
        }
    }
};

// ── 工具函数 ────────────────────────────────────────────────────────────────────

/// 浮点 Rect → 整数像素半开区间（向外取整到覆盖的像素）。
fn roundRect(rect: Rect) IRect {
    return .{
        .x0 = @intFromFloat(@floor(rect.left)),
        .y0 = @intFromFloat(@floor(rect.top)),
        .x1 = @intFromFloat(@ceil(rect.right)),
        .y1 = @intFromFloat(@ceil(rect.bottom)),
    };
}

/// 圆角矩形在点 (px, py) 的覆盖率 [0,1]。仅四角做距离抗锯齿，边内恒为 1。
fn roundedCoverage(rect: Rect, r: f32, px: f32, py: f32) f32 {
    // 把点夹到内矩形（四角圆心所在的矩形）。若点落在内矩形内，dx=dy=0。
    const cx = std.math.clamp(px, rect.left + r, rect.right - r);
    const cy = std.math.clamp(py, rect.top + r, rect.bottom - r);
    const dx = px - cx;
    const dy = py - cy;
    if (dx == 0 and dy == 0) return 1.0; // 不在角区
    const dist = @sqrt(dx * dx + dy * dy);
    // 距离圆心 r 处覆盖率从 1 线性降到 0（1px 过渡带做抗锯齿）。
    return std.math.clamp(r - dist + 0.5, 0.0, 1.0);
}

/// 把 Color（0..1）以覆盖率 cov 混合（source-over）到目标像素 *dst（ARGB8888）。
fn blend(dst: *u32, c: Color, cov: f32) void {
    const sa = std.math.clamp(c.a, 0.0, 1.0) * std.math.clamp(cov, 0.0, 1.0);
    if (sa <= 0) return;

    const sr = std.math.clamp(c.r, 0.0, 1.0);
    const sg = std.math.clamp(c.g, 0.0, 1.0);
    const sb = std.math.clamp(c.b, 0.0, 1.0);

    const old = dst.*;
    const da: f32 = @as(f32, @floatFromInt((old >> 24) & 0xFF)) / 255.0;
    const dr: f32 = @as(f32, @floatFromInt((old >> 16) & 0xFF)) / 255.0;
    const dg: f32 = @as(f32, @floatFromInt((old >> 8) & 0xFF)) / 255.0;
    const db: f32 = @as(f32, @floatFromInt(old & 0xFF)) / 255.0;

    // source-over（预乘空间计算后再解预乘）：out = src + dst * (1 - sa)
    const oa = sa + da * (1 - sa);
    const orr = sr * sa + dr * da * (1 - sa);
    const og = sg * sa + dg * da * (1 - sa);
    const ob = sb * sa + db * da * (1 - sa);

    const inv: f32 = if (oa > 0) 1.0 / oa else 0;
    dst.* = pack(orr * inv, og * inv, ob * inv, oa);
}

fn to8(v: f32) u32 {
    return @intFromFloat(@round(std.math.clamp(v, 0.0, 1.0) * 255.0));
}

fn pack(r: f32, g: f32, b: f32, a: f32) u32 {
    return (to8(a) << 24) | (to8(r) << 16) | (to8(g) << 8) | to8(b);
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const testing = std.testing;

/// 测试用：一块固定尺寸的 ARGB 像素缓冲。
fn TestSurface(comptime w: u32, comptime h: u32) type {
    return struct {
        pixels: [w * h]u32 = [_]u32{0} ** (w * h),

        fn target(self: *@This()) RenderTarget {
            return .{ .pixels = &self.pixels, .width = w, .height = h, .stride = w };
        }
        fn at(self: *const @This(), x: u32, y: u32) u32 {
            return self.pixels[y * w + x];
        }
    };
}

fn argb(a: u8, r: u8, g: u8, b: u8) u32 {
    return (@as(u32, a) << 24) | (@as(u32, r) << 16) | (@as(u32, g) << 8) | b;
}

test "fillRect 填充不透明矩形" {
    var surf = TestSurface(10, 10){};
    var be = SoftwareBackend.init();
    const backend = be.interface();

    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try scene.fillRect(Rect.fromXywh(2, 2, 4, 4), Color.white);

    try backend.beginFrame(surf.target());
    try backend.submit(&scene);
    try backend.endFrame();

    // (3,3) 在矩形内 → 白色不透明
    try testing.expectEqual(argb(255, 255, 255, 255), surf.at(3, 3));
    // (0,0) 在矩形外 → 保持透明
    try testing.expectEqual(@as(u32, 0), surf.at(0, 0));
    // 边界外一格 (6,6) 不应被填充（矩形是 [2,6)）
    try testing.expectEqual(@as(u32, 0), surf.at(6, 6));
}

test "alpha 混合：半透明白叠加在黑底上得到灰" {
    var surf = TestSurface(4, 4){};
    for (&surf.pixels) |*p| p.* = argb(255, 0, 0, 0); // 不透明黑底

    var be = SoftwareBackend.init();
    const backend = be.interface();
    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try scene.fillRect(Rect.fromXywh(0, 0, 4, 4), Color.rgba(1, 1, 1, 0.5));

    try backend.beginFrame(surf.target());
    try backend.submit(&scene);
    try backend.endFrame();

    const px = surf.at(1, 1);
    const r: u32 = (px >> 16) & 0xFF;
    try testing.expect(r >= 126 and r <= 130); // 0.5 白 over 黑 ≈ 128
    try testing.expectEqual(@as(u32, 255), (px >> 24) & 0xFF); // 不透明
}

test "裁剪：push_clip 限制绘制范围" {
    var surf = TestSurface(10, 10){};
    var be = SoftwareBackend.init();
    const backend = be.interface();

    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try scene.pushClip(Rect.fromXywh(0, 0, 4, 4), 0);
    try scene.fillRect(Rect.fromXywh(0, 0, 10, 10), Color.white);
    try scene.popClip();

    try backend.beginFrame(surf.target());
    try backend.submit(&scene);
    try backend.endFrame();

    try testing.expectEqual(argb(255, 255, 255, 255), surf.at(2, 2)); // 裁剪区内
    try testing.expectEqual(@as(u32, 0), surf.at(5, 5)); // 裁剪区外
}

test "嵌套裁剪求交" {
    var surf = TestSurface(10, 10){};
    var be = SoftwareBackend.init();
    const backend = be.interface();

    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try scene.pushClip(Rect.fromXywh(0, 0, 6, 6), 0);
    try scene.pushClip(Rect.fromXywh(3, 3, 6, 6), 0);
    try scene.fillRect(Rect.fromXywh(0, 0, 10, 10), Color.white);
    try scene.popClip();
    try scene.popClip();

    try backend.beginFrame(surf.target());
    try backend.submit(&scene);
    try backend.endFrame();

    try testing.expectEqual(argb(255, 255, 255, 255), surf.at(4, 4)); // 交集内
    try testing.expectEqual(@as(u32, 0), surf.at(1, 1)); // 被内层裁掉
    try testing.expectEqual(@as(u32, 0), surf.at(7, 7)); // 被外层裁掉
}

test "圆角：角外像素覆盖率低于角内" {
    var surf = TestSurface(20, 20){};
    var be = SoftwareBackend.init();
    const backend = be.interface();

    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try scene.fillRoundedRect(Rect.fromXywh(0, 0, 20, 20), 6, Color.white);

    try backend.beginFrame(surf.target());
    try backend.submit(&scene);
    try backend.endFrame();

    // 左上角最角落 (0,0) 被圆角裁掉（半透明或更低）
    try testing.expect(((surf.at(0, 0) >> 24) & 0xFF) < 128);
    // 中心 (10,10) 实心
    try testing.expectEqual(argb(255, 255, 255, 255), surf.at(10, 10));
}

test "帧外 submit 报错" {
    var be = SoftwareBackend.init();
    const backend = be.interface();
    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try testing.expectError(BackendError.NotInFrame, backend.submit(&scene));
}

test "无像素缓冲（无头目标）不崩溃" {
    var be = SoftwareBackend.init();
    const backend = be.interface();
    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try scene.fillRect(Rect.fromXywh(0, 0, 10, 10), Color.white);

    try backend.beginFrame(.{ .width = 100, .height = 100 }); // pixels = null
    try backend.submit(&scene); // 应安全跳过
    try backend.endFrame();
    try testing.expectEqual(@as(u32, 1), be.frame_count);
}

test "draw_text 占位绘制半透明条" {
    var surf = TestSurface(40, 20){};
    var be = SoftwareBackend.init();
    const backend = be.interface();
    var scene = Scene.init(testing.allocator);
    defer scene.deinit();
    try scene.drawText(.{ .text = "hi", .x = 0, .y = 0, .font_size = 10, .color = Color.white, .layout_width = 20 });

    try backend.beginFrame(surf.target());
    try backend.submit(&scene);
    try backend.endFrame();

    // 基线条应有半透明白像素
    var found = false;
    var y: u32 = 0;
    while (y < 20) : (y += 1) {
        const av = (surf.at(1, y) >> 24) & 0xFF;
        if (av > 0 and av < 255) found = true;
    }
    try testing.expect(found);
}
