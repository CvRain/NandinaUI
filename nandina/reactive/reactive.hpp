//
// Created by cvrain on 2026/7/3.
//
// reactive — 响应式核心层 (Angular signal 风格), 伞形头文件。
//
// 提供 signal / computed / effect / batch 等原语, 基于显式调度图 Graph
// (无全局状态、多实例隔离)。移植自 NandinaUI v2 (Zig) 的 reactive 层, 语义一致:
// Push 失效 + Pull 取值, computed 惰性重算, effect 由调度队列执行,
// 菱形依赖 glitch-free, 动态依赖自动重追踪。
//
// ## 心智模型
//   var g = Graph{};
//   Signal<int> count{g, 0};
//   auto* doubled = make_computed(g, [&]{ return count.get() * 2; });
//   make_effect(g, [&]{ std::printf("%d\n", doubled->get()); });
//   count.set(5);  // effect 重跑, doubled = 10
//
// 生命周期: Signal 由调用方持有 (栈/成员), 须在 Graph 之前析构;
// Computed/Effect 由 Graph 持有, 可提前 dispose() 或交给 Graph 析构统一释放。
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_HPP
#define NANDINA_EXPERIMENT_REACTIVE_HPP

#include "batch.hpp"
#include "computed.hpp"
#include "effect.hpp"
#include "event.hpp"
#include "graph.hpp"
#include "property.hpp"
#include "scope.hpp"
#include "signal.hpp"

#endif // NANDINA_EXPERIMENT_REACTIVE_HPP
