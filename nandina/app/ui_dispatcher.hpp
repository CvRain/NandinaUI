//
// app/ui_dispatcher — UI-thread task queue and background execution.
//

#ifndef NANDINA_EXPERIMENT_APP_UI_DISPATCHER_HPP
#define NANDINA_EXPERIMENT_APP_UI_DISPATCHER_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <thread>

namespace nandina::app
{
    class UiDispatcher {
    public:
        UiDispatcher();
        ~UiDispatcher();

        UiDispatcher(const UiDispatcher&) = delete;
        auto operator=(const UiDispatcher&) -> UiDispatcher& = delete;
        UiDispatcher(UiDispatcher&&) = delete;
        auto operator=(UiDispatcher&&) -> UiDispatcher& = delete;

        /// 可从任意线程投递；关闭后拒绝新任务并返回 false。
        [[nodiscard]] auto post(std::move_only_function<void()> task) -> bool;

        /// 在 UI 线程执行本轮开始前已经入队的任务。
        auto drain() -> std::size_t;
        void shutdown() noexcept;

        [[nodiscard]] auto is_ui_thread() const noexcept -> bool;
        void require_ui_thread() const;
        [[nodiscard]] auto pending_count() const noexcept -> std::size_t;

    private:
        struct State;
        std::unique_ptr<State> state_;
        std::thread::id ui_thread_;
    };

    class BackgroundExecutor {
    public:
        explicit BackgroundExecutor(std::size_t worker_count = 0);
        ~BackgroundExecutor();

        BackgroundExecutor(const BackgroundExecutor&) = delete;
        auto operator=(const BackgroundExecutor&) -> BackgroundExecutor& = delete;
        BackgroundExecutor(BackgroundExecutor&&) = delete;
        auto operator=(BackgroundExecutor&&) -> BackgroundExecutor& = delete;

        [[nodiscard]] auto submit(std::move_only_function<void()> task) -> bool;
        void shutdown() noexcept;
        [[nodiscard]] auto worker_count() const noexcept -> std::size_t;

    private:
        struct State;
        std::unique_ptr<State> state_;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_UI_DISPATCHER_HPP
