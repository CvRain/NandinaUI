//
// app/async_scope — owner-bound cancellable background work.
//

#include "async_scope.hpp"

#include <mutex>

namespace nandina::app
{
    struct AsyncScope::State {
        std::mutex mutex;
        CancellationSource cancellation;
        std::uint64_t generation = 0;
        bool alive = true;
        bool active = false;
    };

    AsyncScope::AsyncScope(UiDispatcher& dispatcher, BackgroundExecutor& executor):
        dispatcher_(&dispatcher),
        executor_(&executor),
        state_(std::make_shared<State>()) {
        dispatcher_->require_ui_thread();
    }

    AsyncScope::~AsyncScope() {
        clear();
        std::scoped_lock lock(state_->mutex);
        state_->alive = false;
    }

    auto AsyncScope::begin() -> Ticket {
        dispatcher_->require_ui_thread();
        std::scoped_lock lock(state_->mutex);
        state_->cancellation.request_stop();
        state_->cancellation = CancellationSource {};
        ++state_->generation;
        state_->active = true;
        return Ticket {
            .state = state_,
            .generation = state_->generation,
            .token = state_->cancellation.token(),
        };
    }

    auto AsyncScope::is_current(const Ticket& ticket) noexcept -> bool {
        const auto state = ticket.state.lock();
        if (!state || ticket.token.stop_requested()) {
            return false;
        }
        std::scoped_lock lock(state->mutex);
        if (!state->alive || state->generation != ticket.generation) {
            return false;
        }
        state->active = false;
        return true;
    }

    void AsyncScope::cancel(const Ticket& ticket) noexcept {
        if (const auto state = ticket.state.lock()) {
            std::scoped_lock lock(state->mutex);
            if (state->generation == ticket.generation) {
                state->cancellation.request_stop();
                state->active = false;
            }
        }
    }

    void AsyncScope::clear() noexcept {
        std::scoped_lock lock(state_->mutex);
        state_->cancellation.request_stop();
        ++state_->generation;
        state_->active = false;
    }

    auto AsyncScope::active() const noexcept -> bool {
        std::scoped_lock lock(state_->mutex);
        return state_->active;
    }

} // namespace nandina::app
