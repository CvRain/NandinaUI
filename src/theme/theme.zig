//! theme —— 主题与设计令牌层
//!
//! 提供 token / palette / theme schema 与运行时主题应用机制，不与具体 widget
//! 实现耦合。遵循 design-system-first 原则：先 token/theme/semantic API，再 widget。
//!
//! 依赖方向：theme 依赖 foundation（颜色等）。
//!
//! 现状：骨架占位。下一步定义颜色 token 表、间距/圆角 scale 与 Theme resolver。
const std = @import("std");

// TODO(theme): 定义颜色 token（background / foreground / primary / border ...）
// TODO(theme): 定义 spacing / radius / typography scale
// TODO(theme): 定义 Theme 结构与 resolver

test {
    std.testing.refAllDecls(@This());
}
