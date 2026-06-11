//! widgets —— 组件库层
//!
//! 消费 runtime / reactive / layout / theme / text 能力，提供语义一致的组件 API。
//! 遵循组合优于继承：先 primitives（Surface / Pressable），再 controls
//! （Label / Button / Panel / Card ...）。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / theme / text / runtime。
//!
//! 现状：骨架占位。下一步落地 Surface / Label / Button 最小集合。
const std = @import("std");

// TODO(widgets): primitives —— Surface / Pressable
// TODO(widgets): controls —— Label / Button / Panel / Card
// TODO(widgets): 组件统一接收只读输入（Prop / ReadState），内部状态用 State

test {
    std.testing.refAllDecls(@This());
}
