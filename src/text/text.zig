//! text —— 文本能力层
//!
//! 提供字体加载、文本测量与文本布局能力，供 widgets 层绘制文本时消费。
//!
//! 依赖方向：text 依赖 foundation；测量结果供 layout / render 使用。
//!
//! 现状：骨架占位。下一步定义 Font 句柄与 measureText(str, font) -> Size。
const std = @import("std");

// TODO(text): 定义 Font 句柄抽象
// TODO(text): 定义 measureText(str, font) -> Size
// TODO(text): 定义 text layout（换行 / 对齐）

test {
    std.testing.refAllDecls(@This());
}
