//
// probe/cross_platform/desktop — SDL3 后端探测
//
// 两条并行的证据线，互不替代：
//
//   1. **IRenderDevice 路径**：`SdlRenderDevice` 用 SDL 2D API（含 CPU 生成几何）实现 Nandina 的
//      设备契约，本文件通过接口画一组图元。这条线回答的是「框架的绘制契约在这个后端上能不能
//      落地」——包括 capability 查询、裁剪语义、alpha 图集上传与按子矩形更新。
//   2. **GPU render state 路径**：用 glslc 预编译的 HLSL→SPIR-V fragment shader 画一个 SDF
//      圆角矩形并做裁剪。这条线回答的是「SDL 3.4 的 GPU-backed renderer 能不能吃自定义 shader」。
//
// 退出：ESC 或关闭窗口。自动化证据：设 `NANDINA_PROBE_CAPTURE=<path.bmp>` 会在第 30 帧
// 读回帧缓冲存成 BMP 后退出（帧缓冲读回不依赖窗口在合成器里的位置）。
//
// 见 probe/cross_platform/README.md 与 docs/references/cross_platform_backend_probe.md。

#include <nandina/render/backends/sdl_device.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>

#include <nandina/foundation/nandina_color.hpp>
#include <nandina/foundation/nandina_color_space.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using nandina::foundation::NanColor;
    using nandina::foundation::NanHexRgb;
    using nandina::foundation::NanPoint;
    using nandina::foundation::NanRect;

    constexpr int kCaptureFrame = 30;

    [[noreturn]] void fail(const std::string& operation) {
        throw std::runtime_error(operation + ": " + SDL_GetError());
    }

    void check_sdl(const bool ok, const std::string& operation) {
        if (!ok) {
            fail(operation);
        }
    }

    auto read_shader() -> std::vector<Uint8> {
        const char* base_path = SDL_GetBasePath();
        if (base_path == nullptr) {
            fail("SDL_GetBasePath");
        }
        const auto shader_path = std::filesystem::path(base_path) / "assets" / "sdf.frag.spv";
        std::ifstream input(shader_path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Cannot open shader artifact: " + shader_path.string());
        }
        const auto begin = std::istreambuf_iterator<char> {input};
        const auto end = std::istreambuf_iterator<char> {};
        std::vector<Uint8> bytes(begin, end);
        if (bytes.empty() || bytes.size() % sizeof(std::uint32_t) != 0) {
            throw std::runtime_error("SPIR-V artifact is empty or malformed");
        }
        return bytes;
    }

    auto normalized_key(const SDL_Keycode key) -> const char* {
        switch (key) {
            case SDLK_SPACE:
                return "Space";
            case SDLK_LEFT:
                return "ArrowLeft";
            case SDLK_RIGHT:
                return "ArrowRight";
            case SDLK_UP:
                return "ArrowUp";
            case SDLK_DOWN:
                return "ArrowDown";
            case SDLK_ESCAPE:
                return "Escape";
            default:
                return "Unmapped";
        }
    }

    /// 64×64 的 A8 字形图集：手写块状字形，够验证上传/子矩形更新/缩放绘制三条路径。
    auto make_alpha_atlas() -> std::vector<std::uint8_t> {
        std::vector<std::uint8_t> alpha(64 * 64, 0);
        for (int y = 8; y < 56; ++y) {
            for (int x = 8; x < 56; ++x) {
                const bool stem = x >= 16 && x < 22;
                const bool diagonal = x >= 22 && x < 46 && (x - 22) / 2 == (y - 8) / 4;
                const bool right_stem = x >= 42 && x < 48;
                if (stem || diagonal || right_stem) {
                    alpha[static_cast<std::size_t>(y) * 64 + x] = 255;
                }
            }
        }
        return alpha;
    }

    auto color_rgba(const std::uint32_t packed) -> NanColor {
        return NanColor::from(NanHexRgb::from_uint32(packed));
    }

    /// 通过 IRenderDevice 画一整组图元。刻意覆盖：填充/描边/圆角/圆/线段/文字/裁剪。
    void draw_device_scene(nandina::render::SdlRenderDevice& device) {
        device.clear(color_rgba(0xf5f6f8ffU));

        // 填充与描边
        device.draw_rect(NanRect(40.0F, 40.0F, 220.0F, 120.0F), color_rgba(0xe8ecf2ffU));
        device.draw_rect_outline(
            NanRect(40.0F, 40.0F, 220.0F, 120.0F),
            2.0F,
            color_rgba(0x9aa7b8ffU)
        );

        // 圆角矩形（CPU 三角形扇）
        device.draw_rounded_rect(
            NanRect(40.0F, 150.0F, 220.0F, 260.0F),
            16.0F,
            color_rgba(0x2f6f5effU)
        );
        device.draw_rounded_rect(
            NanRect(60.0F, 170.0F, 200.0F, 240.0F),
            10.0F,
            color_rgba(0x33b88cffU)
        );

        // 圆与线段
        device.draw_circle(NanPoint(300.0F, 100.0F), 48.0F, color_rgba(0xd98b2bffU));
        device.draw_circle(NanPoint(300.0F, 100.0F), 24.0F, color_rgba(0xf5f6f8ffU));
        device.draw_line(
            NanPoint(260.0F, 200.0F),
            NanPoint(420.0F, 260.0F),
            6.0F,
            color_rgba(0x5b6b7cffU)
        );

        // 接口的默认实现会走 draw_line/draw_circle —— 用来证明「不覆写也能得到合理结果」
        device
            .draw_arc(NanPoint(480.0F, 160.0F), 30.0F, 56.0F, 0.0F, 4.7F, color_rgba(0x6f4ad9ffU));
        device.draw_circle_clipped_rounded_rect(
            NanPoint(560.0F, 160.0F),
            60.0F,
            NanRect(520.0F, 120.0F, 640.0F, 220.0F),
            20.0F,
            color_rgba(0x4a7bd9ffU)
        );

        // 文字：接口路径（SDL 内置字体；真正的文本路径看下面的 alpha 图集）
        device.draw_text(
            "Nandina UI over SDL",
            NanPoint(40.0F, 300.0F),
            16.0F,
            color_rgba(0x1b1f24ffU)
        );

        // 裁剪：同一组图元在裁剪矩形内只能画出一部分
        device.set_clip(NanRect(320.0F, 300.0F, 520.0F, 400.0F));
        device.draw_rounded_rect(
            NanRect(340.0F, 280.0F, 520.0F, 420.0F),
            12.0F,
            color_rgba(0xc23b3bffU)
        );
        device.clear_clip();
    }
} // namespace

