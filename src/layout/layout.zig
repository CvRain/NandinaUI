//! layout —— 布局系统层
//!
//! 提供 constraints / preferred size / measure-layout 协议，以及 Row / Column /
//! Stack 等基础容器语义。Yoga 作为后续复杂 flex 容器的可插拔求解后端预留，
//! 不主导当前语义层设计。
//!
//! 依赖方向：layout 依赖 foundation。
//!
//! 现状：骨架占位。下一步定义 Constraints（min/max width/height）与
//! measure/layout 协议接口。
const std = @import("std");

// TODO(layout): 定义 Constraints（minWidth/maxWidth/minHeight/maxHeight）
// TODO(layout): 定义 measure(constraints) -> Size 协议
// TODO(layout): 定义 Row / Column / Stack 基础容器

test {
    std.testing.refAllDecls(@This());
}
