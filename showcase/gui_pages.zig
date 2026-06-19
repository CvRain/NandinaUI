//! showcase/gui_pages —— Showcase GUI 页面构建
//!
//! 本文件包含所有页面的构建函数，展示框架核心能力。
//! 由 `showcase/gui.zig` 导入使用，不属于 NandinaUI 库。

const std = @import("std");
const nandina = @import("NandinaUI");

const app = nandina.app;
const authoring = app.authoring;
const widgets = nandina.widgets;
const runtime = nandina.runtime;
const reactive = nandina.reactive;
const f = nandina.foundation;

const Color = f.Color;
const Insets = f.Insets;

// ═════════════════════════════════════════════════════════════════════════════
// Catppuccin Mocha 调色板
// ═════════════════════════════════════════════════════════════════════════════

pub const C = Colors{
    .base = Color.fromHexRgb(0x1E1E2E),
    .mantle = Color.fromHexRgb(0x181825),
    .crust = Color.fromHexRgb(0x11111B),
    .text = Color.fromHexRgb(0xCDD6F4),
    .blue = Color.fromHexRgb(0x89B4FA),
    .green = Color.fromHexRgb(0xA6E3A1),
    .red = Color.fromHexRgb(0xF38BA8),
    .peach = Color.fromHexRgb(0xFAB387),
    .mauve = Color.fromHexRgb(0xCBA6F7),
    .yellow = Color.fromHexRgb(0xF9E2AF),
    .teal = Color.fromHexRgb(0x94E2D5),
    .surface0 = Color.fromHexRgb(0x313244),
    .surface1 = Color.fromHexRgb(0x45475A),
    .surface2 = Color.fromHexRgb(0x585B70),
};

pub const Colors = struct {
    base: Color,
    mantle: Color,
    crust: Color,
    text: Color,
    blue: Color,
    green: Color,
    red: Color,
    peach: Color,
    mauve: Color,
    yellow: Color,
    teal: Color,
    surface0: Color,
    surface1: Color,
    surface2: Color,
};

// ═════════════════════════════════════════════════════════════════════════════
// 辅助
// ═════════════════════════════════════════════════════════════════════════════

/// 分区标题 — 带彩色强调的 section header。
pub fn sectionHeader(owner: *app.SignalOwner, a: std.mem.Allocator, g: *reactive.Graph, title: []const u8, accent: Color) !*runtime.Node {
    const hdr = try authoring.surface(owner, a, g, .{ .bg_color = accent.withAlpha(0.15), .corner_radius = 4, .padding = Insets.symmetric(12, 6) });
    try hdr.addChild(a, try authoring.label(owner, a, g, title, .{ .color = accent, .font_size = 15 }));
    return hdr;
}

// ═════════════════════════════════════════════════════════════════════════════
// 页面构建函数
// ═════════════════════════════════════════════════════════════════════════════

/// 概述页
pub fn buildOverview(a: std.mem.Allocator, g: *reactive.Graph, owner: *app.SignalOwner) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 20 });

    try col.addChild(a, try authoring.label(owner, a, g, "NandinaUI", .{ .color = C.text, .font_size = 30 }));
    try col.addChild(a, try authoring.label(owner, a, g, "Zig 原生 UI 框架 · 响应式 · 组件化 · 多后端渲染", .{ .color = C.blue, .font_size = 14 }));

    try col.addChild(a, try sectionHeader(owner, a, g, "分层架构", C.mauve));
    try col.addChild(a, try authoring.card(owner, a, g, "依赖方向", "foundation → reactive / render / layout / theme / text → runtime → widgets → app", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));

    try col.addChild(a, try sectionHeader(owner, a, g, "开发进度", C.green));
    const status_items = [_]struct { label: []const u8 }{
        .{ .label = "M0 ✅ Foundation — 几何 / 颜色" },
        .{ .label = "M1 ✅ Reactive  — Signal / Computed / Effect" },
        .{ .label = "M2 ✅ Render    — Scene / Backend 接口 / SoftwareBackend" },
        .{ .label = "M2 ✅ Layout    — Constraints / Flex / Flow / Anchors" },
        .{ .label = "M3 ✅ Theme     — Tokens / Palette / Resolver" },
        .{ .label = "M3 ✅ Text      — 字体测量 / HarfBuzz+FreeType 后端" },
        .{ .label = "M4 ✅ Runtime   — Node 树 / 事件 / Tree 主循环" },
        .{ .label = "M5 ✅ Widgets   — Surface / Label / Button / Card / Panel / Icon / TextField / Checkbox / Switch / Field" },
        .{ .label = "M6 ✅ App       — Page / Router / PageHost / Authoring DSL" },
        .{ .label = "P1 ✅ 平台      — SDL3 窗口 + 字体后端" },
        .{ .label = "P2 ✅ C ABI     — 导出层（libnandina_abi.a）" },
        .{ .label = "P3 ✅ ThorVG   — 矢量渲染后端" },
    };
    const status_col = try authoring.column(a, .{ .gap = 6 });
    for (status_items) |item| {
        try status_col.addChild(a, try authoring.label(owner, a, g, item.label, .{ .color = C.text, .font_size = 13 }));
    }
    try col.addChild(a, status_col);

    return col;
}

