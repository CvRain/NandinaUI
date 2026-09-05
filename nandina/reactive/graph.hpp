//
// Created by cvrain on 2026/7/3.
//
// reactive/graph — 响应式核心调度图 (移植自 v2 Zig 版, C++ 化)
//
// 一个 Graph 拥有所有响应式节点 (signal / computed / effect) 的调度关系:
//   - current_reader_: 当前正在执行的 Reactor, 用于自动建立依赖边;
//   - pending_:        待执行的 effect 队列;
//   - batch/deferred:  显式 batch 与应用 tick 延迟域只入队, 在边界统一 flush;
//   - reactors_:       Graph 持有的 Computed/Effect (unique_ptr), 统一释放。
//
// 设计要点 (与 v2 一致):
//   - 无全局状态: 所有调度状态挂在 Graph 上, 多实例彼此独立, 单测天然隔离。
//   - 双向边: Source 维护 subscribers, Reactor 维护 sources, 订阅/解绑双向同步。
//   - 去重: 同一 reactor 一次执行内重复读同一 source 只建一条边。
//   - Push 失效 + Pull 取值: notify 递归标记订阅者 dirty;
//     computed 读时若 dirty 才重算 (lazy); effect 由 flush 调度执行 (eager)。
//   - 职责分离: Reactor 暴露 on_invalidate (如何传播失效) 与 run (被 flush 时如何执行)。
//
// 本文件不直接面向业务, 由 signal / computed / effect 组合使用。
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_GRAPH_HPP
#define NANDINA_EXPERIMENT_REACTIVE_GRAPH_HPP

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

namespace nandina::reactive
{

    class Graph;

    /// 响应者状态。
    enum class ReactorState : std::uint8_t {
        clean, ///< 已是最新值, 依赖未变化。
        dirty, ///< 至少一个直接依赖被失效, 需要重新执行 / 重算。
    };

    /// 数据源: 可被订阅、值变化时通知所有订阅者的节点 (signal / computed 内嵌持有)。
    using Source = struct Source {
        std::uint64_t id = 0;
        /// 单调递增版本号, 每次值真正变化时 +1。供更精细的脏检查使用。
        std::uint64_t version = 1;
        /// 当前订阅它的 reactor 列表 (非拥有)。
        std::vector<class Reactor*> subs;
    };

    /// 响应者: 能在依赖变化时被通知的节点 (effect / computed)。
    /// 抽象基类, 通过虚函数分发, 不使用 RTTI。
    class Reactor {
    public:
        Reactor() = default;
        virtual ~Reactor() = default;

        Reactor(const Reactor&) = delete;
        auto operator=(const Reactor&) -> Reactor& = delete;
        Reactor(Reactor&&) = delete;
        auto operator=(Reactor&&) -> Reactor& = delete;

        /// 依赖变化时的失效传播逻辑。
        /// - effect:   标记 dirty 并入队等待 flush。
        /// - computed: 标记自身 dirty, 并把失效继续传播给它自己的订阅者。
        virtual void on_invalidate(Graph& graph) = 0;

        /// 被 flush 调度时的执行逻辑。仅 effect 会进入 pending 队列;
        /// computed 是 pull 模型, 永远不会被 flush 调用。
        virtual void run(Graph& graph) = 0;

        std::uint64_t id = 0;
        ReactorState state = ReactorState::clean;
        /// 该 reactor 正在订阅的 source 列表 (非拥有)。
        std::vector<Source*> sources;
        bool in_queue = false; ///< 是否已在 pending 队列中 (去重)。
        bool disposed = false; ///< 是否已被 dispose。
    };

    /// 响应式调度图。所有 signal / computed / effect 必须依附于一个 Graph 实例。
    class Graph {
    public:
        class DeferredEffects {
        public:
            explicit DeferredEffects(Graph& graph);
            ~DeferredEffects();

            DeferredEffects(const DeferredEffects&) = delete;
            auto operator=(const DeferredEffects&) -> DeferredEffects& = delete;
            DeferredEffects(DeferredEffects&& other) noexcept;
            auto operator=(DeferredEffects&& other) noexcept -> DeferredEffects&;

