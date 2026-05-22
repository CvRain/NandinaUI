// nan_var_impl.cpp — nandina.app.var 模块实现单元
//
// 职责：
//   - 定义 VarImpl<T>（含 State<T> + ScopedConnection，GCC 不序列化此文件）
//   - 实现所有 Var<T> 方法体
//   - 提供显式实例化（避免消费者在不知道 VarImpl 的情况下尝试隐式实例化）

// Global Module Fragment：#include 必须在 module 声明之前
module;
#include <functional>
#include <memory>
#include <string>
#include <utility>

module nandina.app.var;

import nandina.reactive;

// ── 模块私有实现（不进入 .gcm，GCC 不序列化）────────────────────────────────────
namespace {

template<typename T>
struct VarImpl {
    nandina::reactive::State<T>         state;
    nandina::reactive::ScopedConnection text_conn;
    nandina::reactive::ScopedConnection watch_conn;

    explicit VarImpl(T v) : state(std::move(v)) {}
};

} // anonymous namespace

// ── Var<T> 方法实现 ────────────────────────────────────────────────────────────
namespace nandina::app {

template<typename T>
Var<T>::Var(T initial)
    : m_impl(std::make_shared<VarImpl<T>>(std::move(initial))) {}

template<typename T>
auto Var<T>::operator()() const -> const T& {
    return static_cast<VarImpl<T>*>(m_impl.get())->state();
}

template<typename T>
auto Var<T>::get() const -> const T& {
    return static_cast<VarImpl<T>*>(m_impl.get())->state.get();
}

template<typename T>
auto Var<T>::set(T val) -> void {
    static_cast<VarImpl<T>*>(m_impl.get())->state.set(std::move(val));
}

template<typename T>
auto Var<T>::bind_text_impl(std::function<void(const T&)> update_fn) -> void {
    auto* d = static_cast<VarImpl<T>*>(m_impl.get());
    d->text_conn = nandina::reactive::ScopedConnection{
        d->state.on_change(std::move(update_fn))
    };
}

template<typename T>
auto Var<T>::watch(std::function<void(const T&)> cb) -> void {
    auto* d = static_cast<VarImpl<T>*>(m_impl.get());
    d->watch_conn = nandina::reactive::ScopedConnection{
        d->state.on_change(std::move(cb))
    };
}

// ── 显式实例化 ─────────────────────────────────────────────────────────────────
// 如需支持其他类型，在此添加 `template class Var<MyType>;` 并重建。
template class Var<int>;
template class Var<float>;
template class Var<double>;
template class Var<bool>;

} // namespace nandina::app
