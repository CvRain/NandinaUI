//! app/router —— 路由导航器
//!
//! Router 管理页面导航，支持按名称（键）导航。
//! 与 PageHost 配合使用：Router.navigate 调用 PageHost.navigateTo。

const std = @import("std");
const reactive = @import("../reactive/reactive.zig");
const page = @import("page.zig");

const PageHost = page.PageHost;
const Page = page.Page;

/// 路由配置：一个键对应一个页面。
pub const Route = struct {
    key: []const u8,
    page: Page,
};

/// 路由导航器。
pub const Router = struct {
    allocator: std.mem.Allocator,
    routes: []const Route,
    page_host: *PageHost,

    /// 当前路由键的可读信号（用于响应式更新导航 UI）。
    current_key: []const u8 = "",

    pub fn init(allocator: std.mem.Allocator, routes: []const Route, host: *PageHost) Router {
        return .{
            .allocator = allocator,
            .routes = routes,
            .page_host = host,
        };
    }

    /// 按路由键导航。若未找到，不执行任何操作。
    pub fn navigate(self: *Router, key: []const u8) void {
        for (self.routes, 0..) |route, i| {
            if (std.mem.eql(u8, route.key, key)) {
                self.page_host.navigateTo(i) catch {};
                self.current_key = route.key;
                return;
            }
        }
    }
};
