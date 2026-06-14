//! widgets/card —— 卡片组件（M5 响应式版）
//!
//! Card 是带 title + description header 的结构化容器：
//!
//! ```
//! ┌──────────────────────────────┐
//! │ Title（标题，较大字体）       │
//! │ Description（副标题）        │
//! │                              │
//! │  Children（内容区）          │
//! │                              │
//! └──────────────────────────────┘
//! ```
//!
//! 所有视觉属性通过 `ReadSignal` 输入，内部 `EffectScope` 追踪变化并触发
//! `markLayoutDirty`，实现 M5 响应式数据流。
//!
//! 依赖方向：widgets 依赖 foundation / reactive / layout / render / runtime / text。

const std = @import("std");
const foundation = @import("../foundation/foundation.zig");
const reactive = @import("../reactive/reactive.zig");
const layout_mod = @import("../layout/layout.zig");
const render = @import("../render/render.zig");
const runtime = @import("../runtime/runtime.zig");
const text_mod = @import("../text/text.zig");

const Allocator = std.mem.Allocator;
const Color = foundation.Color;
const Insets = foundation.Insets;
const Rect = foundation.Rect;
const Size = foundation.Size;
const Constraints = layout_mod.Constraints;
const Scene = render.Scene;
const Node = runtime.Node;
const VTable = runtime.VTable;

/// 灰色（副标题默认颜色）。
const default_description_color: Color = Color.rgba8(107, 114, 128, 255);

// ── 公共类型 ─────────────────────────────────────────────────────────────────

/// Card 的输入属性，全部为只读信号，由调用方持有后备 Signal 的生命周期。
pub const CardProps = struct {
    title: reactive.ReadSignal([]const u8),
    description: reactive.ReadSignal([]const u8),
    bg_color: reactive.ReadSignal(Color),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(Insets),
    title_font_size: reactive.ReadSignal(f32),
    description_font_size: reactive.ReadSignal(f32),
};

