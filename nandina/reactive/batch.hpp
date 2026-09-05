//
// Created by cvrain on 2026/7/3.
//
// reactive/batch — 批量更新 (移植自 v2 Zig 版)
//
// batch(g, fn) 在一个批处理作用域内执行 fn(): 作用域内对 signal 的多次 set 只标记
// 失效、不立即触发 effect; 作用域退出 (最外层) 时统一 flush 一次, 把每个受影响的
// effect 合并为单次执行。支持嵌套; 若外层还有应用 tick 延迟域, 则留到其 reactive phase。
// 即使 fn 抛异常, endBatch 也保证被调用 (RAII guard), 不会卡在批量模式。
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_BATCH_HPP
#define NANDINA_EXPERIMENT_REACTIVE_BATCH_HPP

#include "graph.hpp"

#include <utility>

namespace nandina::reactive
{

    /// 在批处理作用域内执行 fn。fn 签名为 void()。
    /// std::invocable 约束 Fn 可无参数调用。
    template<typename Fn>
        requires std::invocable<Fn>
    void batch(Graph& graph, Fn&& fn) {
        struct Guard {
            Graph& g;
            explicit Guard(Graph& graph_ref): g(graph_ref) {
                g.begin_batch();
            }
            ~Guard() {
                g.end_batch();
            }
            Guard(const Guard&) = delete;
            auto operator=(const Guard&) -> Guard& = delete;
            Guard(Guard&&) = delete;
            auto operator=(Guard&&) -> Guard& = delete;
        } guard {graph};
        std::forward<Fn>(fn)();
    }

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_BATCH_HPP
