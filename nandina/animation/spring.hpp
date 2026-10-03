#ifndef NANDINA_ANIMATION_SPRING_HPP
#define NANDINA_ANIMATION_SPRING_HPP

#include "../foundation/motion/spring.hpp"

namespace nandina::animation
{
    using ::nandina::motion::SpringSpec;

    template<typename T>
    using Spring = ::nandina::motion::Spring<T>;
} // namespace nandina::animation

#endif // NANDINA_ANIMATION_SPRING_HPP
