#ifndef NANDINA_EXPERIMENT_REACTIVE_PROPERTY_HPP
#define NANDINA_EXPERIMENT_REACTIVE_PROPERTY_HPP

#include "effect.hpp"
#include "event.hpp"
#include "scope.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <utility>

namespace nandina::reactive
{

    template<typename T>
    class ReadProperty;

    template<typename T>
    class Property {
    public:
        using Apply = std::function<void(const T&)>;

        explicit Property(T initial, Apply apply = {}):
            value_(std::move(initial)),
            apply_(std::move(apply)) {}

        Property(const Property&) = delete;
        auto operator=(const Property&) -> Property& = delete;
        Property(Property&&) = delete;
        auto operator=(Property&&) -> Property& = delete;

        [[nodiscard]] auto get() const -> const T& {
            return value_;
        }

        /// A direct write replaces the current binding, including equal-value writes.
        auto set(T value) -> bool {
            unbind();
            return apply_value(std::move(value));
        }

        /// Preserve the current value; cancelled callbacks are reclaimed with their scope.
        void unbind() noexcept {
            binding_.reset();
        }

        [[nodiscard]] auto is_bound() const noexcept -> bool {
            return !binding_.expired();
        }

        [[nodiscard]] auto as_readonly() -> ReadProperty<T> {
            return ReadProperty<T> {*this};
        }

        [[nodiscard]] auto as_readonly() const -> ReadProperty<T> {
            return ReadProperty<T> {*this};
        }

        [[nodiscard]] auto changed() -> Event<const T&>& {
            return changed_;
        }
        [[nodiscard]] auto changed() const -> const Event<const T&>& {
            return changed_;
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const T&>;
            }
        void bind(EffectScope& scope, Source& source) {
            bind_in([&scope](auto callback) { scope.add(std::move(callback)); }, source);
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const T&>;
            }
        void bind(ReactiveScope& scope, Source& source) {
            bind_in([&scope](auto callback) { scope.effect(std::move(callback)); }, source);
        }

    private:
        auto apply_value(T value) -> bool {
            if (!has_changed(value)) {
                return false;
            }
            value_ = std::move(value);
            if (apply_) {
                apply_(value_);
            }
            changed_.emit(value_);
            return true;
        }

        template<typename Install, typename Source>
        void bind_in(Install install, Source& source) {
            if (!lifetime_) {
                lifetime_ = std::make_shared<int>(0);
            }
            auto binding = std::make_shared<int>(0);
            binding_ = binding;
            install([this, lifetime = std::weak_ptr<void>(lifetime_), binding, &source] {
                // Check target lifetime and binding identity before touching the source.
                // The scope may retain an old callback after set/unbind/rebind or destruction.
                if (lifetime.expired() || binding_.lock() != binding) {
                    return;
                }
                auto next = T(source.get());
                // Evaluating a custom/computed source may itself replace the binding.
                if (lifetime.expired() || binding_.lock() != binding) {
                    return;
                }
                apply_value(std::move(next));
            });
        }

        [[nodiscard]] auto has_changed(const T& value) const -> bool {
            if constexpr (std::equality_comparable<T>) {
                return !(value_ == value);
            }
            return true;
        }

        T value_;
        Apply apply_;
        Event<const T&> changed_;
        std::weak_ptr<void> binding_;
        std::shared_ptr<void> lifetime_;
    };

    template<typename T>
    class ReadProperty {
    public:
        explicit ReadProperty(const Property<T>& property): property_(&property) {}

        [[nodiscard]] auto get() const -> const T& {
            return property_->get();
        }

        [[nodiscard]] auto is_bound() const noexcept -> bool {
            return property_->is_bound();
        }

        [[nodiscard]] auto observe(typename Event<const T&>::Handler handler) const
            -> Subscription {
            return property_->changed().subscribe(std::move(handler));
        }

    private:
        const Property<T>* property_;
    };

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_PROPERTY_HPP
