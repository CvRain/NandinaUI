//! app —— 应用开发层（authoring layer）
//!
//! 承载页面（Page）、路由（Router）、窗口/控制器编排与 authoring API。
//! authoring 体验吸收 Flutter 式组合 + Angular 式 page/component/signal 思路。
//!
//! 依赖方向：app 依赖 widgets（及其下层），不反向侵入 runtime 内核。
//!
//! 现状：骨架占位。下一步定义 App / Page / Router 与统一挂载入口。
const std = @import("std");

// TODO(app): 定义 App（应用入口 + 窗口编排）
// TODO(app): 定义 Page / Router / PageHost
// TODO(app): 定义统一根挂载入口与 Ref/Handle/Key 访问机制

test {
    std.testing.refAllDecls(@This());
}
