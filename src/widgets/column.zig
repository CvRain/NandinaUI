//! widgets/column —— Column 容器（垂直堆叠子节点）
//!
//! 把子节点按垂直方向依次排列，主轴（Y 轴）对齐方式可配，交叉轴（X 轴）默认 stretch。
//! 依赖 layout.flex 求解器做实际计算，本层只做 measure/layout 的 vtable 桥接。
//!
//! 当前为最简版，后续可扩展 gap/alignment 的响应式输入。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const layout = @import("../layout/layout.zig");
const node_mod = @import("../runtime/node.zig");

const Allocator = std.mem.Allocator;
const Size = foundation.Size;
const Rect = foundation.Rect;
const Constraints = layout.Constraints;
const Node = node_mod.Node;

/// Column 容器属性。
pub const ColumnProps = struct {
    /// 子项间距（像素）。
    gap: f32 = 0,
    /// 主轴（垂直）对齐。
    main_align: layout.Align = .start,
    /// 交叉轴（水平）对齐。
    cross_align: layout.Align = .stretch,
};

/// Column 容器：垂直堆叠子节点。
pub const Column = struct {
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

    pub fn create(allocator: Allocator, props: ColumnProps) !*Column {
        const self = try allocator.create(Column);
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
        const self: *Column = @fieldParentPtr("node", node);
        var total_h: f32 = 0;
        var max_w: f32 = 0;
        for (node.children.items, 0..) |child, i| {
            const loose = constraints.loosen();
            const cs = child.measure(loose);
            total_h += cs.height;
            if (i > 0) total_h += self.gap;
            if (cs.width > max_w) max_w = cs.width;
        }
        return constraints.constrain(.{ .width = max_w, .height = total_h });
    }

    fn layoutImpl(node: *Node) void {
        const self: *Column = @fieldParentPtr("node", node);
        const n = node.children.items.len;
        if (n == 0) return;

        var specs_buf: [64]layout.flex.ChildSpec = undefined;
        var frames_buf: [64]Rect = undefined;

        for (node.children.items, 0..) |child, i| {
            specs_buf[i] = .{ .preferred = child.measured };
        }

        layout.flex.solve(.{
            .axis = .column,
            .gap = self.gap,
            .main_align = self.main_align,
            .cross_align = self.cross_align,
        }, node.bounds, specs_buf[0..n], frames_buf[0..n]);

        for (node.children.items, 0..) |child, i| {
            child.setBounds(frames_buf[i]);
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        allocator.destroy(@as(*Column, @fieldParentPtr("node", node)));
    }
};
