// scene/visual_property - typed paths shared by all 2D scene nodes.

#ifndef NANDINA_SCENE_VISUAL_PROPERTY_HPP
#define NANDINA_SCENE_VISUAL_PROPERTY_HPP

#include "../foundation/geometry.hpp"

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
} // namespace nandina::scene::visual

#endif // NANDINA_SCENE_VISUAL_PROPERTY_HPP
