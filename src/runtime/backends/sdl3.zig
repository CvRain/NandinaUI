//! runtime/backends/sdl3 —— SDL3 窗口后端
//!
//! 把 SDL3 包装为 NandinaUI 的平台后端：窗口管理、事件翻译、渲染循环。
//! 作为 `runtime.Backend` 的实现（渲染）+ 事件源（窗口回调）。
//!
//! 依赖方向：本文件位于 runtime 内部，不污染 runtime 公共 API（规则第 7 条）。
//!
//! ## 用法
//!
//! ```zig
//! const sdl_backend = @import("runtime/backends/sdl3.zig");
//!
//! var window = try sdl_backend.Window.init(allocator, "Hello", 800, 600);
//! defer window.deinit();
//!
//! // 将 NandinaUI Tree 挂到窗口上
//! window.setTree(&my_tree);
//!
//! // 主循环
//! while (window.pollEvent()) {
//!     // 事件已自动翻译并分发到 tree
//!     _ = try window.frame();
//!     window.present();
//! }
//! ```
//!
//! ## 事件翻译映射
//!
//! | SDL 事件 | NandinaUI Event |
//! |-----------|-----------------|
//! | SDL_EVENT_MOUSE_MOTION | pointer_move |
//! | SDL_EVENT_MOUSE_BUTTON_DOWN | pointer_down |
//! | SDL_EVENT_MOUSE_BUTTON_UP | pointer_up |
//! | SDL_EVENT_MOUSE_WHEEL | pointer_wheel |
//! | SDL_EVENT_KEY_DOWN | key_down |
//! | SDL_EVENT_KEY_UP | key_up |
//! | SDL_EVENT_TEXT_INPUT | text_input |
//! | SDL_EVENT_WINDOW_RESIZED | window_resize |
//! | SDL_EVENT_WINDOW_CLOSE_REQUESTED | window_close |
//! | SDL_EVENT_FOCUS_GAINED / LOST | focus_in / focus_out |

const std = @import("std");
const nandina = @import("NandinaUI");
const foundation = nandina.foundation;
const render = nandina.render;
const runtime = nandina.runtime;

const Allocator = std.mem.Allocator;
const Size = foundation.Size;
const Event = runtime.Event;
const Tree = runtime.Tree;
const Backend = render.Backend;
const RenderTarget = render.RenderTarget;
const SoftwareBackend = render.SoftwareBackend;

// ─────────────────────────────────────────────────────────────────────────────
// SDL3 C 头文件
// ─────────────────────────────────────────────────────────────────────────────

const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

// ─────────────────────────────────────────────────────────────────────────────
// 窗口后端
// ─────────────────────────────────────────────────────────────────────────────

/// 后端类型选择。
pub const BackendKind = enum {
    software,
    thorvg,
};

