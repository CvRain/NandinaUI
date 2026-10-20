//
// 生产 SDF 路径的设备级像素测试。
//
// 同一组几何分别走 SFDL（GPU renderer → SdlRenderDevice 的 SDF 路径）与 CPU 几何（软件
// renderer → 三角形扇路径），再与解析预期（straight-alpha over）对照。采样点只取内部、
// 外部与描边带中心，避开 1–2px 的抗锯齿过渡带——AA 质量由探针的解析断言负责。
//
// 无桌面显示、或环境拿不到 GPU renderer 时以 77 退出（Meson 视为 skip）；有显示但像素不符
// 才返回 1。
//

#include <nandina/app/nan_application.hpp>
#include <nandina/foundation/nandina_color.hpp>
#include <nandina/foundation/nandina_color_space.hpp>
#include <nandina/render/backends/sdl_device.hpp>
#include <nandina/render/draw_context.hpp>
#include <nandina/text/font_pipeline.hpp>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
    constexpr int kWidth = 240;
    constexpr int kHeight = 160;
    constexpr int kTolerance = 4;

    struct Rgba {
        int red = 0;
        int green = 0;
        int blue = 0;
        int alpha = 255;
    };

    [[nodiscard]] auto to_rgba(const nandina::foundation::NanColor& color) -> Rgba {
        const auto rgb = color.to<nandina::foundation::NanHexRgb>();
        return {rgb.red, rgb.green, rgb.blue, rgb.alpha};
    }

    /// 与 SDF shader 的 straight-alpha 合成一致（8 位、四舍五入）。
    [[nodiscard]] auto over(Rgba background, Rgba foreground) -> Rgba {
        const auto alpha = static_cast<float>(foreground.alpha) / 255.0F;
        const auto blend = [alpha](int base, int value) {
            return static_cast<int>(std::lround(base * (1.0F - alpha) + value * alpha));
        };
        return {
            blend(background.red, foreground.red),
            blend(background.green, foreground.green),
            blend(background.blue, foreground.blue),
            255,
        };
    }

    [[nodiscard]] auto pixel(SDL_Surface* surface, int horizontal, int vertical) -> Rgba {
        Rgba result {};
        if (!SDL_ReadSurfacePixel(
                surface,
                horizontal,
                vertical,
                reinterpret_cast<Uint8*>(&result.red),
                reinterpret_cast<Uint8*>(&result.green),
                reinterpret_cast<Uint8*>(&result.blue),
                reinterpret_cast<Uint8*>(&result.alpha)
            ))
        {
            std::fprintf(
                stderr,
                "read pixel (%d,%d) failed: %s\n",
                horizontal,
                vertical,
                SDL_GetError()
            );
            return {};
        }
        return result;
    }

    auto require(bool condition, const char* message) -> bool {
        if (!condition) {
            std::fprintf(stderr, "FAIL: %s\n", message);
        }
        return condition;
    }

    auto expect_pixel(
        SDL_Surface* surface,
        int horizontal,
        int vertical,
        Rgba expected,
        const char* label
    ) -> bool {
        const auto actual = pixel(surface, horizontal, vertical);
        if (std::abs(actual.red - expected.red) <= kTolerance
            && std::abs(actual.green - expected.green) <= kTolerance
            && std::abs(actual.blue - expected.blue) <= kTolerance)
        {
            return true;
        }
        std::fprintf(
            stderr,
            "FAIL: %s at (%d,%d): actual=(%d,%d,%d) expected=(%d,%d,%d)\n",
            label,
            horizontal,
            vertical,
            actual.red,
            actual.green,
            actual.blue,
            expected.red,
            expected.green,
            expected.blue
        );
        return false;
    }

    struct Scene {
        nandina::foundation::NanColor background;
        nandina::foundation::NanColor fill;
        nandina::foundation::NanColor outline;
        nandina::foundation::NanColor circle;
        nandina::foundation::NanRect fill_bounds;
        nandina::foundation::NanRect outline_bounds;
        nandina::foundation::NanPoint circle_center;
        float circle_radius = 0.0F;
    };

    auto draw_scene(nandina::render::SdlRenderDevice& device, const Scene& scene) -> void {
        device.begin_frame();
        device.clear(scene.background);
        device.draw_rounded_rect(scene.fill_bounds, 12.0F, scene.fill);
        device.draw_rounded_rect_outline(scene.outline_bounds, 10.0F, 4.0F, scene.outline);
        device.draw_circle(scene.circle_center, scene.circle_radius, scene.circle);
        device.end_frame();
    }

    /// 64×64 子像素的面积积分：几何覆盖率，独立于 shader 的 smoothstep 实现。
    [[nodiscard]] auto
    rect_coverage(float pixel_x, float pixel_y, float left, float top, float right, float bottom)
        -> float {
        constexpr int samples = 64;
        int inside = 0;
        for (int vertical = 0; vertical < samples; ++vertical) {
            for (int horizontal = 0; horizontal < samples; ++horizontal) {
                const float x =
                    pixel_x + (static_cast<float>(horizontal) + 0.5F) / static_cast<float>(samples);
                const float y =
                    pixel_y + (static_cast<float>(vertical) + 0.5F) / static_cast<float>(samples);
                inside += (x >= left && x < right && y >= top && y < bottom) ? 1 : 0;
            }
        }
        return static_cast<float>(inside) / static_cast<float>(samples * samples);
    }

    /// 直线边的覆盖率对照：把渲染像素反解成覆盖率，与几何面积积分逐边缘像素比较。
    /// 只采样边缘 2px 带，避免平坦区域把误差稀释掉。
    auto check_straight_edge_area(SDL_Renderer* renderer) -> bool {
        constexpr float left = 20.5F;
        constexpr float top = 30.25F;
        constexpr float right = 160.75F;
        constexpr float bottom = 90.5F;
        const Rgba background {32, 48, 64, 255};
        const Rgba fill {200, 60, 20, 255};
        bool ok = true;
        auto* previous_target = SDL_GetRenderTarget(renderer);
        auto* scratch =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 320, 200);
        if (!require(scratch != nullptr, "create AA scratch target")) {
            return false;
        }
        SDL_SetRenderTarget(renderer, scratch);
        for (const float scale: {1.0F, 1.25F, 1.5F}) {
            SDL_SetRenderScale(renderer, scale, scale);
            nandina::render::SdlRenderDevice device(*renderer);
            device.begin_frame();
            device.clear(nandina::foundation::NanColor::from_hex(0x203040));
            device.draw_rect(
                nandina::foundation::NanRect::from_ltrb(left, top, right, bottom),
                nandina::foundation::NanColor::from_hex(0xC83C14)
            );
            device.end_frame();
            // 读回前复位缩放：SDL_RenderReadPixels 的区域按 pixel viewport 计算，
            // 非 1.0 缩放下会读回一个缩放后的子区域（表现为越界像素全黑）。
            SDL_SetRenderScale(renderer, 1, 1);
            auto* surface = SDL_RenderReadPixels(renderer, nullptr);
            if (!require(surface != nullptr, "read straight-edge frame")) {
                SDL_SetRenderScale(renderer, 1, 1);
                SDL_SetRenderTarget(renderer, previous_target);
                SDL_DestroyTexture(scratch);
                return false;
            }
            std::fprintf(
                stderr,
                "[aa] scale=%.2f surface=%dx%d corner=%d,%d\n",
                scale,
                surface->w,
                surface->h,
                pixel(surface, 0, 0).red,
                pixel(surface, 0, 0).green
            );
            const auto device_left = static_cast<int>(std::floor(left * scale)) - 2;
            const auto device_top = static_cast<int>(std::floor(top * scale)) - 2;
            const auto device_right = static_cast<int>(std::ceil(right * scale)) + 2;
            const auto device_bottom = static_cast<int>(std::ceil(bottom * scale)) + 2;
            double error_sum = 0.0;
            double error_max = 0.0;
            int edge_pixels = 0;
            for (int y = device_top; y <= device_bottom; ++y) {
                for (int x = device_left; x <= device_right; ++x) {
                    const auto actual = pixel(surface, x, y);
                    const double measured = (static_cast<double>(actual.red) - background.red)
                        / (static_cast<double>(fill.red) - background.red);
                    const double expected = rect_coverage(
                        static_cast<float>(x),
                        static_cast<float>(y),
                        left * scale,
                        top * scale,
                        right * scale,
                        bottom * scale
                    );
                    // 只比较直边过渡带：排除角点附近的像素（SDF 的距离场在直角处与面积
                    // 积分本来就会不同，这里检验的是直线边），预期覆盖率也不能接近 0/1。
                    const bool near_corner = (x < device_left + 6 || x > device_right - 6)
                        && (y < device_top + 6 || y > device_bottom - 6);
                    if (!near_corner && expected > 0.03 && expected < 0.97) {
                        const double error = std::abs(measured - expected);
                        if (error > error_max) {
                            error_max = error;
                            if (std::getenv("NANDINA_AA_DEBUG") != nullptr) {
                                std::fprintf(
                                    stderr,
                                    "[aa] scale=%.2f (%d,%d) measured=%.3f expected=%.3f\n",
                                    scale,
                                    x,
                                    y,
                                    measured,
                                    expected
                                );
                            }
                        }
                        error_sum += error;
                        ++edge_pixels;
                    }
                }
            }
            SDL_DestroySurface(surface);
            const double mae = edge_pixels > 0 ? error_sum / edge_pixels : 0.0;
            std::printf(
                "straight edge AA scale=%.2f edge_mae=%.3f max=%.3f edges=%d\n",
                scale,
                mae,
                error_max,
                edge_pixels
            );
            ok &= require(edge_pixels > 50, "edge band has enough pixels");
            ok &= require(mae <= 0.12, "straight edge area integral MAE");
            ok &= require(error_max <= 0.20, "straight edge area integral max");
        }
        SDL_SetRenderScale(renderer, 1, 1);
        SDL_SetRenderTarget(renderer, previous_target);
        SDL_DestroyTexture(scratch);
        return ok;
    }

    auto check_pixels(SDL_Surface* surface, const Scene& scene, const char* path) -> bool {
        const auto background = to_rgba(scene.background);
        const auto fill = over(background, to_rgba(scene.fill));
        const auto outline = over(background, to_rgba(scene.outline));
        const auto circle = over(background, to_rgba(scene.circle));
        bool ok = true;
        // 填充内部、圆角外侧。
        ok &= expect_pixel(surface, 60, 50, fill, path);
        ok &= expect_pixel(surface, 22, 22, background, path);
        // 描边带中心、中空。
        ok &= expect_pixel(surface, 175, 22, outline, path);
        ok &= expect_pixel(surface, 175, 50, background, path);
        // 圆内、圆外。
        ok &= expect_pixel(surface, 70, 120, circle, path);
        ok &= expect_pixel(surface, 70, 90, background, path);
        return ok;
    }

    auto check_coordinates(SDL_Renderer* renderer, bool sdf) -> bool {
        bool ok = true;
        nandina::render::SdlRenderDevice device(*renderer);
        for (const bool letterbox: {false, true}) {
            for (const float scale: {1.0F, 1.25F, 1.5F}) {
                for (int path = 0; path < (sdf ? 3 : 2); ++path) {
                    ok &= require(SDL_SetRenderScale(renderer, 1, 1), "reset scale");
                    ok &= require(SDL_SetRenderViewport(renderer, nullptr), "reset viewport");
                    ok &= require(
                        SDL_SetRenderLogicalPresentation(
                            renderer,
                            letterbox ? 120 : 0,
                            letterbox ? 100 : 0,
                            letterbox ? SDL_LOGICAL_PRESENTATION_LETTERBOX
                                      : SDL_LOGICAL_PRESENTATION_DISABLED
                        ),
                        "set presentation"
                    );
                    ok &= require(SDL_SetRenderScale(renderer, scale, scale), "set scale");
                    const SDL_Rect viewport {13, 9, 60, 40};
                    ok &= require(SDL_SetRenderViewport(renderer, &viewport), "set viewport");
                    SDL_FRect destination {};
                    SDL_GetRenderLogicalPresentationRect(renderer, &destination);
                    const float effective = scale * (letterbox ? destination.w / 120.0F : 1.0F);
                    const int x = static_cast<int>(std::floor(13 * effective + destination.x));
                    const int y = static_cast<int>(std::floor(9 * effective + destination.y));
                    device.begin_frame();
                    device.clear(nandina::foundation::NanColor::from_hex(0x203040));
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    const SDL_FRect rect {3, 3, 6, 6};
                    if (path == 0) {
                        ok &= require(SDL_RenderFillRect(renderer, &rect), "native fill");
                    }
                    else if (path == 1) {
                        const SDL_FColor white {1, 1, 1, 1};
                        const SDL_Vertex vertices[] {
                            {{3, 3}, white, {}},
                            {{9, 3}, white, {}},
                            {{9, 9}, white, {}},
                            {{3, 9}, white, {}}
                        };
                        const int indices[] {0, 1, 2, 0, 2, 3};
                        ok &= require(
                            SDL_RenderGeometry(renderer, nullptr, vertices, 4, indices, 6),
                            "native geometry"
                        );
                    }
                    else {
                        device.draw_rect(
                            nandina::foundation::NanRect::from_ltrb(3, 3, 9, 9),
                            nandina::foundation::NanColor::from_hex(0xFFFFFF)
                        );
                    }
                    device.end_frame();
                    auto* cropped = SDL_RenderReadPixels(renderer, nullptr);
                    ok &= require(cropped != nullptr, "read viewport-cropped frame");
                    if (cropped != nullptr) {
                        ok &= expect_pixel(
                            cropped,
                            static_cast<int>(6 * effective),
                            static_cast<int>(6 * effective),
                            {255, 255, 255},
                            "cropped readback rebases the viewport origin"
                        );
                        SDL_DestroySurface(cropped);
                    }
                    // ReadPixels is viewport-cropped. Flush before changing view, then read
                    // the complete target, so samples are absolute framebuffer coordinates.
                    ok &= require(SDL_FlushRenderer(renderer), "flush coordinate frame");
                    SDL_SetRenderLogicalPresentation(
                        renderer,
                        0,
                        0,
                        SDL_LOGICAL_PRESENTATION_DISABLED
                    );
                    SDL_SetRenderScale(renderer, 1, 1);
                    SDL_SetRenderViewport(renderer, nullptr);
                    auto* pixels = SDL_RenderReadPixels(renderer, nullptr);
                    ok &= require(pixels != nullptr, "read full coordinate frame");
                    if (pixels != nullptr) {
                        std::printf(
                            "coordinates renderer=%s path=%d letterbox=%d scale=%.2f origin=%d,%d read=%dx%d\n",
                            SDL_GetRendererName(renderer),
                            path,
                            letterbox,
                            scale,
                            x,
                            y,
                            pixels->w,
                            pixels->h
                        );
                        ok &= expect_pixel(
                            pixels,
                            x + static_cast<int>(6 * effective),
                            y + static_cast<int>(6 * effective),
                            {255, 255, 255},
                            "shifted small shape"
                        );
                        ok &= expect_pixel(
                            pixels,
                            static_cast<int>(6 * effective),
                            static_cast<int>(6 * effective),
                            {32, 48, 64},
                            "unshifted location empty"
                        );
                        ok &= expect_pixel(
                            pixels,
                            x + static_cast<int>(12 * effective),
                            y + static_cast<int>(6 * effective),
                            {32, 48, 64},
                            "outside small shape"
                        );
                        SDL_DestroySurface(pixels);
                    }
                }
            }
        }
        return ok;
    }
    auto check_font_interleave(SDL_Renderer* renderer) -> bool {
        using nandina::foundation::NanPoint;
        using nandina::foundation::NanRect;
        nandina::render::SdlRenderDevice device(*renderer);
        auto application = nandina::app::NanApplication(
            nandina::app::NanApplicationConfig::for_process("com.nandina.test.sdf-font")
        );
        nandina::text::FontPipelineCache fonts(
            device,
            application.font_loader(),
            application.font_families()
        );
        const auto font = fonts.get({}, {.atlas_width = 32, .atlas_height = 32});
        if (!require(font.has_value(), "load actual font pipeline")) {
            return false;
        }
        const auto layout = (*font)->backend().layout({
            .text = "SDF atlas Ag 012",
            .style = {.font_size = 20.0F},
            .constraints = nandina::foundation::NanLayoutConstraints::loose(),
        });
        const auto white = nandina::foundation::NanColor::from_hex(0xFFFFFF);
        const auto background = nandina::foundation::NanColor::from_hex(0x203040);
        nandina::render::DrawContext context(device);
        const auto draw = [&](bool interleave) {
            device.begin_frame();
            device.clear(background);
            if (interleave) {
                device.draw_circle(NanPoint(20, 110), 8, white);
            }
            (*font)->pipeline().renderer->draw(layout, context, NanPoint(10, 10), white);
            if (interleave) {
                device.draw_rounded_rect(NanRect::from_ltrb(60, 100, 90, 130), 3, white);
            }
            // A second run also exercises an already-uploaded atlas after a fresh SDF bind.
            (*font)->pipeline().renderer->draw(layout, context, NanPoint(10, 50), white);
            if (interleave) {
                device.draw_circle(NanPoint(120, 110), 8, white);
            }
            device.end_frame();
            return SDL_RenderReadPixels(renderer, nullptr);
        };
        const auto uploads_before = device.stats().alpha_uploads;
        auto* actual = draw(true);
        const auto cold_uploads = device.stats().alpha_uploads - uploads_before;
        const auto uploads_after_cold = device.stats().alpha_uploads;
        auto* reference = draw(false);
        bool ok = require(reference != nullptr && actual != nullptr, "read real glyph frames");
        ok &= require(cold_uploads > 0, "cold interleaved run uploads the actual glyph atlas");
        ok &= require(
            device.stats().alpha_uploads == uploads_after_cold,
            "warm glyph run reuses uploaded atlas"
        );
        ok &= require(
            (*font)->atlas_pixel_count() > 2 * 32 * 32,
            "glyph atlas grows from small initial allocation"
        );
        int foreground = 0;
        int mismatches = 0;
        if (reference != nullptr && actual != nullptr) {
            for (int y = 0; y < 90; ++y) {
                for (int x = 0; x < kWidth; ++x) {
                    const auto expected = pixel(reference, x, y);
                    const auto observed = pixel(actual, x, y);
                    foreground += expected.red > 40;
                    mismatches += std::abs(expected.red - observed.red) > 2
                        || std::abs(expected.green - observed.green) > 2
                        || std::abs(expected.blue - observed.blue) > 2;
                }
            }
            ok &= require(foreground > 100, "reference contains rasterized glyphs");
            ok &= require(mismatches == 0, "real glyph atlas unchanged by SDF interleave");
            ok &= expect_pixel(actual, 120, 110, {255, 255, 255}, "SDF after real glyphs");
            ok &= expect_pixel(actual, 20, 110, {255, 255, 255}, "SDF before real glyphs");
        }
        std::printf(
            "font interleave: foreground=%d mismatches=%d atlas_pixels=%zu binds=%llu unbinds=%llu\n",
            foreground,
            mismatches,
            (*font)->atlas_pixel_count(),
            static_cast<unsigned long long>(device.sdf_diagnostics().frame_binds),
            static_cast<unsigned long long>(device.sdf_diagnostics().frame_unbinds)
        );
        SDL_DestroySurface(reference);
        SDL_DestroySurface(actual);
        return ok;
    }

    auto check_antialiasing(SDL_Renderer* renderer) -> bool {
        using nandina::foundation::NanPoint;
        using nandina::foundation::NanRect;
        nandina::render::SdlRenderDevice device(*renderer);
        const auto white = nandina::foundation::NanColor::from_hex(0xFFFFFF);
        const auto black = nandina::foundation::NanColor::from_hex(0x000000);
        bool ok = true;
        for (const float scale: {1.0F, 1.25F, 1.5F}) {
            SDL_SetRenderScale(renderer, scale, scale);
            for (const float thickness: {0.75F, 1.0F, 1.25F}) {
                device.begin_frame();
                device.clear(black);
                device.draw_rounded_rect_outline(
                    NanRect::from_ltrb(20.25F, 20.25F, 60.25F, 50.25F),
                    2,
                    thickness,
                    white
                );
                device.end_frame();
                auto* surface = SDL_RenderReadPixels(renderer, nullptr);
                ok &= require(surface != nullptr, "read thin outline AA");
                if (surface != nullptr) {
                    // At the middle of a horizontal edge the signed-distance derivative is 1.
                    // The contract's cubic filter is independently evaluated at pixel centers.
                    for (int y = static_cast<int>(19 * scale); y < static_cast<int>(24 * scale);
                         ++y)
                    {
                        // Outline outer bounds snap to logical integer pixels by contract.
                        const double center = (20.0 + thickness / 2.0) * scale;
                        const double distance =
                            std::abs(y + 0.5 - center) - thickness * scale / 2.0;
                        const double t = std::clamp(0.5 - distance, 0.0, 1.0);
                        const int expected =
                            static_cast<int>(std::lround(255 * t * t * (3 - 2 * t)));
                        ok &= expect_pixel(
                            surface,
                            static_cast<int>(40 * scale),
                            y,
                            {expected, expected, expected},
                            "thin outline cubic AA"
                        );
                    }
                    SDL_DestroySurface(surface);
                }
            }
            for (const float radius: {2.0F, 4.0F}) {
                device.begin_frame();
                device.clear(black);
                device.draw_circle(NanPoint(30.25F, 30.25F), radius, white);
                device.end_frame();
                auto* surface = SDL_RenderReadPixels(renderer, nullptr);
                ok &= require(surface != nullptr, "read small circle AA");
                int maximum = 0;
                double sum = 0;
                int edge_count = 0;
                if (surface != nullptr) {
                    const double center = 30.25 * scale;
                    const double physical_radius = radius * scale;
                    for (int y = static_cast<int>(center - physical_radius - 2);
                         y <= center + physical_radius + 2;
                         ++y)
                    {
                        for (int x = static_cast<int>(center - physical_radius - 2);
                             x <= center + physical_radius + 2;
                             ++x)
                        {
                            int inside = 0;
                            // Independent 64x64 area integration, not shader distance/fwidth.
                            for (int sy = 0; sy < 64; ++sy) {
                                for (int sx = 0; sx < 64; ++sx) {
                                    const double dx = x + (sx + 0.5) / 64 - center;
                                    const double dy = y + (sy + 0.5) / 64 - center;
                                    inside +=
                                        dx * dx + dy * dy <= physical_radius * physical_radius;
                                }
                            }
                            const int expected =
                                static_cast<int>(std::lround(255.0 * inside / 4096));
                            const auto observed = pixel(surface, x, y);
                            const int error = std::max(
                                {std::abs(observed.red - expected),
                                 std::abs(observed.green - expected),
                                 std::abs(observed.blue - expected)}
                            );
                            maximum = std::max(maximum, error);
                            if ((expected > 0 && expected < 255)
                                || (observed.red > 0 && observed.red < 255))
                            {
                                sum += error;
                                ++edge_count;
                            }
                        }
                    }
                    // Cubic L1 derivative AA is not exact box coverage. Bound local errors,
                    // never dilute them by averaging the full background image.
                    ok &= require(edge_count > 0, "small circle contains AA edge pixels");
                    ok &=
                        require(maximum <= 64, "circle area-reference maximum error <= 25 percent");
                    ok &= require(
                        edge_count > 0 && sum / edge_count <= 24,
                        "circle area-reference edge MAE <= 9.5 percent"
                    );
                    std::printf(
                        "circle AA scale=%.2f radius=%.1f max=%d edge_mae=%.3f edges=%d\n",
                        scale,
                        radius,
                        maximum,
                        edge_count ? sum / edge_count : 0,
                        edge_count
                    );
                    SDL_DestroySurface(surface);
                }
            }
        }
        SDL_SetRenderScale(renderer, 1, 1);
        return ok;
    }
} // namespace

