//
// animation/motion - declarative motion spec sugar for builder .behavior/.spring.
//
// 提供 QML 式的声明式动效描述：motion::tween(duration).easing(...) 生成固定时长
// 缓动，motion::spring().stiffness(...).damping(...) 生成阻尼弹簧。两者分别映射到
// Behavior<T> 与 SpringSpec，供 NodeBuilder 的 .behavior/.spring 直接消费。
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_MOTION_HPP
#define NANDINA_EXPERIMENT_ANIMATION_MOTION_HPP

#include "behavior.hpp"
#include "easing.hpp"
#include "spring.hpp"

namespace nandina::animation::motion
{
    // 命名缓动曲线（ease 已被 animation::ease 函数占用，故用 ease_* 前缀）。
    inline constexpr auto ease_linear = Easing::linear;
    inline constexpr auto ease_in = Easing::ease_in;
    inline constexpr auto ease_out = Easing::ease_out;
    inline constexpr auto ease_standard = Easing::ease_in_out;

    /// 固定时长缓动描述：可链式 .easing/.enabled，并物化为具体 Behavior<T>。
    class TweenSpec {
    public:
        explicit TweenSpec(const float duration): duration_(duration) {
            if (!std::isfinite(duration) || duration < 0.0F) {
                throw std::invalid_argument("motion::tween duration must be finite and non-negative");
            }
        }

        auto easing(const Easing easing) noexcept -> TweenSpec& {
            easing_ = easing;
            return *this;
        }

        auto enabled(const bool enabled) noexcept -> TweenSpec& {
            enabled_ = enabled;
            return *this;
        }

        [[nodiscard]] auto duration() const noexcept -> float {
            return duration_;
        }

        [[nodiscard]] auto easing_curve() const noexcept -> Easing {
            return easing_;
        }

        [[nodiscard]] auto enabled_flag() const noexcept -> bool {
            return enabled_;
        }

        template<typename T>
        [[nodiscard]] auto behavior() const -> Behavior<T> {
            return Behavior<T>(duration_, easing_, enabled_);
        }

    private:
        float duration_;
        Easing easing_ = Easing::ease_in_out;
        bool enabled_ = true;
    };

    /// 固定时长缓动。
    [[nodiscard]] inline auto tween(const float duration) -> TweenSpec {
        return TweenSpec(duration);
    }

    /// 阻尼弹簧（默认参数，可用 .stiffness/.damping/.mass 定制）。
    [[nodiscard]] inline auto spring() -> SpringSpec {
        return SpringSpec {};
    }

    [[nodiscard]] inline auto
    spring(const float stiffness, const float damping, const float mass = 1.0F) -> SpringSpec {
        return SpringSpec(stiffness, damping, mass);
    }
} // namespace nandina::animation::motion

#endif // NANDINA_EXPERIMENT_ANIMATION_MOTION_HPP
