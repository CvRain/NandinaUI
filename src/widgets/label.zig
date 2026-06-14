//! widgets/label —— 响应式文本标签组件
//!
//! `Label` 接收三个只读 signal 输入（text / color / font_size），当任一 signal 变化时，
//! 内部 effect 自动失效文本测量缓存并把节点标记为布局脏，框架下一帧会重新 measure + paint。
//!
//! 使用示例：
//! ```zig
//! var text_sig = reactive.Signal([]const u8).init(&g, "Hello");
//! var color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
//! var fs_sig = reactive.Signal(f32).init(&g, 14.0);
//! defer text_sig.deinit();
//! defer color_sig.deinit();
//! defer fs_sig.deinit();
//!
//! const label = try Label.create(allocator, &g, .{
//!     .text = text_sig.asReadonly(),
//!     .color = color_sig.asReadonly(),
//!     .font_size = fs_sig.asReadonly(),
//! });
//! defer label.node.deinitTree(allocator);
//!
//! text_sig.set("World"); // -> effect 触发 -> cached_layout 失效 -> markLayoutDirty
//! ```

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const text_mod = @import("../text/text.zig");
const runtime = @import("../runtime/runtime.zig");
const layout_mod = @import("../layout/layout.zig");

const Allocator = std.mem.Allocator;
const Node = runtime.Node;
const Size = foundation.Size;
const Constraints = layout_mod.Constraints;
const Scene = @import("../render/scene.zig").Scene;

// ── 公共类型 ─────────────────────────────────────────────────────────────────

/// Label 的构造属性：所有主要显示属性均以只读 signal 传入，确保响应式联动。
pub const LabelProps = struct {
    text: reactive.ReadSignal([]const u8),
    color: reactive.ReadSignal(foundation.Color),
    font_size: reactive.ReadSignal(f32),
    /// 溢出策略（非响应式，构造时固定；有需要可升级为 ReadSignal）。
    overflow: text_mod.Overflow = .ellipsis,
    /// 最大行数（非响应式；0 表示不限制）。
    max_lines: usize = 0,
};

/// 响应式文本标签。
pub const Label = struct {
    node: Node,
    allocator: Allocator,

    // ── 响应式输入（只读视图，不持有所有权）────────────────────────────────
    text: reactive.ReadSignal([]const u8),
    color: reactive.ReadSignal(foundation.Color),
    font_size: reactive.ReadSignal(f32),

    // ── 普通属性 ──────────────────────────────────────────────────────────
    overflow: text_mod.Overflow,
    max_lines: usize,

    // ── 文本测量缓存 ───────────────────────────────────────────────────────
    cached_layout: ?text_mod.TextLayout,
    /// 上次 measure 使用的约束（用于判断缓存是否仍有效）。
    cached_constraints: ?text_mod.Constraints,

    // ── 响应式作用域 ───────────────────────────────────────────────────────
    scope: reactive.EffectScope,

    // ── vtable ───────────────────────────────────────────────────────────
    const vtable = runtime.VTable{
        .measure = measureImpl,
        .paint = paintImpl,
        .deinit = deinitImpl,
    };

    // ── 生命周期 ──────────────────────────────────────────────────────────

    /// 创建一个 Label 并注册响应式 effect。
    /// 返回的指针由调用方管理；使用 `node.deinitTree(allocator)` 释放。
    pub fn create(allocator: Allocator, g: *reactive.Graph, props: LabelProps) !*Label {
        const self = try allocator.create(Label);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .text = props.text,
            .color = props.color,
            .font_size = props.font_size,
            .overflow = props.overflow,
            .max_lines = props.max_lines,
            .cached_layout = null,
            .cached_constraints = null,
            .scope = reactive.EffectScope.init(g),
        };
        // Effect：追踪所有三个 signal；任一变化 → 失效缓存 → 标记布局脏。
        _ = try self.scope.add(self, struct {
            fn f(s: *Label) void {
                // 建立依赖（需用 .get() 而非 .peek()）
                _ = s.text.get();
                _ = s.color.get();
                _ = s.font_size.get();
                // 失效缓存
                if (s.cached_layout) |*cl| {
                    cl.deinit();
                    s.cached_layout = null;
                }
                s.cached_constraints = null;
                // 通知框架需要重新布局与绘制
                s.node.markLayoutDirty();
            }
        }.f);
        return self;
    }

    // ── VTable 实现 ───────────────────────────────────────────────────────

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *Label = @fieldParentPtr("node", node);

        // 把 layout.Constraints（inf = 无界）转换为 text.Constraints（0 = 无界）
        const text_constraints = text_mod.Constraints{
            .max_width = if (constraints.hasBoundedWidth()) constraints.max_width else 0,
            .max_lines = self.max_lines,
        };

        const txt = self.text.peek();
        const fs = self.font_size.peek();

        // 如果缓存有效（信号未变且约束未变），直接复用。
        if (self.cached_layout != null) {
            const cc = self.cached_constraints orelse text_mod.Constraints{};
            if (cc.max_width == text_constraints.max_width and
                cc.max_lines == text_constraints.max_lines)
            {
                return self.cached_layout.?.size;
            }
        }

        const style = text_mod.TextStyle{ .font_size = fs };
        var metrics_backend = text_mod.MonospaceMetrics{};
        const metrics = metrics_backend.interface();

        // 失效旧缓存（约束变化触发）
        if (self.cached_layout) |*cl| cl.deinit();

        self.cached_layout = text_mod.measure(
            self.allocator,
            txt,
            style,
            self.overflow,
            text_constraints,
            metrics,
        ) catch |err| {
            // 测量失败时回退到空尺寸，避免崩溃。
            std.log.err("Label.measure failed: {}", .{err});
            return .{};
        };
        self.cached_constraints = text_constraints;

        return self.cached_layout.?.size;
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Label = @fieldParentPtr("node", node);

        const txt = self.text.peek();
        const col = self.color.peek();
        const fs = self.font_size.peek();

        try scene.drawText(.{
            .text = txt,
            .x = node.bounds.x(),
            .y = node.bounds.y(),
            .font_size = fs,
            .color = col,
            .layout_width = node.bounds.width(),
            .layout_height = node.bounds.height(),
        });
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Label = @fieldParentPtr("node", node);
        // 先 dispose effect（在 signal 仍存活时解绑依赖）
        self.scope.deinit();
        // 释放测量缓存
        if (self.cached_layout) |*cl| cl.deinit();
        // ReadSignal 无需 deinit（不持有所有权）
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Label 创建与基本 measure" {
    const allocator = std.testing.allocator;

    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var text_sig = reactive.Signal([]const u8).init(&g, "Hello");
    defer text_sig.deinit();
    var color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer color_sig.deinit();
    var fs_sig = reactive.Signal(f32).init(&g, 14.0);
    defer fs_sig.deinit();

    const label = try Label.create(allocator, &g, .{
        .text = text_sig.asReadonly(),
        .color = color_sig.asReadonly(),
        .font_size = fs_sig.asReadonly(),
    });
    defer label.node.deinitTree(allocator);

    const sz = label.node.measure(Constraints.loose(200, 100));
    try std.testing.expect(sz.width > 0);
    try std.testing.expect(sz.height > 0);
}

