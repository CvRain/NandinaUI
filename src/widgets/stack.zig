//! widgets/stack —— Stack 容器（子节点层叠）
//!
//! 把所有子节点叠放在同一区域，各自按对齐方式在容器内定位（后添加的在上层）。
//! 依赖 layout.flex 求解器的 stack 轴做实际计算，本层只做 measure/layout 的 vtable 桥接。
//!
//! 对标 QML 的 Item 叠放 / Qt 的 QStackedLayout（但不隐藏非当前项）。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const layout = @import("../layout/layout.zig");
const node_mod = @import("../runtime/node.zig");

const Allocator = std.mem.Allocator;
const Size = foundation.Size;
const Rect = foundation.Rect;
const Constraints = layout.Constraints;
const Node = node_mod.Node;

/// Stack 容器属性。
pub const StackProps = struct {
    /// 水平对齐（交叉轴）。
    cross_align: layout.Align = .start,
    /// 垂直对齐（主轴，stack 下用于 Y 定位）。
    main_align: layout.Align = .start,
};

/// Stack 容器：子节点层叠。
pub const Stack = struct {
    node: Node,
    allocator: Allocator,
    main_align: layout.Align,
    cross_align: layout.Align,

    const vtable = node_mod.VTable{
        .measure = measureImpl,
        .layout = layoutImpl,
        .deinit = deinitImpl,
    };

    pub fn create(allocator: Allocator, props: StackProps) !*Stack {
        const self = try allocator.create(Stack);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .main_align = props.main_align,
            .cross_align = props.cross_align,
        };
        return self;
    }

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        var max_w: f32 = 0;
        var max_h: f32 = 0;
        for (node.children.items) |child| {
            const loose = constraints.loosen();
            const cs = child.measure(loose);
            if (cs.width > max_w) max_w = cs.width;
            if (cs.height > max_h) max_h = cs.height;
        }
        return constraints.constrain(.{ .width = max_w, .height = max_h });
    }

    fn layoutImpl(node: *Node) void {
        const self: *Stack = @fieldParentPtr("node", node);
        const n = node.children.items.len;
        if (n == 0) return;

        var specs_buf: [64]layout.flex.ChildSpec = undefined;
        var frames_buf: [64]Rect = undefined;

        for (node.children.items, 0..) |child, i| {
            specs_buf[i] = .{ .preferred = child.measured };
        }

        layout.flex.solve(.{
            .axis = .stack,
            .main_align = self.main_align,
            .cross_align = self.cross_align,
        }, node.bounds, specs_buf[0..n], frames_buf[0..n]);

        for (node.children.items, 0..) |child, i| {
            child.setBounds(frames_buf[i]);
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        allocator.destroy(@as(*Stack, @fieldParentPtr("node", node)));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const reactive = @import("../reactive/reactive.zig");
const surface_mod = @import("surface.zig");

test "Stack 子节点层叠居中" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    const stack = try Stack.create(a, .{ .main_align = .center, .cross_align = .center });
    defer stack.node.deinitTree(a);

    var bg1 = reactive.Signal(foundation.Color).init(&g, foundation.Color.white);
    defer bg1.deinit();
    var cr1 = reactive.Signal(f32).init(&g, 0);
    defer cr1.deinit();
    var pad1 = reactive.Signal(foundation.Insets).init(&g, foundation.Insets.zero);
    defer pad1.deinit();
    var bc1 = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer bc1.deinit();
    var bw1 = reactive.Signal(f32).init(&g, 0);
    defer bw1.deinit();

    const c1 = try surface_mod.Surface.create(a, &g, .{
        .bg_color = bg1.asReadonly(),
        .corner_radius = cr1.asReadonly(),
        .padding = pad1.asReadonly(),
        .border_color = bc1.asReadonly(),
        .border_width = bw1.asReadonly(),
    });
    try stack.node.addChild(a, &c1.node);
    c1.node.measured = .{ .width = 40, .height = 40 };

    stack.node.setBounds(Rect.fromXywh(0, 0, 100, 100));
    stack.node.layout();

    // 居中：(100-40)/2 = 30
    try std.testing.expectEqual(@as(f32, 30), c1.node.bounds.left);
    try std.testing.expectEqual(@as(f32, 30), c1.node.bounds.top);
}