/// 卡片组件。
pub const Card = struct {
    node: Node,
    allocator: Allocator,

    // 输入属性（只读信号，调用方持有后备 Signal）
    title: reactive.ReadSignal([]const u8),
    description: reactive.ReadSignal([]const u8),
    bg_color: reactive.ReadSignal(Color),
    corner_radius: reactive.ReadSignal(f32),
    padding: reactive.ReadSignal(Insets),
    title_font_size: reactive.ReadSignal(f32),
    description_font_size: reactive.ReadSignal(f32),

    // 响应式作用域
    scope: reactive.EffectScope,

    // ── vtable ────────────────────────────────────────────────────────────────

    const vtable = VTable{
        .measure = measureImpl,
        .layout = layoutImpl,
        .paint = paintImpl,
        .deinit = deinitImpl,
    };

    // ── 构造 ──────────────────────────────────────────────────────────────────

    pub fn create(
        allocator: Allocator,
        g: *reactive.Graph,
        props: CardProps,
    ) !*Card {
        const self = try allocator.create(Card);
        self.* = .{
            .node = .{ .vtable = &vtable },
            .allocator = allocator,
            .title = props.title,
            .description = props.description,
            .bg_color = props.bg_color,
            .corner_radius = props.corner_radius,
            .padding = props.padding,
            .title_font_size = props.title_font_size,
            .description_font_size = props.description_font_size,
            .scope = reactive.EffectScope.init(g),
        };
        _ = try self.scope.add(self, struct {
            fn f(s: *Card) void {
                // 建立对所有输入信号的依赖追踪
                _ = s.title.get();
                _ = s.description.get();
                _ = s.bg_color.get();
                _ = s.corner_radius.get();
                _ = s.padding.get();
                _ = s.title_font_size.get();
                _ = s.description_font_size.get();
                s.node.markLayoutDirty();
            }
        }.f);
        return self;
    }

    // ── vtable 实现 ───────────────────────────────────────────────────────────

    fn headerHeight(self: *Card) f32 {
        const tfs = self.title_font_size.peek();
        const dfs = self.description_font_size.peek();
        const title_h = if (self.title.peek().len > 0) tfs * 1.2 else 0;
        const desc_h = if (self.description.peek().len > 0) dfs * 1.2 else 0;
        const spacing: f32 = if (title_h > 0 and desc_h > 0) 4 else 0;
        return title_h + spacing + desc_h;
    }

    fn measureImpl(node: *Node, constraints: Constraints) Size {
        const self: *Card = @fieldParentPtr("node", node);
        const pad = self.padding.peek();

        const header_h = self.headerHeight();

        // 度量子节点
        const avail_w = if (constraints.hasBoundedWidth())
            @max(0.0, constraints.max_width - pad.horizontal())
        else
            std.math.inf(f32);
        var content_h: f32 = 0;
        for (node.children.items) |child| {
            const csz = child.measure(Constraints.loose(avail_w, std.math.inf(f32)));
            if (csz.height > content_h) content_h = csz.height;
        }

        const gap: f32 = if (header_h > 0 and content_h > 0) 4 else 0;
        const natural = Size{
            .width = if (constraints.hasBoundedWidth())
                constraints.max_width
            else
                avail_w + pad.horizontal(),
            .height = pad.top + header_h + gap + content_h + pad.bottom,
        };
        return constraints.constrain(natural);
    }

    fn layoutImpl(node: *Node) void {
        const self: *Card = @fieldParentPtr("node", node);
        const bounds = node.bounds;
        const pad = self.padding.peek();
        const header_h = self.headerHeight();
        const gap: f32 = if (header_h > 0) 4 else 0;

        const content_y = bounds.top + pad.top + header_h + gap;
        const content_x = bounds.left + pad.left;
        const content_w = @max(0, bounds.width() - pad.horizontal());
        const content_h = @max(0, bounds.bottom - content_y - pad.bottom);

        for (node.children.items) |child| {
            child.setBounds(Rect.fromXywh(content_x, content_y, content_w, content_h));
            child.layout();
        }
    }

    fn paintImpl(node: *Node, scene: *Scene) anyerror!void {
        const self: *Card = @fieldParentPtr("node", node);
        const bounds = node.bounds;
        const pad = self.padding.peek();

        // 背景圆角矩形
        try scene.fillRoundedRect(bounds, self.corner_radius.peek(), self.bg_color.peek());

        // title 文本
        const title = self.title.peek();
        if (title.len > 0) {
            const tfs = self.title_font_size.peek();
            try scene.drawText(.{
                .text = title,
                .x = bounds.left + pad.left,
                .y = bounds.top + pad.top,
                .font_size = tfs,
                .color = Color.black,
                .layout_width = @max(0, bounds.width() - pad.horizontal()),
                .layout_height = tfs * 1.2,
            });
        }

        // description 文本（在 title 下方）
        const desc = self.description.peek();
        if (desc.len > 0) {
            const tfs = self.title_font_size.peek();
            const dfs = self.description_font_size.peek();
            const title_h = if (title.len > 0) tfs * 1.2 + 4 else 0;
            try scene.drawText(.{
                .text = desc,
                .x = bounds.left + pad.left,
                .y = bounds.top + pad.top + title_h,
                .font_size = dfs,
                .color = default_description_color,
                .layout_width = @max(0, bounds.width() - pad.horizontal()),
                .layout_height = dfs * 1.2,
            });
        }
    }

    fn deinitImpl(node: *Node, allocator: Allocator) void {
        const self: *Card = @fieldParentPtr("node", node);
        self.scope.deinit();
        // ReadSignal 不持有所有权，无需 deinit
        allocator.destroy(self);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

const testing = std.testing;

/// 辅助：为每个属性创建 Signal 并组装 CardProps
const TestSignals = struct {
    title_sig: reactive.Signal([]const u8),
    desc_sig: reactive.Signal([]const u8),
    bg_sig: reactive.Signal(Color),
    cr_sig: reactive.Signal(f32),
    pad_sig: reactive.Signal(Insets),
    tfs_sig: reactive.Signal(f32),
    dfs_sig: reactive.Signal(f32),

    fn init(g: *reactive.Graph) TestSignals {
        return .{
            .title_sig = reactive.Signal([]const u8).init(g, "Title"),
            .desc_sig = reactive.Signal([]const u8).init(g, "Description"),
            .bg_sig = reactive.Signal(Color).init(g, Color.white),
            .cr_sig = reactive.Signal(f32).init(g, 8.0),
            .pad_sig = reactive.Signal(Insets).init(g, Insets.all(16)),
            .tfs_sig = reactive.Signal(f32).init(g, 18.0),
            .dfs_sig = reactive.Signal(f32).init(g, 14.0),
        };
    }

    fn deinit(self: *TestSignals) void {
        self.title_sig.deinit();
        self.desc_sig.deinit();
        self.bg_sig.deinit();
        self.cr_sig.deinit();
        self.pad_sig.deinit();
        self.tfs_sig.deinit();
        self.dfs_sig.deinit();
    }

    fn props(self: *TestSignals) CardProps {
        return .{
            .title = self.title_sig.asReadonly(),
            .description = self.desc_sig.asReadonly(),
            .bg_color = self.bg_sig.asReadonly(),
            .corner_radius = self.cr_sig.asReadonly(),
            .padding = self.pad_sig.asReadonly(),
            .title_font_size = self.tfs_sig.asReadonly(),
            .description_font_size = self.dfs_sig.asReadonly(),
        };
    }
};

test "Card 创建与 deinit 无内存泄漏" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const card = try Card.create(a, &g, sigs.props());
    card.node.deinitTree(a);
}

test "Card measure 包含 padding + header 高度" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const card = try Card.create(a, &g, sigs.props());
    defer card.node.deinitTree(a);

    const sz = card.node.measure(Constraints.loose(300, std.math.inf(f32)));
    // 至少: pad.top(16) + title_h(18*1.2) + spacing(4) + desc_h(14*1.2) + pad.bottom(16) > 70
    try testing.expect(sz.height > 70);
    try testing.expect(sz.width > 0);
}

