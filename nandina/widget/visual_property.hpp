//
// widget/visual_property - typed visual part + field property paths.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_VISUAL_PROPERTY_HPP
#define NANDINA_EXPERIMENT_WIDGET_VISUAL_PROPERTY_HPP

#include "../animation/behavior.hpp"
#include "../animation/spring.hpp"
#include "../foundation/nandina_color.hpp"
#include "../scene/node2d.hpp"
#include "../scene/visual_property.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace nandina::widget::visual
{
    using scene::visual::node_t;
    using scene::visual::opacity_t;
    using scene::visual::PropertyPath;
    using scene::visual::scale_t;
    using scene::visual::translate_t;
    inline constexpr auto opacity = scene::visual::opacity;
    inline constexpr auto translate = scene::visual::translate;
    inline constexpr auto scale = scene::visual::scale;

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
    namespace detail
    {
        template<typename Node, typename Part>
            requires requires(Node& node, Part part) { node.visual_part(part); }
        [[nodiscard]] auto visual_part(Node& node, Part part) -> decltype(auto) {
            return node.visual_part(part);
        }

        /// 场景层路径（`scene::visual::node_t`）的兜底：先把节点当作 `NanNode2D` 再取。
        ///
        /// 这不是多余的转发。组件自己声明的 `visual_part` 重载**会隐藏基类那个**，而它们
        /// 都没有 `using scene::NanNode2D::visual_part;`：`widget::Button` 只有
        /// `visual_part(label_t)` / `visual_part(container_t)`，`primitives::Text` 只有
        /// `visual_part(label_t)`。所以在一个 `Button` 上写
        /// `button->visual_part(scene::visual::node)` 是**编译不过的**（报错只会列出
        /// label/container 两个候选）。上面那条泛型重载在这种情况下不满足约束，这条
        /// 显式回到基类的重载才让 `widget::visual::opacity` / `translate` / `scale`
        /// 对组件同样成立。
        ///
        /// 组件是新增的重载源，让每个组件都记得写 `using` 不可靠 —— 兜底留在这一处。
        /// 代价是"直接读裸 endpoint"必须走本文件，见 `tests/animation_tests.cpp` 里
        /// 组件级 spring 测试的写法。
        template<typename Node>
            requires std::derived_from<Node, scene::NanNode2D>
        [[nodiscard]] auto visual_part(Node& node, scene::visual::node_t)
            -> scene::NodePresentation& {
            return static_cast<scene::NanNode2D&>(node).visual_part(scene::visual::node);
        }
    } // namespace detail

    template<typename Path>
    using value_t = typename std::remove_cvref_t<Path>::value_type;

    template<typename Node, typename Path>
    concept Writable = visual::Path<Path> && requires(Node& node, value_t<Path> value) {
        {
            detail::visual_part(node, typename std::remove_cvref_t<Path>::part_type {})
                .property(typename std::remove_cvref_t<Path>::field_type {})
                .set(std::move(value))
        } -> std::same_as<void>;
    };

    template<typename Node, typename Path, typename Value>
    concept WritableValue = Writable<Node, Path> && std::convertible_to<Value, value_t<Path>>;

    template<typename Node, typename Path>
    concept Animatable = Writable<Node, Path> && requires(Node& node) {
        detail::visual_part(node, typename std::remove_cvref_t<Path>::part_type {})
            .property(typename std::remove_cvref_t<Path>::field_type {})
            .set_behavior(animation::Behavior<value_t<Path>>(0.0F));
    };

    /// 弹簧仅适用于浮点值路径。
    template<typename Node, typename Path>
    concept Springable =
        Animatable<Node, Path> && std::is_floating_point_v<value_t<Path>> && requires(Node& node) {
            detail::visual_part(node, typename std::remove_cvref_t<Path>::part_type {})
                .property(typename std::remove_cvref_t<Path>::field_type {})
                .set_spring(animation::SpringSpec());
        };

    template<typename Node, visual::Path Path, typename Value>
        requires WritableValue<Node, Path, Value>
    void write(Node& node, Path, Value&& value) {
        auto endpoint = detail::visual_part(node, typename std::remove_cvref_t<Path>::part_type {})
                            .property(typename std::remove_cvref_t<Path>::field_type {});
        endpoint.set(value_t<Path>(std::forward<Value>(value)));
    }

    template<typename Node, visual::Path Path>
        requires Animatable<Node, Path>
    void set_behavior(Node& node, Path, animation::Behavior<value_t<Path>> behavior) {
        auto endpoint = detail::visual_part(node, typename std::remove_cvref_t<Path>::part_type {})
                            .property(typename std::remove_cvref_t<Path>::field_type {});
        endpoint.set_behavior(std::move(behavior));
    }

    template<typename Node, visual::Path Path>
        requires Springable<Node, Path>
    void set_spring(Node& node, Path, animation::SpringSpec spec) {
        auto endpoint = detail::visual_part(node, typename std::remove_cvref_t<Path>::part_type {})
                            .property(typename std::remove_cvref_t<Path>::field_type {});
        endpoint.set_spring(std::move(spec));
    }
} // namespace nandina::widget::property

#endif // NANDINA_EXPERIMENT_WIDGET_VISUAL_PROPERTY_HPP