auto main(int argc, char** argv) -> int {
    const bool require_gpu = argc > 1 && std::string(argv[1]) == "--require-gpu";
    std::string expect_format;
    for (int index = 1; index < argc; ++index) {
        if (std::string(argv[index]) == "--expect-format" && index + 1 < argc) {
            expect_format = argv[index + 1];
        }
    }
    if (SDL_getenv("DISPLAY") == nullptr && SDL_getenv("WAYLAND_DISPLAY") == nullptr) {
        std::printf("SKIP: no desktop display\n");
        return require_gpu ? 1 : 77;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    auto* gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    auto* window = SDL_CreateWindow("nandina sdf device", kWidth, kHeight, SDL_WINDOW_HIDDEN);
    auto* renderer =
        gpu != nullptr && window != nullptr ? SDL_CreateGPURenderer(gpu, window) : nullptr;
    if (renderer == nullptr) {
        std::printf("SKIP: no GPU renderer available (%s)\n", SDL_GetError());
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
        if (gpu != nullptr) {
            SDL_DestroyGPUDevice(gpu);
        }
        SDL_Quit();
        return require_gpu ? 1 : 77;
    }

    const Scene scene {
        .background = nandina::foundation::NanColor::from_hex(0x203040),
        .fill = nandina::foundation::NanColor::from_hex(0xCC3319, 0.6F),
        .outline = nandina::foundation::NanColor::from_hex(0x1ACC33),
        .circle = nandina::foundation::NanColor::from_hex(0x334DE6, 0.7F),
        .fill_bounds = nandina::foundation::NanRect::from_ltrb(20.0F, 20.0F, 120.0F, 90.0F),
        .outline_bounds = nandina::foundation::NanRect::from_ltrb(140.0F, 20.0F, 210.0F, 80.0F),
        .circle_center = nandina::foundation::NanPoint(70.0F, 120.0F),
        .circle_radius = 24.0F,
    };

    bool ok = true;
    ok &= check_coordinates(renderer, true);
    auto* software_window = SDL_CreateWindow("nandina software coordinates", kWidth, kHeight, 0);
    ok &= require(software_window != nullptr, "create software window");
    if (software_window != nullptr) {
        auto* software_renderer = SDL_CreateRenderer(software_window, "software");
        ok &= require(software_renderer != nullptr, "create window software renderer");
        if (software_renderer != nullptr) {
            ok &= check_coordinates(software_renderer, false);
            SDL_DestroyRenderer(software_renderer);
        }
        SDL_DestroyWindow(software_window);
    }
    auto* target = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_TARGET,
        kWidth,
        kHeight
    );
    ok &= require(target != nullptr, "create render target");
    if (target != nullptr) {
        SDL_SetRenderTarget(renderer, target);
        ok &= check_coordinates(renderer, true);
        ok &= check_font_interleave(renderer);
        ok &= check_antialiasing(renderer);
        {
            nandina::render::SdlRenderDevice device(*renderer);
            if (!require(device.analytic_antialiasing(), "GPU renderer must use the SDF path")) {
                std::fprintf(
                    stderr,
                    "reason: %s\n",
                    std::string(device.sdf_unavailable_reason()).c_str()
                );
                return 1;
            }
            draw_scene(device, scene);
            ok &= check_straight_edge_area(renderer);
            const auto first = device.sdf_diagnostics();
            // Linux 上只有 Vulkan 可实机验证；DXIL/MSL 分支在 Windows/macOS 目标设备上另行验收。
            ok &= require(device.sdf_shader_format() == "spirv", "Vulkan uses the SPIR-V artifact");
            if (!expect_format.empty()) {
                ok &= require(
                    device.sdf_shader_format() == expect_format,
                    "--expect-format must match the artifact selected for this GPU driver"
                );
            }
            ok &= require(first.misses == 3, "frame 1 creates three parameter states");
            ok &= require(first.hits == 0, "frame 1 has no cache hits");
            ok &= require(first.cached == 3, "cache keeps the three states");
            ok &= require(first.binds == 3, "frame 1 binds once per parameter block");
            auto* surface = SDL_RenderReadPixels(renderer, nullptr);
            if (require(surface != nullptr, "read SDF frame")) {
                ok &= check_pixels(surface, scene, "sdf");
                SDL_DestroySurface(surface);
            }
            // 第二帧：同样的几何必须全部命中缓存，只有绑定次数增长。
            draw_scene(device, scene);
            const auto second = device.sdf_diagnostics();
            ok &= require(second.misses == first.misses, "frame 2 must not create new states");
            ok &= require(second.hits == 3, "frame 2 hits the cache three times");
            ok &= require(second.binds == 6, "frame 2 binds once per block");
            // 契约要的是"每帧状态切换次数"：逐帧绑定 + 解绑，不能只看累计值。
            ok &= require(second.frame_binds == 3, "frame 2 reports three binds");
            ok &= require(second.frame_unbinds == 1, "frame 2 reports the end-frame unbind");

            // viewport shifts fragment coordinates, including fractional render scales.
            for (const float scale: {1.0F, 1.25F, 1.5F}) {
                device.begin_frame();
                device.clear(scene.background);
                SDL_SetRenderScale(renderer, scale, scale);
                const SDL_Rect viewport {13, 9, 120, 90};
                SDL_SetRenderViewport(renderer, &viewport);
                device.draw_rect(nandina::foundation::NanRect(10, 10, 40, 40), scene.outline);
                device.end_frame();
                SDL_SetRenderViewport(renderer, nullptr);
                auto* shifted = SDL_RenderReadPixels(renderer, nullptr);
                ok &= require(shifted != nullptr, "read viewport frame");
                if (shifted != nullptr) {
                    ok &= expect_pixel(
                        shifted,
                        static_cast<int>(48 * scale),
                        static_cast<int>(44 * scale),
                        to_rgba(scene.outline),
                        "viewport offset"
                    );
                    SDL_DestroySurface(shifted);
                }
            }
            SDL_SetRenderScale(renderer, 1, 1);
            SDL_SetRenderLogicalPresentation(
                renderer,
                120,
                100,
                SDL_LOGICAL_PRESENTATION_LETTERBOX
            );
            device.begin_frame();
            device.clear(scene.background);
            device.draw_rect(nandina::foundation::NanRect(10, 10, 40, 40), scene.outline);
            device.end_frame();
            SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
            auto* logical = SDL_RenderReadPixels(renderer, nullptr);
            ok &= require(logical != nullptr, "read logical presentation");
            if (logical != nullptr) {
                ok &= expect_pixel(
                    logical,
                    80,
                    55,
                    to_rgba(scene.outline),
                    "logical scale and letterbox"
                );
                SDL_DestroySurface(logical);
            }
            SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
            device.begin_frame();
            device.clear(scene.background);
            device.draw_rect(nandina::foundation::NanRect(10, 10, 20, 20), scene.fill);
            SDL_SetRenderScale(renderer, 2, 1);
            ok &= require(!device.analytic_antialiasing(), "nonuniform scale reports CPU quality");
            device.draw_rect(nandina::foundation::NanRect(60, 60, 80, 80), scene.outline);
            device.end_frame();
            SDL_SetRenderScale(renderer, 1, 1);
            auto* fallback = SDL_RenderReadPixels(renderer, nullptr);
            ok &= require(fallback != nullptr, "read nonuniform fallback");
            if (fallback != nullptr) {
                ok &= expect_pixel(
                    fallback,
                    130,
                    70,
                    to_rgba(scene.outline),
                    "fallback restores state"
                );
                SDL_DestroySurface(fallback);
            }

            device.begin_frame();
            device.clear(scene.background);
            for (int index = 0; index < 300; ++index) {
                device.draw_rect(
                    nandina::foundation::NanRect(10, 10, 20 + index * 0.01F, 20),
                    scene.outline
                );
            }
            device.end_frame();
            ok &= require(device.sdf_diagnostics().cached == 256, "GPU cache remains bounded");
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            const SDL_FRect native_rect {120, 100, 30, 30};
            SDL_RenderFillRect(renderer, &native_rect);
            auto* native = SDL_RenderReadPixels(renderer, nullptr);
            ok &= require(native != nullptr, "read native draw after frame");
            if (native != nullptr) {
                ok &= expect_pixel(native, 130, 110, {255, 0, 0, 255}, "end frame restores state");
                ok &= expect_pixel(native, 15, 15, to_rgba(scene.outline), "evicted queued states");
                SDL_DestroySurface(native);
            }
        }
        // 纹理（字形图集与图片都走 draw_texture_region）与 SDF 交错：纹理之后再画 SDF，
        // 必须重新绑定自定义 state，且纹理本身不能被污染。
        {
            static constexpr std::array<std::uint8_t, 16> alpha {
                255,
                255,
                255,
                255,
                255,
                255,
                255,
                255,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
            };
            nandina::render::SdlRenderDevice device(*renderer);
            device.begin_frame();
            device.clear(scene.background);
            device.draw_rect(
                nandina::foundation::NanRect::from_ltrb(10, 10, 40, 40),
                scene.outline
            );
            const auto texture = device.create_alpha_texture(4, 4, alpha);
            ok &= require(texture.value != 0, "create alpha texture");
            device.draw_texture_region(
                texture,
                nandina::foundation::NanRect(0, 0, 4, 4),
                nandina::foundation::NanRect::from_ltrb(80, 20, 88, 28),
                nandina::foundation::NanColor::from_hex(0xFFFFFF)
            );
            device.draw_rounded_rect(
                nandina::foundation::NanRect::from_ltrb(120, 60, 160, 100),
                8,
                scene.fill
            );
            device.end_frame();
            auto* surface = SDL_RenderReadPixels(renderer, nullptr);
            ok &= require(surface != nullptr, "read texture interleave");
            if (surface != nullptr) {
                // 8px 的目标对应 4 个 texel（每 texel 2px）：row 0/1 = 255，row 2/3 = 0。
                ok &= expect_pixel(surface, 84, 21, {255, 255, 255, 255}, "texture after SDF");
                ok &= expect_pixel(surface, 84, 27, to_rgba(scene.background), "texture mask zero");
                ok &= expect_pixel(surface, 25, 20, to_rgba(scene.outline), "SDF before texture");
                ok &= expect_pixel(
                    surface,
                    140,
                    80,
                    over(to_rgba(scene.background), to_rgba(scene.fill)),
                    "SDF after texture"
                );
                SDL_DestroySurface(surface);
            }
            device.destroy_texture(texture);
        }

        // 驱动逃生舱：显式禁用 SDF 后能力查询如实为 false，CPU 几何仍然正确。
        {
            SDL_setenv_unsafe("NANDINA_SDL_SDF", "off", 1);
            nandina::render::SdlRenderDevice device(*renderer);
            ok &= require(!device.analytic_antialiasing(), "escape switch disables SDF");
            ok &= require(device.sdf_shader_format().empty(), "escape switch has no shader format");
            ok &=
                require(!device.sdf_unavailable_reason().empty(), "escape switch reports a reason");
            device.begin_frame();
            device.clear(scene.background);
            device.draw_rect(
                nandina::foundation::NanRect::from_ltrb(30, 30, 70, 60),
                scene.outline
            );
            device.end_frame();
            auto* surface = SDL_RenderReadPixels(renderer, nullptr);
            ok &= require(surface != nullptr, "read escape frame");
            if (surface != nullptr) {
                ok &= expect_pixel(surface, 50, 45, to_rgba(scene.outline), "escape CPU pixels");
                SDL_DestroySurface(surface);
            }
            SDL_setenv_unsafe("NANDINA_SDL_SDF", "", 1);
        }

        // 运行期失败注入：第三次 state 创建失败后，本设备的 SDF 路径停用，该图元由 CPU 画出。
        {
            SDL_setenv_unsafe("NANDINA_SDL_SDF_TEST_FAIL_AFTER", "2", 1);
            nandina::render::SdlRenderDevice device(*renderer);
            device.begin_frame();
            device.clear(scene.background);
            device.draw_rect(nandina::foundation::NanRect(10, 10, 20, 20), scene.outline);
            device.draw_rect(
                nandina::foundation::NanRect::from_ltrb(40, 10, 60, 30),
                scene.outline
            );
            device.draw_rect(
                nandina::foundation::NanRect::from_ltrb(70, 10, 90, 30),
                scene.outline
            );
            device.end_frame();
            ok &= require(!device.analytic_antialiasing(), "injected failure disables SDF");
            ok &= require(
                device.sdf_shader_format().empty(),
                "injected failure has no shader format"
            );
            ok &= require(
                device.sdf_unavailable_reason().find("injected") != std::string_view::npos,
                "injected failure is reported"
            );
            auto* surface = SDL_RenderReadPixels(renderer, nullptr);
            ok &= require(surface != nullptr, "read injected failure frame");
            if (surface != nullptr) {
                ok &= expect_pixel(
                    surface,
                    80,
                    20,
                    to_rgba(scene.outline),
                    "failed draw falls back to CPU"
                );
                SDL_DestroySurface(surface);
            }
            SDL_setenv_unsafe("NANDINA_SDL_SDF_TEST_FAIL_AFTER", "0", 1);
        }

        SDL_SetRenderTarget(renderer, nullptr);
        SDL_DestroyTexture(target);
    }

    // CPU 几何路径：同一几何在软件 renderer 上渲染，作为 SDF 路径的对照。
    auto* cpu_surface = SDL_CreateSurface(kWidth, kHeight, SDL_PIXELFORMAT_RGBA32);
    ok &= require(cpu_surface != nullptr, "create CPU surface");
    if (cpu_surface != nullptr) {
        auto* cpu_renderer = SDL_CreateSoftwareRenderer(cpu_surface);
        ok &= require(cpu_renderer != nullptr, "create software renderer");
        if (cpu_renderer != nullptr) {
            ok &= check_coordinates(cpu_renderer, false);
            nandina::render::SdlRenderDevice device(*cpu_renderer);
            ok &=
                require(!device.analytic_antialiasing(), "software renderer must fall back to CPU");
            ok &= require(!device.sdf_unavailable_reason().empty(), "fallback reason reported");
            draw_scene(device, scene);
            // 软件 renderer 绘入内部 backbuffer：用 RenderReadPixels 读回，而不是直接读传入的 surface
            // （实测：未 present 前 surface 仍是全零）。
            auto* cpu_read = SDL_RenderReadPixels(cpu_renderer, nullptr);
            ok &= require(cpu_read != nullptr, "read CPU frame");
            if (cpu_read != nullptr) {
                ok &= check_pixels(cpu_read, scene, "cpu");
                SDL_DestroySurface(cpu_read);
            }
            SDL_DestroyRenderer(cpu_renderer);
        }
        SDL_DestroySurface(cpu_surface);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyGPUDevice(gpu);
    SDL_Quit();
    if (!ok) {
        return 1;
    }
    std::printf(
        "SDF device pixels, viewport scales, fallback, frame restore and eviction passed\n"
    );
    return 0;
}