/// SDL3 窗口后端。持有窗口、渲染后端、事件缓冲。
pub const Window = struct {
    allocator: Allocator,
    sdl_window: *c.SDL_Window,
    sdl_renderer: *c.SDL_Renderer,
    /// 用于上传像素的纹理（跨帧复用）。
    sdl_texture: ?*c.SDL_Texture,
    /// 像素缓冲。
    pixels: []u32,
    width: u32,
    height: u32,

    /// 当前选用的渲染后端类型。
    backend_kind: BackendKind = .software,
    /// 软件后端实例（默认）。
    sw_renderer: SoftwareBackend = .{},

    /// 关联的 NandinaUI 树（可选）。
    tree: ?*Tree,
    /// 窗口是否正在运行（未收到关闭请求）。
    running: bool,
    /// 事件缓冲：SDL 事件 → runtime.Event，在主循环中分发。
    event_queue: std.ArrayList(Event),

    /// 初始化 SDL3 并创建窗口。
    pub fn init(allocator: Allocator, title: []const u8, width: u32, height: u32) !Window {
        try sdlInit();

        const sdl_window = c.SDL_CreateWindow(
            title.ptr,
            @intCast(width),
            @intCast(height),
            c.SDL_WINDOW_RESIZABLE,
        ) orelse {
            std.log.err("SDL_CreateWindow failed: {s}", .{c.SDL_GetError()});
            return error.SdlCreateWindowFailed;
        };

        // 创建渲染器（用于纹理上传和呈现）
        const sdl_renderer = c.SDL_CreateRenderer(sdl_window, null) orelse {
            std.log.err("SDL_CreateRenderer failed: {s}", .{c.SDL_GetError()});
            c.SDL_DestroyWindow(sdl_window);
            return error.SdlCreateRendererFailed;
        };

        // 分配像素缓冲
        const pixels = try allocator.alloc(u32, width * height);

        return Window{
            .allocator = allocator,
            .sdl_window = sdl_window,
            .sdl_renderer = sdl_renderer,
            .sdl_texture = null,
            .pixels = pixels,
            .width = width,
            .height = height,
            .tree = null,
            .running = true,
            .event_queue = .empty,
        };
    }

    /// 析构窗口，释放资源。
    pub fn deinit(self: *Window) void {
        self.event_queue.deinit(self.allocator);
        if (self.sdl_texture) |tex| c.SDL_DestroyTexture(tex);
        self.allocator.free(self.pixels);
        c.SDL_DestroyRenderer(self.sdl_renderer);
        c.SDL_DestroyWindow(self.sdl_window);
        sdlDeinit();
        self.* = undefined;
    }

    /// 关联一棵 NandinaUI 树。窗口会将事件分发到该树，并驱动其帧。
    pub fn setTree(self: *Window, tree: *Tree) void {
        self.tree = tree;
        tree.setViewport(Size{ .width = @floatFromInt(self.width), .height = @floatFromInt(self.height) });
    }

    // ── 主循环 ──────────────────────────────────────────────────────────────

    /// 轮询一个 SDL 事件，翻译后分发到关联的 tree。
    /// 返回 false 表示窗口收到关闭请求，应退出循环。
    /// 可能返回错误（如 text_input 分配失败）。
    pub fn pollEvent(self: *Window) !bool {
        var sdl_event: c.SDL_Event = undefined;
        while (c.SDL_PollEvent(&sdl_event)) {
            switch (sdl_event.type) {
                c.SDL_EVENT_QUIT => {
                    self.running = false;
                    return false;
                },
                c.SDL_EVENT_WINDOW_RESIZED => {
                    const data = sdl_event.window;
                    const w: u32 = @intCast(data.data1);
                    const h: u32 = @intCast(data.data2);
                    self.handleResize(w, h);
                },
                c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => {
                    self.running = false;
                    return false;
                },
                c.SDL_EVENT_MOUSE_MOTION => {
                    const data = sdl_event.motion;
                    const ev = Event{ .pointer_move = .{
                        .x = data.x,
                        .y = data.y,
                        .dx = data.xrel,
                        .dy = data.yrel,
                    } };
                    try self.event_queue.append(self.allocator, ev);
                },
                c.SDL_EVENT_MOUSE_BUTTON_DOWN, c.SDL_EVENT_MOUSE_BUTTON_UP => {
                    const is_down = sdl_event.type == c.SDL_EVENT_MOUSE_BUTTON_DOWN;
                    const data = sdl_event.button;
                    const button = translateMouseButton(data.button);
                    const ev = if (is_down)
                        Event{ .pointer_down = .{ .button = button, .x = data.x, .y = data.y, .click_count = @intCast(data.clicks) } }
                    else
                        Event{ .pointer_up = .{ .button = button, .x = data.x, .y = data.y, .click_count = @intCast(data.clicks) } };
                    try self.event_queue.append(self.allocator, ev);
                },
                c.SDL_EVENT_MOUSE_WHEEL => {
                    const data = sdl_event.wheel;
                    const ev = Event{ .pointer_wheel = .{
                        .x = data.mouse_x,
                        .y = data.mouse_y,
                        .dx = data.x,
                        .dy = data.y,
                    } };
                    try self.event_queue.append(self.allocator, ev);
                },
                c.SDL_EVENT_KEY_DOWN, c.SDL_EVENT_KEY_UP => {
                    const is_down = sdl_event.type == c.SDL_EVENT_KEY_DOWN;
                    const data = sdl_event.key;
                    const mods = translateKeyModifiers(data.mod);
                    const ev = if (is_down)
                        Event{ .key_down = .{ .code = @intCast(data.key), .modifiers = mods, .repeat = data.repeat } }
                    else
                        Event{ .key_up = .{ .code = @intCast(data.key), .modifiers = mods, .repeat = data.repeat } };
                    try self.event_queue.append(self.allocator, ev);
                },
                c.SDL_EVENT_TEXT_INPUT => {
                    const data = sdl_event.text;
                    // text 是 C 字符串（以 null 结尾），转为 Zig 切片
                    const text_slice = std.mem.sliceTo(@as([*:0]const u8, @ptrCast(&data.text)), 0);
                    const text_copy = try self.allocator.dupe(u8, text_slice);
                    const ev = Event{ .text_input = .{ .text = text_copy } };
                    try self.event_queue.append(self.allocator, ev);
                },
                c.SDL_EVENT_WINDOW_FOCUS_GAINED => {
                    try self.event_queue.append(self.allocator, Event{ .focus_in = .{ .gained = true } });
                },
                c.SDL_EVENT_WINDOW_FOCUS_LOST => {
                    try self.event_queue.append(self.allocator, Event{ .focus_out = .{ .gained = false } });
                },
                else => {},
            }
        }
        return true;
    }

    /// 分发所有已排队的事件到关联的 tree。
    pub fn dispatchEvents(self: *Window) void {
        const maybe_tree = self.tree;
        for (self.event_queue.items) |*ev| {
            if (maybe_tree) |tree| {
                _ = tree.dispatchEvent(ev.*);
            }
            // text_input 事件含有堆分配的 text，需要释放
            switch (ev.*) {
                .text_input => |ti| self.allocator.free(ti.text),
                else => {},
            }
        }
        self.event_queue.clearRetainingCapacity();
    }

    /// 执行一帧：布局 + 绘制 → 软件光栅 → 呈现到窗口。
    pub fn frame(self: *Window) !bool {
        const tree = self.tree orelse return false;
        const frame_produced = try tree.frame();
        if (!frame_produced) return false;

        const target = RenderTarget{
            .pixels = self.pixels.ptr,
            .width = self.width,
            .height = self.height,
            .stride = self.width,
        };

        const backend = self.sw_renderer.interface();
        try backend.beginFrame(target);
        try backend.submit(&tree.scene);
        try backend.endFrame();

        return true;
    }

    /// 把当前像素缓冲呈现到 SDL 窗口。
    /// 使用 SDL_UpdateTexture + SDL_RenderPresent 管线，高效且跨平台。
    pub fn present(self: *Window) void {
        // 延迟创建纹理（窗口首次呈现时）
        if (self.sdl_texture == null) {
            const t = c.SDL_CreateTexture(
                self.sdl_renderer,
                c.SDL_PIXELFORMAT_ARGB8888,
                c.SDL_TEXTUREACCESS_STREAMING,
                @intCast(self.width),
                @intCast(self.height),
            ) orelse return;
            self.sdl_texture = t;
        }
        const texture = self.sdl_texture.?;

        // 上传像素到纹理
        _ = c.SDL_UpdateTexture(
            texture,
            null,
            self.pixels.ptr,
            @intCast(self.width * 4),
        );

        // 复制纹理到渲染目标并呈现
        _ = c.SDL_RenderTexture(self.sdl_renderer, texture, null, null);
        _ = c.SDL_RenderPresent(self.sdl_renderer);
    }

    /// 处理窗口 resize：重新分配像素缓冲 + 重建纹理 + 通知 tree。
    fn handleResize(self: *Window, w: u32, h: u32) void {
        // 重新分配像素缓冲
        const new_pixels = self.allocator.realloc(self.pixels, w * h) catch return;
        self.pixels = new_pixels.ptr[0 .. w * h];
        self.width = w;
        self.height = h;

        // 标记纹理需要重建（销毁旧纹理，下次 present 重新创建）
        if (self.sdl_texture) |tex| {
            c.SDL_DestroyTexture(tex);
            self.sdl_texture = null;
        }

        if (self.tree) |tree| {
            tree.setViewport(Size{ .width = @floatFromInt(w), .height = @floatFromInt(h) });
            _ = tree.dispatchEvent(Event{ .window_resize = .{ .width = w, .height = h } });
        }
    }

    /// 查询窗口是否仍在运行。
    pub fn isRunning(self: *const Window) bool {
        return self.running;
    }

    /// 请求关闭窗口。
    pub fn close(self: *Window) void {
        self.running = false;
    }
};

