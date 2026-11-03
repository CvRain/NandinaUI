// Shared validity rules for author-controlled content insets.
#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_INSET_VALIDATION_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_INSET_VALIDATION_HPP

#include "../../foundation/geometry.hpp"

#include <cmath>
#include <initializer_list>
#include <stdexcept>

namespace nandina::widget::internal
{
    inline void require_content_insets(const foundation::NanInsets insets) {
        for (const float value:
             {insets.get_left(), insets.get_right(), insets.get_top(), insets.get_bottom()})
        {
            if (!std::isfinite(value) || value < 0.0F) {
                throw std::invalid_argument("content insets must be finite and non-negative");
            }
        }
        if (!std::isfinite(insets.horizontal_sum()) || !std::isfinite(insets.vertical_sum())) {
            throw std::invalid_argument("content inset sums must be finite");
        }
    }
} // namespace nandina::widget::internal
#endif // NANDINA_EXPERIMENT_WIDGET_INTERNAL_INSET_VALIDATION_HPP
