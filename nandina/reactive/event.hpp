#ifndef NANDINA_EXPERIMENT_REACTIVE_EVENT_HPP
#define NANDINA_EXPERIMENT_REACTIVE_EVENT_HPP

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace nandina::reactive
{

    class Subscription {
    public:
        Subscription() = default;
        explicit Subscription(std::function<void()> disconnect):
            disconnect_(std::move(disconnect)) {}

        ~Subscription() { disconnect(); }

        Subscription(const Subscription&) = delete;
        auto operator=(const Subscription&) -> Subscription& = delete;

        Subscription(Subscription&& other) noexcept:
            disconnect_(std::exchange(other.disconnect_, {})) {}

        auto operator=(Subscription&& other) noexcept -> Subscription& {
            if (this != &other) {
                disconnect();
                disconnect_ = std::exchange(other.disconnect_, {});
            }
            return *this;
        }

        void disconnect() {
            if (disconnect_) {
                auto callback = std::exchange(disconnect_, {});
                callback();
            }
        }

        [[nodiscard]] auto connected() const -> bool {
            return static_cast<bool>(disconnect_);
        }

    private:
        std::function<void()> disconnect_;
    };

    template<typename... Args>
    class Event {
    public:
        using Handler = std::function<void(Args...)>;

        [[nodiscard]] auto subscribe(Handler handler) const -> Subscription {
            const auto id = state_->next_id++;
            state_->handlers.push_back(Entry {.id = id, .handler = std::move(handler)});
            std::weak_ptr<State> state = state_;
            return Subscription {[state, id] {
                if (auto locked = state.lock()) {
                    std::erase_if(locked->handlers, [id](const Entry& entry) {
                        return entry.id == id;
                    });
                }
            }};
        }

        void emit(Args... args) const {
            const auto snapshot = state_->handlers;
            for (const auto& entry: snapshot) {
                const auto still_connected = std::ranges::any_of(
                    state_->handlers,
                    [&entry](const Entry& current) { return current.id == entry.id; }
                );
                if (still_connected) {
                    entry.handler(args...);
                }
            }
        }

        [[nodiscard]] auto subscriber_count() const -> std::size_t {
            return state_->handlers.size();
        }

    private:
        struct Entry {
            std::size_t id;
            Handler handler;
        };

        struct State {
            std::size_t next_id = 1;
            std::vector<Entry> handlers;
        };

        std::shared_ptr<State> state_ = std::make_shared<State>();
    };

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_EVENT_HPP
