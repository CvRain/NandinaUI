//
// reactive/scope — lifecycle owner for page/local reactive values.
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_SCOPE_HPP
#define NANDINA_EXPERIMENT_REACTIVE_SCOPE_HPP

#include "computed.hpp"
#include "effect.hpp"
#include "event.hpp"
#include "signal.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::reactive
{

    class ReactiveScope {
    public:
        explicit ReactiveScope(Graph& graph): graph_(&graph), lifetime_(std::make_shared<int>(0)) {}

        ~ReactiveScope() {
            lifetime_.reset();
            clear_owned();
        }

        ReactiveScope(const ReactiveScope&) = delete;
        auto operator=(const ReactiveScope&) -> ReactiveScope& = delete;
        ReactiveScope(ReactiveScope&&) = delete;
        auto operator=(ReactiveScope&&) -> ReactiveScope& = delete;

        template<typename T, typename... Args>
        auto signal(Args&&... args) -> Signal<T>& {
            auto owned = std::make_unique<SignalHolder<T>>(*graph_, T(std::forward<Args>(args)...));
            auto* raw = &owned->value;
            signals_.push_back(std::move(owned));
            return *raw;
        }

        template<typename T>
        auto signal_value(T initial) -> Signal<T>& {
            auto owned = std::make_unique<SignalHolder<T>>(*graph_, std::move(initial));
            auto* raw = &owned->value;
            signals_.push_back(std::move(owned));
            return *raw;
        }

        template<typename Fn>
            requires std::invocable<Fn>
        auto computed(Fn&& fn) -> Computed<std::invoke_result_t<Fn>>& {
            auto* c = make_computed(*graph_, std::forward<Fn>(fn));
            computeds_.push_back([c] { c->dispose(); });
            return *c;
        }

        template<typename Fn>
            requires std::invocable<Fn>
        auto effect(Fn&& fn) -> Effect& {
            auto* e = make_effect(*graph_, std::forward<Fn>(fn));
            effects_.push_back(e);
            return *e;
        }

        template<typename... Args, typename Handler>
            requires std::constructible_from<typename Event<Args...>::Handler, Handler>
        void connect(const Event<Args...>& event, Handler&& handler) {
            subscriptions_.push_back(event.subscribe(std::forward<Handler>(handler)));
        }

        /// Own a lifetime-bound non-reactive service: one instance per type per scope.
        /// The first construction arguments win until clear() starts a new generation.
        /// Services are destroyed after effects/computed values and before scope-owned
        /// signals, so a service can release external observers without dangling.
        template<typename T, typename... Args>
            requires std::constructible_from<T, Args...>
        auto own(Args&&... args) -> T& {
            const void* token = &own_token<T>();
            for (auto& entry: owned_services_) {
                if (entry.token == token) {
                    return static_cast<OwnedHolder<T>&>(*entry.value).value;
                }
            }
            auto holder = std::make_unique<OwnedHolder<T>>(std::forward<Args>(args)...);
            auto* raw = &holder->value;
            owned_services_.push_back(OwnedService {.token = token, .value = std::move(holder)});
            return *raw;
        }

        /// The current generation expires before owned reactive values are destroyed.
        [[nodiscard]] auto lifetime() const noexcept -> std::weak_ptr<void> {
            return lifetime_;
        }

        void clear() {
            lifetime_.reset();
            clear_owned();
            lifetime_ = std::make_shared<int>(0);
        }

        [[nodiscard]] auto signal_count() const -> std::size_t {
            return signals_.size();
        }

        [[nodiscard]] auto computed_count() const -> std::size_t {
            return computeds_.size();
        }

        [[nodiscard]] auto effect_count() const -> std::size_t {
            return effects_.size();
        }

        [[nodiscard]] auto subscription_count() const -> std::size_t {
            return subscriptions_.size();
        }

    private:
        void clear_owned() {
            // 先断开外部事件，避免后续资源拆除期间再次进入当前作用域。
            subscriptions_.clear();

            for (auto* e: effects_) {
                e->dispose();
            }
            effects_.clear();

            for (auto& dispose: computeds_) {
                dispose();
            }
            computeds_.clear();

            // 服务释放外部观察，必须在它们可能引用的 scope 信号销毁之前。
            owned_services_.clear();

            signals_.clear();
        }

        struct OwnedServiceBase {
            virtual ~OwnedServiceBase() = default;
        };

        template<typename T>
        struct OwnedHolder final: OwnedServiceBase {
            template<typename... Args>
            explicit OwnedHolder(Args&&... args): value(std::forward<Args>(args)...) {}

            T value;
        };

        struct OwnedService {
            const void* token;
            std::unique_ptr<OwnedServiceBase> value;
        };

        /// 每种服务类型一个稳定 token（无 RTTI 的类型身份）。
        template<typename T>
        static auto own_token() noexcept -> const int& {
            static const int token = 0;
            return token;
        }

        struct SignalHolderBase {
            virtual ~SignalHolderBase() = default;
        };

        template<typename T>
        struct SignalHolder final: SignalHolderBase {
            template<typename U>
            SignalHolder(Graph& graph, U&& initial): value(graph, std::forward<U>(initial)) {}

            Signal<T> value;
        };

        Graph* graph_;
        std::shared_ptr<void> lifetime_;
        std::vector<std::unique_ptr<SignalHolderBase>> signals_;
        std::vector<std::function<void()>> computeds_;
        std::vector<Effect*> effects_;
        std::vector<Subscription> subscriptions_;
        std::vector<OwnedService> owned_services_;
    };

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_SCOPE_HPP
