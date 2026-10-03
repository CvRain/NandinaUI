#ifndef NANDINA_ANIMATION_ANIMATED_PROPERTY_HPP
#define NANDINA_ANIMATION_ANIMATED_PROPERTY_HPP

#include "../foundation/motion/animated_property.hpp"

namespace nandina::animation
{
    template<typename T>
    using AnimatedProperty = ::nandina::motion::AnimatedProperty<T>;
}

#endif // NANDINA_ANIMATION_ANIMATED_PROPERTY_HPP
