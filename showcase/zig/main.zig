//! showcase/zig/main —— NandinaUI Zig 前端 showcase
//!
//! `zig build run` 启动：打开 SDL3 窗口，展示多页面组件画廊。
//! 全程只消费 `frontend/zig/nandina.zig`（前端层），不直接 `@import("NandinaUI")`。

const std = @import("std");
const nd = @import("nandina");
const pages = @import("gui_pages.zig");

const C = pages.C;
const Color = nd.Color;
const Insets = nd.Insets;
const Node = nd.Node;

const WINDOW_W: u32 = 800;
const WINDOW_H: u32 = 600;

// 全局导航器（供按钮回调使用）。
var g_router: ?*nd.Router = null;

pub fn main(init: std.process.Init) !void {
    const io = init.io;
    var buf: [1024]u8 = undefined;
    var wtr = std.Io.File.Writer.init(.stdout(), io, &buf);
    const out = &wtr.interface;

    try out.print("NandinaUI v{d}.{d}.{d}\n", .{ nd.version.major, nd.version.minor, nd.version.patch });
    try out.print("Zig 前端 showcase（消费 frontend/zig/nandina.zig）\n", .{});

    var app = nd.App.init(init.gpa, .{ .title = "NandinaUI Showcase", .width = WINDOW_W, .height = WINDOW_H }) catch |err| {
        try out.print("\nSDL3 窗口不可用（{s}），无法启动图形界面。\n", .{@errorName(err)});
        try out.flush();
        return;
    };
    defer app.deinit();

    try out.print("SDL3 窗口已打开（{d}x{d}），Showcase 启动中……\n", .{ WINDOW_W, WINDOW_H });
    try out.flush();

    // 应用级资源（页面/导航/SignalOwner），随窗口生命周期存活，run 返回后统一释放。
    var res = AppResources{ .allocator = init.gpa };
    defer res.deinit();

    const root = try buildRoot(&app, &res);
    try app.run(root);
}

/// 持有应用级长生命周期资源，便于 run 结束后统一释放（避免 DebugAllocator 误报泄漏）。
const AppResources = struct {
    allocator: std.mem.Allocator,
    root_owner: ?*nd.SignalOwner = null,
    router: ?*nd.Router = null,
    pages: ?[]nd.Page = null,

    fn deinit(self: *AppResources) void {
        if (self.root_owner) |o| {
            o.deinit();
            self.allocator.destroy(o);
        }
        if (self.router) |r| self.allocator.destroy(r);
        if (self.pages) |p| self.allocator.free(p);
    }
};

fn buildRoot(app: *nd.App, res: *AppResources) !*Node {
    const a = app.allocator;
    const g = app.graph();

    // 根级 SignalOwner（随 app 生命周期，由 res 释放）。
    const root_owner = try a.create(nd.SignalOwner);
    root_owner.* = nd.SignalOwner.init(a);
    res.root_owner = root_owner;

    // 页面列表
    const page_list = [_]nd.Page{
        .{ .title = "概述", .build = struct {
            fn f(al: std.mem.Allocator, gr: *nd.Graph, o: *nd.SignalOwner) !*Node {
                return pages.buildOverview(al, gr, o);
            }
        }.f },
        .{ .title = "Widgets", .build = struct {
            fn f(al: std.mem.Allocator, gr: *nd.Graph, o: *nd.SignalOwner) !*Node {
                return pages.buildWidgets(al, gr, o);
            }
        }.f },
        .{ .title = "Layout", .build = struct {
            fn f(al: std.mem.Allocator, gr: *nd.Graph, o: *nd.SignalOwner) !*Node {
                return pages.buildLayout(al, gr, o);
            }
        }.f },
        .{ .title = "Reactive", .build = struct {
            fn f(al: std.mem.Allocator, gr: *nd.Graph, o: *nd.SignalOwner) !*Node {
                return pages.buildReactive(al, gr, o);
            }
        }.f },
        .{ .title = "Theme", .build = struct {
            fn f(al: std.mem.Allocator, gr: *nd.Graph, o: *nd.SignalOwner) !*Node {
                return pages.buildTheme(al, gr, o);
            }
        }.f },
    };
    const owned_pages = try a.dupe(nd.Page, &page_list);
    res.pages = owned_pages;

    const page_host = try nd.PageHost.create(a, g, owned_pages, 0);

    const router = try a.create(nd.Router);
    router.* = nd.Router.init(a, &.{}, page_host);
    res.router = router;
    g_router = router;

    // 根 surface + 垂直主列
    const root = try nd.surface(root_owner, a, g, .{ .bg_color = C.base });
    const main_col = try nd.column(a, .{ .gap = 0, .cross_align = .stretch });
    try root.addChild(a, main_col);

    // ── 顶栏：标题 + 导航按钮（Row 水平排列）──────────────────────────────
    {
        const header = try nd.surface(root_owner, a, g, .{ .bg_color = C.mantle, .padding = Insets.symmetric(16, 12) });
        try main_col.addChild(a, header);

        const nav_col = try nd.column(a, .{ .gap = 8 });
        try nav_col.addChild(a, try nd.label(root_owner, a, g, "NandinaUI Showcase", .{ .color = C.text, .font_size = 20 }));

        const nav_row = try nd.row(a, .{ .gap = 8 });
        inline for (.{ "概述", "Widgets", "Layout", "Reactive", "Theme" }, 0..) |name, i| {
            const btn = try nd.button(root_owner, a, g, name, .{
                .bg_color = C.surface0,
                .bg_hover_color = C.blue,
                .bg_pressed_color = C.teal,
                .text_color = C.text,
                .font_size = 13,
                .corner_radius = 4,
                .padding = Insets.symmetric(12, 6),
                .on_click = struct {
                    fn cb(_: ?*anyopaque) callconv(.c) void {
                        if (g_router) |r| r.page_host.navigateTo(i) catch {};
                    }
                }.cb,
            });
            try nav_row.addChild(a, btn);
        }
        try nav_col.addChild(a, nav_row);
        try header.addChild(a, nav_col);
    }

    // ── 内容区：挂 PageHost ──────────────────────────────────────────────
    {
        const content = try nd.surface(root_owner, a, g, .{ .bg_color = C.base, .padding = Insets.all(24) });
        try main_col.addChild(a, content);
        try content.addChild(a, &page_host.node);
    }

    return root;
}
