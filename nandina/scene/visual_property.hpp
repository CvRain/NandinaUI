// scene/visual_property - typed paths shared by all 2D scene nodes.

#ifndef NANDINA_SCENE_VISUAL_PROPERTY_HPP
#define NANDINA_SCENE_VISUAL_PROPERTY_HPP

#include "../foundation/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nandina::scene::visual
{
    template<typename Part, typename Field, typename Value>
    struct PropertyPath {
        using part_type = Part;
        using field_type = Field;
        using value_type = Value;
    };

    struct opacity_t {};
    struct translate_t {};
    struct scale_t {};

    struct node_t {
        PropertyPath<node_t, opacity_t, float> opacity;
        PropertyPath<node_t, translate_t, foundation::NanPoint> translate;
        PropertyPath<node_t, scale_t, foundation::NanPoint> scale;
    };

    inline constexpr node_t node;
    inline constexpr auto opacity = node.opacity;
    inline constexpr auto translate = node.translate;
    inline constexpr auto scale = node.scale;

    /// Setters and deferred animation declarations share the same target contract.
    [[nodiscard]] inline auto validated_target(opacity_t, const float value) -> float {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("node presentation opacity must be finite");
        }
        return std::clamp(value, 0.0F, 1.0F);
    }

    [[nodiscard]] inline auto validated_target(translate_t, foundation::NanPoint value)
        -> foundation::NanPoint {
        return value;
    }

    [[nodiscard]] inline auto validated_target(scale_t, foundation::NanPoint value)
        -> foundation::NanPoint {
        if (!std::isfinite(value.get_x()) || !std::isfinite(value.get_y())) {
            throw std::invalid_argument("node presentation scale must be finite");
        }
        return value;
    }
} // namespace nandina::scene::visual

#endif // NANDINA_SCENE_VISUAL_PROPERTY_HPP
