//! NandinaUI - 库根模块（聚合导出）
//!
//! 这是 NandinaUI 包的入口。所有面向消费者的公共声明都从这里再导出。
//! 分层依赖方向（单向）：
//!   foundation -> reactive / render / layout / theme / text -> widgets -> app
//!
//! 各层职责见 docs/ARCHITECTURE.md。
const std = @import("std");

// ---- 分层模块再导出 ----
pub const foundation = @import("foundation/foundation.zig");
pub const reactive = @import("reactive/reactive.zig");
pub const render = @import("render/render.zig");
pub const layout = @import("layout/layout.zig");
pub const theme = @import("theme/theme.zig");
pub const text = @import("text/text.zig");
pub const runtime = @import("runtime/runtime.zig");
pub const widgets = @import("widgets/widgets.zig");
pub const app = @import("app/app.zig");

/// 库版本信息。
pub const version = std.SemanticVersion{ .major = 0, .minor = 0, .patch = 0 };

// 保证 `zig build test` 会递归收集所有子模块里的 test 块。
test {
    std.testing.refAllDecls(@This());
}
