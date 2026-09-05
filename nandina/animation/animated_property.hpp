//
// animation/animated_property - logical target plus current presentation value.
//
// 支持三种互斥的插值策略：默认固定时长 Tween（Behavior）、浮点类型的阻尼弹簧
// （SpringSpec）、以及任意 copyable 类型的关键帧（Keyframes）。三者共享同一
// target/value 语义与 finish/retarget 契约。
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_ANIMATED_PROPERTY_HPP
#define NANDINA_EXPERIMENT_ANIMATION_ANIMATED_PROPERTY_HPP

#include "behavior.hpp"
#include "keyframes.hpp"
#include "spring.hpp"
#include "tween.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

namespace nandina::animation
{
    /// 非浮点类型不用弹簧，用空占位避免实例化 Spring<T>（约束不满足会硬报错）。
    struct SpringPlaceholder {};

    template<typename T, bool = std::is_floating_point_v<T>>
    struct SpringMemberSelector {
        using type = SpringPlaceholder;
    };

    template<typename T>
    struct SpringMemberSelector<T, true> {
        using type = Spring<T>;
    };

    template<typename T>
    using SpringMember = typename SpringMemberSelector<T>::type;

    template<typename T>
        requires std::copyable<T> && std::equality_comparable<T>
    class AnimatedProperty {
    public:
        explicit AnimatedProperty(T initial = {}): target_(initial), tween_(std::move(initial)) {
            if constexpr (std::is_floating_point_v<T>) {
                spring_member_ = Spring<T>(initial);
            }
        }

        void set_target(T target) {
            keyframes_spec_.reset();
            if (target_ == target) {
                return;
            }
            target_ = std::move(target);
            if constexpr (std::is_floating_point_v<T>) {
                if (spring_spec_) {
                    spring_member_.start(spring_member_.value(), target_, *spring_spec_);
                    return;
                }
            }
            if (!behavior_ || !behavior_->enabled() || behavior_->duration() == 0.0F) {
                tween_.reset(target_);
                return;
            }
            tween_.start(tween_.value(), target_, behavior_->duration(), behavior_->easing());
        }

        [[nodiscard]] auto target() const noexcept -> const T& {
            return target_;
        }

        [[nodiscard]] auto value() const noexcept -> const T& {
            if (keyframes_spec_) {
                return keyframes_.value();
            }
            if constexpr (std::is_floating_point_v<T>) {
                if (spring_spec_) {
                    return spring_member_.value();
                }
            }
            return tween_.value();
        }

        void set_behavior(Behavior<T> behavior) {
            behavior_ = std::move(behavior);
            keyframes_spec_.reset();
            if constexpr (std::is_floating_point_v<T>) {
                spring_spec_.reset();
            }
            if (!is_animating()) {
                return;
            }
            if (!behavior_->enabled() || behavior_->duration() == 0.0F) {
                tween_.reset(target_);
                return;
            }
            tween_.start(tween_.value(), target_, behavior_->duration(), behavior_->easing());
        }

        void clear_behavior() {
            behavior_.reset();
            tween_.reset(target_);
        }

        /// 为浮点类型安装弹簧行为（与 Behavior/Keyframes 互斥）。非浮点类型不可调用。
        void set_spring(SpringSpec spec)
            requires std::is_floating_point_v<T>
        {
            const T current = value();
            spring_spec_ = std::move(spec);
            behavior_.reset();
            keyframes_spec_.reset();
            spring_member_ = Spring<T>(current);
            if (!(current == target_)) {
                spring_member_.start(current, target_, *spring_spec_);
            }
        }

        void clear_spring()
            requires std::is_floating_point_v<T>
        {
            spring_spec_.reset();
            tween_.reset(target_);
        }

        /// 安装关键帧（与 Behavior/Spring 互斥），并立即从首帧开始播放。
        void set_keyframes(std::vector<Keyframe<T>> keyframes) {
            keyframes_spec_ = std::move(keyframes);
            behavior_.reset();
            if constexpr (std::is_floating_point_v<T>) {
                spring_spec_.reset();
            }
            keyframes_.start(*keyframes_spec_);
            target_ = keyframes_.target();
        }

        void clear_keyframes() {
            keyframes_spec_.reset();
            tween_.reset(target_);
        }

        [[nodiscard]] auto behavior() const noexcept -> const std::optional<Behavior<T>>& {
            return behavior_;
        }

        [[nodiscard]] auto spring() const noexcept -> std::optional<SpringSpec> {
            return spring_spec_;
        }

        [[nodiscard]] auto keyframes() const noexcept
            -> const std::optional<std::vector<Keyframe<T>>>& {
            return keyframes_spec_;
        }

        [[nodiscard]] auto tick(const float dt) -> const T& {
            if (keyframes_spec_) {
                return keyframes_.tick(dt);
            }
            if constexpr (std::is_floating_point_v<T>) {
                if (spring_spec_) {
                    return spring_member_.tick(dt);
                }
            }
            return tween_.tick(dt);
        }

        void finish() {
            if (keyframes_spec_) {
                keyframes_.finish();
                return;
            }
            if constexpr (std::is_floating_point_v<T>) {
                if (spring_spec_) {
                    spring_member_.finish();
                    return;
                }
            }
            tween_.finish();
        }

        [[nodiscard]] auto is_animating() const noexcept -> bool {
            if (keyframes_spec_) {
                return !keyframes_.is_finished();
            }
            if constexpr (std::is_floating_point_v<T>) {
                if (spring_spec_) {
                    return !spring_member_.is_finished();
                }
            }
            return !tween_.is_finished();
        }

        [[nodiscard]] auto progress() const noexcept -> float {
            if (keyframes_spec_) {
                return keyframes_.is_finished() ? 1.0F : 0.0F;
            }
            if constexpr (std::is_floating_point_v<T>) {
                if (spring_spec_) {
                    return spring_member_.is_finished() ? 1.0F : 0.0F;
                }
            }
            return tween_.progress();
        }

    private:
        T target_;
        Tween<T> tween_;
        Keyframes<T> keyframes_ {};
        std::optional<Behavior<T>> behavior_;
        std::optional<std::vector<Keyframe<T>>> keyframes_spec_;
        [[no_unique_address]] SpringMember<T> spring_member_ {};
        std::optional<SpringSpec> spring_spec_;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_ANIMATED_PROPERTY_HPP