// ── SDL3 子系统的全局生命周期 ──────────────────────────────────────────────────
// Window.init/sdlInit 和 Window.deinit/sdlDeinit 通过引用计数管理 SDL_Init/SDL_Quit。

var sdl_init_count: u32 = 0;

/// 初始化 SDL3 子系统。可多次调用（引用计数）。
pub fn sdlInit() !void {
    if (sdl_init_count > 0) {
        sdl_init_count += 1;
        return;
    }
    if (!c.SDL_Init(c.SDL_INIT_VIDEO | c.SDL_INIT_EVENTS)) {
        std.log.err("SDL_Init failed: {s}", .{c.SDL_GetError()});
        return error.SdlInitFailed;
    }
    sdl_init_count = 1;
}

/// 反初始化 SDL3 子系统（引用计数归零时真正 Quit）。
pub fn sdlDeinit() void {
    if (sdl_init_count > 1) {
        sdl_init_count -= 1;
        return;
    }
    if (sdl_init_count > 0) {
        c.SDL_Quit();
        sdl_init_count = 0;
    }
}

// ── 辅助翻译函数 ──────────────────────────────────────────────────────────────

fn translateMouseButton(sdl_button: u8) runtime.PointerButton {
    return switch (sdl_button) {
        c.SDL_BUTTON_LEFT => .left,
        c.SDL_BUTTON_MIDDLE => .middle,
        c.SDL_BUTTON_RIGHT => .right,
        else => .other,
    };
}

