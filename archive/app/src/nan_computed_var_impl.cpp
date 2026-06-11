// nan_computed_var_impl.cpp — nandina.app.computed_var 模块实现单元
//
// 职责：
//   - 定义 ComputedVarImpl<T>（含 State<T> + Effect，GCC 不序列化此文件）
//   - 实现所有 ComputedVar<T> 方法体
//   - 提供显式实例化（int, float, double, bool）

// Global Module Fragment：#include 必须在 module 声明之前
module;
#include <functional>
#include <memory>
#include <utility>

module nandina.app.computed_var;

import nandina.reactive;

// ── 模块私有实现（不进入 .gcm，GCC 不序列化）────────────────────────────────────
namespace nandina::app::impl {

template<typename T>
struct ComputedVarImpl {
    std::function<T()>           fn_;
    nandina::reactive::State<T>  cached_state;
    nandina::reactive::Effect    compute_effect;

    // 成员初始化顺序 = 声明顺序：fn_ → cached_state → compute_effect
    //   - cached_state(fn_()) : 计算初始值（无 Effect 上下文，不注册依赖）
    //   - compute_effect{...} : 立即运行 fn_()，此时在 Effect 上下文中注册依赖
    //                           值与 cached_state 相同，set() 不触发通知
    explicit ComputedVarImpl(std::function<T()> fn)
        : fn_(std::move(fn))
        , cached_state(fn_())
        , compute_effect([this]{ cached_state.set(fn_()); })
    {}
};

} // namespace nandina::app::impl

// ── ComputedVar<T> 方法实现 ────────────────────────────────────────────────────
namespace nandina::app {

template<typename T>
auto ComputedVar<T>::init_impl(std::function<T()> fn) -> void {
    m_impl = std::make_shared<impl::ComputedVarImpl<T>>(std::move(fn));
}

template<typename T>
auto ComputedVar<T>::operator()() const -> const T& {
    // 调用 State<T>::operator()()，触发 track_access()，
    // 使调用此函数的外部 Effect 将 cached_state 纳入其依赖图。
    return static_cast<impl::ComputedVarImpl<T>*>(m_impl.get())->cached_state();
}

// ── 显式实例化 ─────────────────────────────────────────────────────────────────
template class ComputedVar<int>;
template class ComputedVar<float>;
template class ComputedVar<double>;
template class ComputedVar<bool>;

} // namespace nandina::app
