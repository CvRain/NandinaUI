//! showcase/registry —— demo 注册表与运行框架
//!
//! showcase 子项目的核心抽象。每个 demo 自描述（name / summary）并提供一个 `run`
//! 函数，演示某个子系统的能力。`registry.demos` 汇总全部 demo，由 `main.zig` 调度。
//!
//! 设计意图（前向兼容）：当前阶段没有渲染后端与 widgets，demo 通过向 `DemoContext.out`
//! 打印文本来「展示运行效果」。等 render / runtime / widgets 落地后，`DemoContext`
//! 可扩展出 scene / 根节点字段，demo 的 `run` 返回一棵 widget 树，框架即演进为
//! 真正的可视化组件画廊 —— 而 demo 的注册方式、目录结构保持不变。

const std = @import("std");
const nandina = @import("NandinaUI");

/// 传给每个 demo 的运行上下文。
pub const DemoContext = struct {
    /// 临时分配器（如 reactive Graph 需要）。
    allocator: std.mem.Allocator,
    /// 文本输出目标。
    out: *std.Io.Writer,
    /// 共享的响应式调度图，供 reactive 相关 demo 使用。
    graph: *nandina.reactive.Graph,
};

/// 一个可运行的 demo。
pub const Demo = struct {
    /// 唯一标识，命令行按此名运行（如 `zig build showcase -- reactive-counter`）。
    name: []const u8,
    /// 一句话描述。
    summary: []const u8,
    /// 演示逻辑。向 `ctx.out` 输出运行效果。
    run: *const fn (ctx: *DemoContext) anyerror!void,
};

const reactive_counter = @import("demos/reactive_counter.zig");
const reactive_derived = @import("demos/reactive_derived.zig");
const reactive_batch = @import("demos/reactive_batch.zig");
const foundation_geometry = @import("demos/foundation_geometry.zig");
const foundation_color = @import("demos/foundation_color.zig");
const color_space = @import("demos/color_space.zig");
const render_scene = @import("demos/render_scene.zig");
const layout_box = @import("demos/layout_box.zig");
const theme_demo = @import("demos/theme.zig");

/// 全部已注册的 demo，按演示顺序排列。
pub const demos = [_]Demo{
    foundation_geometry.demo,
    foundation_color.demo,
    color_space.demo,
    reactive_counter.demo,
    reactive_derived.demo,
    reactive_batch.demo,
    render_scene.demo,
    layout_box.demo,
    theme_demo.demo,
};

/// 按名查找 demo。
pub fn find(name: []const u8) ?Demo {
    for (demos) |d| {
        if (std.mem.eql(u8, d.name, name)) return d;
    }
    return null;
}
