//
// animation/behavior - declarative transition policy for a typed property.
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_BEHAVIOR_HPP
#define NANDINA_EXPERIMENT_ANIMATION_BEHAVIOR_HPP

#include "easing.hpp"

#include <cmath>
#include <stdexcept>

namespace nandina::animation
{
    template<typename T>
    class Behavior {
    public:
        using value_type = T;

        explicit Behavior(
            const float duration,
            const Easing easing = Easing::ease_in_out,
            const bool enabled = true
        ):
            duration_(checked_duration(duration)),
            easing_(easing),
            enabled_(enabled) {}

        [[nodiscard]] auto duration() const noexcept -> float {
            return duration_;
        }

        [[nodiscard]] auto easing() const noexcept -> Easing {
            return easing_;
        }

        [[nodiscard]] auto enabled() const noexcept -> bool {
            return enabled_;
        }

        auto set_duration(const float duration) -> Behavior& {
            duration_ = checked_duration(duration);
            return *this;
        }

        auto set_easing(const Easing easing) noexcept -> Behavior& {
            easing_ = easing;
            return *this;
        }

        auto set_enabled(const bool enabled) noexcept -> Behavior& {
            enabled_ = enabled;
            return *this;
        }

    private:
        [[nodiscard]] static auto checked_duration(const float duration) -> float {
            if (!std::isfinite(duration) || duration < 0.0F) {
                throw std::invalid_argument(
                    "animation behavior duration must be finite and non-negative"
                );
            }
            return duration;
        }

        float duration_ = 0.0F;
        Easing easing_ = Easing::ease_in_out;
        bool enabled_ = true;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_BEHAVIOR_HPP
