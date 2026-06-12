//! text —— 文本能力层
//!
//! 提供字体度量接口与文本测量 / 布局。核心是把**约束宽度 + 换行 + 行数上限 + 溢出策略**
//! 作为一等公民输入，从根上保证布局结果不超过约束 —— 杜绝「无界换行导致文字溢出组件」。
//!
//! 依赖方向：text 仅依赖 foundation；测量结果供 layout / render 使用。text 不做裁剪
//! （裁剪由 render 的 push_clip/pop_clip + 未来统一 ClipNode 负责）。
//!
//! ## 用法
//!
//! ```zig
//! const text = @import("NandinaUI").text;
//!
//! var metrics_backend = text.MonospaceMetrics{};
//! const metrics = metrics_backend.interface();
//!
//! var layout = try text.measure(
//!     allocator,
//!     "一段可能很长的文本……",
//!     .{ .font_size = 14 },
//!     .ellipsis,                       // 默认策略：省略号
//!     .{ .max_width = 200, .max_lines = 2 },
//!     metrics,
//! );
//! defer layout.deinit();
//! // layout.size 即文本实际占用尺寸，可上报给 layout 层
//! ```

const std = @import("std");

pub const font = @import("font.zig");
pub const layout = @import("layout.zig");

// ── 公共 API 再导出 ─────────────────────────────────────────────────────────────

/// 字重。
pub const FontWeight = font.FontWeight;
/// 文本样式（字号 / 字重 / 行高 / 字间距）。
pub const TextStyle = font.TextStyle;
/// 字体度量接口（vtable）。
pub const FontMetrics = font.FontMetrics;
/// 垂直度量。
pub const VMetrics = font.VMetrics;
/// 等宽估算占位后端。
pub const MonospaceMetrics = font.MonospaceMetrics;

/// 溢出策略：clip / ellipsis（默认）/ wrap / scale。
pub const Overflow = layout.Overflow;
/// 文本布局约束（max_width / max_lines）。
pub const Constraints = layout.Constraints;
/// 单行布局结果。
pub const Line = layout.Line;
/// 文本布局结果（多行 + 实际尺寸 + 是否截断）。
pub const TextLayout = layout.TextLayout;
/// 测量并布局文本。
pub const measure = layout.measure;

test {
    std.testing.refAllDecls(@This());
}
