#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_ANCHORED_POSITIONER_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_ANCHORED_POSITIONER_HPP

#include "../../foundation/geometry.hpp"

namespace nandina::widget::internal
{
    enum class OverlayPlacement {
        top,
        right,
        bottom,
        left,
    };

    enum class OverlayAlignment {
        start,
        center,
        end,
    };

    struct AnchoredPositionOptions {
        OverlayPlacement placement = OverlayPlacement::bottom;
        OverlayAlignment alignment = OverlayAlignment::center;
        float gap = 0.0F;
        foundation::NanPoint offset = foundation::NanPoint::zero();
        float viewport_padding = 0.0F;
        bool flip = true;
        bool shift = true;
    };

    struct AnchoredPosition {
        foundation::NanRect rect;
        OverlayPlacement placement = OverlayPlacement::bottom;
        bool flipped = false;
        bool shifted = false;
    };

    [[nodiscard]] auto position_anchored_overlay(
        foundation::NanRect anchor,
        foundation::NanSize overlay_size,
        foundation::NanRect viewport,
        const AnchoredPositionOptions& options = {}
    ) -> AnchoredPosition;
}

#endif
