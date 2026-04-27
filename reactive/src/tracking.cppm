//
// Created by cvrain on 2026/4/26.
//

module;

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <utility>
#include <vector>

export module nandina.reactive.tracking;

export namespace nandina::reactive::detail {

// ═════════════════════════════════════════════════════════════════════════════
// § 依赖追踪上下文
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 表示一个正在执行的 Effect 或 Computed 的追踪上下文。
 *
 * @note 依赖追踪仅在**同一线程**内有效。
 *       当 State 被读取时，会检查当前线程的 current_tracking_context，
 *       若存在则注册该上下文对应的 observer。
 */
struct TrackingContext {
    std::size_t                      id = 0;
    std::function<void()>* invalidate = nullptr;
};

/// 当前线程的活跃追踪上下文（nullptr 表示没有活跃的 Effect/Computed）
inline thread_local TrackingContext* current_tracking_context = nullptr;

/**
 * @brief RAII guard，在作用域内设置 current_tracking_context。
 *
 * 在 Effect::run() 和 Computed::recompute() 开始时 push，
 * 退出时自动恢复为之前的上下文（支持嵌套 Effect）。
 */
class TrackingContextGuard {
public:
    explicit TrackingContextGuard(TrackingContext& context) noexcept
        : previous_(current_tracking_context) {
        current_tracking_context = &context;
    }

    TrackingContextGuard(const TrackingContextGuard&) = delete;
    auto operator=(const TrackingContextGuard&) -> TrackingContextGuard& = delete;

    ~TrackingContextGuard() {
        current_tracking_context = previous_;
    }

private:
    TrackingContext* previous_ = nullptr;
};

/// 生成全局唯一的追踪 ID
inline auto next_tracking_id() -> std::size_t {
    static std::atomic_size_t next_id{1};
    return next_id.fetch_add(1, std::memory_order_relaxed);
}

// ═════════════════════════════════════════════════════════════════════════════
// § Observer 条目与辅助
// ═════════════════════════════════════════════════════════════════════════════

struct ObserverEntry {
    std::size_t            id = 0;
    bool                   active = true;
    std::function<void()>  invalidate;
};

inline auto has_observer_id(const std::vector<ObserverEntry>& observers, std::size_t id) -> bool {
    for (const auto& observer : observers) {
        if (observer.active && observer.id == id) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 在通知过程中保护 observer 列表的弱不变性。
 *
 * 当 State 通知其 observers 时，某个 observer 的回调（invalidate）可能会
 * 重新注册新的 observer。PendingObserverRestore 确保在通知完成后，
 * 如果有新增的 observer 未被遍历到，会被追加到 observers_ 列表的末尾。
 */
class PendingObserverRestore {
public:
    PendingObserverRestore(std::vector<ObserverEntry>& observers,
                           const std::vector<ObserverEntry>& snapshot,
                           std::size_t& next_index) noexcept
        : observers_(observers), snapshot_(snapshot), next_index_(next_index) {}

    PendingObserverRestore(const PendingObserverRestore&) = delete;
    auto operator=(const PendingObserverRestore&) -> PendingObserverRestore& = delete;

    ~PendingObserverRestore() {
        if (committed_) {
            return;
        }

        for (std::size_t index = next_index_; index < snapshot_.size(); ++index) {
            const auto& observer = snapshot_[index];
            if (!observer.active || has_observer_id(observers_, observer.id)) {
                continue;
            }
            observers_.push_back(observer);
        }
    }

    auto commit() noexcept -> void {
        committed_ = true;
    }

private:
    std::vector<ObserverEntry>&       observers_;
    const std::vector<ObserverEntry>& snapshot_;
    std::size_t&                      next_index_;
    bool                              committed_ = false;
};

// ═════════════════════════════════════════════════════════════════════════════
// § 批处理（Pending Invalidation Queue）
// ═════════════════════════════════════════════════════════════════════════════

struct PendingInvalidation {
    std::size_t            id = 0;
    std::function<void()>  invalidate;
};

/// 批处理嵌套深度（0 表示不在批量更新中）
inline thread_local std::size_t batch_depth = 0;

/// 当前线程暂存的待失效回调
inline thread_local std::vector<PendingInvalidation> pending_invalidations;

[[nodiscard]] inline auto is_batching() noexcept -> bool {
    return batch_depth > 0;
}

/// 将失效回调加入待处理队列（去重）
inline auto queue_invalidation(std::size_t id, const std::function<void()>& invalidate) -> void {
    for (const auto& pending : pending_invalidations) {
        if (pending.id == id) {
            return;
        }
    }
    pending_invalidations.push_back(PendingInvalidation{id, invalidate});
}

/// 执行所有暂存的失效回调（支持重入：执行过程中可能产生新的暂存）
inline auto flush_pending_invalidations() -> void {
    std::exception_ptr first_exception;

    while (!pending_invalidations.empty()) {
        auto pending = std::move(pending_invalidations);
        pending_invalidations.clear();

        for (auto& entry : pending) {
            try {
                entry.invalidate();
            } catch (...) {
                if (!first_exception) {
                    first_exception = std::current_exception();
                }
            }
        }
    }

    if (first_exception) {
        std::rethrow_exception(first_exception);
    }
}

/**
 * @brief RAII 批处理作用域。
 *
 * batch(fn) 的内部机制。进入时递增嵌套深度，
 * 退出时递减，并且只有最外层（depth == 1）的
 * 析构才会触发 flush_pending_invalidations()。
 */
class BatchScope {
public:
    BatchScope() noexcept {
        ++batch_depth;
    }

    BatchScope(const BatchScope&) = delete;
    auto operator=(const BatchScope&) -> BatchScope& = delete;

    ~BatchScope() {
        if (batch_depth == 0) {
            return;
        }
        --batch_depth;
    }

    [[nodiscard]] auto should_flush() const noexcept -> bool {
        return batch_depth == 1;
    }
};

} // namespace nandina::reactive::detail