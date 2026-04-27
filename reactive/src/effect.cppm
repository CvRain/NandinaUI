//
// Created by cvrain on 2026/4/26.
//

module;

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

export module nandina.reactive.effect;

export import nandina.reactive.tracking;

export namespace nandina::reactive {

// ═════════════════════════════════════════════════════════════════════════════
// § Effect — 响应式副作用
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 响应式副作用。
 *
 * 构造时立即执行一次 fn()，并在其内部读取的任意 State 发生变化时
 * 自动重新执行。
 *
 * Effect 在析构时停止追踪。它是不可复制的、不可移动的，应在组件中
 * 作为具名成员声明。
 *
 * @note Effect 的重新执行通过依赖追踪系统自动调度。使用 batch(fn)
 *       可以合并多个 State 变化，只触发一次重新执行。
 *
 * @code
 * State<int> count{0};
 * Effect e([&]{ fmt::print("count = {}\n", count()); });
 * count.set(1);  // 自动重打
 * @endcode
 */
class Effect {
public:
    template<typename F>
        requires std::invocable<F>
    explicit Effect(F&& fn)
        : fn_(std::forward<F>(fn)), observer_id_(detail::next_tracking_id()) {
        run();
    }

    Effect(const Effect&)         = delete;
    auto operator=(const Effect&) = delete;
    Effect(Effect&&)              = delete;
    auto operator=(Effect&&)      = delete;

    /// 析构时停止追踪
    ~Effect() { active_ = false; }

private:
    /// 在依赖追踪上下文中执行用户函数
    auto run() -> void {
        if (!active_) { return; }
        self_invalidator_ = [this]{ run(); };
        detail::TrackingContext context{observer_id_, &self_invalidator_};
        detail::TrackingContextGuard guard{context};
        fn_();
    }

    std::function<void()>  fn_;
    bool                   active_ = true;
    std::function<void()>  self_invalidator_;
    std::size_t            observer_id_;
};

// ═════════════════════════════════════════════════════════════════════════════
// § EffectScope — Effect 容器
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 管理多个 Effect 生命周期的容器。
 *
 * Effect 不可移动，因此不能直接存储在 std::vector 中。
 * EffectScope 通过 unique_ptr 间接存储。
 *
 * push(fn) 向作用域添加一个 Effect。
 * clear() 移除所有 Effect。
 * 析构时自动清理。
 */
class EffectScope {
public:
    EffectScope() = default;
    EffectScope(const EffectScope&)            = delete;
    EffectScope& operator=(const EffectScope&) = delete;
    EffectScope(EffectScope&&)                 = delete;
    EffectScope& operator=(EffectScope&&)      = delete;

    ~EffectScope() = default;

    /// 添加一个新的 Effect 到作用域
    template<typename F>
        requires std::invocable<F>
    auto push(F&& fn) -> void {
        effects_.push_back(std::make_unique<Effect>(std::forward<F>(fn)));
    }

    /// 移除所有 Effect
    auto clear() -> void {
        effects_.clear();
    }

    /// 当前 Effect 数量
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return effects_.size();
    }

    /// 是否为空
    [[nodiscard]] auto empty() const noexcept -> bool {
        return effects_.empty();
    }

private:
    std::vector<std::unique_ptr<Effect>> effects_;
};

} // namespace nandina::reactive