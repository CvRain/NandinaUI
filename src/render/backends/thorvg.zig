//! render/backends/thorvg —— ThorVG 矢量渲染后端
//!
//! 通过 ThorVG C API（`thorvg_capi.h`）实现 `render.Backend` vtable。
//! ThorVG 提供专业的矢量渲染（抗锯齿、描边、渐变、transform），
//! 替代纯 Zig 软件光栅作为默认后端。
//!
//! 像素格式 ARGB8888（高位到低位 A,R,G,B），与 ThorVG `TVG_COLORSPACE_ARGB8888` 对齐。

const std = @import("std");
const foundation = @import("../../foundation/foundation.zig");
const scene_mod = @import("../scene.zig");
const backend_mod = @import("../backend.zig");

const Color = foundation.Color;
const Rect = foundation.Rect;
const Scene = scene_mod.Scene;
const Backend = backend_mod.Backend;
const RenderTarget = backend_mod.RenderTarget;
const BackendError = backend_mod.BackendError;

// ── ThorVG C API ──────────────────────────────────────────────────────────────

const c = @cImport({
    @cDefine("TVG_STATIC", {});
    @cInclude("thorvg-1/thorvg_capi.h");
});

const Tvg_Canvas = c.Tvg_Canvas;
const Tvg_Paint = c.Tvg_Paint;

// ThorVG C 枚举值（C 中 typedef 为 unsigned int）
const TVG_RESULT_SUCCESS: c_uint = 0;
const TVG_ENGINE_OPTION_DEFAULT: c_uint = 1;
const TVG_COLORSPACE_ARGB8888: c_uint = 1;

