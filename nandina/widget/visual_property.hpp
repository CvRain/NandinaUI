//
// widget/visual_property - typed visual part + field property paths.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_VISUAL_PROPERTY_HPP
#define NANDINA_EXPERIMENT_WIDGET_VISUAL_PROPERTY_HPP

#include "../animation/behavior.hpp"
#include "../animation/spring.hpp"
#include "../foundation/nandina_color.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace nandina::widget::visual
{
    template<typename Part, typename Field, typename Value>
    struct PropertyPath {
        using part_type = Part;
        using field_type = Field;
        using value_type = Value;
    };

    struct color_t {};
    struct font_size_t {};
    struct fill_t {};
    struct border_color_t {};
    struct border_width_t {};
    struct radius_t {};

    struct label_t {
        PropertyPath<label_t, color_t, foundation::NanColor> color;
        PropertyPath<label_t, font_size_t, float> font_size;
    };

    struct container_t {
        PropertyPath<container_t, fill_t, foundation::NanColor> fill;
        PropertyPath<container_t, border_color_t, foundation::NanColor> border_color;
        PropertyPath<container_t, border_width_t, float> border_width;
        PropertyPath<container_t, radius_t, float> radius;
    };

    inline constexpr label_t label;
    inline constexpr container_t container;

    template<typename Candidate>
    concept Path = requires {
        typename std::remove_cvref_t<Candidate>::part_type;
        typename std::remove_cvref_t<Candidate>::field_type;
        typename std::remove_cvref_t<Candidate>::value_type;
    };
} // namespace nandina::widget::visual

namespace nandina::widget::property
{
    template<typename Path>
    using value_t = typename std::remove_cvref_t<Path>::value_type;

    template<typename Node, typename Path>
    concept Writable = visual::Path<Path> && requires(Node& node, value_t<Path> value) {
        {
            node.visual_part(typename std::remove_cvref_t<Path>::part_type {})
                .property(typename std::remove_cvref_t<Path>::field_type {})
                .set(std::move(value))
        } -> std::same_as<void>;
    };

    template<typename Node, typename Path, typename Value>
    concept WritableValue = Writable<Node, Path> && std::convertible_to<Value, value_t<Path>>;

    template<typename Node, typename Path>
    concept Animatable = Writable<Node, Path> && requires(Node& node) {
        node.visual_part(typename std::remove_cvref_t<Path>::part_type {})
            .property(typename std::remove_cvref_t<Path>::field_type {})
            .set_behavior(animation::Behavior<value_t<Path>>(0.0F));
    };

    /// 弹簧仅适用于浮点值路径。
    template<typename Node, typename Path>
    concept Springable = Animatable<Node, Path> && std::is_floating_point_v<value_t<Path>>
        && requires(Node& node) {
            node.visual_part(typename std::remove_cvref_t<Path>::part_type {})
                .property(typename std::remove_cvref_t<Path>::field_type {})
                .set_spring(animation::SpringSpec());
        };

    template<typename Node, visual::Path Path, typename Value>
        requires WritableValue<Node, Path, Value>
    void write(Node& node, Path, Value&& value) {
        auto endpoint = node.visual_part(typename std::remove_cvref_t<Path>::part_type {})
                            .property(typename std::remove_cvref_t<Path>::field_type {});
        endpoint.set(value_t<Path>(std::forward<Value>(value)));
    }

    template<typename Node, visual::Path Path>
        requires Animatable<Node, Path>
    void set_behavior(Node& node, Path, animation::Behavior<value_t<Path>> behavior) {
        auto endpoint = node.visual_part(typename std::remove_cvref_t<Path>::part_type {})
                            .property(typename std::remove_cvref_t<Path>::field_type {});
        endpoint.set_behavior(std::move(behavior));
    }

    template<typename Node, visual::Path Path>
        requires Springable<Node, Path>
    void set_spring(Node& node, Path, animation::SpringSpec spec) {
        auto endpoint = node.visual_part(typename std::remove_cvref_t<Path>::part_type {})
                            .property(typename std::remove_cvref_t<Path>::field_type {});
        endpoint.set_spring(std::move(spec));
    }
} // namespace nandina::widget::property

#endif // NANDINA_EXPERIMENT_WIDGET_VISUAL_PROPERTY_HPP
