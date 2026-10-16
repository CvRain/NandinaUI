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

#include <nandina/render/backends/sdl_device.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>

#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_router.hpp>
#include <nandina/app/shell_context.hpp>
#include <nandina/foundation/nandina_color.hpp>
#include <nandina/foundation/nandina_color_space.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/text/font_pipeline.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/primitives/pressable.hpp>
#include <showcase/components/sidebar.hpp>
#include <showcase/pages/anchors_page.hpp>
#include <showcase/pages/home_page.hpp>
#include <showcase/pages/label_page.hpp>

#include <memory>
#include <string_view>

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

    /// 交互回归的可核对结果。全部由**框架自己的状态**读出（Pressable 的 hover/press、
    /// Button 的 treatment 与解析后的圆角、Router 的当前页键），不看截图猜。
    struct InteractionReport {
        bool hovered = false;
        bool pressed = false;
        bool route_switched = false;
        bool selected_treatment = false;
        /// hover 前的基础圆角（用于记录观察，不作为断言）。
        float radius_before = 0.0F;
    };

    [[nodiscard]] auto find_button(nandina::scene::NanNode* node, const std::string_view text)
        -> nandina::widget::Button* {
        if (node == nullptr) {
            return nullptr;
        }
        if (auto* button = dynamic_cast<nandina::widget::Button*>(node->as_control());
            button != nullptr && button->text() == text)
        {
            return button;
        }
        for (std::size_t index = 0; index < node->child_count(); ++index) {
            if (auto* found = find_button(node->get_child(index), text); found != nullptr) {
                return found;
            }
        }
        return nullptr;
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

        nandina::render::SdlRenderDevice device(*renderer);
        // 真实字体管线：FontPipelineCache 直接接收本设备，所以字形图集（GlyphAtlasTexture）
        // 的创建、扩容与子矩形更新都走这个后端 —— 这正是探测记录里未验证的那条路径。
        auto fonts = nandina::text::FontPipelineCache(
            device,
            application.font_loader(),
            application.font_families()
        );

        nandina::scene::NanSceneTree tree;
        tree.set_font_context(fonts);
        tree.set_theme_manager(application.theme_manager());
        const auto default_pipeline = fonts.get({});
        if (!default_pipeline) {
            throw std::runtime_error(
                "cannot create the default text pipeline: " + default_pipeline.error().message
            );
        }
        tree.set_default_text_pipeline((*default_pipeline)->pipeline());

        auto overlay_host = nandina::scene::OverlayHost::create();
        auto router = nandina::app::NanRouter(
            application.graph(),
            application.theme_manager(),
            nullptr,
            {},
            &application.resources(),
            &application.font_loader(),
            &application.font_families(),
            &application.dispatcher(),
            &application.background_executor(),
            overlay_host.get()
        );
        // 路由表：Router 只有 configure() 之后才进入 route 模式，`start<Page>()` 才可用。
        // 三个 showcase 页面都注册，用 NANDINA_PROBE_PAGE 选择挂载哪一个 —— 消费者回归需要
        // 「每个页面都过一遍这个后端」，而不是只看一页。
        //
        // title / type 与 showcase/main_window.cpp **逐字一致**：外壳的导航条目文字直接来自
        // `RouteOptions::title`，分组来自 `type`。用英文标题会让 shell 截图与真实应用不同，
        // 也会让「按标题找按钮」的交互回归找不到目标（踩过一次）。
        const auto* page_env = std::getenv("NANDINA_PROBE_PAGE");
        const std::string_view page = page_env != nullptr ? page_env : "label";
        const auto routes = nandina::app::Routes {
            nandina::app::route<nandina::showcase::ShowcaseHomePage>(nandina::app::RouteOptions {
                .address = "home",
                .title = "首页",
                .type = "手册",
            }),
            nandina::app::route<nandina::showcase::AnchorsPage>(nandina::app::RouteOptions {
                .address = "anchors",
                .title = "锚点",
                .type = "手册",
            }),
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

        const bool shell_enabled = std::getenv("NANDINA_PROBE_SHELL") != nullptr;
        auto shell_scope = std::make_unique<nandina::reactive::ReactiveScope>(application.graph());
        if (shell_enabled) {
            nandina::app::ShellContext shell_context(
                application.graph(),
                *shell_scope,
                application.theme_manager(),
                &application.resources(),
                overlay_host.get(),
                nullptr,
                router.navigation(),
                outlet,
                routes,
                router.current_page()
            );
            const nandina::showcase::Sidebar sidebar(shell_context);
            auto shell_view = sidebar.build_shell();
            auto* shell_control = shell_view ? shell_view->as_control() : nullptr;
            if (shell_control == nullptr) {
                throw std::runtime_error("showcase sidebar did not build a control root");
            }
            auto shell_root =
                std::shared_ptr<nandina::scene::NanControl>(shell_view, shell_control);
            overlay_host->set_content(std::move(shell_root));
            tree.set_root(overlay_host);
        }
        else {
            tree.set_root(outlet);
        }

        auto started = false;
        if (page == "home") {
            started = router.start<nandina::showcase::ShowcaseHomePage>();
        }
        else if (page == "anchors") {
            started = router.start<nandina::showcase::AnchorsPage>();
        }
        else if (page == "label") {
            started = router.start<nandina::showcase::LabelPage>();
        }
        else {
            throw std::runtime_error("unknown NANDINA_PROBE_PAGE: " + std::string(page));
        }
        if (!started) {
            throw std::runtime_error("router.start failed for page: " + std::string(page));
        }

        // 可复跑的消费者回归：真实 Sidebar 中「锚点」导航按钮 hover → press → release，
        // 检查 Pressable 状态、shell transition 和 Router 路由结果。无需依赖人工鼠标时序。
        const bool inject_interaction = std::getenv("NANDINA_PROBE_INTERACTION") != nullptr;
        InteractionReport interaction {};
        if (inject_interaction && !shell_enabled) {
            throw std::runtime_error("NANDINA_PROBE_INTERACTION requires NANDINA_PROBE_SHELL=1");
        }

        const auto* capture_path = std::getenv("NANDINA_PROBE_CAPTURE");
        const auto frames_to_run = [] {
            const auto* value = std::getenv("NANDINA_PROBE_FRAMES");
            return value != nullptr ? std::max(std::atoi(value), 1) : 30;
        }();

        std::cout << "SDL " << SDL_GetVersion() << ", renderer="
                  << (SDL_GetRendererName(renderer) ? SDL_GetRendererName(renderer) : "unknown")
                  << ", page=" << page << '\n';
        // 能力摘要就是「这个后端与生产后端的差异清单」的可执行版本：消费者按这些查询选路。
        std::cout << "device capabilities: alpha_textures=" << device.supports_alpha_textures()
                  << " rounded_rect=" << device.supports_rounded_rect()
                  << " rounded_clip=" << device.supports_rounded_clip()
                  << " soft_shadows=" << device.supports_soft_shadows() << '\n';

        bool running = true;
        auto frame = 0;
        constexpr float kProbeDt = 1.0F / 60.0F;
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

            // 宿主契约（P4 的一手证据）：Router 的换页是**投递给 dispatcher** 的，宿主必须在
            // 每帧的 tasks 阶段把它取出来。`NanWindow` 在 `FramePhase::tasks` 里做同一件事
            // （nan_window.cpp:462）。最小宿主漏了这一步时，点击回调执行了、路由却不换 ——
            // 表现为「按了没反应」。
            (void)application.dispatcher().drain();
            if (shell_enabled) {
                // 另一条宿主契约：动画只在宿主推进时间时前进（`NanWindow` 的 process 阶段）。
                // 用**固定步长**而不是墙钟差：这个探测不锁帧（几千 fps），墙钟 dt 会让动画在
                // 模拟时间里几乎不动，交互回归就永远看不到过渡结束的状态。
                tree.process(kProbeDt);
            }

            // 真实页面：布局 → 绘制，全部走 Nandina 自己的路径。
            device.begin_frame();
            // 宿主清屏色：与 raylib 窗口的默认清屏色保持一致（白），否则跨后端对照会把
            // 「宿主选择」误读成「后端差异」。页面自己只画卡片/文本，最外层背景由宿主决定 ——
            // 这正是 P4 要说清楚的一件事（见 platform_backend_contract.md §2）。
            device.clear(color_rgba(0xffffffffU));
            const auto layout_started = std::chrono::steady_clock::now();
            (void)tree.layout_root(viewport);
            const auto layout_ended = std::chrono::steady_clock::now();

            if (inject_interaction && frame == 2) {
                auto* anchors_button = find_button(overlay_host.get(), "锚点");
                if (anchors_button == nullptr) {
                    throw std::runtime_error(
                        "could not find the Anchors navigation button in shell"
                    );
                }
                const auto bounds = anchors_button->global_bounds();
                const auto point = nandina::foundation::NanPoint(
                    (bounds.get_left() + bounds.get_right()) * 0.5F,
                    (bounds.get_top() + bounds.get_bottom()) * 0.5F
                );
                interaction.radius_before = anchors_button->resolved_style().container.radius;
                tree.dispatch_mouse_move(
                    nandina::scene::MouseMoveEvent {point, nandina::foundation::NanPoint::zero()}
                );
                interaction.hovered = anchors_button->hovered();
                tree.dispatch_mouse_button(
                    nandina::scene::MouseButtonEvent {
                        nandina::scene::MouseButtonEvent::Button::left,
                        nandina::scene::MouseButtonEvent::Action::press,
                        point,
                    }
                );
                interaction.pressed = anchors_button->pressed();
                tree.dispatch_mouse_button(
                    nandina::scene::MouseButtonEvent {
                        nandina::scene::MouseButtonEvent::Button::left,
                        nandina::scene::MouseButtonEvent::Action::release,
                        point,
                    }
                );
                // 换页是投递给 dispatcher 的：宿主在 tasks 阶段取出来（与 NanWindow 一致）。
                (void)application.dispatcher().drain();
                interaction.route_switched = router.current_page_key()
                    == nandina::app::page_key<nandina::showcase::AnchorsPage>();
                interaction.selected_treatment =
                    anchors_button->treatment() == nandina::theme::ButtonTreatment::filled;
            }
            else if (inject_interaction && frame >= 3 && frame <= 24 && frame % 3 == 0) {
                auto* button = find_button(overlay_host.get(), "锚点");
                std::cout << "  [anim] frame=" << frame << " radius="
                          << (button != nullptr ? button->resolved_style().container.radius : -1.0F)
                          << " state="
                          << (button != nullptr ? static_cast<int>(button->visual_state()) : -1)
                          << " pending=" << application.graph().has_pending_effects() << '\n';
            }

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

        if (inject_interaction) {
            std::cout << "interaction report: hovered=" << interaction.hovered
                      << " pressed=" << interaction.pressed
                      << " route_switched=" << interaction.route_switched
                      << " selected_treatment=" << interaction.selected_treatment << '\n';
            // 观察（不是断言）：侧边栏 hover 的圆角增益没有通过 `resolved_style().container.radius`
            // 反映出来。这条链（signal → computed → bind → tween）与后端无关，属于 shell/动画层的
            // 独立问题，不作为 SDL 后端差异记账；见探测记录 §2.3.4。
            std::cout << "  note: hover radius gain not observed through resolved_style "
                         "(backend-independent)\n";
        }

        const auto& stats = device.stats();
        std::cout << "device stats: frames=" << stats.frames << " rects=" << stats.rects
                  << " outlines=" << stats.rect_outlines << " rounded=" << stats.rounded_rects
                  << " rounded_outlines=" << stats.rounded_outlines << " circles=" << stats.circles
                  << " lines=" << stats.lines << " texts=" << stats.texts
                  << " textures=" << stats.texture_draws << " alpha_uploads=" << stats.alpha_uploads
                  << " clips=" << stats.clips << " cpu_vertices=" << stats.generated_vertices
                  << '\n';

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