/// Widgets 页 — 组件库画廊
pub fn buildWidgets(a: std.mem.Allocator, g: *reactive.Graph, owner: *app.SignalOwner) !*runtime.Node {
    const page = try authoring.column(a, .{ .gap = 20 });

    try page.addChild(a, try authoring.label(owner, a, g, "组件库", .{ .color = C.text, .font_size = 24 }));
    try page.addChild(a, try authoring.label(owner, a, g, "Primitives · Controls · Inputs · Form", .{ .color = C.blue, .font_size = 13 }));

    // ── Primitives ────────────────────────────────────────────────────────
    try page.addChild(a, try sectionHeader(owner, a, g, "Primitives", C.mauve));
    {
        const row = try authoring.column(a, .{ .gap = 10 });
        const s = try authoring.surface(owner, a, g, .{ .bg_color = C.blue.withAlpha(0.2), .corner_radius = 6, .padding = Insets.all(12), .border_color = C.blue.withAlpha(0.4), .border_width = 1 });
        try s.addChild(a, try authoring.label(owner, a, g, "Surface", .{ .color = C.text, .font_size = 12 }));
        try row.addChild(a, s);
        const icons = [_]struct { name: []const u8, color: Color, shape: widgets.IconShape }{
            .{ .name = "  ● 指示点", .color = C.green, .shape = .circle },
            .{ .name = "  ■ 标记", .color = C.blue, .shape = .rect },
            .{ .name = "  ● 警告", .color = C.peach, .shape = .circle },
        };
        for (icons) |ic| {
            const item = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 4, .padding = Insets.all(8) });
            try item.addChild(a, try authoring.label(owner, a, g, ic.name, .{ .color = C.text, .font_size = 12 }));
            try item.addChild(a, try authoring.icon(owner, a, g, .{ .color = ic.color, .size = 10, .shape = ic.shape }));
            try row.addChild(a, item);
        }
        try page.addChild(a, row);
    }

    // ── Controls ──────────────────────────────────────────────────────────
    try page.addChild(a, try sectionHeader(owner, a, g, "Controls", C.teal));
    {
        const row = try authoring.column(a, .{ .gap = 10 });
        const lbl_box = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 6, .padding = Insets.all(10) });
        try lbl_box.addChild(a, try authoring.label(owner, a, g, "★ Label 文本标签", .{ .color = C.text, .font_size = 14 }));
        try row.addChild(a, lbl_box);

        const btn_box = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 6, .padding = Insets.all(10) });
        try btn_box.addChild(a, try authoring.button(owner, a, g, "  Button 按钮  ", .{
            .bg_color = C.blue,
            .bg_hover_color = Color.fromHexRgb(0x74C7EC),
            .bg_pressed_color = Color.fromHexRgb(0x89DCEB),
            .text_color = C.base,
            .font_size = 13,
            .corner_radius = 5,
            .padding = Insets.symmetric(16, 8),
        }));
        try row.addChild(a, btn_box);

        const pnl = try authoring.panel(owner, a, g, .{ .bg_color = C.mantle, .corner_radius = 6, .padding = Insets.all(12), .border_color = C.surface1, .border_width = 1 });
        try pnl.addChild(a, try authoring.label(owner, a, g, "Panel  面板容器", .{ .color = C.text, .font_size = 13 }));
        try row.addChild(a, pnl);

        try row.addChild(a, try authoring.card(owner, a, g, "Card 卡片", "标题 + 描述的结构化容器", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
        try page.addChild(a, row);
    }

    // ── Inputs ────────────────────────────────────────────────────────────
    try page.addChild(a, try sectionHeader(owner, a, g, "Inputs", C.green));
    {
        const row = try authoring.column(a, .{ .gap = 10 });

        const tf_box = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 6, .padding = Insets.all(10) });
        try tf_box.addChild(a, try authoring.label(owner, a, g, "TextField  输入框", .{ .color = C.text, .font_size = 12 }));
        const tf = try authoring.textField(owner, a, g, .{ .placeholder = "在此输入...", .min_width = 180 });
        try tf_box.addChild(a, &tf.node);
        try row.addChild(a, tf_box);

        const cb_row = try authoring.column(a, .{ .gap = 8 });
        {
            const cb_unchecked = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 6, .padding = Insets.all(10) });
            try cb_unchecked.addChild(a, try authoring.label(owner, a, g, "☐ Checkbox  未选中", .{ .color = C.text, .font_size = 12 }));
            const c1 = try widgets.Checkbox.create(a, g, .{ .checked = authoring.readOnly(owner, bool, g, false), .color = authoring.readOnly(owner, Color, g, C.blue), .disabled = authoring.readOnly(owner, bool, g, false) });
            try cb_unchecked.addChild(a, &c1.node);
            try cb_row.addChild(a, cb_unchecked);

            const cb_checked = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 6, .padding = Insets.all(10) });
            try cb_checked.addChild(a, try authoring.label(owner, a, g, "☑ Checkbox  已选中", .{ .color = C.text, .font_size = 12 }));
            const c2 = try widgets.Checkbox.create(a, g, .{ .checked = authoring.readOnly(owner, bool, g, true), .color = authoring.readOnly(owner, Color, g, C.green), .disabled = authoring.readOnly(owner, bool, g, false) });
            try cb_checked.addChild(a, &c2.node);
            try cb_row.addChild(a, cb_checked);
        }
        try row.addChild(a, cb_row);

        const sw_row = try authoring.column(a, .{ .gap = 8 });
        {
            const sw_off = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 6, .padding = Insets.all(10) });
            try sw_off.addChild(a, try authoring.label(owner, a, g, "Switch  关闭", .{ .color = C.text, .font_size = 12 }));
            const s1 = try widgets.Switch.create(a, g, .{ .checked = authoring.readOnly(owner, bool, g, false), .color = authoring.readOnly(owner, Color, g, C.blue), .disabled = authoring.readOnly(owner, bool, g, false) });
            try sw_off.addChild(a, &s1.node);
            try sw_row.addChild(a, sw_off);

            const sw_on = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 6, .padding = Insets.all(10) });
            try sw_on.addChild(a, try authoring.label(owner, a, g, "Switch  开启", .{ .color = C.text, .font_size = 12 }));
            const s2 = try widgets.Switch.create(a, g, .{ .checked = authoring.readOnly(owner, bool, g, true), .color = authoring.readOnly(owner, Color, g, C.green), .disabled = authoring.readOnly(owner, bool, g, false) });
            try sw_on.addChild(a, &s2.node);
            try sw_row.addChild(a, sw_on);
        }
        try row.addChild(a, sw_row);

        try page.addChild(a, row);
    }

    // ── Form ──────────────────────────────────────────────────────────────
    try page.addChild(a, try sectionHeader(owner, a, g, "Form 表单", C.peach));
    {
        const form_box = try authoring.surface(owner, a, g, .{ .bg_color = C.surface0, .corner_radius = 8, .padding = Insets.all(14) });
        const inner = try authoring.column(a, .{ .gap = 12 });

        const f1 = try authoring.field(owner, a, g, .{ .label = "用户名", .helper = "字母开头，3-20 个字符" });
        const ftf1 = try authoring.textField(owner, a, g, .{ .placeholder = "请输入用户名...", .min_width = 200 });
        try f1.node.addChild(a, &ftf1.node);
        try inner.addChild(a, &f1.node);

        const f2 = try authoring.field(owner, a, g, .{ .label = "邮箱", .helper = "例如 user@example.com" });
        const ftf2 = try authoring.textField(owner, a, g, .{ .placeholder = "请输入邮箱...", .min_width = 200 });
        try f2.node.addChild(a, &ftf2.node);
        try inner.addChild(a, &f2.node);

        try form_box.addChild(a, inner);
        try page.addChild(a, form_box);
    }

    return page;
}

