#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    [[noreturn]] void fail(const std::string& operation) {
        throw std::runtime_error(operation + ": " + SDL_GetError());
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

    void check_sdl(const bool ok, const std::string& operation) {
        if (!ok) {
            fail(operation);
        }
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
        default:
            return "Unmapped";
        }
    }

    auto make_alpha_atlas() -> std::array<Uint32, 64 * 64> {
        std::array<Uint32, 64 * 64> pixels {};
        for (int y = 8; y < 56; ++y) {
            for (int x = 8; x < 56; ++x) {
                const bool stem = x >= 16 && x < 22;
                const bool diagonal = x >= 22 && x < 46 && (x - 22) / 2 == (y - 8) / 4;
                const bool right_stem = x >= 42 && x < 48;
                if (stem || diagonal || right_stem) {
                    pixels[static_cast<std::size_t>(y) * 64 + x] = 0xffffffffU;
                }
            }
        }
        return pixels;
    }

} // namespace

int main() {
    SDL_Window* window = nullptr;
    SDL_GPUDevice* device = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GPUShader* fragment = nullptr;
    SDL_GPURenderState* state = nullptr;
    SDL_Texture* atlas = nullptr;
    SDL_Texture* shape_texture = nullptr;
    int exit_code = 0;

    try {
        check_sdl(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
        device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
        if (device == nullptr) {
            fail("SDL_CreateGPUDevice(vulkan, SPIR-V)");
        }
        window = SDL_CreateWindow(
            "Nandina SDL GPU feasibility probe",
            800,
            600,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
        );
        if (window == nullptr) {
            fail("SDL_CreateWindow");
        }
        renderer = SDL_CreateGPURenderer(device, window);
        if (renderer == nullptr) {
            fail("SDL_CreateGPURenderer");
        }

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
        fragment = SDL_CreateGPUShader(device, &shader_info);
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

        atlas = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, 64, 64
        );
        if (atlas == nullptr) {
            fail("SDL_CreateTexture(alpha glyph atlas)");
        }
        const auto atlas_pixels = make_alpha_atlas();
        check_sdl(
            SDL_UpdateTexture(atlas, nullptr, atlas_pixels.data(), 64 * sizeof(Uint32)),
            "SDL_UpdateTexture(alpha glyph atlas)"
        );
        check_sdl(SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND), "SDL_SetTextureBlendMode");

        shape_texture = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, 1, 1
        );
        if (shape_texture == nullptr) {
            fail("SDL_CreateTexture(SDF source)");
        }
        constexpr Uint32 white_pixel = 0xffffffffU;
        check_sdl(
            SDL_UpdateTexture(shape_texture, nullptr, &white_pixel, sizeof(white_pixel)),
            "SDL_UpdateTexture(SDF source)"
        );
        check_sdl(SDL_SetTextureBlendMode(shape_texture, SDL_BLENDMODE_BLEND),
                  "SDL_SetTextureBlendMode(SDF source)");
        check_sdl(SDL_SetTextureColorMod(shape_texture, 51, 184, 140),
                  "SDL_SetTextureColorMod(SDF source)");

        std::cout << "SDL " << SDL_GetVersion() << ", renderer="
                  << (SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "unknown")
                  << ", renderer-device=" << SDL_GetGPURendererDevice(renderer)
                  << ", shader-formats=0x" << std::hex << SDL_GetGPUShaderFormats(device)
                  << std::dec << ", DPI=" << SDL_GetWindowDisplayScale(window) << '\n';
        std::cout << "SPACE/arrow keys log normalized framework key names; resize to inspect pixel size.\n";

        bool running = true;
        while (running) {
            SDL_Event event {};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                    std::cout << "keycode=" << event.key.key << " scancode="
                              << static_cast<int>(event.key.scancode) << " framework="
                              << normalized_key(event.key.key) << '\n';
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                }
                else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                    int width = 0;
                    int height = 0;
                    SDL_GetWindowSizeInPixels(window, &width, &height);
                    std::cout << "pixel-size=" << width << 'x' << height << '\n';
                }
            }

            check_sdl(SDL_SetRenderDrawColor(renderer, 245, 246, 248, 255), "SDL_SetRenderDrawColor");
            check_sdl(SDL_RenderClear(renderer), "SDL_RenderClear");
            check_sdl(SDL_SetGPURenderState(renderer, state), "SDL_SetGPURenderState(state)");
            const SDL_Rect clip {100, 100, 320, 260};
            check_sdl(SDL_SetRenderClipRect(renderer, &clip), "SDL_SetRenderClipRect");
            const SDL_FRect shape_destination {120.0F, 120.0F, 400.0F, 260.0F};
            check_sdl(SDL_RenderTexture(renderer, shape_texture, nullptr, &shape_destination),
                      "SDL_RenderTexture with custom SDF fragment shader");
            check_sdl(SDL_SetRenderClipRect(renderer, nullptr), "SDL_SetRenderClipRect(clear)");
            check_sdl(SDL_SetGPURenderState(renderer, nullptr), "SDL_SetGPURenderState(clear)");

            const SDL_FRect atlas_destination {560.0F, 160.0F, 128.0F, 128.0F};
            check_sdl(SDL_RenderTexture(renderer, atlas, nullptr, &atlas_destination),
                      "SDL_RenderTexture(alpha glyph atlas)");
            check_sdl(SDL_RenderPresent(renderer), "SDL_RenderPresent");
        }
    }
    catch (const std::exception& error) {
        std::cerr << "SDL GPU probe failed: " << error.what() << '\n';
        exit_code = 1;
    }

    if (shape_texture != nullptr) SDL_DestroyTexture(shape_texture);
    if (atlas != nullptr) SDL_DestroyTexture(atlas);
    if (state != nullptr) SDL_DestroyGPURenderState(state);
    if (fragment != nullptr && device != nullptr) SDL_ReleaseGPUShader(device, fragment);
    if (renderer != nullptr) SDL_DestroyRenderer(renderer);
    if (device != nullptr) SDL_DestroyGPUDevice(device);
    if (window != nullptr) SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}
