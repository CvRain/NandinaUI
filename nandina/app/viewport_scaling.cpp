//
// app/viewport_scaling — fixed-design viewport scaling policy and coordinate mapping.
//

#include "viewport_scaling.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nandina::app
{
    namespace
    {
        [[nodiscard]] auto valid_size(const foundation::NanSize size) -> bool {
            return std::isfinite(size.get_width()) && std::isfinite(size.get_height())
                && size.is_valid();
        }

        [[nodiscard]] auto anchor_factor(const ViewportAnchor anchor) -> float {
            switch (anchor) {
                case ViewportAnchor::start:
                    return 0.0F;
                case ViewportAnchor::center:
                    return 0.5F;
                case ViewportAnchor::end:
                    return 1.0F;
            }
            return 0.5F;
        }
    } // namespace

    auto ViewportMapping::transform() const -> foundation::NanTransform2D {
        return foundation::NanTransform2D {
            offset,
            0.0F,
            foundation::NanPoint(scale, scale),
        };
    }

    auto ViewportMapping::logical_to_screen(const foundation::NanPoint point) const
        -> foundation::NanPoint {
        return transform().transform_point(point);
    }

    auto ViewportMapping::screen_to_logical(const foundation::NanPoint point) const
        -> foundation::NanPoint {
        return transform().inverse_transform_point(point);
    }

    auto ViewportMapping::content_bounds() const -> foundation::NanRect {
        return foundation::NanRect::from_origin_size(offset, logical_size * scale);
    }

    auto make_viewport_mapping(
        const foundation::NanSize screen_size,
        const ViewportScalePolicy& policy
    ) -> ViewportMapping {
        if (!valid_size(screen_size)) {
            throw std::invalid_argument("viewport screen size must be finite and positive");
        }
        if (!valid_size(policy.design_size)) {
            throw std::invalid_argument("viewport design size must be finite and positive");
        }

        const float scale_x = screen_size.get_width() / policy.design_size.get_width();
        const float scale_y = screen_size.get_height() / policy.design_size.get_height();
        const float scale = policy.fit == ViewportFit::cover ? std::max(scale_x, scale_y)
                                                             : std::min(scale_x, scale_y);
        const auto scaled = policy.design_size * scale;
        const float remaining_x = screen_size.get_width() - scaled.get_width();
        const float remaining_y = screen_size.get_height() - scaled.get_height();

        return {
            .screen_size = screen_size,
            .logical_size = policy.design_size,
            .scale = scale,
            .offset = foundation::NanPoint(
                remaining_x * anchor_factor(policy.horizontal_anchor),
                remaining_y * anchor_factor(policy.vertical_anchor)
            ),
        };
    }

} // namespace nandina::app
