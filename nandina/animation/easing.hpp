//
// animation/easing - standard easing curves for tweens.
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_EASING_HPP
#define NANDINA_EXPERIMENT_ANIMATION_EASING_HPP

#include <algorithm>

namespace nandina::animation
{
    enum class Easing {
        linear,
        ease_in,
        ease_out,
        ease_in_out,
    };

    /// 把归一化进度 t ∈ [0,1] 映射为缓动后的进度。
    [[nodiscard]] inline auto ease(const Easing easing, const float t) -> float {
        const float x = std::clamp(t, 0.0F, 1.0F);
        switch (easing) {
            case Easing::linear:
                return x;
            case Easing::ease_in:
                return x * x;
            case Easing::ease_out:
                return 1.0F - (1.0F - x) * (1.0F - x);
            case Easing::ease_in_out: {
                if (x < 0.5F) {
                    return 2.0F * x * x;
                }
                const float rest = -2.0F * x + 2.0F;
                return 1.0F - rest * rest * 0.5F;
            }
        }
        return x;
    }
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_EASING_HPP
