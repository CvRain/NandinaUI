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
    [[nodiscard]] auto
    make_window_metrics(foundation::NanSize screen_size, foundation::NanSize framebuffer_size)
        -> WindowMetrics;

    /**
     * OS content scale from a platform display scale and pixel density.
     *
     * `display_scale` is physical pixels per native window coordinate; `pixel_density` is
     * framebuffer pixels per native window coordinate. Their ratio is the content scale that maps
     * logical/authoring units to native window coordinates:
     * - Windows at 175%: display_scale 1.75, pixel_density 1.0 -> 1.75 (window coordinates are
     *   physical pixels, so logical content must be multiplied by 1.75);
     * - Retina/macOS: display_scale 2.0, pixel_density 2.0 -> 1.0 (window coordinates are points,
     *   and the 2x framebuffer is content-density only).
     *
     * A non-finite or non-positive input means the platform query is unavailable; the fallback is
     * 1.0 (no content scaling). This helper is pure and platform-independent so the mapping can be
     * unit tested; the SDL seam feeds it `SDL_GetWindowDisplayScale` and
     * `SDL_GetWindowPixelDensity`.
     */
    [[nodiscard]] auto content_scale_from_display(float display_scale, float pixel_density) noexcept
        -> float;

    /**
     * Convert a logical (author/layout) window size to native platform window coordinates.
     * The size must be finite and strictly positive; `content_scale` falls back to 1.0 when it is
     * not finite/positive. The result is not rounded; platform backends round for their APIs.
     */
    [[nodiscard]] auto
    window_size_from_logical_size(foundation::NanSize logical_size, float content_scale)
        -> foundation::NanSize;

    /**
     * Convert a native platform window size back to logical (author/layout) window units.
     * Inverse of `window_size_from_logical_size` with the same validation and fallback.
     */
    [[nodiscard]] auto
    logical_size_from_window_size(foundation::NanSize window_size, float content_scale)
        -> foundation::NanSize;

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_WINDOW_METRICS_HPP