int main(int argc, char** argv) {
    SDL_Window* window = nullptr;
    SDL_GPUDevice* gpu_device = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GPUShader* fragment = nullptr;
    SDL_GPURenderState* state = nullptr;
    SDL_Texture* shape_texture = nullptr;
    int exit_code = 0;

    (void)argc;
    (void)argv;

    try {
        check_sdl(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
        gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
        if (gpu_device == nullptr) {
            fail("SDL_CreateGPUDevice(vulkan, SPIR-V)");
        }
        window = SDL_CreateWindow(
            "Nandina SDL backend probe",
            800,
            600,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
        );
        if (window == nullptr) {
            fail("SDL_CreateWindow");
        }
        renderer = SDL_CreateGPURenderer(gpu_device, window);
        if (renderer == nullptr) {
            fail("SDL_CreateGPURenderer");
        }

        // ---- GPU render state 路径：自定义 SDF fragment shader ----
        const auto shader_code = read_shader();
        const SDL_GPUShaderCreateInfo shader_info {
            .code_size = shader_code.size(),
            .code = shader_code.data(),
            .entrypoint = "main",
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 1,
            .num_storage_textures = 0,
            .num_storage_buffers = 0,
            .num_uniform_buffers = 0,
            .props = 0,
        };
        fragment = SDL_CreateGPUShader(gpu_device, &shader_info);
        if (fragment == nullptr) {
            fail("SDL_CreateGPUShader(fragment SPIR-V)");
        }
        const SDL_GPURenderStateCreateInfo state_info {
            .fragment_shader = fragment,
            .num_sampler_bindings = 0,
            .sampler_bindings = nullptr,
            .num_storage_textures = 0,
            .storage_textures = nullptr,
            .num_storage_buffers = 0,
            .storage_buffers = nullptr,
            .props = 0,
        };
        state = SDL_CreateGPURenderState(renderer, &state_info);
        if (state == nullptr) {
            fail("SDL_CreateGPURenderState");
        }
        shape_texture =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, 1, 1);
        if (shape_texture == nullptr) {
            fail("SDL_CreateTexture(SDF source)");
        }
        constexpr Uint32 white_pixel = 0xffffffffU;
        check_sdl(
            SDL_UpdateTexture(shape_texture, nullptr, &white_pixel, sizeof(white_pixel)),
            "SDL_UpdateTexture(SDF source)"
        );
        check_sdl(
            SDL_SetTextureBlendMode(shape_texture, SDL_BLENDMODE_BLEND),
            "SDL_SetTextureBlendMode(SDF source)"
        );
        check_sdl(
            SDL_SetTextureColorMod(shape_texture, 51, 184, 140),
            "SDL_SetTextureColorMod(SDF source)"
        );

        // ---- IRenderDevice 路径 ----
        nandina::render::SdlRenderDevice device(*renderer);
        const auto atlas_alpha = make_alpha_atlas();
        const auto atlas = device.create_alpha_texture(64, 64, atlas_alpha);
        if (!atlas) {
            throw std::runtime_error("create_alpha_texture returned an empty handle");
        }
        // 子矩形更新：只改右下角 16×16，用来验证 update_alpha_texture_region 的路径。
        std::vector<std::uint8_t> updated(64 * 64, 0);
        for (int y = 40; y < 56; ++y) {
            for (int x = 40; x < 56; ++x) {
                updated[static_cast<std::size_t>(y) * 64 + x] =
                    static_cast<std::uint8_t>((x + y) % 2 == 0 ? 255 : 90);
            }
        }
        device.update_alpha_texture_region(
            atlas,
            64,
            64,
            NanRect(40.0F, 40.0F, 56.0F, 56.0F),
            updated
        );

        std::cout << "SDL " << SDL_GetVersion() << ", renderer="
                  << (SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "unknown")
                  << ", shader-formats=0x" << std::hex << SDL_GetGPUShaderFormats(gpu_device)
                  << std::dec << ", DPI=" << SDL_GetWindowDisplayScale(window) << '\n';
        std::cout << "device capabilities: alpha_textures=" << device.supports_alpha_textures()
                  << " rounded_rect=" << device.supports_rounded_rect() << '\n';

        const auto* capture_path = std::getenv("NANDINA_PROBE_CAPTURE");
        auto frame = 0;
        bool running = true;
        while (running) {
            SDL_Event event {};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                    std::cout << "keycode=" << event.key.key
                              << " scancode=" << static_cast<int>(event.key.scancode)
                              << " framework=" << normalized_key(event.key.key) << '\n';
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                }
            }

            device.begin_frame();
            draw_device_scene(device);

            // 图集绘制走设备的纹理路径（源矩形 → 目标矩形 + tint）。
            device.draw_texture_region(
                atlas,
                NanRect(0.0F, 0.0F, 64.0F, 64.0F),
                NanRect(600.0F, 300.0F, 696.0F, 396.0F),
                color_rgba(0x1b1f24ffU)
            );

            // GPU render state 路径：自定义 SDF fragment shader 画一个带阴影的圆角矩形。
            check_sdl(SDL_SetGPURenderState(renderer, state), "SDL_SetGPURenderState(state)");
            const SDL_Rect clip {240, 380, 320, 160};
            check_sdl(SDL_SetRenderClipRect(renderer, &clip), "SDL_SetRenderClipRect");
            const SDL_FRect shape_destination {260.0F, 400.0F, 320.0F, 160.0F};
            check_sdl(
                SDL_RenderTexture(renderer, shape_texture, nullptr, &shape_destination),
                "SDL_RenderTexture with custom SDF fragment shader"
            );
            check_sdl(SDL_SetRenderClipRect(renderer, nullptr), "SDL_SetRenderClipRect(clear)");
            check_sdl(SDL_SetGPURenderState(renderer, nullptr), "SDL_SetGPURenderState(clear)");

            // 设备在帧尾自己清裁剪，避免把状态留给下一帧。
            device.clear_clip();
            device.end_frame();

            check_sdl(SDL_RenderPresent(renderer), "SDL_RenderPresent");
            ++frame;

            if (capture_path != nullptr && frame >= kCaptureFrame) {
                auto* shot = SDL_RenderReadPixels(renderer, nullptr);
                if (shot == nullptr) {
                    fail("SDL_RenderReadPixels");
                }
                check_sdl(SDL_SaveBMP(shot, capture_path), "SDL_SaveBMP");
                SDL_DestroySurface(shot);
                std::cout << "captured frame " << frame << " to " << capture_path << '\n';
                running = false;
            }
        }

        const auto& stats = device.stats();
        std::cout << "device stats: frames=" << stats.frames << " rects=" << stats.rects
                  << " outlines=" << stats.rect_outlines << " rounded=" << stats.rounded_rects
                  << " circles=" << stats.circles << " lines=" << stats.lines
                  << " texts=" << stats.texts << " textures=" << stats.texture_draws
                  << " alpha_uploads=" << stats.alpha_uploads << " clips=" << stats.clips
                  << " cpu_vertices=" << stats.generated_vertices << '\n';
    }
    catch (const std::exception& error) {
        std::cerr << "SDL backend probe failed: " << error.what() << '\n';
        exit_code = 1;
    }

    if (shape_texture != nullptr)
        SDL_DestroyTexture(shape_texture);
    if (state != nullptr)
        SDL_DestroyGPURenderState(state);
    if (fragment != nullptr && gpu_device != nullptr)
        SDL_ReleaseGPUShader(gpu_device, fragment);
    if (renderer != nullptr)
        SDL_DestroyRenderer(renderer);
    if (gpu_device != nullptr)
        SDL_DestroyGPUDevice(gpu_device);
    if (window != nullptr)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}
