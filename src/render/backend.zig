//! render/backend —— 渲染后端接口
//!
//! `Backend` 是一个 vtable 接口：上层把一帧的 `Scene` 提交给后端执行。后端可以是
//! GPU、软件光栅器，或测试用的录制器。本层不实现任何真实后端，只定义边界 ——
//! 这是架构文档列出的优先稳定边界之一（render 与具体后端的抽象边界）。
//!
//! 渲染目标用 `RenderTarget` 描述（像素缓冲 + 尺寸），帧生命周期为
//! `beginFrame → submit → endFrame`。
//!
//! `RecordingBackend` 是内置的内存后端：把提交的命令拷贝进自有缓冲，供测试断言
//! 命令序列，也供 showcase 在无图形环境下展示「渲染产物」。

const std = @import("std");
const scene_mod = @import("scene.zig");

const Scene = scene_mod.Scene;
const DrawCommand = scene_mod.DrawCommand;

/// 像素格式。当前仅支持 32 位 ARGB。
pub const PixelFormat = enum {
    argb8888,
};

/// 渲染目标视图：后端把场景绘制到这块像素缓冲。
/// 软件后端会写入 `pixels`；录制 / 无头后端可忽略它。
pub const RenderTarget = struct {
    pixels: ?[*]u32 = null,
    width: u32 = 0,
    height: u32 = 0,
    /// 每行像素数（含 padding）。0 表示等于 width。
    stride: u32 = 0,
    format: PixelFormat = .argb8888,
};

/// 后端可能返回的错误。
pub const BackendError = error{
    /// 在 beginFrame 之前调用了 submit / endFrame。
    NotInFrame,
    OutOfMemory,
};

/// 字形渲染器接口（vtable）。
/// 渲染后端用此接口绘制真实文字 glyph。若不设置，则回退到占位符绘制。
pub const GlyphRenderer = struct {
    ptr: *anyopaque,
    vtable: *const VTable,

    pub const VTable = struct {
        /// 渲染一个码点到像素缓冲（写入 ARGB8888 格式）。
        /// `pixels` 为输出缓冲，`stride` 为每行像素数。
        /// 返回渲染的字形宽度（像素）。
        render_codepoint: *const fn (ptr: *anyopaque, codepoint: u21, font_size: f32, color: u32, x: i32, y: i32, pixels: [*]u32, width: u32, height: u32, stride: u32) i32,
        /// 获取码点的水平步进宽度（像素）。
        advance: *const fn (ptr: *anyopaque, codepoint: u21, font_size: f32) f32,
        /// 获取垂直度量（行高、基线位置）。
        vmetrics: *const fn (ptr: *anyopaque, font_size: f32) GlyphVMetrics,
    };

    pub fn renderCodepoint(self: GlyphRenderer, codepoint: u21, font_size: f32, color: u32, x: i32, y: i32, pixels: [*]u32, width: u32, height: u32, stride: u32) i32 {
        return self.vtable.render_codepoint(self.ptr, codepoint, font_size, color, x, y, pixels, width, height, stride);
    }

    pub fn advance(self: GlyphRenderer, codepoint: u21, font_size: f32) f32 {
        return self.vtable.advance(self.ptr, codepoint, font_size);
    }

    pub fn vmetrics(self: GlyphRenderer, font_size: f32) GlyphVMetrics {
        return self.vtable.vmetrics(self.ptr, font_size);
    }
};

/// 字形渲染器的垂直度量。
pub const GlyphVMetrics = struct {
    ascent: f32,
    descent: f32,
    line_gap: f32,
};

