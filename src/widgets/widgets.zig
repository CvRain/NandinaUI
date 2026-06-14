//! widgets —— 组件库层
//!
//! 消费 runtime / reactive / layout / theme / text 能力，提供语义一致的组件 API。
//! 遵循组合优于继承：先 primitives（Surface / Pressable），再 controls
//! （Label / Button / Panel / Card ...）。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / theme / text / runtime。
const std = @import("std");

// ── primitives ────────────────────────────────────────────────────────────────

const surface_mod = @import("surface.zig");
/// Surface —— 背景色 / 圆角 / padding / 描边容器（基础构建块）。
pub const Surface = surface_mod.Surface;
pub const SurfaceProps = surface_mod.SurfaceProps;

// TODO(widgets): Pressable / Label / Button / Panel / Card

test {
    std.testing.refAllDecls(@This());
    _ = surface_mod;
}
