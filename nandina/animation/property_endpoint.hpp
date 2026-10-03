#ifndef NANDINA_ANIMATION_PROPERTY_ENDPOINT_HPP
#define NANDINA_ANIMATION_PROPERTY_ENDPOINT_HPP

#include "../scene/property_endpoint.hpp"

namespace nandina::animation
{
    template<typename T>
    using PropertyEndpoint = scene::PropertyEndpoint<T>;
}

#endif // NANDINA_ANIMATION_PROPERTY_ENDPOINT_HPP
