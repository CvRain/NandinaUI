//
// widget/primitives/text_presentation - instance presentation overrides for a text part.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_PRESENTATION_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_PRESENTATION_HPP

#include "../../animation/property_endpoint.hpp"
#include "../../scene/control.hpp"
#include "../../theme/design_system.hpp"
#include "../visual_property.hpp"
#include "text_layout.hpp"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace nandina::widget::primitives
{
    class TextPresentation {
    public:
        explicit TextPresentation(scene::NanControl& owner) noexcept:
            color_(owner, scene::DirtyFlags::paint),
            font_size_(owner, scene::layout_dirty_flags | scene::DirtyFlags::paint) {}

        class ColorProperty {
        public:
            explicit ColorProperty(TextPresentation& text) noexcept: text_(&text) {}
            void set(foundation::NanColor color) {
                text_->set_color(std::move(color));
            }
            void set_behavior(animation::Behavior<foundation::NanColor> behavior) {
                text_->color_.set_behavior(std::move(behavior));
            }
            [[nodiscard]] auto value() const noexcept -> const foundation::NanColor* {
                return text_->color_.value();
            }
            [[nodiscard]] auto target() const noexcept -> const foundation::NanColor* {
                return text_->color_.target();
            }

        private:
            TextPresentation* text_;
        };

        class FontSizeProperty {
        public:
            explicit FontSizeProperty(TextPresentation& text) noexcept: text_(&text) {}
            void set(float size) {
                text_->set_font_size(size);
            }
            void set_behavior(animation::Behavior<float> behavior) {
                text_->font_size_.set_behavior(std::move(behavior));
            }
            void set_spring(animation::SpringSpec spec) {
                text_->font_size_.set_spring(std::move(spec));
            }
            [[nodiscard]] auto value() const noexcept -> const float* {
                return text_->font_size_.value();
            }
            [[nodiscard]] auto target() const noexcept -> const float* {
                return text_->font_size_.target();
            }

        private:
            TextPresentation* text_;
        };

        [[nodiscard]] auto property(visual::color_t) noexcept -> ColorProperty {
            return ColorProperty {*this};
        }

        [[nodiscard]] auto property(visual::font_size_t) noexcept -> FontSizeProperty {
            return FontSizeProperty {*this};
        }

        void apply(theme::ResolvedTypeStyle& style) const {
            if (const auto* color = color_.value(); color != nullptr) {
                style.color = *color;
            }
            if (const auto* font_size = font_size_.value(); font_size != nullptr) {
                style.font_size = *font_size;
            }
        }

        void apply(TextStyle& style) const {
            if (const auto* color = color_.value(); color != nullptr) {
                style.color = *color;
            }
            if (const auto* font_size = font_size_.value(); font_size != nullptr) {
                style.font_size = *font_size;
            }
        }

        void clear_font_size() noexcept {
            font_size_.clear();
        }

    private:
        void set_color(foundation::NanColor color) {
            color_.set(std::move(color));
        }

        void set_font_size(const float size) {
            if (!std::isfinite(size) || size <= 0.0F) {
                throw std::invalid_argument(
                    "text presentation font size must be finite and positive"
                );
            }
            font_size_.set(size);
        }

        animation::PropertyEndpoint<foundation::NanColor> color_;
        animation::PropertyEndpoint<float> font_size_;
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_PRESENTATION_HPP
