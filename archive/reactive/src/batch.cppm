//
// Created by cvrain on 2026/4/26.
//

module;

#include <exception>
#include <type_traits>
#include <utility>

export module nandina.reactive.batch;

export import nandina.reactive.tracking;

export namespace nandina::reactive {

// ═════════════════════════════════════════════════════════════════════════════
// § batch(fn) — 批量变更，合并通知
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief 批量更新作用域。
 *
 * 在 batch(fn) 内部修改多个 State，会在 fn 执行完毕后再统一
 * 派发一次失效通知。避免连续修改导致的多次重复计算。
 *
 * 支持嵌套：内部 batch 不会触发 flush，只有最外层的 batch
 * 退出时才会执行 pending invalidations。
 *
 * 异常安全：如果 fn 抛出异常，会确保 batch 状态恢复，
 * 不会导致永久"卡在批量模式"的问题。
 *
 * @code
 * State<int> a{1}, b{2};
 * Effect e([&]{ fmt::print("sum = {}\n", a() + b()); });
 *
 * // 以下只触发一次 Effect 重新执行
 * batch([&]{
 *     a.set(10);
 *     b.set(20);
 *     // Effect 此时不会执行
 * });
 * // 离开 batch 后，Effect 执行一次
 * @endcode
 *
 * @tparam F 可调用类型，接受 void() 签名
 */
template<typename F>
    requires std::invocable<F>
auto batch(F&& fn) -> void {
    detail::BatchScope scope;
    if constexpr (std::is_nothrow_invocable_v<F>) {
        fn();
        if (scope.should_flush()) {
            detail::flush_pending_invalidations();
        }
    } else {
        std::exception_ptr eptr;
        try {
            fn();
        } catch (...) {
            eptr = std::current_exception();
        }
        // Flush pending invalidations before rethrowing.
        if (scope.should_flush()) {
            detail::flush_pending_invalidations();
        }
        if (eptr) {
            std::rethrow_exception(eptr);
        }
    }
}

} // namespace nandina::reactive