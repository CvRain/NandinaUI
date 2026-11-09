//
// app/window_metrics — platform-independent window and framebuffer metrics.
//

#include "window_metrics.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace nandina::app
{
    namespace
    {
        constexpr float uniform_scale_relative_tolerance = 0.01F;

        [[nodiscard]] auto valid_size(const foundation::NanSize size) -> bool {
            return std::isfinite(size.get_width()) && std::isfinite(size.get_height())
                && size.is_valid();
        }

        [[nodiscard]] auto sanitized_content_scale(const float content_scale) noexcept -> float {
            return std::isfinite(content_scale) && content_scale > 0.0F ? content_scale : 1.0F;
        }

        void require_valid_size(const foundation::NanSize size, const char* message) {
            if (!valid_size(size)) {
                throw std::invalid_argument(message);
            }
        }
    } // namespace

    auto make_window_metrics(
        const foundation::NanSize screen_size,
        const foundation::NanSize framebuffer_size
    ) -> WindowMetrics {
        if (!valid_size(screen_size)) {
            throw std::invalid_argument("window screen size must be finite and positive");
        }
        if (!valid_size(framebuffer_size)) {
            throw std::invalid_argument("window framebuffer size must be finite and positive");
        }

        const float scale_x = framebuffer_size.get_width() / screen_size.get_width();
        const float scale_y = framebuffer_size.get_height() / screen_size.get_height();
        if (!std::isfinite(scale_x) || scale_x <= 0.0F || !std::isfinite(scale_y)
            || scale_y <= 0.0F)
        {
            throw std::invalid_argument("window framebuffer scales must be finite and positive");
        }
        const float scale_difference = std::abs(scale_x - scale_y);
        const float largest_scale = std::max(scale_x, scale_y);

        return {
            .screen_size = screen_size,
            .framebuffer_size = framebuffer_size,
            .framebuffer_scale_x = scale_x,
            .framebuffer_scale_y = scale_y,
            .screen_to_physical = std::min(scale_x, scale_y),
            .framebuffer_scale_is_uniform =
                scale_difference <= largest_scale * uniform_scale_relative_tolerance,
        };
    }

    auto content_scale_from_display(const float display_scale, const float pixel_density) noexcept
        -> float {
        if (!std::isfinite(display_scale) || display_scale <= 0.0F || !std::isfinite(pixel_density)
            || pixel_density <= 0.0F)
        {
            return 1.0F;
        }
        return sanitized_content_scale(display_scale / pixel_density);
    }

    auto
    window_size_from_logical_size(const foundation::NanSize logical_size, const float content_scale)
        -> foundation::NanSize {
        require_valid_size(logical_size, "logical window size must be finite and positive");
        const float scale = sanitized_content_scale(content_scale);
        return foundation::NanSize(
            logical_size.get_width() * scale,
            logical_size.get_height() * scale
        );
    }

    auto
    logical_size_from_window_size(const foundation::NanSize window_size, const float content_scale)
        -> foundation::NanSize {
        require_valid_size(window_size, "native window size must be finite and positive");
        const float scale = sanitized_content_scale(content_scale);
        return foundation::NanSize(
            window_size.get_width() / scale,
            window_size.get_height() / scale
        );
    }

} // namespace nandina::app
