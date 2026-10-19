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

#include <nandina/foundation/nandina_color.hpp>
#include <nandina/foundation/nandina_color_space.hpp>
#include <nandina/render/backends/sdl_device.hpp>

#include <SDL3/SDL.h>

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
} // namespace

auto main() -> int {
    if (SDL_getenv("DISPLAY") == nullptr && SDL_getenv("WAYLAND_DISPLAY") == nullptr) {
        std::printf("SKIP: no desktop display\n");
        return 77;
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
        return 77;
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
            const auto first = device.sdf_diagnostics();
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
    std::printf("SDF device path matches CPU geometry on %d sampled pixels\n", 6 + 6);
    return 0;
}
