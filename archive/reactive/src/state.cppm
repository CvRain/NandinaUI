//
// Created by cvrain on 2026/4/26.
//

module;

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <type_traits>

export module nandina.reactive.state;

export import nandina.reactive.tracking;
export import nandina.reactive.event_signal;

export namespace nandina::reactive {

// ═════════════════════════════════════════════════════════════════════════════
// 前向声明
// ═════════════════════════════════════════════════════════════════════════════

template<typename T> class State;
template<typename T> class ReadState;

// ═════════════════════════════════════════════════════════════════════════════
// § State<T> — 响应式状态值
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 可变的响应式值容器。
 *
 * 在 Effect 或 Computed 内部读取 State 会自动注册依赖追踪；
 * 写入 State 则会通知所有注册的 observer。
 *
 * 构造后不可移动（ReadState 持有指向它的原始指针）。
 * 在 widget 中作为具名成员变量声明。
 *
 * 变化检测：
 *   - 如果 T 满足 std::equality_comparable，则仅在值真正变化时通知
 *   - 否则每次 set() 都通知 observer
 *
 * @code
 * State<int> count{0};
 * Effect e([&]{ fmt::print("count = {}\n", count()); });
 * count.set(1);  // Effect 自动重跑
 * @endcode
 */
template<typename T>
class State {
public:
    explicit State(T initial_value) : value_(std::move(initial_value)) {}

    State(const State&)          = delete;
    auto operator=(const State&) = delete;
    State(State&&)               = delete;
    auto operator=(State&&)      = delete;

    /// 读取值，同时在当前 Effect/Computed 中注册依赖
    [[nodiscard]] auto operator()() const -> const T& {
        track_access();
        return value_;
    }

    /// 读取值的别名，语义与 operator() 相同
    [[nodiscard]] auto get() const -> const T& { return (*this)(); }

    /// 写入新值，仅在值实际发生变化时通知 observer
    auto set(T new_value) -> void {
        if (has_changed(new_value)) {
            value_ = std::move(new_value);
            notify();
        }
    }

    /// 原地修改：fn(T&) -> void，避免拷贝（适合 ++/-- 等操作）
    template<typename Fn>
        requires std::invocable<Fn, T&> && std::is_void_v<std::invoke_result_t<Fn, T&>>
    auto update(Fn fn) -> void {
        T new_value = value_;
        fn(new_value);
        set(std::move(new_value));
    }

    /// 值语义修改：fn(const T&) -> T（Angular 风格，返回新值）
    template<typename Fn>
        requires std::invocable<Fn, const T&> &&
                 (!std::is_void_v<std::invoke_result_t<Fn, const T&>>)
    auto update(Fn fn) -> void {
        set(T{fn(value_)});
    }

    // ── 外部订阅（脱离 Effect 追踪系统） ──

    /**
     * @brief 注册一个值变化回调（非 Effect 方式）。
     *
     * 不会参与依赖追踪。用于需要响应 State 变化但不属于 Effect 的场景，
     * 例如将 State 绑定到 UI 属性。
     */
    auto on_change(std::function<void(const T&)> callback) const -> Connection {
        return change_signal_.connect(std::move(callback));
    }

    /// 返回一个只读视图
    [[nodiscard]] auto as_read_only() const -> ReadState<T>;

private:
    /// Effect 依赖追踪注册
    auto track_access() const -> void {
        if (!detail::current_tracking_context || !detail::current_tracking_context->invalidate) {
            return;
        }
        for (const auto& obs : observers_) {
            if (obs.active && obs.id == detail::current_tracking_context->id) { return; }
        }
        observers_.push_back({
            detail::current_tracking_context->id,
            true,
            *detail::current_tracking_context->invalidate,
            detail::current_tracking_context->alive
                ? *detail::current_tracking_context->alive
                : std::shared_ptr<bool>{}
        });
    }

    /// 通知所有 observer（支持批处理）
    auto notify() -> void {
        auto snapshot = std::move(observers_);
        observers_    = {};
        std::size_t next_observer = 0;
        detail::PendingObserverRestore restore_guard{observers_, snapshot, next_observer};

        for (; next_observer < snapshot.size(); ++next_observer) {
            const auto& entry = snapshot[next_observer];
            if (!entry.active || !detail::observer_is_live(entry) || !entry.invalidate) {
                continue;
            }

            if (detail::is_batching()) {
                detail::queue_invalidation(entry.id, entry.invalidate);
            } else {
                entry.invalidate();
            }
        }

        restore_guard.commit();
        change_signal_.emit(value_);
    }

