//
// Created by cvrain on 2026/7/3.
//
// reactive/computed — 惰性派生值 (移植自 v2 Zig 版)
//
// Computed: 用于创建派生值，能够随依赖变化而自动重新计算。
//
// make_computed<T>(g, fn) 创建一个派生值: fn() 返回 T, 执行期间读取的
// signal / 其它 computed 自动成为依赖。它同时是:
//   - 一个 Reactor: 订阅上游依赖, 依赖变化时把自己标 dirty 并把失效继续向下游传播;
//   - 一个 Source:  可被下游 computed / effect 依赖。
//
// 求值策略: lazy pull。依赖变化时只标记 dirty 并传播失效, 不立即重算;
// 下一次 get() 读到 dirty 才重新执行并缓存。读取顺序天然保证「先算依赖、再算派生」,
// 无需显式拓扑排序, 也避免了中间态 (glitch)。
//
// 句柄: 返回 Computed<T>* (Graph 持有其所有权)。dispose() 提前释放;
// 未 dispose 的由 Graph 析构统一释放。
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_COMPUTED_HPP
#define NANDINA_EXPERIMENT_REACTIVE_COMPUTED_HPP

#include "graph.hpp"

#include <functional>
#include <memory>
#include <utility>

namespace nandina::reactive
{

    /// 惰性缓存、自动失效的派生值。
    template<typename T>
    class Computed final: public Reactor {
    public:
        /// 由 make_computed 构造。compute 捕获所需的上游 signal/computed。
        Computed(Graph& graph, std::function<T()> compute):
            graph_(&graph),
            compute_(std::move(compute)) {
            id = graph.next_id();
            // 初始为 dirty: 尚未求值, 首次 get 触发计算。
            state = ReactorState::dirty;
            source_.id = graph.next_id();
        }

        ~Computed() override {
            disposed = true;
            if (!graph_->is_tearing_down()) {
                graph_->clear_reactor_deps(*this);
                graph_->detach_source(source_);
            }
        }

        /// 读取派生值; 若依赖已变化则重算。在追踪上下文中自动注册依赖。
        [[nodiscard]] auto get() -> const T& {
            graph_->track(source_);
            if (state == ReactorState::dirty) {
                recompute();
            }
            return cached_;
        }

        /// 不建立依赖地读取; 若 dirty 仍会重算以返回最新值。
        [[nodiscard]] auto peek() -> const T& {
            if (state == ReactorState::dirty) {
                recompute();
            }
            return cached_;
        }

        /// 提前释放该 computed: 解绑上下游依赖、释放内存。幂等。
        void dispose() {
            graph_->dispose_reactor(this);
        }

        // ── Reactor 接口 ──────────────────────────────────────────────────────────

        /// 上游变化时把失效继续传播给本 computed 的订阅者。
        /// 因 invalidate_reactor 在置 dirty 前会判重, 此处不会重复传播。
        void on_invalidate(Graph& graph) override {
            // 失效传播可能改变订阅关系，遍历稳定快照而不是原容器。
            const auto snapshot = source_.subs;
            for (auto* sub: snapshot) {
                graph.invalidate_reactor(*sub);
            }
        }

        /// computed 是 pull 模型, 永不进入 pending 队列, run 不应被调用。
        void run(Graph& /*graph*/) override {
            // unreachable
        }

    private:
        void recompute() {
            graph_->clear_reactor_deps(*this);
            state = ReactorState::clean;
            auto read = graph_->enter_read(this);
            cached_ = compute_();
        }

        Graph* graph_;
        Source source_;
        std::function<T()> compute_;
        T cached_ {};
    };

    /// 创建一个 computed。fn 捕获上游依赖, 签名为 T()。
    /// 求值惰性: 构造时不执行 fn, 首次 get() 时才计算并缓存。
    template<typename Fn>
    [[nodiscard]] auto make_computed(Graph& graph, Fn&& fn) {
        using T = std::invoke_result_t<Fn>;
        auto owned = std::make_unique<Computed<T>>(graph, std::function<T()>(std::forward<Fn>(fn)));
        auto* raw = owned.get();
        graph.adopt(std::move(owned));
        return raw;
    }

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_COMPUTED_HPP
