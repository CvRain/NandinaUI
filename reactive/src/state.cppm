//
// Created by cvrain on 2026/4/26.
//

module;

#include <cstddef>
#include <functional>
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
            *detail::current_tracking_context->invalidate
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
            if (!entry.active || !entry.invalidate) {
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

} // namespace nandina::reactive