test "Label text 变化使缓存失效并标记布局脏" {
    const allocator = std.testing.allocator;

    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var text_sig = reactive.Signal([]const u8).init(&g, "Hello");
    defer text_sig.deinit();
    var color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer color_sig.deinit();
    var fs_sig = reactive.Signal(f32).init(&g, 14.0);
    defer fs_sig.deinit();

    const label = try Label.create(allocator, &g, .{
        .text = text_sig.asReadonly(),
        .color = color_sig.asReadonly(),
        .font_size = fs_sig.asReadonly(),
    });
    defer label.node.deinitTree(allocator);

    // 先 measure，建立缓存
    _ = label.node.measure(Constraints.loose(200, 100));
    try std.testing.expect(label.cached_layout != null);

    // 清除 dirty 标志
    label.node.layout_dirty = false;
    label.node.paint_dirty = false;

    // 改变 text signal → effect 触发 → 缓存失效 + 标脏
    text_sig.set("A much longer string that changes the measured width");

    try std.testing.expect(label.cached_layout == null);
    try std.testing.expect(label.node.layout_dirty);
    try std.testing.expect(label.node.paint_dirty);
}

test "Label font_size 变化使缓存失效" {
    const allocator = std.testing.allocator;

    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var text_sig = reactive.Signal([]const u8).init(&g, "Hello");
    defer text_sig.deinit();
    var color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer color_sig.deinit();
    var fs_sig = reactive.Signal(f32).init(&g, 14.0);
    defer fs_sig.deinit();

    const label = try Label.create(allocator, &g, .{
        .text = text_sig.asReadonly(),
        .color = color_sig.asReadonly(),
        .font_size = fs_sig.asReadonly(),
    });
    defer label.node.deinitTree(allocator);

    _ = label.node.measure(Constraints.loose(200, 100));
    try std.testing.expect(label.cached_layout != null);

    label.node.layout_dirty = false;
    label.node.paint_dirty = false;

    // 改变 font_size → 缓存失效 + 标脏
    fs_sig.set(28.0);

    try std.testing.expect(label.cached_layout == null);
    try std.testing.expect(label.node.layout_dirty);
}

test "Label color 变化标记节点脏" {
    const allocator = std.testing.allocator;

    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var text_sig = reactive.Signal([]const u8).init(&g, "Hello");
    defer text_sig.deinit();
    var color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer color_sig.deinit();
    var fs_sig = reactive.Signal(f32).init(&g, 14.0);
    defer fs_sig.deinit();

    const label = try Label.create(allocator, &g, .{
        .text = text_sig.asReadonly(),
        .color = color_sig.asReadonly(),
        .font_size = fs_sig.asReadonly(),
    });
    defer label.node.deinitTree(allocator);

    label.node.layout_dirty = false;
    label.node.paint_dirty = false;

    // 改变 color → effect 触发 → markLayoutDirty（含 paint_dirty）
    color_sig.set(foundation.Color.white);

    try std.testing.expect(label.node.paint_dirty);
    try std.testing.expect(label.node.layout_dirty);
}

test "Label paint 输出 draw_text 命令" {
    const allocator = std.testing.allocator;

    var g = reactive.Graph.init(allocator);
    defer g.deinit();

    var text_sig = reactive.Signal([]const u8).init(&g, "Hi");
    defer text_sig.deinit();
    var color_sig = reactive.Signal(foundation.Color).init(&g, foundation.Color.black);
    defer color_sig.deinit();
    var fs_sig = reactive.Signal(f32).init(&g, 16.0);
    defer fs_sig.deinit();

    const label = try Label.create(allocator, &g, .{
        .text = text_sig.asReadonly(),
        .color = color_sig.asReadonly(),
        .font_size = fs_sig.asReadonly(),
    });
    defer label.node.deinitTree(allocator);

    var scene = @import("../render/scene.zig").Scene.init(allocator);
    defer scene.deinit();

    try label.node.paint(&scene);

    try std.testing.expectEqual(@as(usize, 1), scene.count());
    try std.testing.expect(scene.commands.items[0] == .draw_text);
    try std.testing.expectEqualStrings("Hi", scene.commands.items[0].draw_text.text);
    try std.testing.expectEqual(@as(f32, 16.0), scene.commands.items[0].draw_text.font_size);
}
