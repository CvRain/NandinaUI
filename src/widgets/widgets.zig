//! widgets —— 组件库层
//!
//! 消费 runtime / reactive / layout / theme / text 能力，提供语义一致的组件 API。
//! 遵循组合优于继承：先 primitives（Surface / Pressable），再 controls
//! （Label / Button / Panel / Card ...）。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / theme / text / runtime。
const std = @import("std");

// ── primitives ────────────────────────────────────────────────────────────────

const pressable_mod = @import("pressable.zig");
/// Pressable —— 交互状态机（hover / pressed / focused / disabled）。
pub const Pressable = pressable_mod.Pressable;
pub const PressableProps = pressable_mod.PressableProps;

// TODO(widgets): Surface / Label / Button / Panel / Card

test {
    std.testing.refAllDecls(@This());
    _ = pressable_mod;
}
