//! render —— 渲染抽象层
//!
//! 定义 `Scene` / `DrawCommand` 中间表示与渲染 `Backend` 接口（vtable），让上层只产出
//! 后端无关的绘制命令，由具体后端（GPU / 软件 / 录制）执行。
//!
//! 依赖方向：render 依赖 foundation（几何 / 颜色），不依赖任何更高层。
//!
//! ## 用法
//!
//! ```zig
//! const render = @import("NandinaUI").render;
//!
//! var scene = render.Scene.init(allocator);
//! defer scene.deinit();
//! try scene.fillRoundedRect(rect, 8, color);
//!
//! var rec = render.RecordingBackend.init(allocator);
//! defer rec.deinit();
//! const backend = rec.interface();
//!
//! try backend.beginFrame(.{ .width = 800, .height = 600 });
//! try backend.submit(&scene);
//! try backend.endFrame();
//! ```

const std = @import("std");

pub const scene = @import("scene.zig");
pub const backend = @import("backend.zig");
pub const backends = struct {
    pub const software = @import("backends/software.zig");
    pub const thorvg = @import("backends/thorvg.zig");
};

// ── 公共 API 再导出 ─────────────────────────────────────────────────────────────

/// 绘制命令联合体。
pub const DrawCommand = scene.DrawCommand;
pub const FillRect = scene.FillRect;
pub const FillRoundedRect = scene.FillRoundedRect;
pub const DrawText = scene.DrawText;
pub const PushClip = scene.PushClip;
pub const PopClip = scene.PopClip;
/// 绘制命令缓冲。
pub const Scene = scene.Scene;

/// 渲染后端接口（vtable）。
pub const Backend = backend.Backend;
/// 渲染目标视图。
pub const RenderTarget = backend.RenderTarget;
pub const PixelFormat = backend.PixelFormat;
pub const BackendError = backend.BackendError;
/// 字形渲染器接口（后端用于绘制真实文字）。
pub const GlyphRenderer = backend.GlyphRenderer;
pub const GlyphVMetrics = backend.GlyphVMetrics;
/// 内存录制后端（测试 / 无头展示）。
pub const RecordingBackend = backend.RecordingBackend;
/// 纯 Zig 软件光栅后端（把 Scene 画成 ARGB8888 像素，无外部依赖）。
pub const SoftwareBackend = backends.software.SoftwareBackend;

test {
    std.testing.refAllDecls(@This());
    _ = backends.software;
}
