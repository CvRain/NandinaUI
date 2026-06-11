//
// Created by cvrain on 2026/5/13.
//

module;

#include <functional>
#include <type_traits>
#include <utility>

export module nandina.reactive.bindable_prop;

import nandina.reactive.state;
import nandina.reactive.event_signal;

/**
 * @file bindable_prop.cppm
 *
 * BindableProp<T> — 可绑定到响应式 State 的属性值包装器。
 *
 * 设计目标：
 *   连接 reactive::State<T> 与 widget 属性层，让 State 变化时 widget 自动标记脏。
 *
 * 用法示例：
 *   // 在 widget 内部：
 *   BindableProp<std::string> m_text{"default"};
 *
 *   // 静态赋值（同 Prop）：
 *   m_text = "hello";
 *
 *   // 绑定到 State（响应式连接）：
 *   State<std::string> title{"World"};
 *   m_text.bind(title, [this](const auto&) { mark_layout_dirty(); });
 *
 *   // 读取当前值（同 get()）：
 *   const auto& val = m_text.get();
 */

export namespace nandina::reactive {

/**
 * @brief 可绑定到响应式 State 的属性值。
 *
 * BindableProp 可以持有：
 *   - 一个静态值（通过 set/operator= 赋值）
 *   - 一个到 State<T> 的响应式绑定（通过 bind() 建立）
 *
 * 当绑定到 State 时，每次 State 变化会触发已注册的 invalidation callback
 * （通常是 widget 的 mark_layout_dirty / mark_dirty）。
 *
 * @tparam T 属性类型，必须可复制
 */
template<typename T>
class BindableProp {
public:
    BindableProp() noexcept = default;

    explicit BindableProp(T initial_value)
        : value_(std::move(initial_value)), has_value_(true) {}

    BindableProp(const BindableProp& other)
        : value_(other.value_), has_value_(other.has_value_)
    {
        // 不复制 state_connection_ — 每个副本独立
    }

    BindableProp& operator=(const BindableProp& other) {
        if (this != &other) {
            value_     = other.value_;
            has_value_ = other.has_value_;
            // 不复制 state_connection_
        }
        return *this;
    }

    BindableProp(BindableProp&&) noexcept = default;
    BindableProp& operator=(BindableProp&&) noexcept = default;

    // ── 静态赋值 ─────────────────────────────────────────────

    auto operator=(const T& new_value) -> BindableProp& {
        if (is_bound_) {
            // 解除绑定：自动断开 State 连接
            state_connection_ = {};
            is_bound_         = false;
        }
        value_     = new_value;
        has_value_ = true;
        return *this;
    }

    auto operator=(T&& new_value) noexcept -> BindableProp& {
        if (is_bound_) {
            state_connection_ = {};
            is_bound_         = false;
        }
        value_     = std::move(new_value);
        has_value_ = true;
        return *this;
    }

    auto set(const T& new_value) -> void {
        *this = new_value;
    }

    // ── 响应式绑定 ─────────────────────────────────────────

    /**
     * @brief 绑定到 State<T>。
     *
     * 每次 State 变化时，会先更新内部缓存值，然后调用 invalidation_fn。
     *
     * @param state     目标 State（必须存活期长于本 BindableProp）
     * @param on_change 失效回调（通常为 [this] { mark_dirty(); }）
     */
    template<typename F>
    auto bind(const State<T>& state, F&& on_change) -> void {
        // 先解除已有绑定
        state_connection_ = {};
        is_bound_         = false;

        value_     = state.get();
        has_value_ = true;
        is_bound_  = true;

        state_connection_ = state.on_change(
            [this, cb = std::forward<F>(on_change)](const T& new_value) {
                value_ = new_value;
                cb(new_value);
            }
        );
    }

    /**
     * @brief 绑定到 ReadState<T>（只读）。
     */
    template<typename F>
    auto bind(const ReadState<T>& read_state, F&& on_change) -> void {
        state_connection_ = {};
        is_bound_         = false;

        value_     = read_state.get();
        has_value_ = true;
        is_bound_  = true;

        state_connection_ = read_state.on_change(
            [this, cb = std::forward<F>(on_change)](const T& new_value) {
                value_ = new_value;
                cb(new_value);
            }
        );
    }

    /// 解除绑定的便捷方法
    auto unbind() -> void {
        state_connection_ = {};
        is_bound_         = false;
    }

    // ── 读取 ─────────────────────────────────────────────────

    [[nodiscard]] auto get() const -> const T& {
        return value_;
    }

    [[nodiscard]] operator const T&() const {
        return value_;
    }

    // ── 查询 ─────────────────────────────────────────────────

    [[nodiscard]] auto has_value() const noexcept -> bool {
        return has_value_;
    }

    [[nodiscard]] auto is_bound() const noexcept -> bool {
        return is_bound_;
    }

private:
    T          value_{};
    bool       has_value_   = false;
    bool       is_bound_    = false;
    Connection state_connection_;
};

} // namespace nandina::reactive