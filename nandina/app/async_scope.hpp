//
// app/async_scope — owner-bound cancellable background work.
//

#ifndef NANDINA_EXPERIMENT_APP_ASYNC_SCOPE_HPP
#define NANDINA_EXPERIMENT_APP_ASYNC_SCOPE_HPP

#include "ui_dispatcher.hpp"

#include <atomic>
#include <cstdint>
#include <exception>
#include <expected>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace nandina::app
{
    class CancellationToken {
    public:
        CancellationToken() = default;

        [[nodiscard]] auto stop_requested() const noexcept -> bool {
            return state_ && state_->load(std::memory_order_acquire);
        }

        explicit operator bool() const noexcept {
            return static_cast<bool>(state_);
        }

    private:
        explicit CancellationToken(std::shared_ptr<std::atomic_bool> state):
            state_(std::move(state)) {}

        std::shared_ptr<std::atomic_bool> state_;

        friend class CancellationSource;
    };

    class CancellationSource {
    public:
        CancellationSource(): state_(std::make_shared<std::atomic_bool>(false)) {}

        [[nodiscard]] auto token() const noexcept -> CancellationToken {
            return CancellationToken {state_};
        }

        void request_stop() const noexcept {
            state_->store(true, std::memory_order_release);
        }

    private:
        std::shared_ptr<std::atomic_bool> state_;
    };

    class AsyncScope {
    public:
        AsyncScope(UiDispatcher& dispatcher, BackgroundExecutor& executor);
        ~AsyncScope();

        AsyncScope(const AsyncScope&) = delete;
        auto operator=(const AsyncScope&) -> AsyncScope& = delete;
        AsyncScope(AsyncScope&&) = delete;
        auto operator=(AsyncScope&&) -> AsyncScope& = delete;

        /// 启动最新一代任务。前一代会收到取消信号，其结果不会进入 UI 回调。
        template<typename Work, typename Completion>
            requires std::invocable<Work, CancellationToken>
        void run(Work&& work, Completion&& completion) {
            using Result = std::invoke_result_t<Work, CancellationToken>;
            using Outcome = std::expected<Result, std::exception_ptr>;
            static_assert(!std::is_reference_v<Result>, "AsyncScope work must return a value");
            static_assert(
                std::invocable<Completion, Outcome>,
                "AsyncScope completion must accept std::expected<Result, std::exception_ptr>"
            );

            auto ticket = begin();
            auto* dispatcher = dispatcher_;
            auto task = [dispatcher,
                         ticket,
                         work = std::forward<Work>(work),
                         completion = std::forward<Completion>(completion)]() mutable {
                if (ticket.token.stop_requested()) {
                    return;
                }
                auto outcome = invoke<Result>(work, ticket.token);
                if (ticket.token.stop_requested()) {
                    return;
                }
                try {
                    if (!dispatcher->post([ticket,
                                           completion = std::move(completion),
                                           outcome = std::move(outcome)]() mutable {
                            if (is_current(ticket)) {
                                std::invoke(completion, std::move(outcome));
                            }
                        }))
                    {
                        cancel(ticket);
                    }
                }
                catch (...) {
                    cancel(ticket);
                }
            };
            try {
                if (!executor_->submit(std::move(task))) {
                    cancel(ticket);
                }
            }
            catch (...) {
                cancel(ticket);
                throw;
            }
        }

        void clear() noexcept;
        [[nodiscard]] auto active() const noexcept -> bool;

    private:
        struct State;
        struct Ticket {
            std::weak_ptr<State> state;
            std::uint64_t generation = 0;
            CancellationToken token;
        };

        [[nodiscard]] auto begin() -> Ticket;
        static auto is_current(const Ticket& ticket) noexcept -> bool;
        static void cancel(const Ticket& ticket) noexcept;

        template<typename Result, typename Work>
        [[nodiscard]] static auto invoke(Work& work, const CancellationToken token)
            -> std::expected<Result, std::exception_ptr> {
            try {
                if constexpr (std::is_void_v<Result>) {
                    std::invoke(work, token);
                    return {};
                }
                else {
                    return std::invoke(work, token);
                }
            }
            catch (...) {
                return std::unexpected(std::current_exception());
            }
        }

        UiDispatcher* dispatcher_;
        BackgroundExecutor* executor_;
        std::shared_ptr<State> state_;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_ASYNC_SCOPE_HPP
