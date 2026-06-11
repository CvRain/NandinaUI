//! render —— 渲染抽象层
//!
//! 定义 Scene / DrawCommand 中间表示与渲染后端接口（backend interface），
//! 让上层只产出绘制命令，由具体后端（如 ThorVG / 自绘）执行。
//!
//! 依赖方向：render 依赖 foundation（几何 / 颜色）。
//!
//! 现状：骨架占位。下一步定义 DrawCommand（FillRect / FillRoundedRect /
//! DrawText / PushClip / PopClip）与 Backend vtable 接口。
const std = @import("std");

// TODO(render): 定义 DrawCommand 联合体（FillRect / FillRoundedRect / DrawText / PushClip / PopClip）
// TODO(render): 定义 Scene（绘制命令缓冲）
// TODO(render): 定义 Backend 接口（vtable），由具体后端实现

test {
    std.testing.refAllDecls(@This());
}
