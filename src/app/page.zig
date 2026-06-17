//! app/page —— Page 生命周期与 PageHost 容器
//!
//! Page 是应用层的基本页面单元，有明确的创建/激活/停用/销毁生命周期。
//! PageHost 是一个嵌入 widget 树的容器节点，管理当前页面的显示与切换。
//!
//! 设计参照：Angular 的 page/component + Flutter 的 Navigator。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const render = @import("../render/render.zig");
const layout = @import("../layout/layout.zig");
const runtime = @import("../runtime/runtime.zig");

const Node = runtime.Node;
const VTable = runtime.VTable;
const EventResult = runtime.EventResult;
const Event = runtime.Event;
const Constraints = layout.Constraints;
const Scene = render.Scene;

// ─────────────────────────────────────────────────────────────────────────────
// Page 接口
// ─────────────────────────────────────────────────────────────────────────────

/// 页面生命周期。具体页面类型实现此接口。
pub const Page = struct {
    /// 页面标题（用于展示在导航 / 标签页上）。
    title: []const u8 = "",
    /// 构建此页面的 widget 树根节点。
    /// 在 PageHost 挂载时自动调用。
    build: *const fn (allocator: std.mem.Allocator, g: *reactive.Graph) anyerror!*Node,
    /// 页面激活时回调（可选）。
    on_activate: ?*const fn (*Node) void = null,
    /// 页面停用时回调（可选）。
    on_deactivate: ?*const fn (*Node) void = null,
};

// ─────────────────────────────────────────────────────────────────────────────
// PageHost —— 嵌入 widget 树的页面容器
// ─────────────────────────────────────────────────────────────────────────────

/// PageHost 是一个 widget 节点，内部持有当前页面的根节点，
/// 切换页面时自动替换子节点。
pub const PageHost = struct {
    node: Node,
    allocator: std.mem.Allocator,
    graph: *reactive.Graph,

    /// 当前页面在 pages 列表中的索引（-1 表示无页面）。
    current_index: i32 = -1,
    /// 页面列表。
    pages: []const Page,

    const vtable = VTable{
        .measure = measureImpl,
        .layout = layoutImpl,
        .deinit = deinitImpl,
    };

    /// 创建 PageHost。`pages` 是页面定义列表，`initial_index` 指定初始显示哪个页面。
    pub fn create(allocator: std.mem.Allocator, g: *reactive.Graph, pages: []const Page, initial_index: usize) !*PageHost {
        const self = try allocator.create(PageHost);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .graph = g,
            .pages = pages,
            .current_index = -1,
        };
        // 挂载初始页面
        try self.navigateTo(initial_index);
        return self;
    }

    /// 导航到指定索引的页面。自动替换子节点树。
    pub fn navigateTo(self: *PageHost, index: usize) !void {
        if (index >= self.pages.len) return;

        // 停用当前页面
        if (self.current_index >= 0 and self.current_index < self.pages.len) {
            const old_page = &self.pages[@as(usize, @intCast(self.current_index))];
            if (old_page.on_deactivate) |cb| {
                for (self.node.children.items) |child| cb(child);
            }
        }

        // 释放旧子节点
        if (self.node.children.items.len > 0) {
            // deinitTree 释放所有子节点
            for (self.node.children.items) |child| {
                child.deinitTree(self.allocator);
            }
            self.node.children.clearRetainingCapacity();
        }

        // 构建新页面
        const new_page = &self.pages[index];
        const root = try new_page.build(self.allocator, self.graph);

        // 添加为子节点（PageHost 的 layout 会将其铺满自身）
        try self.node.addChild(self.allocator, root);

        self.current_index = @as(i32, @intCast(index));

        // 激活新页面
        if (new_page.on_activate) |cb| cb(root);
    }

    fn measureImpl(node: *Node, constraints: Constraints) foundation.Size {
        // PageHost 自身不增加尺寸，内容由子页面决定
        if (node.children.items.len > 0) {
            return node.children.items[0].measure(constraints);
        }
        return .{};
    }

    fn layoutImpl(node: *Node) void {
        // 把当前页面子节点铺满自身 bounds
        for (node.children.items) |child| {
            child.setBounds(node.bounds);
        }
    }

    fn deinitImpl(node: *Node, allocator: std.mem.Allocator) void {
        _ = node;
        _ = allocator;
        // 子节点由 deinitTree 递归释放
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Ref —— 组件后续访问句柄
// ─────────────────────────────────────────────────────────────────────────────

/// Ref(T) 是一个轻量句柄，在组件挂载时自动被回填。
/// 用于后续访问已挂载的组件，替代保留所有权变量的方式。
pub fn Ref(comptime T: type) type {
    return struct {
        const Self = @This();

        ptr: ?*T = null,

        /// 获取组件指针（若已挂载）。
        pub fn get(self: *const Self) ?*T {
            return self.ptr;
        }

        /// 内部绑定：组件工厂在创建时调用此方法注册自身。
        pub fn bind(self: *Self, p: *T) void {
            self.ptr = p;
        }
    };
}
