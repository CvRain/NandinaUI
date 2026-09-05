//
// Created by cvrain on 2026/7/3.
//
// reactive/signal — 可写响应式状态 (移植自 v2 Zig 版)
//
// Signal<T> 是最基础的可写状态容器, 内嵌一个 graph::Source。
//   - get():        读取值; 若处于 effect/computed 执行上下文中, 自动建立依赖边。
//   - set(v)/update(fn): 写入; 仅在值真正变化时通知订阅者并触发调度。
//   - as_readonly(): 返回只读视图 ReadSignal<T>, 把读权限传给子组件而不泄漏写权限。
//
// 变化检测: 若 T 满足 std::equality_comparable, 仅在不等时通知; 否则每次 set 都通知
// (保守策略, 调用方可改用 update 精确控制)。
//
// 生命周期: Signal<T> 由调用方持有 (栈或结构体成员), 析构时解绑依赖边。
// 必须在所属 Graph 析构之前析构 (或由更上层的 Runtime 统一管理)。
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_SIGNAL_HPP
#define NANDINA_EXPERIMENT_REACTIVE_SIGNAL_HPP

#include "graph.hpp"

#include <concepts>
#include <utility>

namespace nandina::reactive
{

    template<typename T>
    class ReadSignal;

    template<typename T>
    class Signal;

    template<typename T>
    using UniqueSignal = std::unique_ptr<Signal<T>>;

    /// 可写的响应式状态容器。
    template<typename T>
    class Signal {
    public:
        /// 在 g 上创建一个初值为 initial 的 signal。
        Signal(Graph& graph, T initial): graph_(&graph), value_(std::move(initial)) {
            source_.id = graph.next_id();
        }

        ~Signal() {
            graph_->detach_source(source_);
        }

        Signal(const Signal&) = delete;
        auto operator=(const Signal&) -> Signal& = delete;
        Signal(Signal&&) = delete;
        auto operator=(Signal&&) -> Signal& = delete;

        /// 读取当前值; 在追踪上下文中自动注册依赖。
        [[nodiscard]] auto get() -> const T& {
            graph_->track(source_);
            return value_;
        }

        /// 不建立依赖地读取当前值 (peek)。用于不希望产生订阅关系的场景。
        [[nodiscard]] auto peek() const -> const T& {
            return value_;
        }

        /// 写入新值; 仅在值真正变化时通知订阅者并触发调度。
        void set(T new_value) {
            if (!has_changed(new_value)) {
                return;
            }
            value_ = std::move(new_value);
            graph_->notify_source(source_);
        }

        /// 原地修改: fn(T&) -> void, 适合 ++/-- 等操作, 避免显式读改写。
        /// 修改后总是通知 (无法判断 fn 是否真正改变了值)。
        template<typename Fn>
            requires std::invocable<Fn, T&> && std::is_void_v<std::invoke_result_t<Fn, T&>>
        void update(Fn&& fn) {
            std::forward<Fn>(fn)(value_);
            graph_->notify_source(source_);
        }

        /// 返回只读视图。
        [[nodiscard]] auto as_readonly() -> ReadSignal<T>;

        /// 内部访问 (供 ReadSignal / 测试)。
        [[nodiscard]] auto source_version() const -> std::uint64_t {
            return source_.version;
        }

    private:
        [[nodiscard]] auto has_changed(const T& new_value) const -> bool {
            if constexpr (std::equality_comparable<T>) {
                return !(value_ == new_value);
            }
            else {
                return true;
            }
        }

        Graph* graph_;
        Source source_;
        T value_;
    };

    /// Signal<T> 的只读视图: 可读取 (含依赖追踪), 但不能写入。
    /// 持有对源 signal 的非拥有指针, 调用方须保证源生命周期不短于本视图。
    template<typename T>
    class ReadSignal {
    public:
        explicit ReadSignal(Signal<T>* signal): signal_(signal) {}

        /// 读取当前值; 在追踪上下文中自动注册依赖。
        [[nodiscard]] auto get() const -> const T& {
            return signal_->get();
        }

        /// 不建立依赖地读取当前值。
        [[nodiscard]] auto peek() const -> const T& {
            return signal_->peek();
        }

    private:
        Signal<T>* signal_;
    };

    template<typename T>
    auto Signal<T>::as_readonly() -> ReadSignal<T> {
        return ReadSignal<T> {this};
    }

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_SIGNAL_HPP