            /// Resume automatic flushing and run one pending effect wave.
            /// Signal values remain synchronous while the scope is active.
            void commit();

        private:
            Graph* graph_;
        };

        class ReadScope {
        public:
            ReadScope(Graph& graph, Reactor* reactor):
                graph_(&graph),
                previous_(graph.begin_read(reactor)) {}

            ~ReadScope() {
                graph_->end_read(previous_);
            }

            ReadScope(const ReadScope&) = delete;
            auto operator=(const ReadScope&) -> ReadScope& = delete;

        private:
            Graph* graph_;
            Reactor* previous_;
        };

        Graph() = default;
        ~Graph();

        Graph(const Graph&) = delete;
        auto operator=(const Graph&) -> Graph& = delete;
        Graph(Graph&&) = delete;
        auto operator=(Graph&&) -> Graph& = delete;

        /// 分配下一个唯一 id。
        [[nodiscard]] auto next_id() -> std::uint64_t;

        // ── 节点持有 ────────────────────────────────────────────────────────────
        /// 把 Computed/Effect 的所有权交给 Graph, 返回裸指针供业务持有。
        auto adopt(std::unique_ptr<Reactor> reactor) -> Reactor*;

        /// 提前销毁单个 reactor: 解绑依赖后释放。幂等。
        void dispose_reactor(Reactor* reactor);

        // ── 依赖追踪 ────────────────────────────────────────────────────────────
        /// 把当前 reader (若存在) 登记为 source 的订阅者, 双向去重。
        void track(Source& source) const;

        /// 进入读取上下文, 返回先前的 reader (用于嵌套恢复)。
        [[nodiscard]] auto begin_read(Reactor* reactor) -> Reactor*;
        void end_read(Reactor* previous);
        [[nodiscard]] auto enter_read(Reactor* reactor) -> ReadScope {
            return ReadScope {*this, reactor};
        }

        /// 清空 reactor 的所有依赖边 (双向)。在 reactor 即将重新执行时调用。
        void clear_reactor_deps(Reactor& reactor);

        /// 解除 source 与其全部订阅者的双向绑定 (source 即将销毁时调用)。
        void detach_source(Source& source);

        // ── 失效与调度 ──────────────────────────────────────────────────────────
        /// source 值变化后调用: 递增 version, 把全部订阅者标 dirty,
        /// 非 batch / deferred scope 且非 flush 中时立即 flush。
        void notify_source(Source& source);

        /// 把 reactor 标记 dirty 并执行其失效传播逻辑 (判重, 幂等)。
        void invalidate_reactor(Reactor& reactor);

        /// 把 reactor 加入待执行队列 (去重)。由 effect 的 on_invalidate 调用。
        void enqueue(Reactor& reactor);

        /// 执行所有待处理 reactor。同步模式下由 source 变化或 batch 退出自动调用。
        void flush();
        void run_effect_once(Reactor& reactor);

        [[nodiscard]] auto has_pending_effects() const -> bool;

        /// Defer effect flushing until commit(). This is the application tick
        /// boundary; standalone Graph users retain synchronous behavior.
        [[nodiscard]] auto defer_effects() -> DeferredEffects {
            return DeferredEffects {*this};
        }

        // ── batch ───────────────────────────────────────────────────────────────
        void begin_batch();
        void end_batch();

        [[nodiscard]] auto is_tearing_down() const -> bool {
            return tearing_down_;
        }

    private:
        void begin_deferred_effects();
        void end_deferred_effects(bool flush_pending);

        std::uint64_t next_id_ = 1;
        Reactor* current_reader_ = nullptr;
        std::vector<Reactor*> pending_;
        std::vector<Reactor*> next_pending_;
        std::unordered_set<Reactor*> ran_in_flush_;
        std::uint32_t batch_depth_ = 0;
        std::uint32_t deferred_effect_depth_ = 0;
        bool flushing_ = false;
        bool tearing_down_ = false;
        /// Graph 持有的 Computed/Effect。Signal 由调用方持有, 不在此列。
        std::vector<std::unique_ptr<Reactor>> reactors_;
    };

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_GRAPH_HPP
