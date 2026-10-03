#ifndef NANDINA_ANIMATION_BEHAVIOR_HPP
#define NANDINA_ANIMATION_BEHAVIOR_HPP

#include "../foundation/motion/behavior.hpp"
#include "easing.hpp"

namespace nandina::animation
{
    template<typename T>
    using Behavior = ::nandina::motion::Behavior<T>;
}

#endif // NANDINA_ANIMATION_BEHAVIOR_HPP
