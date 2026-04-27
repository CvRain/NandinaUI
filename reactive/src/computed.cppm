//
// Created by cvrain on 2026/4/26.
//

module;

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

export module nandina.reactive.computed;

export import nandina.reactive.tracking;

export namespace nandina::reactive {

// ═════════════════════════════════════════════════════════════════════════════
// § Computed<F> — 惰性求值的派生值
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 惰性缓存、自动失效的派生值。
 *
 * Computed 封装一个计算函数，在其内部读取的 State 发生变化时自动标记为
 * "脏"（stale）。下一次读取时重新求值并缓存结果。
 *
 * @note Computed 是 move-only、不可复制的。在组件中应作为具名成员声明。
 * @note 求值是惰性的：只有调用 operator() 时才重新计算。
 *
 * 与 Effect 的区别：
 *   - Computed 是惰性的，而 Effect 是 eager 的（构造时立即执行）。
 *   - Computed 产生一个缓存值，Effect 产生副作用。
 *
 * @tparam F 调用签名，必须是 std::invocable
 *
 * @code
 * State<int> a{1}, b{2};
 * Computed sum{[&]{ return a() + b(); }};
 * fmt::print("{}\n", sum());  // 3
 * a.set(10);
 * fmt::print("{}\n", sum());  // 12（自动重算）
 * @endcode
 */
template<typename F>
    requires std::invocable<F>
class Computed {
public:
    using ValueType = std::invoke_result_t<F>;

    explicit Computed(F compute_fn)
        : compute_fn_(std::move(compute_fn)), stale_(true),
          observer_id_(detail::next_tracking_id()) {}

    Computed(const Computed&)          = delete;
    auto operator=(const Computed&)    = delete;
    Computed(Computed&&)               = delete;
    auto operator=(Computed&&)         = delete;

    /// 获取计算值。如果脏了则重新求值。
    [[nodiscard]] auto operator()() const -> const ValueType& {
        if (stale_) { recompute(); }
        return cached_;
    }

    /// 获取计算值的别名
    [[nodiscard]] auto get() const -> const ValueType& { return (*this)(); }

private:
    /// 在依赖追踪上下文中重算
    auto recompute() const -> void {
        auto invalidator = std::function<void()>{ [this]{ stale_ = true; } };
        detail::TrackingContext context{observer_id_, &invalidator};
        detail::TrackingContextGuard guard{context};

        auto recomputed_value = compute_fn_();
        cached_ = std::move(recomputed_value);
        stale_ = false;
    }

    F                  compute_fn_;
    mutable ValueType  cached_{};
    mutable bool       stale_;
    std::size_t        observer_id_;
};

/// CTAD 推导指引
template<typename F>
Computed(F) -> Computed<F>;

} // namespace nandina::reactive