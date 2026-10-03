#ifndef NANDINA_ANIMATION_TWEEN_HPP
#define NANDINA_ANIMATION_TWEEN_HPP

#include "../foundation/motion/tween.hpp"

namespace nandina::animation
{
    using ::nandina::motion::lerp;

    template<typename T>
    using Tween = ::nandina::motion::Tween<T>;
} // namespace nandina::animation

#endif // NANDINA_ANIMATION_TWEEN_HPP
