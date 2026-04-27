//
// Created by cvrain on 2026/4/26.
//

module;

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

export module nandina.reactive.prop;

export import nandina.reactive.event_signal;
export import nandina.reactive.state;

export namespace nandina::reactive {

// ═════════════════════════════════════════════════════════════════════════════
// § Prop<T> — 属性绑定
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 表示一个可由外部设置的值（类似组件的"属性"）。
 *
 * Prop 通常用于父组件向子组件传递配置，例如：
 * @code
 * struct MyWidget {
 *     Prop<std::string> label;
 *     Prop<int>         count{0};  // 默认值
 * };
 *
 * auto w = MyWidget{ .label = "Click me", .count = 42 };
 * @endcode
 *
 * 如果通过 on_change(callback) 注册了变化监听，当父组件重新赋值时会被通知。
 * Prop 本身**不参与** Effect 依赖追踪系统——它不包含 State 语义。
 * 若需要响应式绑定，子组件应在内部将 Prop 值复制到一个 State 中，
 * 或使用 Effect 来观察 Prop 的变化。
 *
 * @note 默认构造的 Prop 不连接任何事件源。
 *       如果从未注册 on_change，set/assign 是空操作。
 */
template<typename T>
class Prop {
public:
    Prop() noexcept = default;

    explicit Prop(T initial_value)
        : value_(std::move(initial_value)), has_value_(true) {}

    /// 从 ReadState<T> 构造（响应式绑定）
    explicit Prop(const ReadState<T>& read_state)
        : value_(read_state.get()), has_value_(true), is_reactive_(true)
    {
        state_connection_ = read_state.on_change([this](const T& new_value) {
            value_ = new_value;
            if (on_change_slot_) {
                on_change_slot_(value_);
            }
        });
    }

    Prop(const Prop& other)
        : value_(other.value_), has_value_(other.has_value_),
          is_reactive_(other.is_reactive_),
          on_change_slot_(other.on_change_slot_)
    {
        // state_connection_ is intentionally NOT copied — each copy
        // gets its own subscription set up separately if needed.
    }

    Prop& operator=(const Prop& other) {
        if (this != &other) {
            value_          = other.value_;
            has_value_      = other.has_value_;
            is_reactive_    = other.is_reactive_;
            on_change_slot_ = other.on_change_slot_;
            // state_connection_ is intentionally NOT copied.
        }
        return *this;
    }

    Prop(Prop&&) noexcept        = default;
    Prop& operator=(Prop&&) noexcept = default;

    /// 获取当前值
    [[nodiscard]] auto value() const -> const T& { return value_; }

    /// 获取当前值（同 value()）
    [[nodiscard]] auto get() const -> const T& { return value_; }

    /// 设置新值，如果有连接则触发 on_change
    auto set(const T& new_value) -> void {
        if (!has_value_ || !(value_ == new_value)) {
            value_     = new_value;
            has_value_ = true;
            if (on_change_slot_) {
                on_change_slot_(value_);
            }
        }
    }

    /// set() 的别名
    auto assign(const T& new_value) -> void { set(new_value); }

    /// 隐式转换为 T
    [[nodiscard]] operator const T&() const { return value_; }

    /// 是否连接了响应式数据源
    [[nodiscard]] auto is_reactive() const noexcept -> bool { return is_reactive_; }

    /// 注册变化回调（也可用于双向绑定）
    auto on_change(std::function<void(const T&)> callback) -> Connection {
        if (!is_reactive_) {
            // static Prop — no underlying source, store callback but
            // return a pre-disconnected Connection.
            on_change_slot_ = std::move(callback);
            return {};
        }
        on_change_slot_ = std::move(callback);
        return Connection{
            [this] { on_change_slot_ = {}; },
            [this] { return static_cast<bool>(on_change_slot_); }
        };
    }

    /// 是否已经有值
    [[nodiscard]] auto has_value() const noexcept -> bool { return has_value_; }

private:
    T                           value_{};
    bool                        has_value_ = false;
    bool                        is_reactive_ = false;
    Connection                  state_connection_;
    std::function<void(const T&)> on_change_slot_;
};

} // namespace nandina::reactive
