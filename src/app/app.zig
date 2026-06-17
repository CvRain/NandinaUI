//! app —— 应用开发层（authoring layer）
//!
//! 承载页面（Page）、路由（Router）、窗口/控制器编排与 authoring API。
//! authoring 体验吸收 Flutter 式组合 + Angular 式 page/component/signal 思路。
//!
//! 依赖方向：app 依赖 widgets（及其下层），不反向侵入 runtime 内核。

const std = @import("std");

pub const page = @import("page.zig");
pub const router = @import("router.zig");
pub const authoring = @import("authoring.zig");

// ── 公共 API 再导出 ─────────────────────────────────────────────────────────────

/// Page 生命周期接口。
pub const Page = page.Page;
/// PageHost 容器 widget。
pub const PageHost = page.PageHost;
/// 组件后续访问句柄。
pub const Ref = page.Ref;

/// 路由配置。
pub const Route = router.Route;
/// 路由导航器。
pub const Router = router.Router;

/// Authoring DSL 工厂函数：surface / column / label / button / card / panel。
pub const surface = authoring.surface;
pub const column = authoring.column;
pub const label = authoring.label;
pub const button = authoring.button;
pub const card = authoring.card;
pub const panel = authoring.panel;

test {
    std.testing.refAllDecls(@This());
}
