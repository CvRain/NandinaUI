//
// Created by cvrain on 2026/7/3.
//
// reactive/effect — 响应式副作用 (移植自 v2 Zig 版)
//
// make_effect(g, fn) 注册一个副作用: 构造时立即执行一次 fn(), 执行期间读取的任意
// signal / computed 都会成为它的依赖; 任一依赖变化后, effect 被重新调度执行
// (通常立即执行, batch 或应用 tick 延迟域内合并到一次 flush)。
//
// 句柄: 返回 Effect* (Graph 持有所有权)。dispose() 提前停止追踪并释放;
// 未显式 dispose 的会在 Graph 析构时统一释放。
//
// EffectScope 把一组 effect 的生命周期捆绑, 析构时整体 dispose —— 对应组件卸载时
// 自动清理订阅的场景。
//
// 重入保护: 一次 flush 中每个 effect 最多执行一次。effect 在执行期间再次使自己
// 失效时会进入下一次 flush，避免同一调度波次内自反馈锁死 UI 线程。
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_EFFECT_HPP
#define NANDINA_EXPERIMENT_REACTIVE_EFFECT_HPP

#include "graph.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace nandina::reactive
{

    /// 副作用句柄。由 make_effect 创建, Graph 持有所有权。
    class Effect final: public Reactor {
    public:
        Effect(Graph& graph, std::function<void()> fn): graph_(&graph), fn_(std::move(fn)) {
            id = graph.next_id();
        }

        ~Effect() override {
            disposed = true;
            if (!graph_->is_tearing_down()) {
                graph_->clear_reactor_deps(*this);
            }
        }

        /// 提前停止该 effect: 解绑依赖、释放内存。幂等。dispose 后句柄失效。
        void dispose() {
            graph_->dispose_reactor(this);
        }

        /// 在依赖追踪上下文中执行用户回调, 重建依赖边。
        void execute() {
            graph_->clear_reactor_deps(*this);
            state = ReactorState::clean;
            auto read = graph_->enter_read(this);
            fn_();
        }

        // ── Reactor 接口 ──────────────────────────────────────────────────────────

        /// 依赖变化时把自己排进 pending 队列。
        void on_invalidate(Graph& graph) override {
            graph.enqueue(*this);
        }

        /// 被 flush 调度时重新执行。
        void run(Graph& /*graph*/) override {
            execute();
        }

    private:
        Graph* graph_;
        std::function<void()> fn_;
    };

    /// 创建并立即执行一个 effect。fn 内对 signal/computed 的读取建立依赖。
    template<typename Fn>
        requires std::invocable<Fn>
    auto make_effect(Graph& graph, Fn&& fn) -> Effect* {
        auto owned = std::make_unique<Effect>(graph, std::function<void()>(std::forward<Fn>(fn)));
        // adopt 转移所有权并返回 Graph 持有的稳定地址。
        auto* raw = static_cast<Effect*>(graph.adopt(std::move(owned)));
        graph.run_effect_once(*raw); // 立即执行一次, 建立初始依赖。
        return raw;
    }

    /// 一组 effect 的生命周期容器。析构时整体 dispose, 对应组件卸载清理。
    class EffectScope {
    public:
        explicit EffectScope(Graph& graph): graph_(&graph) {}

        ~EffectScope() {
            clear();
        }

        EffectScope(const EffectScope&) = delete;
        auto operator=(const EffectScope&) -> EffectScope& = delete;
        EffectScope(EffectScope&&) = delete;
        auto operator=(EffectScope&&) -> EffectScope& = delete;

        /// 在本作用域内注册一个 effect。
        template<typename Fn>
            requires std::invocable<Fn>
        auto add(Fn&& fn) -> Effect* {
            auto* e = make_effect(*graph_, std::forward<Fn>(fn));
            effects_.push_back(e);
            return e;
        }

        /// dispose 作用域内全部 effect (保留容器, 可继续 add)。
        void clear() {
            for (auto* e: effects_) {
                e->dispose();
            }
            effects_.clear();
        }

        /// 当前持有的 effect 数量。
        [[nodiscard]] auto count() const -> std::size_t {
            return effects_.size();
        }

    private:
        Graph* graph_;
        std::vector<Effect*> effects_;
    };

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_EFFECT_HPP
