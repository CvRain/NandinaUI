//
// animation/tween - a value tween driven by per-frame dt.
//
// Tween 持有 from/to/current，按 elapsed/duration 用缓动曲线插值；tick(dt)
// 推进并返回当前值。归约动效（reduced motion）由调用方决定是否 start 或
// 直接 finish()。
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_TWEEN_HPP
#define NANDINA_EXPERIMENT_ANIMATION_TWEEN_HPP

#include "../foundation/nandina_color.hpp"
#include "easing.hpp"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <utility>

namespace nandina::animation
{
    /// 默认插值：算术类型线性插值。其它类型可提供 lerp 重载。
    template<typename T>
    [[nodiscard]] auto lerp(const T& from, const T& to, const float t) -> T
        requires std::is_arithmetic_v<T>
    {
        return from + (to - from) * t;
    }

    /// 颜色在框架原生 OKLCH 空间插值，色相沿最短弧移动。
    [[nodiscard]] inline auto
    lerp(const foundation::NanColor& from, const foundation::NanColor& to, const float t)
        -> foundation::NanColor {
        return from.mix(to, t);
    }

    template<typename T>
    class Tween {
    public:
        Tween() = default;

        explicit Tween(T value): from_(value), to_(value), current_(value) {}

        /// 从 from 动画到 to，耗时 duration 秒（<=0 立即完成）。
        void start(T from, T to, const float duration, const Easing easing = Easing::ease_in_out) {
            from_ = std::move(from);
            to_ = std::move(to);
            duration_ = std::isfinite(duration) && duration > 0.0F ? duration : 0.0F;
            easing_ = easing;
            elapsed_ = 0.0F;
            finished_ = duration_ <= 0.0F;
            current_ = finished_ ? to_ : from_;
        }

        /// 立即跳到终点。
        void finish() {
            elapsed_ = duration_;
            current_ = to_;
            finished_ = true;
        }

        /// 重置为静止值（无动画）。
        void reset(T value) {
            from_ = value;
            to_ = value;
            current_ = std::move(value);
            duration_ = 0.0F;
            elapsed_ = 0.0F;
            finished_ = true;
        }

        /// 推进 dt 秒，返回当前值。
        [[nodiscard]] auto tick(const float dt) -> const T& {
            if (finished_) {
                return current_;
            }
            const float step = std::isnan(dt) || dt <= 0.0F ? 0.0F : dt;
            elapsed_ = std::min(duration_, elapsed_ + step);
            const float t = duration_ <= 0.0F ? 1.0F : std::clamp(elapsed_ / duration_, 0.0F, 1.0F);
            current_ = lerp(from_, to_, ease(easing_, t));
            if (elapsed_ >= duration_) {
                finished_ = true;
                current_ = to_;
            }
            return current_;
        }

        [[nodiscard]] auto value() const -> const T& {
            return current_;
        }

        [[nodiscard]] auto is_finished() const -> bool {
            return finished_;
        }

        [[nodiscard]] auto progress() const -> float {
            return duration_ <= 0.0F ? 1.0F : std::clamp(elapsed_ / duration_, 0.0F, 1.0F);
        }

    private:
        T from_ {};
        T to_ {};
        T current_ {};
        float duration_ = 0.0F;
        float elapsed_ = 0.0F;
        Easing easing_ = Easing::ease_in_out;
        bool finished_ = true;
    };

} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_TWEEN_HPP
