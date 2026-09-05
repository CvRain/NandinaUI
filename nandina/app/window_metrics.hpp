//
// app/window_metrics — platform-independent window and framebuffer metrics.
//

#ifndef NANDINA_EXPERIMENT_APP_WINDOW_METRICS_HPP
#define NANDINA_EXPERIMENT_APP_WINDOW_METRICS_HPP

#include "../foundation/geometry.hpp"

namespace nandina::app
{
    /**
     * One-frame snapshot of the window coordinate spaces.
     *
     * `screen_size` is expressed in screen/logical window units; `framebuffer_size` is expressed
     * in physical pixels. The current text/render pipeline accepts one physical scale, so a truly
     * non-uniform framebuffer uses the smaller axis and exposes that fact through
     * `framebuffer_scale_is_uniform`.
     */
    struct WindowMetrics {
        foundation::NanSize screen_size;
        foundation::NanSize framebuffer_size;
        float framebuffer_scale_x = 1.0F;
        float framebuffer_scale_y = 1.0F;
        float screen_to_physical = 1.0F;
        bool framebuffer_scale_is_uniform = true;
    };

    /**
     * Build a validated metrics snapshot without accessing the platform backend.
     * Both sizes must be finite and strictly positive, otherwise `invalid_argument` is thrown.
     */
    [[nodiscard]] auto make_window_metrics(
        foundation::NanSize screen_size,
        foundation::NanSize framebuffer_size
    ) -> WindowMetrics;

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_WINDOW_METRICS_HPP