/// 检查 ThorVG 操作结果，失败时返回错误。
fn check(result: c_uint) BackendError!void {
    if (result != TVG_RESULT_SUCCESS) {
        return BackendError.OutOfMemory;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ThorvgBackend
// ─────────────────────────────────────────────────────────────────────────────

/// ThorVG 渲染后端。
pub const ThorvgBackend = struct {
    canvas: ?Tvg_Canvas = null,
    target: RenderTarget = .{},
    in_frame: bool = false,
    frame_count: u32 = 0,
    allocator: std.mem.Allocator = std.heap.c_allocator,

    /// 裁剪栈：每层对应一个 clip shape。
    clip_paints: std.ArrayList(Tvg_Paint) = .empty,

    pub fn init(allocator: std.mem.Allocator) ThorvgBackend {
        return .{
            .allocator = allocator,
            // clip_paints 使用默认值 .empty
        };
    }

    pub fn deinit(self: *ThorvgBackend) void {
        if (self.canvas) |cvs| {
            _ = c.tvg_canvas_destroy(cvs);
        }
        self.clip_paints.deinit(self.allocator);
        self.* = undefined;
    }

    /// ThorVG 后端暂不支持 glyph renderer 接口（ThorVG 有内置字体渲染）。
    pub fn setGlyphRenderer(_: *ThorvgBackend, _: backend_mod.GlyphRenderer) void {}

    pub fn interface(self: *ThorvgBackend) Backend {
        return .{ .ptr = self, .vtable = &vtable };
    }

    const vtable = Backend.VTable{
        .begin_frame = beginFrame,
        .submit = submit,
        .end_frame = endFrame,
    };

    // ── 帧生命周期 ──────────────────────────────────────────────────────────

    fn beginFrame(ptr: *anyopaque, target: RenderTarget) BackendError!void {
        const self: *ThorvgBackend = @ptrCast(@alignCast(ptr));
        self.target = target;
        self.in_frame = true;

        // 若已有 canvas，先销毁
        if (self.canvas) |cvs| {
            _ = c.tvg_canvas_destroy(cvs);
            self.canvas = null;
        }

        // 创建软件 canvas
        const canvas = c.tvg_swcanvas_create(TVG_ENGINE_OPTION_DEFAULT);
        if (canvas == null) return BackendError.OutOfMemory;

        const buffer = target.pixels orelse return error.NotInFrame;
        const stride: u32 = if (target.stride > 0) target.stride else target.width;

        try check(c.tvg_swcanvas_set_target(
            canvas,
            buffer,
            stride,
            target.width,
            target.height,
            TVG_COLORSPACE_ARGB8888,
        ));

        // 重置裁剪栈
        self.clip_paints.clearRetainingCapacity();
        self.canvas = canvas;
    }

    fn submit(ptr: *anyopaque, scene: *const Scene) BackendError!void {
        const self: *ThorvgBackend = @ptrCast(@alignCast(ptr));
        if (!self.in_frame) return BackendError.NotInFrame;
        const canvas = self.canvas orelse return;

        for (scene.commands.items) |cmd| {
            switch (cmd) {
                .fill_rect => |c_data| try addFillRect(self, canvas, c_data.rect, c_data.color, 0),
                .fill_rounded_rect => |c_data| try addFillRect(self, canvas, c_data.rect, c_data.color, c_data.radius),
                .draw_text => {},
                .push_clip => |c_data| try pushClip(self, canvas, c_data.rect, c_data.radius),
                .pop_clip => popClip(self),
            }
        }
    }

    fn endFrame(ptr: *anyopaque) BackendError!void {
        const self: *ThorvgBackend = @ptrCast(@alignCast(ptr));
        if (!self.in_frame) return BackendError.NotInFrame;
        const canvas = self.canvas orelse return;

        // 更新 canvas
        try check(c.tvg_canvas_update(canvas));

        // 绘制并自动清除所有 paints（clear=true）
        try check(c.tvg_canvas_draw(canvas, true));

        // 同步
        try check(c.tvg_canvas_sync(canvas));

        self.in_frame = false;
        self.frame_count += 1;
    }

    // ── 绘制辅助 ────────────────────────────────────────────────────────────

    fn addFillRect(self: *ThorvgBackend, canvas: Tvg_Canvas, rect: Rect, color: Color, radius: f32) !void {
        const shape = c.tvg_shape_new();
        if (shape == null) return BackendError.OutOfMemory;

        _ = c.tvg_shape_append_rect(
            shape,
            rect.left,
            rect.top,
            rect.width(),
            rect.height(),
            radius,
            radius,
            false,
        );

        _ = c.tvg_shape_set_fill_color(
            shape,
            @intFromFloat(color.r * 255.0),
            @intFromFloat(color.g * 255.0),
            @intFromFloat(color.b * 255.0),
            @intFromFloat(color.a * 255.0),
        );

        // 应用当前裁剪（若有）
        if (self.clip_paints.items.len > 0) {
            const top_clip = self.clip_paints.items[self.clip_paints.items.len - 1];
            _ = c.tvg_paint_set_clip(shape, top_clip);
        }

        _ = c.tvg_canvas_add(canvas, shape);
    }

    fn pushClip(self: *ThorvgBackend, canvas: Tvg_Canvas, rect: Rect, radius: f32) !void {
        const clip_shape = c.tvg_shape_new();
        if (clip_shape == null) return BackendError.OutOfMemory;

        _ = c.tvg_shape_append_rect(
            clip_shape,
            rect.left,
            rect.top,
            rect.width(),
            rect.height(),
            radius,
            radius,
            false,
        );

        // clipper 需要添加到 canvas
        _ = c.tvg_canvas_add(canvas, clip_shape);

        try self.clip_paints.append(self.allocator, clip_shape);
    }

    fn popClip(self: *ThorvgBackend) void {
        if (self.clip_paints.items.len > 0) {
            _ = self.clip_paints.pop();
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "ThorvgBackend 基本绘制" {
    const allocator = std.testing.allocator;

    // 初始化 ThorVG 引擎
    try std.testing.expectEqual(@as(c_uint, 0), c.tvg_engine_init(1));
    defer _ = c.tvg_engine_term();

    var backend = ThorvgBackend.init(allocator);
    defer backend.deinit();

    // 创建测试场景
    var scene = Scene.init(allocator);
    defer scene.deinit();

    try scene.fillRect(Rect.fromXywh(10, 10, 100, 80), Color.red);
    try scene.fillRoundedRect(Rect.fromXywh(130, 10, 80, 80), 8, Color.blue);

    // 创建像素缓冲
    var pixels: [800 * 600]u32 = undefined;
    const target = RenderTarget{
        .pixels = &pixels,
        .width = 800,
        .height = 600,
    };

    const b = backend.interface();
    try b.beginFrame(target);
    try b.submit(&scene);
    try b.endFrame();

    // 验证有像素被写入（非零）
    var any_nonzero = false;
    for (&pixels) |p| {
        if (p != 0) {
            any_nonzero = true;
            break;
        }
    }
    try std.testing.expect(any_nonzero);
}
