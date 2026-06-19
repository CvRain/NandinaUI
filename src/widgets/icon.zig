//! widgets/icon —— 图标 primitive
//!
//! `Icon` 是最基础的视觉指示器：纯色矩形/圆形，用于状态点、图标占位、
//! 色彩指示等场景。不包含文本，不处理交互。
//!
//! 使用示例：
//! ```zig
//! var color_sig = reactive.Signal(Color).init(&g, Color.blue);
//! defer color_sig.deinit();
//!
//! const icon = try Icon.create(allocator, &g, .{
//!     .color = color_sig.asReadonly(),
//!     .size = readOnly(owner, f32, &g, 16),
//! });
//! defer icon.node.deinitTree(allocator);
//! ```

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const render = @import("../render/render.zig");
const layout = @import("../layout/layout.zig");
const runtime = @import("../runtime/runtime.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Rect = foundation.Rect;
const Size = foundation.Size;
const Constraints = layout.Constraints;
const Scene = render.Scene;
const Node = runtime.Node;
const VTable = runtime.VTable;

// ── 公共类型 ─────────────────────────────────────────────────────────────────

/// 图标形状。
pub const IconShape = enum {
    /// 实心矩形（默认）。
    rect,
    /// 实心圆形（以 size 为直径）。
    circle,
};

/// Icon 的构造属性，全部为只读信号。
pub const IconProps = struct {
    /// 图标颜色（默认蓝色）。
    color: reactive.ReadSignal(Color),
    /// 图标尺寸（宽 = 高 = size，默认 16 px）。
    size: reactive.ReadSignal(f32),
    /// 形状（非响应式，构造时固定）。
    shape: IconShape = .rect,
};

// ─────────────────────────────────────────────────────────────────────────────
// Icon
// ─────────────────────────────────────────────────────────────────────────────

pub const Icon = struct {
    node: Node,
    allocator: Allocator,

    // 响应式输入（只读视图）
    color: reactive.ReadSignal(Color),
    size: reactive.ReadSignal(f32),
    shape: IconShape,

    // 响应式作用域
    scope: reactive.EffectScope,

    const vtable = VTable{
        .measure = measureImpl,
        .paint = paintImpl,
        .deinit = deinitImpl,
    };

    /// 创建并初始化 Icon。在 `g` 上注册合并 effect，任一信号变化时
    /// 自动调用 `node.markLayoutDirty()`。
    pub fn create(
        allocator: Allocator,
        g: *reactive.Graph,
        props: IconProps,
    ) !*Icon {
        const self = try allocator.create(Icon);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .color = props.color,
            .size = props.size,
            .shape = props.shape,
            .scope = reactive.EffectScope.init(g),
        };

        _ = try self.scope.add(self, struct {
            fn f(s: *Icon) void {
                _ = s.color.get();
                _ = s.size.get();
                s.node.markLayoutDirty();
            }
        }.f);

        return self;
    }

    // ── vtable 实现 ──────────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *Icon = @fieldParentPtr("node", node);
        const sz = self.size.peek();
        return constraints.constrain(.{ .width = sz, .height = sz });
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Icon = @fieldParentPtr("node", node);
        const c = self.color.peek();
        const b = node.bounds;

        switch (self.shape) {
            .rect => try scene.fillRect(b, c),
            .circle => {
                // 圆形：以 min(w,h) 为直径，在 bounds 内居中
                const r = @min(b.width(), b.height()) / 2.0;
                try scene.fillRoundedRect(b, r, c);
            },
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Icon = @fieldParentPtr("node", node);
        self.scope.deinit();
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Icon 创建与基本属性" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var color_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0xFF0000));
    defer color_sig.deinit();
    var size_sig = reactive.Signal(f32).init(&g, 24);
    defer size_sig.deinit();

    const icon = try Icon.create(allocator, &g, .{
        .color = color_sig.asReadonly(),
        .size = size_sig.asReadonly(),
        .shape = .circle,
    });
    defer icon.node.deinitTree(allocator);

    try std.testing.expectEqual(.circle, icon.shape);
    try std.testing.expectEqual(Color.fromHexRgb(0xFF0000), icon.color.peek());
    try std.testing.expectEqual(@as(f32, 24), icon.size.peek());
}

test "Icon measure 受约束限制" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var color_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0x0000FF));
    defer color_sig.deinit();
    var size_sig = reactive.Signal(f32).init(&g, 100);
    defer size_sig.deinit();

    const icon = try Icon.create(allocator, &g, .{
        .color = color_sig.asReadonly(),
        .size = size_sig.asReadonly(),
    });
    defer icon.node.deinitTree(allocator);

    // 紧缩约束：只能给 20x20
    const sz = icon.node.measure(Constraints.tight(.{ .width = 20, .height = 20 }));
    try std.testing.expectEqual(@as(f32, 20), sz.width);
    try std.testing.expectEqual(@as(f32, 20), sz.height);
}

test "Icon signal 变更触发 markLayoutDirty" {
    const allocator = std.testing.allocator;
    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var color_sig = reactive.Signal(Color).init(&g, Color.fromHexRgb(0xFF0000));
    defer color_sig.deinit();
    var size_sig = reactive.Signal(f32).init(&g, 16);
    defer size_sig.deinit();

    const icon = try Icon.create(allocator, &g, .{
        .color = color_sig.asReadonly(),
        .size = size_sig.asReadonly(),
    });
    defer icon.node.deinitTree(allocator);

    // 初始状态应该不脏（首次构造后 measure 会清除）
    icon.node.layout_dirty = false;

    // 修改 size 信号 → effect 触发 → markLayoutDirty
    size_sig.set(32);
    g.flush();
    try std.testing.expect(icon.node.layout_dirty);
}
