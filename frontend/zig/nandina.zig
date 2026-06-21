//! frontend/zig/nandina —— NandinaUI 的 Zig 前端层
//!
//! 这是「Zig 前端绑定」：直通 Core（`@import("NandinaUI")`），不经过 C ABI，
//! 零开销享受 comptime 与类型安全。它把写界面常用的能力**收敛再导出**到一个
//! 入口模块，让 showcase / 应用代码只 `@import` 本文件，而不必散落地直接引用
//! Core 各子模块。
//!
//! 与 C++ 前端「形似」：两者都对齐 `app.authoring` 的声明式风格，只是语言不同。
//!
//! ## 用法
//!
//! ```zig
//! const nd = @import("nandina"); // frontend/zig/nandina.zig
//!
//! pub fn main(init: std.process.Init) !void {
//!     var app = try nd.App.init(init.gpa, .{ .title = "Demo", .width = 800, .height = 600 });
//!     defer app.deinit();
//!     const root = try buildUi(&app);
//!     try app.run(root);
//! }
//! ```

const std = @import("std");
const nandina = @import("NandinaUI");
const sdl = @import("sdl_backend");

// ── Core 分层再导出 ───────────────────────────────────────────────────────────

pub const foundation = nandina.foundation;
pub const reactive = nandina.reactive;
pub const render = nandina.render;
pub const layout = nandina.layout;
pub const theme = nandina.theme;
pub const text = nandina.text;
pub const runtime = nandina.runtime;
pub const widgets = nandina.widgets;
pub const app = nandina.app;

pub const version = nandina.version;

// ── 常用类型别名（写界面高频用到）──────────────────────────────────────────────

pub const Color = foundation.Color;
pub const Insets = foundation.Insets;
pub const Rect = foundation.Rect;
pub const Size = foundation.Size;

pub const Graph = reactive.Graph;
pub fn Signal(comptime T: type) type {
    return reactive.Signal(T);
}

pub const Node = runtime.Node;
pub const Tree = runtime.Tree;

// ── authoring DSL（声明式工厂）再导出 ─────────────────────────────────────────
// showcase 页面构建只用这一层，不直接 new widget。

pub const authoring = app.authoring;
pub const SignalOwner = app.SignalOwner;
pub const readOnly = app.readOnly;

pub const surface = app.surface;
pub const column = app.column;
pub const row = app.row;
pub const stack = app.stack;
pub const label = app.label;
pub const button = app.button;
pub const card = app.card;
pub const panel = app.panel;
pub const icon = app.icon;
pub const textField = app.textField;
pub const field = app.field;
pub const checkbox = app.checkbox;
pub const switch_ = app.switch_;

pub const IconShape = widgets.IconShape;

// ── Page / Router ────────────────────────────────────────────────────────────

pub const Page = app.Page;
pub const PageHost = app.PageHost;
pub const Router = app.Router;
pub const Route = app.Route;

// ─────────────────────────────────────────────────────────────────────────────
// App —— 全包窗口入口（默认 SDL3 + 软件后端）
// ─────────────────────────────────────────────────────────────────────────────
//
// 与 C ABI 的 `nandina_app_*` 对应的 Zig 版本，但直通 Core，可直接拿到强类型句柄。
// 内部用 sdl_backend.Window + 默认 software 后端 + 可选真实字体。

pub const AppOptions = struct {
    title: []const u8 = "NandinaUI",
    width: u32 = 800,
    height: u32 = 600,
};

pub const App = struct {
    allocator: std.mem.Allocator,
    window: sdl.Window,
    tree: Tree,
    g: Graph,
    ft: ?text.backends.hb_ft.HarfBuzzFreeTypeMetrics = null,

    /// 打开窗口（默认 software 后端 + 可选真实字体，字体失败回退等宽估算）。
    /// 注意：App 持有自身内存，调用方应在 `var app = ...; const p = &app;` 后
    /// 用 `p` 访问，确保指针稳定。
    ///
    /// 字体后端在此创建并存入 `self.ft`，但 **glyph renderer 的注册推迟到 `run`**：
    /// glyphRenderer 句柄会捕获 `&self.ft` 的地址，必须在 App 处于其最终稳定地址
    /// （即调用方持有 App 之后）时注册，否则 `init` 按值返回拷贝会令该地址失效。
    pub fn init(allocator: std.mem.Allocator, opts: AppOptions) !App {
        var window = try sdl.Window.init(allocator, opts.title, opts.width, opts.height);
        errdefer window.deinit();

        var self = App{
            .allocator = allocator,
            .window = window,
            .tree = Tree.init(allocator),
            .g = Graph.init(allocator),
        };

        if (text.backends.hb_ft.HarfBuzzFreeTypeMetrics.init(allocator)) |fb| {
            self.ft = fb;
        } else |_| {}

        return self;
    }

    pub fn deinit(self: *App) void {
        self.tree.deinit();
        self.g.deinit();
        if (self.ft) |*ft| ft.deinit();
        self.window.deinit();
        self.* = undefined;
    }

    /// 内部响应式 Graph（widget 工厂需要它）。
    pub fn graph(self: *App) *Graph {
        return &self.g;
    }

    /// 挂载根节点并进入阻塞主循环，直到窗口关闭。
    pub fn run(self: *App, root: *Node) !void {
        // 此时 self 已是调用方持有的稳定地址，注册 glyph renderer 才安全。
        if (self.ft) |*ft| {
            self.window.sw_renderer.setGlyphRenderer(ft.glyphRenderer());
        }
        self.tree.setRoot(root);
        self.window.setTree(&self.tree);

        while (self.window.isRunning()) {
            if (!try self.window.pollEvent()) break;
            self.window.dispatchEvents();
            _ = try self.window.frame();
            self.window.present();
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "frontend 再导出可用" {
    // 仅验证符号可引用、类型对齐（不开窗口）。
    try std.testing.expect(version.major == 0);
    _ = surface;
    _ = column;
    _ = row;
    _ = stack;
    _ = button;
    _ = App;
    _ = Page;
    _ = PageHost;
    _ = Router;
}
