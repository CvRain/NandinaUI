module;

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

export module nandina.app.var;

// ── 设计说明 ──────────────────────────────────────────────────────────────────
// GCC C++ 模块序列化会把导出类的成员类型写入 .gcm；含 deleted-move 的类型
// （State<T>）或含 std::function 的类型（ScopedConnection）会触发 GCC 序列化 bug。
//
// 解法：模块接口 / 实现分离 + 显式实例化
//   - 接口 (nan_var.cppm)    : Var<T> 只持有 shared_ptr<void>，方法仅做薄封装
//   - 实现 (nan_var_impl.cpp): VarImpl<T>（含 State/ScopedConnection）放匿名 namespace
//                              所有引用 VarImpl<T> 的方法体在此定义，GCC 不序列化
//   - 显式实例化             : extern template（接口）/ template（实现）
//     支持：int, float, double, bool, std::string
//     如需其他类型，在 nan_var_impl.cpp 中添加显式实例化并重建
// ─────────────────────────────────────────────────────────────────────────────

export namespace nandina::app {

/**
 * @brief 页面级响应式变量 —— 可直接作为导出类成员（GCC 模块安全）
 *
 * 内部使用 `shared_ptr<void>` 类型擦除，GCC 序列化时只看到两个指针，
 * 完全避免对 State<T> / ScopedConnection 的序列化需求。
 * 实现细节（VarImpl）放在 nan_var_impl.cpp 的匿名 namespace 中，不进入 .gcm。
 *
 * @code
 *   class MyPage : public NanPage {
 *       Var<int> m_count{1};               // 声明并初始化
 *       Ref<Button> m_label;
 *
 *       auto build() -> NanComponent::Ptr override {
 *           m_count.bind_text(m_label,     // 绑定显示（替换安全，可在 build 中重调）
 *               [](int v){ return std::to_string(v); });
 *
 *           auto btn = button("+").on_click([this]{
 *               m_count = m_count() + 1;   // 直接修改
 *           });
 *       }
 *   };
 * @endcode
 */
template<typename T>
class Var {
public:
    explicit Var(T initial);

    Var(Var&&) noexcept            = default;
    Var& operator=(Var&&) noexcept = default;
    Var(const Var&)                = delete;
    Var& operator=(const Var&)     = delete;

    // ── 值访问 ────────────────────────────────────────────────────────────────

    /// 读取当前值（同时在 Effect/Computed 中注册依赖追踪）
    [[nodiscard]] auto operator()() const -> const T&;
    [[nodiscard]] auto get()        const -> const T&;

    /// 写入新值，仅在实际变化时通知 observer
    auto set(T val) -> void;

    /// 赋值语法糖：`m_count = m_count() + 1;`
    auto operator=(T val) -> Var& { set(std::move(val)); return *this; }

    // ── 绑定 ──────────────────────────────────────────────────────────────────

    /**
     * @brief 将值变化绑定到 widget 的文本（每次调用替换旧连接，在 build() 中安全）。
     *
     * 薄模板包装层，不引用 VarImpl<T>，GCC 可正常序列化。
     * 实际连接由 bind_text_impl 建立（在 nan_var_impl.cpp 中显式实例化）。
     *
     * @param ref  目标 widget 的 Ref<W>（需实现 set_text(std::string_view)）
     * @param fn   转换函数：const T& → std::string
     */
    template<typename Ref, typename Fn>
        requires std::invocable<Fn, const T&> &&
                 std::convertible_to<std::invoke_result_t<Fn, const T&>, std::string>
    auto bind_text(Ref& ref, Fn fn) -> void {
        // 将类型擦除为 std::function<void(const T&)>，再交给不含模板的 impl 处理
        bind_text_impl([&ref, fn = std::move(fn)](const T& v) {
            if (ref) ref->set_text(fn(v));
        });
    }

    /**
     * @brief 注册副作用回调，随 Var 生命周期自动管理（每次调用替换旧 watch）。
     */
    auto watch(std::function<void(const T&)> cb) -> void;

    // ── 高级接口 ──────────────────────────────────────────────────────────────

    /// 内部实现（供 bind_text 调用，也可直接使用）
    auto bind_text_impl(std::function<void(const T&)> update_fn) -> void;

private:
    // 唯一成员：shared_ptr<void> 类型擦除，GCC 模块序列化无需追踪 VarImpl<T>
    std::shared_ptr<void> m_impl;
};

// 显式实例化声明（定义在 nan_var_impl.cpp 中）
// 消费此模块时，这些特化版本直接使用预编译代码，无需 VarImpl<T> 可见
extern template class Var<int>;
extern template class Var<float>;
extern template class Var<double>;
extern template class Var<bool>;

} // namespace nandina::app
