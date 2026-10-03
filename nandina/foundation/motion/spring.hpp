//
// foundation/motion/spring - damped spring interpolation for floating-point values.
//
// 与 Tween 的固定时长缓动不同，Spring 是带速度状态的阻尼谐振子，可产生 overshoot 并
// 渐进收敛。SpringSpec 只描述物理参数；Spring<T> 持有 position/velocity/target，用
// 阻尼振子的解析解推进，并在「接近目标且速度足够慢」时判定收敛（settled）。
// 中途换目标（retarget）保留当前速度，与 Tween 的连续 retarget 语义一致。
//

#ifndef NANDINA_FOUNDATION_MOTION_SPRING_HPP
#define NANDINA_FOUNDATION_MOTION_SPRING_HPP

#include <cmath>
#include <concepts>
#include <stdexcept>
#include <type_traits>

namespace nandina::motion
{
    /// 阻尼弹簧物理参数：stiffness > 0，damping >= 0，mass > 0。
    struct SpringSpec {
        SpringSpec() = default;

        SpringSpec(const float stiffness, const float damping, const float mass = 1.0F):
            stiffness_(checked(stiffness, "stiffness")),
            damping_(checked_non_negative(damping, "damping")),
            mass_(checked(mass, "mass")) {}

        [[nodiscard]] auto stiffness() const noexcept -> float {
            return stiffness_;
        }

        [[nodiscard]] auto damping() const noexcept -> float {
            return damping_;
        }

        [[nodiscard]] auto mass() const noexcept -> float {
            return mass_;
        }

        /// 流式 setter：`motion::spring().stiffness(280.0F).damping(26.0F)`。
        auto stiffness(const float value) -> SpringSpec& {
            stiffness_ = checked(value, "stiffness");
            return *this;
        }

        auto damping(const float value) -> SpringSpec& {
            damping_ = checked_non_negative(value, "damping");
            return *this;
        }

        auto mass(const float value) -> SpringSpec& {
            mass_ = checked(value, "mass");
            return *this;
        }

    private:
        static auto checked(const float value, const char* name) -> float {
            if (!std::isfinite(value) || value <= 0.0F) {
                throw std::invalid_argument("spring parameter must be finite and positive");
            }
            return value;
        }

        static auto checked_non_negative(const float value, const char* name) -> float {
            if (!std::isfinite(value) || value < 0.0F) {
                throw std::invalid_argument("spring parameter must be finite and non-negative");
            }
            return value;
        }

        float stiffness_ = 280.0F;
        float damping_ = 26.0F;
        float mass_ = 1.0F;
    };

    template<typename T>
        requires std::is_floating_point_v<T>
    class Spring {
    public:
        Spring() = default;

        explicit Spring(const T value): position_(value), target_(value) {}

        void start(const T from, const T to, const SpringSpec spec) {
            position_ = from;
            velocity_ = T {};
            target_ = to;
            spec_ = spec;
            finished_ = false;
        }

        /// 中途换目标：保留当前位置与速度，仅改变目标（连续 retarget）。
        void set_target(const T to) {
            target_ = to;
            finished_ = false;
        }

        auto tick(const float dt) -> const T& {
            if (finished_ || !(dt > 0.0F) || !std::isfinite(dt)) {
                return position_;
            }

            // Solve y'' + 2*a*y' + w2*y = 0, where y is displacement from target.
            // Widen before arithmetic: valid float parameters can overflow float ratios.
            const long double mass = spec_.mass();
            const long double a = spec_.damping() / (2.0L * mass);
            const long double w2 = spec_.stiffness() / mass;
            const long double y = static_cast<long double>(position_) - target_;
            const long double v = velocity_;
            const long double time = dt;
            const long double discriminant = a * a - w2;
            long double next_y;
            long double next_v;
            if (discriminant < 0.0L) {
                const long double frequency = std::sqrt(-discriminant);
                const long double decay = std::exp(-a * time);
                const long double sine = decay * std::sin(frequency * time) / frequency;
                const long double cosine = decay * std::cos(frequency * time);
                next_y = cosine * y + sine * (v + a * y);
                next_v = cosine * v - sine * (w2 * y + a * v);
            }
            else if (discriminant == 0.0L) {
                const long double decay = std::exp(-a * time);
                const long double slope = v + a * y;
                next_y = decay * (y + slope * time);
                next_v = decay * (v - a * slope * time);
            }
            else {
                const long double frequency = std::sqrt(discriminant);
                const long double fast_root = -a - frequency;
                // Product of roots is w2; avoid cancellation in -a + frequency.
                const long double slow_root = w2 / fast_root;
                const long double slow_decay = std::exp(slow_root * time);
                const long double fast_decay = std::exp(fast_root * time);
                // Stable even near critical damping, without overflowing cosh/sinh.
                const long double divided_difference =
                    slow_decay * -std::expm1(-2.0L * frequency * time) / (2.0L * frequency);
                next_y = slow_decay * y + divided_difference * (v - slow_root * y);
                next_v = fast_decay * v + slow_root * divided_difference * (v - fast_root * y);
            }
            position_ = static_cast<T>(static_cast<long double>(target_) + next_y);
            velocity_ = next_v;

            const bool settled = std::abs(position_ - target_) <= settle_epsilon_
                && std::abs(velocity_) <= velocity_epsilon_;
            if (settled) {
                position_ = target_;
                velocity_ = T {};
                finished_ = true;
            }
            return position_;
        }

        void finish() {
            position_ = target_;
            velocity_ = T {};
            finished_ = true;
        }

        [[nodiscard]] auto value() const -> const T& {
            return position_;
        }

        [[nodiscard]] auto is_finished() const -> bool {
            return finished_;
        }

    private:
        static constexpr float settle_epsilon_ = 0.001F;
        static constexpr float velocity_epsilon_ = 0.001F;

        T position_ {};
        // A finite displacement can have a velocity beyond T's range for stiff springs.
        long double velocity_ {};
        T target_ {};
        SpringSpec spec_ {};
        bool finished_ = true;
    };
} // namespace nandina::motion

#endif // NANDINA_FOUNDATION_MOTION_SPRING_HPP