test "Card paint 输出 fill_rounded_rect + 2x draw_text" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const card = try Card.create(a, &g, sigs.props());
    defer card.node.deinitTree(a);

    card.node.setBounds(Rect.fromXywh(0, 0, 300, 200));

    var scene = render.Scene.init(a);
    defer scene.deinit();
    try card.node.paint(&scene);

    // 1 fill_rounded_rect + 2 draw_text = 3 commands
    try testing.expectEqual(@as(usize, 3), scene.count());
    try testing.expect(scene.commands.items[0] == .fill_rounded_rect);
    try testing.expect(scene.commands.items[1] == .draw_text);
    try testing.expect(scene.commands.items[2] == .draw_text);

    // 验证 title 文本内容
    try testing.expectEqualStrings("Title", scene.commands.items[1].draw_text.text);
}

test "Card title 信号变化 → markLayoutDirty" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const card = try Card.create(a, &g, sigs.props());
    defer card.node.deinitTree(a);

    card.node.layout_dirty = false;
    card.node.paint_dirty = false;

    sigs.title_sig.update(struct {
        fn f(v: *[]const u8) void {
            v.* = "New Title";
        }
    }.f);

    try testing.expect(card.node.layout_dirty);
}

test "Card bg_color 信号变化 → markLayoutDirty" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const card = try Card.create(a, &g, sigs.props());
    defer card.node.deinitTree(a);

    card.node.layout_dirty = false;
    card.node.paint_dirty = false;

    sigs.bg_sig.set(Color.black);
    try testing.expect(card.node.layout_dirty);
}

test "Card corner_radius 信号变化 → 触发重绘" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var sigs = TestSignals.init(&g);
    defer sigs.deinit();

    const card = try Card.create(a, &g, sigs.props());
    defer card.node.deinitTree(a);

    card.node.layout_dirty = false;
    card.node.paint_dirty = false;

    sigs.cr_sig.set(16.0);
    try testing.expect(card.node.paint_dirty or card.node.layout_dirty);
}

test "Card description 为空时 paint 只输出 2 条命令" {
    const a = testing.allocator;
    var g = reactive.Graph.init(a);
    defer g.deinit();

    var title_sig = reactive.Signal([]const u8).init(&g, "Hello");
    defer title_sig.deinit();
    var desc_sig = reactive.Signal([]const u8).init(&g, ""); // 空描述
    defer desc_sig.deinit();
    var bg_sig = reactive.Signal(Color).init(&g, Color.white);
    defer bg_sig.deinit();
    var cr_sig = reactive.Signal(f32).init(&g, 8.0);
    defer cr_sig.deinit();
    var pad_sig = reactive.Signal(Insets).init(&g, Insets.all(16));
    defer pad_sig.deinit();
    var tfs_sig = reactive.Signal(f32).init(&g, 18.0);
    defer tfs_sig.deinit();
    var dfs_sig = reactive.Signal(f32).init(&g, 14.0);
    defer dfs_sig.deinit();

    const card = try Card.create(a, &g, .{
        .title = title_sig.asReadonly(),
        .description = desc_sig.asReadonly(),
        .bg_color = bg_sig.asReadonly(),
        .corner_radius = cr_sig.asReadonly(),
        .padding = pad_sig.asReadonly(),
        .title_font_size = tfs_sig.asReadonly(),
        .description_font_size = dfs_sig.asReadonly(),
    });
    defer card.node.deinitTree(a);

    card.node.setBounds(Rect.fromXywh(0, 0, 300, 200));

    var scene = render.Scene.init(a);
    defer scene.deinit();
    try card.node.paint(&scene);

    // 1 fill_rounded_rect + 1 draw_text (title only) = 2
    try testing.expectEqual(@as(usize, 2), scene.count());
}
