//
// widget/primitives/box_presentation - instance presentation overrides for a box part.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PRESENTATION_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PRESENTATION_HPP

#include "../../animation/property_endpoint.hpp"
#include "../../scene/control.hpp"
#include "../../theme/design_system.hpp"
#include "../visual_property.hpp"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace nandina::widget::primitives
{
    class BoxPresentation {
    public:
        explicit BoxPresentation(scene::NanControl& owner) noexcept:
            fill_(owner, scene::DirtyFlags::paint),
            border_color_(owner, scene::DirtyFlags::paint),
            border_width_(owner, scene::DirtyFlags::paint),
            radius_(owner, scene::DirtyFlags::paint) {}

        class FillProperty {
        public:
            explicit FillProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(foundation::NanColor color) {
                box_->set_fill(std::move(color));
            }
            void set_behavior(animation::Behavior<foundation::NanColor> behavior) {
                box_->fill_.set_behavior(std::move(behavior));
            }
            [[nodiscard]] auto value() const noexcept -> const foundation::NanColor* {
                return box_->fill_.value();
            }
            [[nodiscard]] auto target() const noexcept -> const foundation::NanColor* {
                return box_->fill_.target();
            }

        private:
            BoxPresentation* box_;
        };

        class BorderColorProperty {
        public:
            explicit BorderColorProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(foundation::NanColor color) {
                box_->set_border_color(std::move(color));
            }
            void set_behavior(animation::Behavior<foundation::NanColor> behavior) {
                box_->border_color_.set_behavior(std::move(behavior));
            }
            [[nodiscard]] auto value() const noexcept -> const foundation::NanColor* {
                return box_->border_color_.value();
            }
            [[nodiscard]] auto target() const noexcept -> const foundation::NanColor* {
                return box_->border_color_.target();
            }

        private:
            BoxPresentation* box_;
        };

        class BorderWidthProperty {
        public:
            explicit BorderWidthProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(float width) {
                box_->set_border_width(width);
            }
            void set_behavior(animation::Behavior<float> behavior) {
                box_->border_width_.set_behavior(std::move(behavior));
            }
            void set_spring(animation::SpringSpec spec) {
                box_->border_width_.set_spring(std::move(spec));
            }
            [[nodiscard]] auto value() const noexcept -> const float* {
                return box_->border_width_.value();
            }
            [[nodiscard]] auto target() const noexcept -> const float* {
                return box_->border_width_.target();
            }

        private:
            BoxPresentation* box_;
        };

        class RadiusProperty {
        public:
            explicit RadiusProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(float radius) {
                box_->set_radius(radius);
            }
            void set_behavior(animation::Behavior<float> behavior) {
                box_->radius_.set_behavior(std::move(behavior));
            }
            void set_spring(animation::SpringSpec spec) {
                box_->radius_.set_spring(std::move(spec));
            }
            [[nodiscard]] auto value() const noexcept -> const float* {
                return box_->radius_.value();
            }
            [[nodiscard]] auto target() const noexcept -> const float* {
                return box_->radius_.target();
            }

        private:
            BoxPresentation* box_;
        };

        [[nodiscard]] auto property(visual::fill_t) noexcept -> FillProperty {
            return FillProperty {*this};
        }

        [[nodiscard]] auto property(visual::border_color_t) noexcept -> BorderColorProperty {
            return BorderColorProperty {*this};
        }

        [[nodiscard]] auto property(visual::border_width_t) noexcept -> BorderWidthProperty {
            return BorderWidthProperty {*this};
        }

        [[nodiscard]] auto property(visual::radius_t) noexcept -> RadiusProperty {
            return RadiusProperty {*this};
        }

        void apply(theme::ResolvedBoxStyle& style) const {
            if (const auto* fill = fill_.value(); fill != nullptr) {
                style.fill = *fill;
            }
            if (const auto* border_color = border_color_.value(); border_color != nullptr) {
                style.border = *border_color;
            }
            if (const auto* border_width = border_width_.value(); border_width != nullptr) {
                style.border_width = *border_width;
            }
            if (const auto* radius = radius_.value(); radius != nullptr) {
                style.radius = *radius;
            }
        }

    private:
        static void require_non_negative(float value, const char* field) {
            if (!std::isfinite(value) || value < 0.0F) {
                throw std::invalid_argument(field);
            }
        }

        void set_fill(foundation::NanColor color) {
            fill_.set(std::move(color));
        }

        void set_border_color(foundation::NanColor color) {
            border_color_.set(std::move(color));
        }

        void set_border_width(const float width) {
            require_non_negative(
                width,
                "box presentation border width must be finite and non-negative"
            );
            border_width_.set(width);
        }

        void set_radius(const float radius) {
            require_non_negative(radius, "box presentation radius must be finite and non-negative");
            radius_.set(radius);
        }

        animation::PropertyEndpoint<foundation::NanColor> fill_;
        animation::PropertyEndpoint<foundation::NanColor> border_color_;
        animation::PropertyEndpoint<float> border_width_;
        animation::PropertyEndpoint<float> radius_;
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PRESENTATION_HPP
