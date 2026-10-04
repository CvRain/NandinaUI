// scene/node_presentation - animatable, layout-independent node appearance.

#ifndef NANDINA_SCENE_NODE_PRESENTATION_HPP
#define NANDINA_SCENE_NODE_PRESENTATION_HPP

#include "../foundation/geometry.hpp"
#include "../foundation/motion/behavior.hpp"
#include "../foundation/motion/spring.hpp"
#include "animation_clip.hpp"
#include "visual_property.hpp"

#include <cstdint>
#include <memory>

namespace nandina::scene
{
    class NanNode2D;

    enum class TransformOrigin : std::uint8_t {
        top_left,
        top,
        top_right,
        left,
        center,
        right,
        bottom_left,
        bottom,
        bottom_right,
    };

    class NodePresentation {
    public:
        explicit NodePresentation(NanNode2D& owner);
        ~NodePresentation();

        NodePresentation(const NodePresentation&) = delete;
        auto operator=(const NodePresentation&) -> NodePresentation& = delete;

        class OpacityProperty {
        public:
            explicit OpacityProperty(NodePresentation& presentation) noexcept;
            void set(float opacity);
            void set_behavior(motion::Behavior<float> behavior);
            /// 弹簧与 behavior 互斥（装上这个会清掉那个）。opacity 是浮点路径，支持弹簧。
            void set_spring(motion::SpringSpec spec);
            void clear_spring();
            /// Borrows the node's stable endpoint; starting adopts this behavior.
            [[nodiscard]] auto clip(float opacity, motion::Behavior<float> behavior)
                -> AnimationClip;
            [[nodiscard]] auto value() const noexcept -> const float*;
            [[nodiscard]] auto target() const noexcept -> const float*;

        private:
            NodePresentation* presentation_;
        };

        class TranslateProperty {
        public:
            explicit TranslateProperty(NodePresentation& presentation) noexcept;
            void set(foundation::NanPoint translate);
            void set_behavior(motion::Behavior<foundation::NanPoint> behavior);
            [[nodiscard]] auto
            clip(foundation::NanPoint translate, motion::Behavior<foundation::NanPoint> behavior)
                -> AnimationClip;
            [[nodiscard]] auto value() const noexcept -> const foundation::NanPoint*;
            [[nodiscard]] auto target() const noexcept -> const foundation::NanPoint*;

        private:
            NodePresentation* presentation_;
        };

        class ScaleProperty {
        public:
            explicit ScaleProperty(NodePresentation& presentation) noexcept;
            void set(foundation::NanPoint scale);
            void set_behavior(motion::Behavior<foundation::NanPoint> behavior);
            [[nodiscard]] auto
            clip(foundation::NanPoint scale, motion::Behavior<foundation::NanPoint> behavior)
                -> AnimationClip;
            [[nodiscard]] auto value() const noexcept -> const foundation::NanPoint*;
            [[nodiscard]] auto target() const noexcept -> const foundation::NanPoint*;

        private:
            NodePresentation* presentation_;
        };

        [[nodiscard]] auto property(visual::opacity_t) noexcept -> OpacityProperty;
        [[nodiscard]] auto property(visual::translate_t) noexcept -> TranslateProperty;
        [[nodiscard]] auto property(visual::scale_t) noexcept -> ScaleProperty;

        /// 呈现用的不透明度，**钳制在 [0, 1]**。动画值本身（`OpacityProperty::value()`）
        /// 不钳制：欠阻尼弹簧会过冲，钳制只发生在呈现边界，物理过程不被打断。
        [[nodiscard]] auto opacity() const noexcept -> float;
        [[nodiscard]] auto translate() const noexcept -> foundation::NanPoint;
        [[nodiscard]] auto scale() const noexcept -> foundation::NanPoint;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace nandina::scene

#endif // NANDINA_SCENE_NODE_PRESENTATION_HPP
