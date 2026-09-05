#ifndef NANDINA_EXPERIMENT_REACTIVE_PROPERTY_HPP
#define NANDINA_EXPERIMENT_REACTIVE_PROPERTY_HPP

#include "effect.hpp"
#include "event.hpp"

#include <concepts>
#include <functional>
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

        [[nodiscard]] auto get() const -> const T& { return value_; }

        auto set(T value) -> bool {
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

        [[nodiscard]] auto as_readonly() -> ReadProperty<T> {
            return ReadProperty<T> {*this};
        }

        [[nodiscard]] auto as_readonly() const -> ReadProperty<T> {
            return ReadProperty<T> {*this};
        }

        [[nodiscard]] auto changed() -> Event<const T&>& { return changed_; }
        [[nodiscard]] auto changed() const -> const Event<const T&>& { return changed_; }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const T&>;
            }
        void bind(EffectScope& scope, Source& source) {
            scope.add([this, &source] { set(T(source.get())); });
        }

    private:
        [[nodiscard]] auto has_changed(const T& value) const -> bool {
            if constexpr (std::equality_comparable<T>) {
                return !(value_ == value);
            }
            return true;
        }

        T value_;
        Apply apply_;
        Event<const T&> changed_;
    };

    template<typename T>
    class ReadProperty {
    public:
        explicit ReadProperty(const Property<T>& property): property_(&property) {}

        [[nodiscard]] auto get() const -> const T& { return property_->get(); }

        [[nodiscard]] auto observe(typename Event<const T&>::Handler handler) const -> Subscription {
            return property_->changed().subscribe(std::move(handler));
        }

    private:
        const Property<T>* property_;
    };

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_PROPERTY_HPP
