//! reactive/signal —— 可写响应式状态
//!
//! `Signal(T)` 是最基础的可写状态容器，内嵌一个 `graph.Source`。
//!   - `get()`：读取值；若处于某个 effect/computed 的执行上下文中，自动建立依赖边。
//!   - `set(v)` / `update(fn)`：写入；仅在值真正变化时通知订阅者并触发调度。
//!   - `asReadonly()`：返回只读视图 `ReadSignal(T)`，用于把读权限传给子组件而不泄漏写权限。
//!
//! 变化检测：若 `T` 支持 `==`（数值 / 枚举 / 指针 / bool 等），仅在不等时通知；
//! 否则每次 `set` 都通知（保守策略，调用方可改用 `update` 精确控制）。
//!
//! 生命周期：`Signal(T)` 由调用方持有（栈或结构体成员），其依赖边在 `deinit` 时解绑。
//! 必须在所属 `Graph.deinit` 之前 `deinit`，或交由 Graph 统一管理（见 `Runtime`）。

const std = @import("std");
const graph = @import("graph.zig");
const Graph = graph.Graph;

/// 判断类型 `T` 是否可用 `==` 比较（用于变化检测）。
/// 注意：切片（slice）虽为 `.pointer` 但 Zig 不支持用 `==` 直接比较，须排除。
fn isEqualityComparable(comptime T: type) bool {
    return switch (@typeInfo(T)) {
        .int, .float, .bool, .@"enum", .comptime_int, .comptime_float => true,
        // 单项指针（*T / [*]T / [*:0]T）可比较地址；切片（[]T）不行。
        .pointer => |p| p.size != .slice,
        .optional => |opt| isEqualityComparable(opt.child),
        else => false,
    };
}

/// 可写的响应式状态容器。
pub fn Signal(comptime T: type) type {
    return struct {
        const Self = @This();

        graph: *Graph,
        source: graph.Source,
        value: T,

        /// 在 `g` 上创建一个初值为 `initial` 的 signal。
        pub fn init(g: *Graph, initial: T) Self {
            return .{
                .graph = g,
                .source = .{ .id = g.nextId() },
                .value = initial,
            };
        }

        /// 解绑该 signal 与其订阅者的依赖边，并释放内部缓冲。
        /// 之后该 signal 不可再使用。
        pub fn deinit(self: *Self) void {
            self.graph.detachSource(&self.source);
            self.source.subs.deinit(self.graph.allocator);
            self.* = undefined;
        }

        /// 读取当前值；在追踪上下文中自动注册依赖。
        pub fn get(self: *Self) T {
            self.graph.track(&self.source);
            return self.value;
        }

        /// 不建立依赖地读取当前值（peek）。用于不希望产生订阅关系的场景。
        pub fn peek(self: *const Self) T {
            return self.value;
        }

        /// 写入新值；仅在值真正变化时通知订阅者并触发调度。
        pub fn set(self: *Self, new_value: T) void {
            if (comptime isEqualityComparable(T)) {
                if (self.value == new_value) return;
            }
            self.value = new_value;
            self.graph.notifySource(&self.source);
        }

        /// 原地修改：`fn(*T) void`，适合 `n.* += 1` 这类操作，避免显式读改写。
        /// 修改后总是通知（无法判断 fn 是否真正改变了值）。
        pub fn update(self: *Self, comptime f: fn (*T) void) void {
            f(&self.value);
            self.graph.notifySource(&self.source);
        }

        /// 返回只读视图。
        pub fn asReadonly(self: *Self) ReadSignal(T) {
            return .{ .signal = self };
        }
    };
}

/// `Signal(T)` 的只读视图：可读取（含依赖追踪），但不能写入。
/// 持有对源 signal 的非 owning 指针，调用方须保证源生命周期不短于本视图。
pub fn ReadSignal(comptime T: type) type {
    return struct {
        const Self = @This();

        signal: *Signal(T),

        /// 读取当前值；在追踪上下文中自动注册依赖。
        pub fn get(self: Self) T {
            return self.signal.get();
        }

        /// 不建立依赖地读取当前值。
        pub fn peek(self: Self) T {
            return self.signal.peek();
        }
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// 测试
// ─────────────────────────────────────────────────────────────────────────────

test "Signal get/set 基本读写" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 10);
    defer s.deinit();

    try std.testing.expectEqual(@as(i32, 10), s.get());
    s.set(42);
    try std.testing.expectEqual(@as(i32, 42), s.get());
}

test "Signal set 相等值不增加版本号" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 1);
    defer s.deinit();

    const v0 = s.source.version;
    s.set(1); // 相等，应被忽略
    try std.testing.expectEqual(v0, s.source.version);
    s.set(2); // 不等，应递增
    try std.testing.expectEqual(v0 + 1, s.source.version);
}

test "Signal update 原地修改" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 0);
    defer s.deinit();

    const inc = struct {
        fn f(v: *i32) void {
            v.* += 5;
        }
    }.f;
    s.update(inc);
    try std.testing.expectEqual(@as(i32, 5), s.get());
}

test "ReadSignal 只读视图" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 7);
    defer s.deinit();

    const ro = s.asReadonly();
    try std.testing.expectEqual(@as(i32, 7), ro.get());
    s.set(8);
    try std.testing.expectEqual(@as(i32, 8), ro.peek());
}

test "peek 不建立依赖" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();

    var s = Signal(i32).init(&g, 3);
    defer s.deinit();

    // 没有 reader 上下文，peek 与 get 都不应崩溃
    try std.testing.expectEqual(@as(i32, 3), s.peek());
    try std.testing.expectEqual(@as(usize, 0), s.source.subs.items.len);
}
