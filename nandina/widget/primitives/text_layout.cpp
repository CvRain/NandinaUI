//
// widget/primitives/text_layout — backend-neutral caret geometry queries.
//

#include "text_layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace nandina::widget::primitives
{
    auto TextLayoutLine::caret_for_source(
        const std::size_t source_offset,
        const TextAffinity affinity
    ) const -> TextCaretStop {
        if (caret_stops.empty()) {
            return {.source_offset = text_offset};
        }

        const auto line_end = text_offset + text_length;
        const auto requested = std::clamp(source_offset, text_offset, line_end);
        std::size_t resolved = text_offset;
        for (const auto& stop: caret_stops) {
            if (stop.source_offset <= requested) {
                resolved = std::max(resolved, stop.source_offset);
            }
        }

        const TextCaretStop* fallback = nullptr;
        for (const auto& stop: caret_stops) {
            if (stop.source_offset != resolved) {
                continue;
            }
            if (stop.affinity == affinity) {
                return stop;
            }
            if (fallback == nullptr) {
                fallback = &stop;
            }
        }
        return fallback != nullptr ? *fallback : caret_stops.front();
    }

    auto TextLayoutLine::caret_for_x(float x) const -> TextCaretStop {
        if (caret_stops.empty()) {
            return {.source_offset = text_offset};
        }
        if (std::isnan(x)) {
            x = 0.0F;
        }
        if (x == -std::numeric_limits<float>::infinity()) {
            return caret_stops.front();
        }
        if (x == std::numeric_limits<float>::infinity()) {
            return caret_stops.back();
        }

        const TextCaretStop* nearest = &caret_stops.front();
        float nearest_distance = std::abs(x - nearest->x);
        for (const auto& stop: caret_stops) {
            const float distance = std::abs(x - stop.x);
            if (distance < nearest_distance) {
                nearest = &stop;
                nearest_distance = distance;
            }
        }
        return *nearest;
    }

    auto TextLayoutResult::caret_for_point(const foundation::NanPoint point) const
        -> TextCaretStop {
        if (lines.empty()) {
            return {};
        }

        float line_top = 0.0F;
        for (const auto& line: lines) {
            const float line_bottom = line_top + line.size.get_height();
            if (point.get_y() <= line_bottom) {
                return line.caret_for_x(point.get_x());
            }
            line_top = line_bottom;
        }
        return lines.back().caret_for_x(point.get_x());
    }
} // namespace nandina::widget::primitives
