//! widgets/row —— Row 容器（水平排列子节点）
//!
//! 把子节点按水平方向依次排列，主轴（X 轴）对齐方式可配，交叉轴（Y 轴）默认 start。
//! 依赖 layout.flex 求解器做实际计算，本层只做 measure/layout 的 vtable 桥接。
//!
//! 与 Column 对称：Column 沿垂直主轴堆叠，Row 沿水平主轴排列。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const layout = @import("../layout/layout.zig");
const node_mod = @import("../runtime/node.zig");

const Allocator = std.mem.Allocator;
const Size = foundation.Size;
const Rect = foundation.Rect;
const Constraints = layout.Constraints;
const Node = node_mod.Node;

/// Row 容器属性。
pub const RowProps = struct {
    /// 子项间距（像素）。
    gap: f32 = 0,
    /// 主轴（水平）对齐。
    main_align: layout.Align = .start,
    /// 交叉轴（垂直）对齐。
    cross_align: layout.Align = .start,
};

/// Row 容器：水平排列子节点。
pub const Row = struct {
    node: Node,
    allocator: Allocator,
    gap: f32,
    main_align: layout.Align,
    cross_align: layout.Align,

    const vtable = node_mod.VTable{
        .measure = measureImpl,
        .layout = layoutImpl,
        .deinit = deinitImpl,
    };

    pub fn create(allocator: Allocator, props: RowProps) !*Row {
        const self = try allocator.create(Row);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .gap = props.gap,
            .main_align = props.main_align,
            .cross_align = props.cross_align,
        };
        return self;
    }

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *Row = @fieldParentPtr("node", node);
        var total_w: f32 = 0;
        var max_h: f32 = 0;
        for (node.children.items, 0..) |child, i| {
            const loose = constraints.loosen();
            const cs = child.measure(loose);
            total_w += cs.width;
            if (i > 0) total_w += self.gap;
            if (cs.height > max_h) max_h = cs.height;
        }
        return constraints.constrain(.{ .width = total_w, .height = max_h });
    }

    fn layoutImpl(node: *Node) void {
        const self: *Row = @fieldParentPtr("node", node);
        const n = node.children.items.len;
        if (n == 0) return;

        var specs_buf: [64]layout.flex.ChildSpec = undefined;
        var frames_buf: [64]Rect = undefined;

        for (node.children.items, 0..) |child, i| {
            specs_buf[i] = .{ .preferred = child.measured };
        }

        layout.flex.solve(.{
            .axis = .row,
            .gap = self.gap,
            .main_align = self.main_align,
            .cross_align = self.cross_align,
        }, node.bounds, specs_buf[0..n], frames_buf[0..n]);

        for (node.children.items, 0..) |child, i| {
            child.setBounds(frames_buf[i]);
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        allocator.destroy(@as(*Row, @fieldParentPtr("node", node)));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const reactive = @import("../reactive/reactive.zig");
const surface_mod = @import("surface.zig");

test "Row 水平排列子节点" {
    const a = std.testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    const row = try Row.create(a, .{ .gap = 10 });
    defer row.node.deinitTree(a);

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
    try row.node.addChild(a, &c1.node);

    // measure 子节点先得到 measured
    c1.node.measured = .{ .width = 40, .height = 20 };

    row.node.setBounds(Rect.fromXywh(0, 0, 200, 50));
    row.node.layout();

    // 单子节点应从左侧 0 开始
    try std.testing.expectEqual(@as(f32, 0), c1.node.bounds.left);
}