fn translateKeyModifiers(sdl_mod: u16) runtime.KeyModifiers {
    return .{
        .shift = (sdl_mod & c.SDL_KMOD_SHIFT) != 0,
        .ctrl = (sdl_mod & c.SDL_KMOD_CTRL) != 0,
        .alt = (sdl_mod & c.SDL_KMOD_ALT) != 0,
        .super = (sdl_mod & c.SDL_KMOD_GUI) != 0,
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "translateMouseButton 映射正确" {
    try std.testing.expectEqual(runtime.PointerButton.left, translateMouseButton(c.SDL_BUTTON_LEFT));
    try std.testing.expectEqual(runtime.PointerButton.middle, translateMouseButton(c.SDL_BUTTON_MIDDLE));
    try std.testing.expectEqual(runtime.PointerButton.right, translateMouseButton(c.SDL_BUTTON_RIGHT));
    try std.testing.expectEqual(runtime.PointerButton.other, translateMouseButton(42));
}

test "translateKeyModifiers 映射正确" {
    const mod = translateKeyModifiers(c.SDL_KMOD_CTRL | c.SDL_KMOD_SHIFT);
    try std.testing.expect(mod.ctrl);
    try std.testing.expect(mod.shift);
    try std.testing.expect(!mod.alt);
    try std.testing.expect(!mod.super);
}
