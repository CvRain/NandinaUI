//! runtime/event —— UI 事件类型
//!
//! 定义框架内的输入事件数据结构与统一 `Event` 联合体，供 Node 树分发与（未来）平台
//! 窗口翻译共用。本文件纯数据，仅依赖 foundation。
//!
//! 平台后端（SDL3 等）将把原生事件翻译成这些类型，再交给 `Tree` 分发 —— 平台库隐藏在
//! runtime 的 backend 实现内，不污染本接口（依赖规则第 7 条）。

const std = @import("std");

/// 指针按钮。
pub const PointerButton = enum(u8) {
    left,
    middle,
    right,
    other,
};

/// 键盘修饰键位标志。
pub const KeyModifiers = packed struct(u8) {
    shift: bool = false,
    ctrl: bool = false,
    alt: bool = false,
    super: bool = false,
    _pad: u4 = 0,

    pub fn shortcut(self: KeyModifiers) bool {
        return self.ctrl or self.super;
    }
};

pub const PointerMove = struct {
    x: f32,
    y: f32,
    dx: f32 = 0,
    dy: f32 = 0,
};

pub const PointerButtonEvent = struct {
    button: PointerButton,
    x: f32,
    y: f32,
    click_count: u8 = 1,
};

pub const PointerWheel = struct {
    x: f32,
    y: f32,
    dx: f32 = 0,
    dy: f32 = 0,
};

pub const Key = struct {
    code: i32,
    modifiers: KeyModifiers = .{},
    repeat: bool = false,
};

pub const TextInput = struct {
    /// UTF-8 文本（IME 最终结果）。借用切片。
    text: []const u8,
};

pub const FocusChange = struct {
    gained: bool,
};

pub const WindowResize = struct {
    width: u32,
    height: u32,
};

/// 统一事件联合体。
pub const Event = union(enum) {
    pointer_move: PointerMove,
    pointer_down: PointerButtonEvent,
    pointer_up: PointerButtonEvent,
    pointer_click: PointerButtonEvent,
    pointer_wheel: PointerWheel,
    key_down: Key,
    key_up: Key,
    text_input: TextInput,
    focus_in: FocusChange,
    focus_out: FocusChange,
    window_resize: WindowResize,
    window_close,

    /// 若是带坐标的指针事件，返回 (x, y)。
    pub fn pointerPos(self: Event) ?struct { x: f32, y: f32 } {
        return switch (self) {
            .pointer_move => |e| .{ .x = e.x, .y = e.y },
            .pointer_down, .pointer_up, .pointer_click => |e| .{ .x = e.x, .y = e.y },
            .pointer_wheel => |e| .{ .x = e.x, .y = e.y },
            else => null,
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Event.pointerPos 提取坐标" {
    const e: Event = .{ .pointer_down = .{ .button = .left, .x = 10, .y = 20 } };
    const p = e.pointerPos().?;
    try std.testing.expectEqual(@as(f32, 10), p.x);
    try std.testing.expectEqual(@as(f32, 20), p.y);

    const k: Event = .{ .key_down = .{ .code = 65 } };
    try std.testing.expect(k.pointerPos() == null);
}

test "KeyModifiers shortcut" {
    const m = KeyModifiers{ .ctrl = true };
    try std.testing.expect(m.shortcut());
    const n = KeyModifiers{ .shift = true };
    try std.testing.expect(!n.shortcut());
}
