//! reactive/graph —— 响应式核心调度图
//!
//! 一个 `Graph` 实例拥有所有响应式节点（signal / computed / effect），并维护：
//!   - `current_reader`：当前正在执行的 reactor，用于自动建立依赖边；
//!   - `pending`：待执行的 effect 队列；
//!   - `batch_depth`：batch 嵌套深度，> 0 时失效只入队不立即 flush；
//!   - 一条侵入式节点链表，统一管理所有节点的内存释放。
//!
//! 设计要点：
//! - **无全局状态**：所有调度状态都挂在 Graph 上，多 Graph 实例彼此独立，
//!   单元测试天然隔离。
//! - **双向边**：Source 维护 subscribers，Reactor 维护 sources，订阅/解绑双向同步。
//! - **去重**：同一 reactor 重复读取同一 source 在一次执行内只建立一条边。
//! - **Push 失效 + Pull 取值**：`Source.notify` 递归把订阅者标记 dirty；
//!   computed 读时若 dirty 才重算（lazy），effect 由 `flush()` 调度执行（eager）。
//! - **职责分离**：Reactor 暴露两个回调 —— `on_invalidate`（依赖变化时如何传播失效）
//!   与 `run`（被 flush 调度时如何重新执行），不再用同一个函数承担两种语义。
//!
//! 本文件不直接暴露给业务代码，由 signal / computed / effect 模块组合使用。

const std = @import("std");
const Allocator = std.mem.Allocator;

/// 响应者状态。
pub const ReactorState = enum(u2) {
    /// 已是最新值，依赖未变化。
    clean,
    /// 至少有一个直接依赖被失效，需要重新执行 / 重算。
    dirty,
};

// ─────────────────────────────────────────────────────────────────────────────
// § Node —— 侵入式节点注册表头
// ─────────────────────────────────────────────────────────────────────────────

/// 所有堆分配的响应式节点（Signal / Computed / Effect）内嵌的注册头。
///
/// Graph 用一条双向链表串起全部节点，`deinit` 时统一拆解释放；
/// `dispose` 可提前单独销毁某个节点（如 effect 退出作用域）。
pub const Node = struct {
    prev: ?*Node = null,
    next: ?*Node = null,
    /// 拆解函数：释放该节点自有的依赖边、闭包与堆内存（含 `destroy(self)`）。
    /// 注意：此函数**不**负责从链表中摘除节点（摘除由 `Graph.disposeNode` /
    /// `Graph.deinit` 在调用前后处理），因此可被两者安全复用。
    teardown_fn: *const fn (node: *Node, graph: *Graph) void,
    /// 是否已被销毁（幂等保护）。
    disposed: bool = false,
};

// ─────────────────────────────────────────────────────────────────────────────
// § Reactor / Source
// ─────────────────────────────────────────────────────────────────────────────

/// 响应者：能在依赖变化时被通知的节点（effect / computed）。
///
/// 由 effect / computed 的具体实现内嵌持有，通过两个函数指针多态分发。
pub const Reactor = struct {
    /// 全局唯一 id（Graph 内分配）。
    id: u64,
    /// 当前状态。
    state: ReactorState = .clean,
    /// 该 reactor 正在订阅的 source 列表。
    sources: std.ArrayList(*Source) = .empty,
    /// 依赖变化时的失效传播逻辑。
    /// - effect：标记 dirty 并入队等待 flush。
    /// - computed：标记自身 dirty，并把失效继续传播给它自己的订阅者。
    on_invalidate: *const fn (reactor: *Reactor, graph: *Graph) void,
    /// 被 flush 调度时的执行逻辑。仅 effect 会进入 pending 队列；
    /// computed 是 pull 模型，永远不会被 flush 调用，其实现可设为 unreachable。
    run: *const fn (reactor: *Reactor, graph: *Graph) void,
    /// 是否已在 graph.pending 队列中（去重）。
    in_queue: bool = false,
    /// 是否已被 dispose（disposed reactor 不再接收通知 / 执行）。
    disposed: bool = false,
};

