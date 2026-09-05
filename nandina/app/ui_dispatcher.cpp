//
// app/ui_dispatcher — UI-thread task queue and background execution.
//

#include "ui_dispatcher.hpp"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nandina::app
{
    struct UiDispatcher::State {
        mutable std::mutex mutex;
        std::deque<std::move_only_function<void()>> tasks;
        bool accepting = true;
    };

    UiDispatcher::UiDispatcher():
        state_(std::make_unique<State>()),
        ui_thread_(std::this_thread::get_id()) {}

    UiDispatcher::~UiDispatcher() {
        shutdown();
    }

    auto UiDispatcher::post(std::move_only_function<void()> task) -> bool {
        if (!task) {
            throw std::invalid_argument("UiDispatcher::post: task is empty");
        }
        std::scoped_lock lock(state_->mutex);
        if (!state_->accepting) {
            return false;
        }
        state_->tasks.push_back(std::move(task));
        return true;
    }

    auto UiDispatcher::drain() -> std::size_t {
        require_ui_thread();
        std::deque<std::move_only_function<void()>> tasks;
        {
            std::scoped_lock lock(state_->mutex);
            tasks.swap(state_->tasks);
        }
        const auto count = tasks.size();
        while (!tasks.empty()) {
            auto task = std::move(tasks.front());
            tasks.pop_front();
            task();
        }
        return count;
    }

    void UiDispatcher::shutdown() noexcept {
        std::scoped_lock lock(state_->mutex);
        state_->accepting = false;
        state_->tasks.clear();
    }

    auto UiDispatcher::is_ui_thread() const noexcept -> bool {
        return std::this_thread::get_id() == ui_thread_;
    }

    void UiDispatcher::require_ui_thread() const {
        if (!is_ui_thread()) {
            throw std::logic_error("UiDispatcher: widget mutation requires the UI thread");
        }
    }

    auto UiDispatcher::pending_count() const noexcept -> std::size_t {
        std::scoped_lock lock(state_->mutex);
        return state_->tasks.size();
    }

    struct BackgroundExecutor::State {
        std::mutex mutex;
        std::condition_variable_any ready;
        std::deque<std::move_only_function<void()>> tasks;
        std::vector<std::jthread> workers;
        bool accepting = true;
    };

    BackgroundExecutor::BackgroundExecutor(std::size_t worker_count):
        state_(std::make_unique<State>()) {
        if (worker_count == 0) {
            worker_count = std::clamp<std::size_t>(std::thread::hardware_concurrency(), 1, 4);
        }
        state_->workers.reserve(worker_count);
        for (std::size_t index = 0; index < worker_count; ++index) {
            state_->workers.emplace_back([state = state_.get()](const std::stop_token stop) {
                while (true) {
                    std::move_only_function<void()> task;
                    {
                        std::unique_lock lock(state->mutex);
                        state->ready.wait(lock, stop, [state] {
                            return !state->tasks.empty() || !state->accepting;
                        });
                        if (stop.stop_requested() || (!state->accepting && state->tasks.empty())) {
                            return;
                        }
                        task = std::move(state->tasks.front());
                        state->tasks.pop_front();
                    }
                    task();
                }
            });
        }
    }

    BackgroundExecutor::~BackgroundExecutor() {
        shutdown();
    }

    auto BackgroundExecutor::submit(std::move_only_function<void()> task) -> bool {
        if (!task) {
            throw std::invalid_argument("BackgroundExecutor::submit: task is empty");
        }
        {
            std::scoped_lock lock(state_->mutex);
            if (!state_->accepting) {
                return false;
            }
            state_->tasks.push_back(std::move(task));
        }
        state_->ready.notify_one();
        return true;
    }

    void BackgroundExecutor::shutdown() noexcept {
        {
            std::scoped_lock lock(state_->mutex);
            if (!state_->accepting) {
                return;
            }
            state_->accepting = false;
            state_->tasks.clear();
        }
        for (auto& worker: state_->workers) {
            worker.request_stop();
        }
        state_->ready.notify_all();
        state_->workers.clear();
    }

    auto BackgroundExecutor::worker_count() const noexcept -> std::size_t {
        return state_->workers.size();
    }

} // namespace nandina::app