/// Layout 页
pub fn buildLayout(a: std.mem.Allocator, g: *reactive.Graph, owner: *app.SignalOwner) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 20 });
    try col.addChild(a, try authoring.label(owner, a, g, "布局系统", .{ .color = C.text, .font_size = 24 }));
    try col.addChild(a, try authoring.label(owner, a, g, "Constraints + Flex / Flow / Anchors 三套纯函数求解器", .{ .color = C.blue, .font_size = 13 }));

    try col.addChild(a, try sectionHeader(owner, a, g, "Flex 盒子模型", C.teal));
    const demo = try authoring.surface(owner, a, g, .{ .bg_color = C.mantle, .corner_radius = 8, .padding = Insets.all(16) });
    const inner = try authoring.column(a, .{ .gap = 8 });
    const items = [_]struct { label: []const u8, color: Color }{
        .{ .label = "Item 1  (flex: 1)", .color = C.blue },
        .{ .label = "Item 2  (flex: 2)", .color = C.green },
        .{ .label = "Item 3  (fixed)", .color = C.peach },
    };
    for (items) |item| {
        const it = try authoring.surface(owner, a, g, .{ .bg_color = item.color.withAlpha(0.3), .corner_radius = 5, .padding = Insets.all(10) });
        try it.addChild(a, try authoring.label(owner, a, g, item.label, .{ .color = C.text, .font_size = 13 }));
        try inner.addChild(a, it);
    }
    try demo.addChild(a, inner);
    try col.addChild(a, demo);

    try col.addChild(a, try sectionHeader(owner, a, g, "求解器特性", C.yellow));
    try col.addChild(a, try authoring.card(owner, a, g, "Flex", "Column / Row / Stack · gap · main_align · cross_align · flex_factor · shrink", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Flow", "流式折行布局 · 自动换行 · 对齐", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Anchors", "QML 风格锚点定位 · fill / centerIn / margins", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));

    return col;
}

