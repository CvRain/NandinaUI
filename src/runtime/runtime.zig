//! runtime —— UI 运行时层
//!
//! 提供生命周期、节点树（widget tree）、事件循环与调度边界。是 widgets 与
//! 具体平台窗口之间的运行时基座。
//!
//! 依赖方向：runtime 依赖 foundation / reactive / render / layout。
//!
//! 现状：骨架占位。下一步定义 Node 树结构、事件类型与主循环骨架。
const std = @import("std");

// TODO(runtime): 定义 Node 树（parent/children，owning 子节点 + handle 访问）
// TODO(runtime): 定义 Event 类型（pointer / key）与分发
// TODO(runtime): 定义 mark_dirty -> reflow -> repaint 主循环骨架
// TODO(runtime): 定义 Window / 平台后端边界

test {
    std.testing.refAllDecls(@This());
}
