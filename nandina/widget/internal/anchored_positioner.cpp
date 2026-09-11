#include "anchored_positioner.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nandina::widget::internal
{
    namespace
    {
        [[nodiscard]] auto finite(const foundation::NanPoint point) -> bool {
            return std::isfinite(point.get_x()) && std::isfinite(point.get_y());
        }

        [[nodiscard]] auto finite(const foundation::NanRect rect) -> bool {
            return std::isfinite(rect.get_left()) && std::isfinite(rect.get_top())
                && std::isfinite(rect.get_right()) && std::isfinite(rect.get_bottom());
        }

        [[nodiscard]] auto finite(const foundation::NanSize size) -> bool {
            return std::isfinite(size.get_width()) && std::isfinite(size.get_height());
        }

        [[nodiscard]] auto opposite(const OverlayPlacement placement) -> OverlayPlacement {
            switch (placement) {
                case OverlayPlacement::top:
                    return OverlayPlacement::bottom;
                case OverlayPlacement::right:
                    return OverlayPlacement::left;
                case OverlayPlacement::bottom:
                    return OverlayPlacement::top;
                case OverlayPlacement::left:
                    return OverlayPlacement::right;
            }
            return placement;
        }

        [[nodiscard]] auto aligned_origin(
            const foundation::NanRect anchor,
            const foundation::NanSize overlay_size,
            const OverlayPlacement placement,
            const OverlayAlignment alignment,
            const float gap
        ) -> foundation::NanPoint {
            float x = anchor.get_left();
            float y = anchor.get_top();
            if (placement == OverlayPlacement::top || placement == OverlayPlacement::bottom) {
                if (alignment == OverlayAlignment::center) {
                    x = anchor.get_center().get_x() - overlay_size.get_width() * 0.5F;
                }
                else if (alignment == OverlayAlignment::end) {
                    x = anchor.get_right() - overlay_size.get_width();
                }
                y = placement == OverlayPlacement::top
                    ? anchor.get_top() - gap - overlay_size.get_height()
                    : anchor.get_bottom() + gap;
            }
            else {
                if (alignment == OverlayAlignment::center) {
                    y = anchor.get_center().get_y() - overlay_size.get_height() * 0.5F;
                }
                else if (alignment == OverlayAlignment::end) {
                    y = anchor.get_bottom() - overlay_size.get_height();
                }
                x = placement == OverlayPlacement::left
                    ? anchor.get_left() - gap - overlay_size.get_width()
                    : anchor.get_right() + gap;
            }
            return foundation::NanPoint(x, y);
        }

        [[nodiscard]] auto candidate(
            const foundation::NanRect anchor,
            const foundation::NanSize overlay_size,
            const AnchoredPositionOptions& options,
            const OverlayPlacement placement
        ) -> foundation::NanRect {
            const auto origin = aligned_origin(
                                    anchor,
                                    overlay_size,
                                    placement,
                                    options.alignment,
                                    options.gap
                                )
                + options.offset;
            return foundation::NanRect::from_origin_size(origin, overlay_size);
        }

        [[nodiscard]] auto overflow(const foundation::NanRect rect, const foundation::NanRect bounds)
            -> float {
            return std::max(0.0F, bounds.get_left() - rect.get_left())
                + std::max(0.0F, rect.get_right() - bounds.get_right())
                + std::max(0.0F, bounds.get_top() - rect.get_top())
                + std::max(0.0F, rect.get_bottom() - bounds.get_bottom());
        }

        [[nodiscard]] auto shifted_into(
            const foundation::NanRect rect,
            const foundation::NanRect bounds
        ) -> foundation::NanRect {
            const float x = rect.get_width() <= bounds.get_width()
                ? std::clamp(rect.get_left(), bounds.get_left(), bounds.get_right() - rect.get_width())
                : bounds.get_left();
            const float y = rect.get_height() <= bounds.get_height()
                ? std::clamp(rect.get_top(), bounds.get_top(), bounds.get_bottom() - rect.get_height())
                : bounds.get_top();
            return rect.with_origin(x, y);
        }
    }

    auto position_anchored_overlay(
        const foundation::NanRect anchor,
        const foundation::NanSize overlay_size,
        const foundation::NanRect viewport,
        const AnchoredPositionOptions& options
    ) -> AnchoredPosition {
        if (!finite(anchor) || !anchor.is_valid()) {
            throw std::invalid_argument("anchored overlay anchor must be finite and valid");
        }
        if (!finite(overlay_size) || !overlay_size.is_valid()) {
            throw std::invalid_argument("anchored overlay size must be finite and positive");
        }
        if (!finite(viewport) || !viewport.is_valid()) {
            throw std::invalid_argument("anchored overlay viewport must be finite and valid");
        }
        if (!finite(options.offset) || !std::isfinite(options.gap) || options.gap < 0.0F
            || !std::isfinite(options.viewport_padding) || options.viewport_padding < 0.0F)
        {
            throw std::invalid_argument("anchored overlay offsets must be finite and non-negative");
        }

        const float max_padding = std::min(viewport.get_width(), viewport.get_height()) * 0.5F;
        const auto bounds = viewport.inset_by(
            foundation::NanInsets::all(std::min(options.viewport_padding, max_padding))
        );
        auto placement = options.placement;
        auto rect = candidate(anchor, overlay_size, options, placement);
        bool flipped = false;
        if (options.flip) {
            const auto alternative_placement = opposite(placement);
            const auto alternative = candidate(anchor, overlay_size, options, alternative_placement);
            if (overflow(alternative, bounds) < overflow(rect, bounds)) {
                placement = alternative_placement;
                rect = alternative;
                flipped = true;
            }
        }

        const auto before_shift = rect;
        if (options.shift) {
            rect = shifted_into(rect, bounds);
        }
        return {
            .rect = rect,
            .placement = placement,
            .flipped = flipped,
            .shifted = rect != before_shift,
        };
    }
}