/// 渲染后端接口（vtable）。具体后端持有自身状态，通过 `interface()` 暴露本接口。
pub const Backend = struct {
    ptr: *anyopaque,
    vtable: *const VTable,

    pub const VTable = struct {
        begin_frame: *const fn (ptr: *anyopaque, target: RenderTarget) BackendError!void,
        submit: *const fn (ptr: *anyopaque, scene: *const Scene) BackendError!void,
        end_frame: *const fn (ptr: *anyopaque) BackendError!void,
    };

    /// 开始一帧，绑定渲染目标。
    pub fn beginFrame(self: Backend, target: RenderTarget) BackendError!void {
        return self.vtable.begin_frame(self.ptr, target);
    }

    /// 提交一个场景到当前帧。可在一帧内多次调用。
    pub fn submit(self: Backend, scene: *const Scene) BackendError!void {
        return self.vtable.submit(self.ptr, scene);
    }

    /// 结束当前帧（呈现 / flush）。
    pub fn endFrame(self: Backend) BackendError!void {
        return self.vtable.end_frame(self.ptr);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § RecordingBackend —— 内存录制后端
// ─────────────────────────────────────────────────────────────────────────────

/// 把提交的所有绘制命令按序录进内存缓冲的后端。
///
/// 不做真实光栅化，用途：
///   - 单元测试断言命令序列；
///   - showcase 在无图形环境下「展示」一帧的渲染产物。
///
/// 注意：录制的是命令的浅拷贝。`DrawText.text` 为借用切片，调用方须保证其
/// 生命周期覆盖到录制内容被读取为止。
pub const RecordingBackend = struct {
    allocator: std.mem.Allocator,
    commands: std.ArrayList(DrawCommand) = .empty,
    target: RenderTarget = .{},
    in_frame: bool = false,
    frame_count: u32 = 0,

    pub fn init(allocator: std.mem.Allocator) RecordingBackend {
        return .{ .allocator = allocator };
    }

    pub fn deinit(self: *RecordingBackend) void {
        self.commands.deinit(self.allocator);
        self.* = undefined;
    }

    /// 清空已录制命令（保留容量），用于复用。
    pub fn reset(self: *RecordingBackend) void {
        self.commands.clearRetainingCapacity();
    }

    /// 暴露为通用 `Backend` 接口。
    pub fn interface(self: *RecordingBackend) Backend {
        return .{ .ptr = self, .vtable = &vtable };
    }

    const vtable = Backend.VTable{
        .begin_frame = beginFrame,
        .submit = submit,
        .end_frame = endFrame,
    };

    fn beginFrame(ptr: *anyopaque, target: RenderTarget) BackendError!void {
        const self: *RecordingBackend = @ptrCast(@alignCast(ptr));
        self.target = target;
        self.in_frame = true;
    }

    fn submit(ptr: *anyopaque, scene: *const Scene) BackendError!void {
        const self: *RecordingBackend = @ptrCast(@alignCast(ptr));
        if (!self.in_frame) return BackendError.NotInFrame;
        try self.commands.appendSlice(self.allocator, scene.commands.items);
    }

    fn endFrame(ptr: *anyopaque) BackendError!void {
        const self: *RecordingBackend = @ptrCast(@alignCast(ptr));
        if (!self.in_frame) return BackendError.NotInFrame;
        self.in_frame = false;
        self.frame_count += 1;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const foundation = @import("../foundation/foundation.zig");
const Rect = foundation.Rect;
const Color = foundation.Color;

test "RecordingBackend 录制提交的命令序列" {
    var rec = RecordingBackend.init(std.testing.allocator);
    defer rec.deinit();
    const backend = rec.interface();

    var scene = Scene.init(std.testing.allocator);
    defer scene.deinit();
    try scene.fillRect(Rect.fromXywh(0, 0, 100, 50), Color.white);
    try scene.fillRoundedRect(Rect.fromXywh(0, 0, 100, 50), 6, Color.black);

    try backend.beginFrame(.{ .width = 800, .height = 600 });
    try backend.submit(&scene);
    try backend.endFrame();

    try std.testing.expectEqual(@as(u32, 1), rec.frame_count);
    try std.testing.expectEqual(@as(usize, 2), rec.commands.items.len);
    try std.testing.expect(rec.commands.items[0] == .fill_rect);
    try std.testing.expect(rec.commands.items[1] == .fill_rounded_rect);
    try std.testing.expectEqual(@as(u32, 800), rec.target.width);
}

test "RecordingBackend 帧外 submit 报错" {
    var rec = RecordingBackend.init(std.testing.allocator);
    defer rec.deinit();
    const backend = rec.interface();

    var scene = Scene.init(std.testing.allocator);
    defer scene.deinit();
    try scene.fillRect(Rect.fromXywh(0, 0, 10, 10), Color.white);

    try std.testing.expectError(BackendError.NotInFrame, backend.submit(&scene));
}

test "RecordingBackend 多帧与 reset" {
    var rec = RecordingBackend.init(std.testing.allocator);
    defer rec.deinit();
    const backend = rec.interface();

    var scene = Scene.init(std.testing.allocator);
    defer scene.deinit();
    try scene.fillRect(Rect.fromXywh(0, 0, 10, 10), Color.white);

    try backend.beginFrame(.{});
    try backend.submit(&scene);
    try backend.endFrame();

    rec.reset();
    try std.testing.expectEqual(@as(usize, 0), rec.commands.items.len);

    try backend.beginFrame(.{});
    try backend.submit(&scene);
    try backend.endFrame();
    try std.testing.expectEqual(@as(u32, 2), rec.frame_count);
    try std.testing.expectEqual(@as(usize, 1), rec.commands.items.len);
}
