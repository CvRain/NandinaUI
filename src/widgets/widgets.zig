//! widgets —— 组件库层
//!
//! 消费 runtime / reactive / layout / theme / text 能力，提供语义一致的组件 API。
//! 遵循组合优于继承：先 primitives（Surface / Pressable），再 controls
//! （Label / Button / Panel / Card ...）。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / theme / text / runtime。
const std = @import("std");

// ── controls ──────────────────────────────────────────────────────────────────

const panel_mod = @import("panel.zig");
/// Panel —— 带圆角、边框、padding 的背景容器。
pub const Panel = panel_mod.Panel;
pub const PanelProps = panel_mod.PanelProps;

const button_mod = @import("button.zig");
/// Button —— 响应式按钮（label/颜色/状态均为 ReadSignal 输入）。
pub const Button = button_mod.Button;
pub const ButtonProps = button_mod.ButtonProps;

const card_mod = @import("card.zig");
/// Card —— title + description header 结构化容器。
pub const Card = card_mod.Card;
pub const CardProps = card_mod.CardProps;

// TODO(widgets): Surface / Pressable / Label

test {
    std.testing.refAllDecls(@This());
    _ = panel_mod;
    _ = button_mod;
    _ = card_mod;
}
