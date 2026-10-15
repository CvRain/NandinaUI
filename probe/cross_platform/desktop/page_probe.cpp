//
// probe/cross_platform/desktop — 「真实页面走 SDL 设备」探测
//
// 与同目录的 CMake 探测（`main.cpp` + `sdl_render_device.*`）分工不同：
//
//   * CMake 那份只依赖 `IRenderDevice` 头文件 + 三个 foundation TU，手写图元调用，
//     回答「这个设备契约能不能在 SDL 上独立实现」；
//   * 本文件链接 libnandina，挂载**真实的 showcase 页面**（`LabelPage`），走真实主题、
//     真实 HarfBuzz 整形与真实 `GlyphAtlasTexture`，然后用 `NanSceneTree::draw(device)`
//     把整棵树画到 SDL 上。它回答的是探测记录 §4 的第 2 件前置条件：「用真实页面负载，
//     而不是 SDK 示例」。
//
// 为什么不用 `NanWindow`：`NanWindow` 同时持有窗口、设备、输入轮询与帧驱动（P4 的待办），
// 它是 raylib 窗口。这里手写一个最小宿主：`NanApplication`（资源/字体/主题/调度）+
// `NanRouter` + `NanSceneTree` + 本 SDL 设备。宿主需要的东西越少，P4 的契约就越清楚。
//
// 运行：`NANDINA_PROBE_CAPTURE=<path.bmp> NANDINA_PROBE_FRAMES=30 ./nandina_sdl_page_probe`

#include "sdl_render_device.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>

#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_router.hpp>
#include <nandina/foundation/nandina_color.hpp>
#include <nandina/foundation/nandina_color_space.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/text/font_pipeline.hpp>
#include <showcase/pages/label_page.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using nandina::foundation::NanColor;
    using nandina::foundation::NanHexRgb;
    using nandina::foundation::NanSize;

    void check_sdl(const bool ok, const std::string& operation) {
        if (!ok) {
            throw std::runtime_error(operation + ": " + SDL_GetError());
        }
    }

    auto color_rgba(const std::uint32_t packed) -> NanColor {
        return NanColor::from(NanHexRgb::from_uint32(packed));
    }
} // namespace

