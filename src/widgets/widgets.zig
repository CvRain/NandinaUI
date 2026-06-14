//! widgets —— 组件库层
//!
//! 消费 runtime / reactive / layout / theme / text 能力，提供语义一致的组件 API。
//! 遵循组合优于继承：先 primitives（Surface / Pressable），再 controls
//! （Label / Button / Panel / Card ...）。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / theme / text / runtime。
const std = @import("std");

// ── controls ──────────────────────────────────────────────────────────────────

const label_mod = @import("label.zig");
/// Label —— 响应式文本标签（text / color / font_size 均为 ReadSignal 输入）。
pub const Label = label_mod.Label;
pub const LabelProps = label_mod.LabelProps;

// TODO(widgets): Surface / Pressable / Button / Panel / Card

test {
    std.testing.refAllDecls(@This());
    _ = label_mod;
}
