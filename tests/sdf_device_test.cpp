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

auto main(int argc, char** argv) -> int {
    const bool require_gpu = argc > 1 && std::string(argv[1]) == "--require-gpu";
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