int main() {
    SDL_Window* window = nullptr;
    SDL_GPUDevice* gpu_device = nullptr;
    SDL_Renderer* renderer = nullptr;
    int exit_code = 0;

    try {
        check_sdl(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
        gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
        if (gpu_device == nullptr) {
            throw std::runtime_error(
                std::string("SDL_CreateGPUDevice(vulkan, SPIR-V): ") + SDL_GetError()
            );
        }
        window = SDL_CreateWindow(
            "Nandina SDL page probe",
            1200,
            800,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
        );
        if (window == nullptr) {
            throw std::runtime_error(std::string("SDL_CreateWindow: ") + SDL_GetError());
        }
        renderer = SDL_CreateGPURenderer(gpu_device, window);
        if (renderer == nullptr) {
            throw std::runtime_error(std::string("SDL_CreateGPURenderer: ") + SDL_GetError());
        }

        // ---- 最小宿主：应用服务 + 设备 + 字体上下文 + 路由 ----
        auto application = nandina::app::NanApplication(
            nandina::app::NanApplicationConfig::for_process("com.nandina.probe.sdl_page")
        );

        nandina::probe::SdlRenderDevice device(*renderer);
        // 真实字体管线：FontPipelineCache 直接接收本设备，所以字形图集（GlyphAtlasTexture）
        // 的创建、扩容与子矩形更新都走这个后端 —— 这正是探测记录里未验证的那条路径。
        auto fonts = nandina::text::FontPipelineCache(
            device,
            application.font_loader(),
            application.font_families()
        );

        nandina::scene::NanSceneTree tree;
        tree.set_font_context(fonts);
        const auto default_pipeline = fonts.get({});
        if (!default_pipeline) {
            throw std::runtime_error(
                "cannot create the default text pipeline: " + default_pipeline.error().message
            );
        }
        tree.set_default_text_pipeline((*default_pipeline)->pipeline());

        auto router = nandina::app::NanRouter(
            application.graph(),
            application.theme_manager(),
            nullptr,
            {},
            &application.resources(),
            &application.font_loader(),
            &application.font_families(),
            &application.dispatcher(),
            &application.background_executor()
        );
        // 路由表：Router 只有 configure() 之后才进入 route 模式，`start<Page>()` 才可用。
        // 这里只注册 Label 一页 —— 探测要的是「真实页面」，不是完整的导航壳。
        const auto routes = nandina::app::Routes {
            nandina::app::route<nandina::showcase::LabelPage>(nandina::app::RouteOptions {
                .address = "form/label",
                .title = "Label",
                .type = "表单",
            }),
        };
        if (const auto configured = router.configure(routes); !configured) {
            throw std::runtime_error("router.configure failed");
        }

        auto outlet = router.outlet();
        if (outlet == nullptr) {
            throw std::runtime_error("router has no outlet");
        }
        tree.set_root(outlet);

        const auto started = router.start<nandina::showcase::LabelPage>();
        if (!started) {
            throw std::runtime_error("router.start<LabelPage> failed");
        }

        const auto* capture_path = std::getenv("NANDINA_PROBE_CAPTURE");
        const auto frames_to_run = [] {
            const auto* value = std::getenv("NANDINA_PROBE_FRAMES");
            return value != nullptr ? std::max(std::atoi(value), 1) : 30;
        }();

        std::cout << "SDL " << SDL_GetVersion() << ", renderer="
                  << (SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "unknown")
                  << ", page=showcase::LabelPage" << '\n';

        bool running = true;
        auto frame = 0;
        // 帧成本：layout + draw + end_frame 的 CPU 时间。真实的页面负载对比需要它
        // （探测记录 §4 第 2 件事），而 draw call 数量由设备统计给出。
        auto layout_ms = 0.0;
        auto draw_ms = 0.0;
        auto measured = 0;
        while (running) {
            SDL_Event event {};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                else if (
                    event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat
                    && event.key.key == SDLK_ESCAPE
                )
                {
                    running = false;
                }
            }

            int pixel_width = 0;
            int pixel_height = 0;
            check_sdl(
                SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height),
                "SDL_GetWindowSizeInPixels"
            );
            const auto viewport =
                NanSize(static_cast<float>(pixel_width), static_cast<float>(pixel_height));

            // 真实页面：布局 → 绘制，全部走 Nandina 自己的路径。
            device.begin_frame();
            device.clear(color_rgba(0xf5f6f8ffU));
            const auto layout_started = std::chrono::steady_clock::now();
            (void)tree.layout_root(viewport);
            const auto layout_ended = std::chrono::steady_clock::now();
            tree.draw(device);
            device.end_frame();
            const auto draw_ended = std::chrono::steady_clock::now();
            if (frame >= 5) {
                layout_ms +=
                    std::chrono::duration<double, std::milli>(layout_ended - layout_started)
                        .count();
                draw_ms +=
                    std::chrono::duration<double, std::milli>(draw_ended - layout_ended).count();
                ++measured;
            }
            check_sdl(SDL_RenderPresent(renderer), "SDL_RenderPresent");

            ++frame;
            if (capture_path != nullptr && frame >= frames_to_run) {
                auto* shot = SDL_RenderReadPixels(renderer, nullptr);
                if (shot == nullptr) {
                    throw std::runtime_error(
                        std::string("SDL_RenderReadPixels: ") + SDL_GetError()
                    );
                }
                check_sdl(SDL_SaveBMP(shot, capture_path), "SDL_SaveBMP");
                SDL_DestroySurface(shot);
                std::cout << "captured frame " << frame << " to " << capture_path << '\n';
                running = false;
            }
        }

        if (measured > 0) {
            std::cout << "frame cost over " << measured
                      << " frames: layout=" << (layout_ms / measured)
                      << " ms, draw=" << (draw_ms / measured)
                      << " ms, total=" << ((layout_ms + draw_ms) / measured) << " ms\n";
        }

        const auto& stats = device.stats();
        std::cout << "device stats: frames=" << stats.frames << " rects=" << stats.rects
                  << " outlines=" << stats.rect_outlines << " rounded=" << stats.rounded_rects
                  << " circles=" << stats.circles << " lines=" << stats.lines
                  << " texts=" << stats.texts << " textures=" << stats.texture_draws
                  << " alpha_uploads=" << stats.alpha_uploads << " clips=" << stats.clips
                  << " cpu_vertices=" << stats.generated_vertices << '\n';

        tree.set_root(nullptr);
    }
    catch (const std::exception& error) {
        std::cerr << "SDL page probe failed: " << error.what() << '\n';
        exit_code = 1;
    }

    if (renderer != nullptr)
        SDL_DestroyRenderer(renderer);
    if (gpu_device != nullptr)
        SDL_DestroyGPUDevice(gpu_device);
    if (window != nullptr)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return exit_code;
}
