module;

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

export module nandina.app.computed_var;

// ── 设计说明 ──────────────────────────────────────────────────────────────────
// 与 Var<T> 相同的 GCC-safe Pimpl 模式。
// ComputedVar<T> 持有 shared_ptr<void> 指向堆上的 ComputedVarImpl<T>。
//
// ComputedVarImpl<T>（在实现单元）内部结构：
//   - std::function<T()>  fn_         — 计算函数
//   - State<T>            cached_state — 实际存储值（参与 Effect 追踪图）
//   - Effect              compute_effect — 监听依赖，重算 cached_state
//
// operator()() 调用 cached_state()，后者调用 State::track_access()，
// 使外部 Effect（如 text(fn)）能将 ComputedVar 纳入依赖图。
//
// 追踪链：
//   m_count 变化 → compute_effect 重跑 fn_() → cached_state.set()
//   → cached_state 通知 → 外部 text(fn) 的 Effect 重跑 → set_text()
// ─────────────────────────────────────────────────────────────────────────────

export namespace nandina::app {

/**
 * @brief 页面级只读派生 signal — 自动计算、自动追踪。
 *
 * 与 Var<T> 相同的 GCC-safe Pimpl 模式，可安全作为导出类成员。
 * 内部使用 State<T> 存储缓存值，因此完整参与 Effect 依赖图。
 *
 * @code
 *   class MyPage : public NanPage {
 *       Var<int>         m_count{0};
 *       ComputedVar<int> m_double{[this]{ return m_count() * 2; }};
 *
 *       auto build() -> ... {
 *           auto lbl = label()
 *               .text([this]{ return std::to_string(m_double()); });
 *           // m_count 变化 → m_double 自动更新 → label 自动重渲
 *       }
 *   };
 * @endcode
 */
template<typename T>
class ComputedVar {
public:
    template<typename F>
        requires std::invocable<F> &&
                 std::convertible_to<std::invoke_result_t<F>, T>
    explicit ComputedVar(F fn) {
        init_impl(std::function<T()>{std::move(fn)});
    }

    ComputedVar(ComputedVar&&) noexcept            = default;
    ComputedVar& operator=(ComputedVar&&) noexcept = default;
    ComputedVar(const ComputedVar&)                = delete;
    ComputedVar& operator=(const ComputedVar&)     = delete;

    // ── 值访问（同时在 Effect/Computed 中注册依赖追踪） ──────────────────────

    /// 读取当前派生值（通过 cached_state()，参与外部 Effect 依赖图）
    [[nodiscard]] auto operator()() const -> const T&;
    [[nodiscard]] auto get()        const -> const T& { return (*this)(); }

private:
    auto init_impl(std::function<T()> fn) -> void;

    std::shared_ptr<void> m_impl;
};

// 显式实例化声明（定义在 nan_computed_var_impl.cpp 中）
extern template class ComputedVar<int>;
extern template class ComputedVar<float>;
extern template class ComputedVar<double>;
extern template class ComputedVar<bool>;

} // namespace nandina::app
