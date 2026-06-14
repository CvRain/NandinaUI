//! runtime —— UI 运行时层
//!
//! 把 reactive / render / layout / text 串成数据驱动的 UI 闭环：
//!   - `Node`：UI 节点树基本单元（嵌入式 + vtable），持有 owning 子节点、几何、dirty 状态。
//!   - `Tree`：拥有根节点，驱动一帧 `relayout → repaint` 并产出 `render.Scene`；分发事件。
//!   - `event`：输入事件类型（指针 / 键盘 / 焦点 / 窗口）。
//!
//! 依赖方向：runtime 依赖 foundation / reactive / render / layout（不感知具体业务组件）。
//! 本层为**纯逻辑核心**：不持有平台窗口，绘制产物是 Scene，可交给任意 Backend。平台窗口
//! 与事件源后续作为 backend 接入（依赖规则第 7 条）。
//!
//! ## 一帧闭环
//!
//! ```
//! signal 变化 → node.markLayoutDirty/markPaintDirty 冒泡到根
//!   → tree.frame()
//!       → relayout: measure(自底向上) + layout(自顶向下，节点内用 layout 求解器)
//!       → repaint:  深度优先 paint，产出 Scene 命令
//!   → backend.submit(scene)
//! ```

const std = @import("std");

pub const event = @import("event.zig");
pub const node = @import("node.zig");
pub const tree = @import("tree.zig");

// ── 公共 API 再导出 ─────────────────────────────────────────────────────────────

/// UI 节点（嵌入式 + vtable）。
pub const Node = node.Node;
pub const VTable = node.VTable;
pub const EventResult = node.EventResult;
pub const ClipRegion = node.ClipRegion;

/// UI 树容器与主循环。
pub const Tree = tree.Tree;

/// 事件类型。
pub const Event = event.Event;
pub const PointerButton = event.PointerButton;
pub const KeyModifiers = event.KeyModifiers;

test {
    std.testing.refAllDecls(@This());
}