    /// 变化检测（SFINAE：支持 operator== 的 T 做比较，否则总是通知）
    auto has_changed(const T& new_value) const -> bool {
        if constexpr (std::equality_comparable<T>) {
            return !(value_ == new_value);
        } else {
            (void)new_value;
            return true;
        }
    }

    T                                         value_;
    mutable std::vector<detail::ObserverEntry> observers_;
    mutable EventSignal<const T&>              change_signal_;
};

// ═════════════════════════════════════════════════════════════════════════════
// § ReadState<T> — State 的只读视图
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief State<T> 的只读引用。
 *
 * 读取时也会自动注册 Effect 依赖追踪。
 * 可复制、可移动。通过 State::as_read_only() 构造。
 *
 * 用于将 State 的只读访问权传递给子组件，而不暴露 set() 接口。
 */
template<typename T>
class ReadState {
public:
    explicit ReadState(const State<T>* source) : source_(source) {
        // source 不允许为 nullptr
    }

    ReadState(const ReadState&) = default;
    auto operator=(const ReadState&) -> ReadState& = default;
    ReadState(ReadState&&) noexcept      = default;
    auto operator=(ReadState&&) noexcept -> ReadState& = default;

    [[nodiscard]] auto operator()() const -> const T& { return (*source_)(); }
    [[nodiscard]] auto get() const -> const T&        { return (*this)(); }
    [[nodiscard]] auto source_ptr() const noexcept -> const State<T>* { return source_; }

    /// 注册值变化回调（代理到源 State）
    auto on_change(std::function<void(const T&)> callback) const -> Connection {
        return source_->on_change(std::move(callback));
    }

private:
    const State<T>* source_;
};

// ── State<T>::as_read_only() 的模板定义 ──

template<typename T>
auto State<T>::as_read_only() const -> ReadState<T> { return ReadState<T>{this}; }

// ═════════════════════════════════════════════════════════════════════════════
// § StateSlot<T> — 可作为导出类成员的响应式状态槽
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 对 State<T> 的可移动包装，可直接声明为导出模块类的成员变量。
 *
 * 动机：State<T> 的 move constructor 被显式 delete，GCC 模块序列化
 * 无法处理含有 deleted-move 成员的导出类，必须借助 Pimpl 绕行。
 * StateSlot<T> 用 std::unique_ptr<State<T>> 封装，本身是可移动的，
 * 对外暴露与 State<T> 完全相同的读写和订阅 API。
 *
 * 初始化是懒惰的（首次访问时才构造 State<T>），因此 StateSlot 本身
 * 可以在类构造阶段安全初始化，不依赖任何运行时前提条件。
 *
 * @code
 * // 在导出类的 private 成员中：
 * StateSlot<int> m_count{0};          // 直接声明，无需匿名 namespace
 * ScopedConnection m_count_conn;      // 同样可直接声明
 *
 * // 在 build() / 初始化函数中：
 * m_count_conn = ScopedConnection{
 *     m_count.on_change([this](const int& v) { update_label(v); })
 * };
 * m_count.set(m_count() + 1);         // 写值
 * @endcode
 */
template<typename T>
class StateSlot {
public:
    explicit StateSlot(T initial) : m_initial(std::move(initial)) {}

    // 可移动（通过 unique_ptr 转移所有权），不可复制
    StateSlot(StateSlot&&) noexcept            = default;
    auto operator=(StateSlot&&) noexcept -> StateSlot& = default;
    StateSlot(const StateSlot&)                = delete;
    auto operator=(const StateSlot&)           = delete;

    /// 读取当前值（同时在 Effect/Computed 中注册依赖追踪）
    [[nodiscard]] auto operator()() const -> const T& { return impl()(); }

    /// 读取当前值的别名
    [[nodiscard]] auto get() const -> const T& { return impl().get(); }

    /// 写入新值，仅在值实际变化时通知订阅者
    auto set(T new_value) -> void { impl().set(std::move(new_value)); }

    /// 注册值变化回调，返回 Connection（可包装为 ScopedConnection）
    auto on_change(std::function<void(const T&)> cb) const -> Connection {
        return impl().on_change(std::move(cb));
    }

    /// 返回对内部 State<T> 的只读视图
    [[nodiscard]] auto as_read_only() const -> ReadState<T> {
        return impl().as_read_only();
    }

private:
    auto impl() const -> State<T>& {
        if (!m_impl) m_impl = std::make_unique<State<T>>(m_initial);
        return *m_impl;
    }

    T                              m_initial;
    mutable std::unique_ptr<State<T>> m_impl;
};

} // namespace nandina::reactive