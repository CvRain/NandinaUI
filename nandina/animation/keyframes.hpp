#ifndef NANDINA_ANIMATION_KEYFRAMES_HPP
#define NANDINA_ANIMATION_KEYFRAMES_HPP

#include "../foundation/motion/keyframes.hpp"

namespace nandina::animation
{
    template<typename T>
    using Keyframe = ::nandina::motion::Keyframe<T>;

    template<typename T>
    using Keyframes = ::nandina::motion::Keyframes<T>;
} // namespace nandina::animation

#endif // NANDINA_ANIMATION_KEYFRAMES_HPP
