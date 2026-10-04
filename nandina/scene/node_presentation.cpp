#include "node_presentation.hpp"

#include "node2d.hpp"
#include "property_endpoint.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::scene
{
    struct NodePresentation::Impl {
        explicit Impl(NanNode2D& owner):
            opacity(owner, 1.0F, DirtyFlags::paint),
            translate(
                owner,
                foundation::NanPoint(0.0F, 0.0F),
                DirtyFlags::paint | DirtyFlags::transform
            ),
            scale(
                owner,
                foundation::NanPoint(1.0F, 1.0F),
                DirtyFlags::paint | DirtyFlags::transform
            ) {}

        PropertyEndpoint<float> opacity;
        PropertyEndpoint<foundation::NanPoint> translate;
        PropertyEndpoint<foundation::NanPoint> scale;
    };

    NodePresentation::NodePresentation(NanNode2D& owner): impl_(std::make_unique<Impl>(owner)) {}
    NodePresentation::~NodePresentation() = default;

    NodePresentation::OpacityProperty::OpacityProperty(NodePresentation& presentation) noexcept:
        presentation_(&presentation) {}

    void NodePresentation::OpacityProperty::set(const float opacity) {
        if (!std::isfinite(opacity)) {
            throw std::invalid_argument("node presentation opacity must be finite");
        }
        presentation_->impl_->opacity.set(std::clamp(opacity, 0.0F, 1.0F));
    }

    void NodePresentation::OpacityProperty::set_behavior(motion::Behavior<float> behavior) {
        presentation_->impl_->opacity.set_behavior(std::move(behavior));
    }

    void NodePresentation::OpacityProperty::set_spring(motion::SpringSpec spec) {
        presentation_->impl_->opacity.set_spring(std::move(spec));
    }

    void NodePresentation::OpacityProperty::clear_spring() {
        presentation_->impl_->opacity.clear_spring();
    }

    auto
    NodePresentation::OpacityProperty::clip(const float opacity, motion::Behavior<float> behavior)
        -> AnimationClip {
        return presentation_->impl_->opacity.clip(opacity, std::move(behavior));
    }

    auto NodePresentation::OpacityProperty::value() const noexcept -> const float* {
        return presentation_->impl_->opacity.value();
    }

    auto NodePresentation::OpacityProperty::target() const noexcept -> const float* {
        return presentation_->impl_->opacity.target();
    }

    NodePresentation::TranslateProperty::TranslateProperty(NodePresentation& presentation) noexcept:
        presentation_(&presentation) {}

    void NodePresentation::TranslateProperty::set(foundation::NanPoint translate) {
        presentation_->impl_->translate.set(std::move(translate));
    }

    void NodePresentation::TranslateProperty::set_behavior(
        motion::Behavior<foundation::NanPoint> behavior
    ) {
        presentation_->impl_->translate.set_behavior(std::move(behavior));
    }

    auto NodePresentation::TranslateProperty::clip(
        foundation::NanPoint translate,
        motion::Behavior<foundation::NanPoint> behavior
    ) -> AnimationClip {
        return presentation_->impl_->translate.clip(std::move(translate), std::move(behavior));
    }

    auto NodePresentation::TranslateProperty::value() const noexcept
        -> const foundation::NanPoint* {
        return presentation_->impl_->translate.value();
    }

    auto NodePresentation::TranslateProperty::target() const noexcept
        -> const foundation::NanPoint* {
        return presentation_->impl_->translate.target();
    }

    NodePresentation::ScaleProperty::ScaleProperty(NodePresentation& presentation) noexcept:
        presentation_(&presentation) {}

    void NodePresentation::ScaleProperty::set(foundation::NanPoint scale) {
        if (!std::isfinite(scale.get_x()) || !std::isfinite(scale.get_y())) {
            throw std::invalid_argument("node presentation scale must be finite");
        }
        presentation_->impl_->scale.set(std::move(scale));
    }

    void
    NodePresentation::ScaleProperty::set_behavior(motion::Behavior<foundation::NanPoint> behavior) {
        presentation_->impl_->scale.set_behavior(std::move(behavior));
    }

    auto NodePresentation::ScaleProperty::clip(
        foundation::NanPoint scale,
        motion::Behavior<foundation::NanPoint> behavior
    ) -> AnimationClip {
        return presentation_->impl_->scale.clip(std::move(scale), std::move(behavior));
    }

    auto NodePresentation::ScaleProperty::value() const noexcept -> const foundation::NanPoint* {
        return presentation_->impl_->scale.value();
    }

    auto NodePresentation::ScaleProperty::target() const noexcept -> const foundation::NanPoint* {
        return presentation_->impl_->scale.target();
    }

    auto NodePresentation::property(visual::opacity_t) noexcept -> OpacityProperty {
        return OpacityProperty(*this);
    }

    auto NodePresentation::property(visual::translate_t) noexcept -> TranslateProperty {
        return TranslateProperty(*this);
    }

    auto NodePresentation::property(visual::scale_t) noexcept -> ScaleProperty {
        return ScaleProperty(*this);
    }

    auto NodePresentation::opacity() const noexcept -> float {
        // 呈现边界钳制：弹簧的中间值可以过冲到 [0,1] 之外，物理状态保持不钳制。
        return std::clamp(*impl_->opacity.value(), 0.0F, 1.0F);
    }

    auto NodePresentation::translate() const noexcept -> foundation::NanPoint {
        return *impl_->translate.value();
    }

    auto NodePresentation::scale() const noexcept -> foundation::NanPoint {
        return *impl_->scale.value();
    }
} // namespace nandina::scene
