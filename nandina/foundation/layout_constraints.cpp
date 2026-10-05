// foundation/layout_constraints - existing layout arithmetic without scene dependencies.

#include "layout_constraints.hpp"

#include <algorithm>
#include <cmath>

namespace nandina::foundation
{
    namespace
    {
        [[nodiscard]] auto finite_or(float value, float fallback) -> float {
            return std::isfinite(value) ? value : fallback;
        }
    } // namespace

    auto NanLayoutConstraints::loose() -> NanLayoutConstraints {
        return {};
    }

    auto NanLayoutConstraints::tight(NanSize size) -> NanLayoutConstraints {
        return {
            .min_width = size.get_width(),
            .max_width = size.get_width(),
            .min_height = size.get_height(),
            .max_height = size.get_height(),
        };
    }

    auto NanLayoutConstraints::constrain(NanSize size) const -> NanSize {
        const float max_w = finite_or(max_width, std::max(size.get_width(), min_width));
        const float max_h = finite_or(max_height, std::max(size.get_height(), min_height));
        return NanSize(
            std::clamp(size.get_width(), min_width, std::max(min_width, max_w)),
            std::clamp(size.get_height(), min_height, std::max(min_height, max_h))
        );
    }

    auto NanLayoutConstraints::deflated(NanInsets insets) const -> NanLayoutConstraints {
        const float horizontal = insets.horizontal_sum();
        const float vertical = insets.vertical_sum();
        return {
            .min_width = std::max(0.0F, min_width - horizontal),
            .max_width =
                std::isfinite(max_width) ? std::max(0.0F, max_width - horizontal) : max_width,
            .min_height = std::max(0.0F, min_height - vertical),
            .max_height =
                std::isfinite(max_height) ? std::max(0.0F, max_height - vertical) : max_height,
        };
    }
} // namespace nandina::foundation