/// Reactive 页
pub fn buildReactive(a: std.mem.Allocator, g: *reactive.Graph, owner: *app.SignalOwner) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 20 });
    try col.addChild(a, try authoring.label(owner, a, g, "响应式核心", .{ .color = C.text, .font_size = 24 }));
    try col.addChild(a, try authoring.label(owner, a, g, "Signal → Computed → Effect  ·  Angular 风格数据流", .{ .color = C.blue, .font_size = 13 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Signal 信号", "可写状态容器 · get / set / update / asReadonly · 版本号追踪", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Computed 派生", "惰性求值 · 自动依赖追踪 · 菱形依赖 glitch-free", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Effect 副作用", "自动重执行 · EffectScope 生命周期 · batch 批量更新", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Graph 调度图", "多实例隔离 · 显式 flush · 动态依赖自动重追踪", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    return col;
}

/// Theme 页
pub fn buildTheme(a: std.mem.Allocator, g: *reactive.Graph, owner: *app.SignalOwner) !*runtime.Node {
    const col = try authoring.column(a, .{ .gap = 20 });
    try col.addChild(a, try authoring.label(owner, a, g, "主题系统", .{ .color = C.text, .font_size = 24 }));
    try col.addChild(a, try authoring.label(owner, a, g, "Design Tokens · Semantic Palette · Theme Resolver", .{ .color = C.blue, .font_size = 13 }));

    try col.addChild(a, try sectionHeader(owner, a, g, "调色板", C.mauve));
    const swatch = try authoring.surface(owner, a, g, .{ .bg_color = C.mantle, .corner_radius = 8, .padding = Insets.all(16) });
    const sw_row = try authoring.column(a, .{ .gap = 8 });
    const swatches = [_]Color{ C.red, C.peach, C.yellow, C.green, C.teal, C.blue, C.mauve };
    for (swatches) |sw| {
        const dot = try authoring.surface(owner, a, g, .{ .bg_color = sw, .corner_radius = 6, .padding = Insets.symmetric(16, 10) });
        try sw_row.addChild(a, dot);
    }
    try swatch.addChild(a, sw_row);
    try col.addChild(a, swatch);

    try col.addChild(a, try sectionHeader(owner, a, g, "设计 Token", C.teal));
    try col.addChild(a, try authoring.card(owner, a, g, "Spacing", "xs(4) · sm(8) · md(16) · lg(24) · xl(32)", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Typography", "FontSize · FontWeight · LineHeight · LetterSpacing", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));
    try col.addChild(a, try authoring.card(owner, a, g, "Color Roles", "Primary · Secondary · Error · Surface · Text · Border", .{ .bg_color = C.mantle, .corner_radius = 8, .title_font_size = 15 }));

    return col;
}
