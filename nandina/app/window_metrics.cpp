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

} // namespace nandina::app
