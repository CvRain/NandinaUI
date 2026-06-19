//! widgets —— 组件库层
//!
//! 消费 runtime / reactive / layout / theme / text 能力，提供语义一致的组件 API。
//! 遵循组合优于继承：先 primitives（Surface / Pressable），再 controls
//! （Label / Button / Panel / Card ...）。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / theme / text / runtime。
//!
//! ## 统一约定（M5）
//!
//! - 组件输入用只读视图 `reactive.ReadSignal(T)`（调用方持有背后的 `Signal`）。
//! - 组件自身的内部状态（如 hovered / pressed）用 `reactive.Signal`，并绑定
//!   `EffectScope`：构造时注册合并 effect 追踪全部输入，任一变化即 `markLayoutDirty`
//!   / `markPaintDirty`，由 runtime 下一帧重新布局 / 绘制。
//! - 通过 `node.deinitTree(allocator)` 释放（先 dispose scope，再释放自身）。

const std = @import("std");

const surface_mod = @import("surface.zig");
const column_mod = @import("column.zig");
const pressable_mod = @import("pressable.zig");
const clip_node_mod = @import("clip_node.zig");
const focus_ring_mod = @import("focus_ring.zig");
const icon_mod = @import("icon.zig");
const label_mod = @import("label.zig");
const button_mod = @import("button.zig");
const panel_mod = @import("panel.zig");
const card_mod = @import("card.zig");
const text_field_mod = @import("text_field.zig");
const field_mod = @import("field.zig");
const checkbox_mod = @import("checkbox.zig");
const switch_mod = @import("switch.zig");

// ── primitives ────────────────────────────────────────────────────────────────

/// Surface —— 背景色 / 圆角 / padding / 描边容器（基础构建块）。
pub const Surface = surface_mod.Surface;
pub const SurfaceProps = surface_mod.SurfaceProps;

/// Column —— 垂直堆叠子节点的容器（基于 layout.flex 求解器）。
pub const Column = column_mod.Column;
pub const ColumnProps = column_mod.ColumnProps;

/// Pressable —— 交互状态机（hover / pressed / focused / disabled）。
pub const Pressable = pressable_mod.Pressable;
pub const PressableProps = pressable_mod.PressableProps;

/// ClipNode —— 通用子树裁剪容器（不可见，统一裁剪机制）。
pub const ClipNode = clip_node_mod.ClipNode;
pub const ClipNodeProps = clip_node_mod.ClipNodeProps;

/// FocusRing —— 焦点可视化覆盖层（active 时绘制描边环，不参与命中测试）。
pub const FocusRing = focus_ring_mod.FocusRing;
pub const FocusRingProps = focus_ring_mod.FocusRingProps;

/// Icon —— 图标 primitive（纯色矩形/圆形，用于状态指示、占位等）。
pub const Icon = icon_mod.Icon;
pub const IconProps = icon_mod.IconProps;
pub const IconShape = icon_mod.IconShape;

// ── controls ──────────────────────────────────────────────────────────────────

/// Label —— 响应式文本标签（建立在 text 层之上）。
pub const Label = label_mod.Label;
pub const LabelProps = label_mod.LabelProps;

/// Button —— 可点击按钮（背景随交互状态变化 + 文本）。
pub const Button = button_mod.Button;
pub const ButtonProps = button_mod.ButtonProps;

/// Panel —— 带圆角 / 边框 / padding 的内容面板。
pub const Panel = panel_mod.Panel;
pub const PanelProps = panel_mod.PanelProps;

/// Card —— 带 title / description header 的结构化容器。
pub const Card = card_mod.Card;
pub const CardProps = card_mod.CardProps;

/// TextField —— 单行文本输入控件。
pub const TextField = text_field_mod.TextField;
pub const TextFieldProps = text_field_mod.TextFieldProps;

/// Field —— 语义表单容器（label + 控件 + helper/error 消息）。
pub const Field = field_mod.Field;
pub const FieldProps = field_mod.FieldProps;

/// Checkbox —— 复选框组件。
pub const Checkbox = checkbox_mod.Checkbox;
pub const CheckboxProps = checkbox_mod.CheckboxProps;

/// Switch —— 开关组件。
pub const Switch = switch_mod.Switch;
pub const SwitchProps = switch_mod.SwitchProps;

test {
    std.testing.refAllDecls(@This());
    // 显式引用各子模块，确保它们的 test 块被 `zig build test` 收集。
    _ = surface_mod;
    _ = column_mod;
    _ = pressable_mod;
    _ = clip_node_mod;
    _ = focus_ring_mod;
    _ = icon_mod;
    _ = label_mod;
    _ = button_mod;
    _ = panel_mod;
    _ = card_mod;
    _ = text_field_mod;
    _ = field_mod;
    _ = checkbox_mod;
    _ = switch_mod;
}
