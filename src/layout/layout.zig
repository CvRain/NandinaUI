//! layout —— 布局系统层
//!
//! 提供 `Constraints` 协议层与三套纯函数布局求解器：
//!   - `flex`：盒子模型（column / row / stack），对标 Qt Widget 布局。
//!   - `flow`：流式折行，对标前端 wrap 布局，是未来 flex / grid 的基础。
//!   - `anchors`：锚点定位，对标 QML anchors，适合页面级大块内容自由定位。
//!
//! 依赖方向：layout 仅依赖 foundation（几何）。求解器都是纯函数（输入规格 → 输出 frames），
//! 不依赖 widget 树、不分配内存，便于独立测试。Yoga 等第三方求解器作为未来「求解层」
//! 的可插拔后端预留，不影响本层对上提供的语义（见 docs/development/layout-strategy.md）。
//!
//! ## 用法
//!
//! ```zig
//! const layout = @import("NandinaUI").layout;
//!
//! const children = [_]layout.flex.ChildSpec{ ... };
//! var frames: [N]foundation.Rect = undefined;
//! layout.flex.solve(.{ .axis = .column, .gap = 8 }, container_bounds, &children, &frames);
//! ```

const std = @import("std");

pub const constraints = @import("constraints.zig");
pub const flex = @import("flex.zig");
pub const flow = @import("flow.zig");
pub const anchors = @import("anchors.zig");

// ── 公共 API 再导出 ─────────────────────────────────────────────────────────────

/// 尺寸约束（协议层核心类型）。
pub const Constraints = constraints.Constraints;

/// 盒子模型主轴方向。
pub const Axis = flex.Axis;
/// 对齐方式。
pub const Align = flex.Align;

test {
    std.testing.refAllDecls(@This());
}