/// 数据源：可被订阅、值变化时通知所有订阅者的节点（signal / computed）。
pub const Source = struct {
    /// 全局唯一 id。
    id: u64,
    /// 单调递增版本号，每次值真正变化时 +1。供未来更精细的脏检查使用。
    version: u64 = 1,
    /// 当前订阅它的 reactor 列表。
    subs: std.ArrayList(*Reactor) = .empty,
};

// ─────────────────────────────────────────────────────────────────────────────
// § Graph
// ─────────────────────────────────────────────────────────────────────────────

/// 响应式调度图。所有 signal / computed / effect 必须依附于一个 Graph 实例。
pub const Graph = struct {
    allocator: Allocator,
    next_id: u64 = 1,
    /// 当前正在执行的 reactor（用于自动建立依赖边）。
    current_reader: ?*Reactor = null,
    /// 等待执行的 effect 队列。
    pending: std.ArrayList(*Reactor) = .empty,
    /// batch 嵌套深度。> 0 时 invalidate 不立即 flush。
    batch_depth: u32 = 0,
    /// 是否正在 flush（防止 flush 内再次入口造成重入）。
    flushing: bool = false,
    /// 是否正在整体析构。为真时各节点 teardown 跳过跨节点的解绑
    /// （所有节点都将被释放，解绑既无必要又有 use-after-free 风险）。
    tearing_down: bool = false,
    /// 节点注册表链表头（最近注册的在前）。
    first_node: ?*Node = null,

    pub fn init(allocator: Allocator) Graph {
        return .{ .allocator = allocator };
    }

    /// 释放整个图：拆解所有仍存活的节点，再释放调度状态。
    pub fn deinit(self: *Graph) void {
        self.tearing_down = true;
        var it = self.first_node;
        while (it) |node| {
            const next = node.next;
            // teardown 会 destroy(self)，因此必须先取出 next。
            node.teardown_fn(node, self);
            it = next;
        }
        self.first_node = null;
        self.pending.deinit(self.allocator);
        self.* = undefined;
    }

    /// 分配下一个唯一 id。
    pub fn nextId(self: *Graph) u64 {
        const id = self.next_id;
        self.next_id += 1;
        return id;
    }

    // ── 节点注册 ──────────────────────────────────────────────────────────────

    /// 把节点登记进链表头。
    pub fn registerNode(self: *Graph, node: *Node) void {
        node.prev = null;
        node.next = self.first_node;
        if (self.first_node) |head| head.prev = node;
        self.first_node = node;
    }

    /// 从链表中摘除节点（不释放内存）。
    fn unlinkNode(self: *Graph, node: *Node) void {
        if (node.prev) |p| {
            p.next = node.next;
        } else if (self.first_node == node) {
            self.first_node = node.next;
        }
        if (node.next) |n| n.prev = node.prev;
        node.prev = null;
        node.next = null;
    }

    /// 提前销毁单个节点：摘除链表后调用其 teardown 释放内存。幂等。
    pub fn disposeNode(self: *Graph, node: *Node) void {
        if (node.disposed) return;
        self.unlinkNode(node);
        node.teardown_fn(node, self);
    }

    // ── 依赖追踪 ──────────────────────────────────────────────────────────────

    /// 把当前 reader（若存在）登记为 source 的订阅者，同时把 source 记入
    /// reader.sources。同一对节点重复登记会自动去重。
    pub fn track(self: *Graph, source: *Source) void {
        const reader = self.current_reader orelse return;
        if (reader.disposed) return;
        for (reader.sources.items) |s| {
            if (s == source) return;
        }
        reader.sources.append(self.allocator, source) catch return;
        source.subs.append(self.allocator, reader) catch {
            _ = reader.sources.pop();
        };
    }

    /// 进入读取上下文，返回先前的 reader（用于嵌套恢复）。
    pub fn beginRead(self: *Graph, reactor: *Reactor) ?*Reactor {
        const prev = self.current_reader;
        self.current_reader = reactor;
        return prev;
    }

    pub fn endRead(self: *Graph, previous: ?*Reactor) void {
        self.current_reader = previous;
    }

    /// 清空 reactor 的所有依赖边（双向）。在 reactor 即将重新执行时调用。
    pub fn clearReactorDeps(self: *Graph, reactor: *Reactor) void {
        _ = self;
        for (reactor.sources.items) |source| {
            removeReactorFromSource(source, reactor);
        }
        reactor.sources.clearRetainingCapacity();
    }

    fn removeReactorFromSource(source: *Source, reactor: *Reactor) void {
        var i: usize = 0;
        while (i < source.subs.items.len) {
            if (source.subs.items[i] == reactor) {
                _ = source.subs.swapRemove(i);
            } else {
                i += 1;
            }
        }
    }

    /// 解除 source 与其全部订阅者的双向绑定（source 即将销毁时调用）。
    pub fn detachSource(self: *Graph, source: *Source) void {
        _ = self;
        for (source.subs.items) |reactor| {
            var i: usize = 0;
            while (i < reactor.sources.items.len) {
                if (reactor.sources.items[i] == source) {
                    _ = reactor.sources.swapRemove(i);
                } else {
                    i += 1;
                }
            }
        }
    }

    // ── 失效与调度 ────────────────────────────────────────────────────────────

    /// source 值变化后调用：递增 version，把全部订阅者标记 dirty，
    /// 非 batch 且非 flush 中时立即 flush。
    pub fn notifySource(self: *Graph, source: *Source) void {
        source.version +%= 1;
        // 订阅者在传播中可能修改 source.subs，先按当前快照逐个失效。
        // invalidate 不会向 source.subs 追加元素，因此正序遍历是安全的。
        var i: usize = 0;
        while (i < source.subs.items.len) : (i += 1) {
            self.invalidateReactor(source.subs.items[i]);
        }
        if (self.batch_depth == 0 and !self.flushing) {
            self.flush();
        }
    }

    /// 把 reactor 标记 dirty 并执行其失效传播逻辑。
    pub fn invalidateReactor(self: *Graph, reactor: *Reactor) void {
        if (reactor.disposed) return;
        // 已是 dirty 的节点无需重复传播（computed 的传播是幂等关键）。
        if (reactor.state == .dirty) return;
        reactor.state = .dirty;
        reactor.on_invalidate(reactor, self);
    }

    /// 把 reactor 加入待执行队列（去重）。由 effect 的 on_invalidate 调用。
    pub fn enqueue(self: *Graph, reactor: *Reactor) void {
        if (reactor.in_queue or reactor.disposed) return;
        self.pending.append(self.allocator, reactor) catch return;
        reactor.in_queue = true;
    }

    /// 执行所有待处理 reactor。batch 退出、或 source 变化（非 batch 中）时自动调用。
    pub fn flush(self: *Graph) void {
        if (self.flushing) return;
        self.flushing = true;
        defer self.flushing = false;

        // 执行过程中可能有新 reactor 入队，故用索引遍历而非 for-each。
        var idx: usize = 0;
        while (idx < self.pending.items.len) : (idx += 1) {
            const reactor = self.pending.items[idx];
            reactor.in_queue = false;
            if (reactor.disposed) continue;
            reactor.run(reactor, self);
        }
        self.pending.clearRetainingCapacity();
    }

    // ── batch ─────────────────────────────────────────────────────────────────

    /// 进入 batch：嵌套深度 +1。
    pub fn beginBatch(self: *Graph) void {
        self.batch_depth += 1;
    }

    /// 退出 batch：嵌套深度 -1，回到最外层时 flush。
    pub fn endBatch(self: *Graph) void {
        std.debug.assert(self.batch_depth > 0);
        self.batch_depth -= 1;
        if (self.batch_depth == 0) self.flush();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 测试：仅覆盖 graph 自身的小工具，端到端测试见 signal / computed / effect 模块。
// ─────────────────────────────────────────────────────────────────────────────

test "Graph nextId 单调递增" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();
    try std.testing.expectEqual(@as(u64, 1), g.nextId());
    try std.testing.expectEqual(@as(u64, 2), g.nextId());
    try std.testing.expectEqual(@as(u64, 3), g.nextId());
}

test "track 在无 reader 时是 no-op" {
    var g = Graph.init(std.testing.allocator);
    defer g.deinit();
    var src: Source = .{ .id = g.nextId() };
    defer src.subs.deinit(g.allocator);
    g.track(&src);
    try std.testing.expectEqual(@as(usize, 0), src.subs.items.len);
}
