#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

namespace
{
    struct AppState {
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* atlas = nullptr;
        bool alternate = false;
        int pixel_width = 0;
        int pixel_height = 0;
        /// 自证：跑到 kReportFrame 帧时读回帧缓冲，统计「非背景像素」与目标色的出现，
        /// 然后把结果 fetch 回服务器 —— 这样一次浏览器运行会留下可核对的服务端记录，
        /// 而不是只有一张「可能停在加载壳上」的截图。
        int frames = 0;
        int reports = 0;
    };

    /// 第一次上报要早：`firefox --headless --screenshot` 截完就可能退出进程。
    constexpr int kFirstReportFrame = 2;
    /// 之后每这么多帧再报一次，让存活更久的页面留下更完整的记录。
    constexpr int kRepeatReportFrames = 30;

    /// 把一条结果发回服务器；服务端日志即证据。**失败路径也要报** —— 否则「黑屏」永远
    /// 分不清「wasm 没加载完」「没有 WebGL2」还是「渲染报错」。非 wasm 构建下是空实现。
    auto report_to_server(const char* stage, const char* detail) -> void {
#ifdef __EMSCRIPTEN__
        EM_ASM(
            {
                fetch(
                    '/probe-report?stage=' + encodeURIComponent(UTF8ToString($0)) + '&detail='
                    + encodeURIComponent(UTF8ToString($1))
                )
                    .catch(function() {});
            },
            stage,
            detail
        );
#else
        (void)stage;
        (void)detail;
#endif
    }

    auto append_triangle(
        std::vector<SDL_Vertex>& vertices,
        const SDL_FPoint a,
        const SDL_FPoint b,
        const SDL_FPoint c,
        const SDL_FColor color
    ) -> void {
        vertices.push_back({a, color, {0.0F, 0.0F}});
        vertices.push_back({b, color, {0.0F, 0.0F}});
        vertices.push_back({c, color, {0.0F, 0.0F}});
    }

    auto rounded_rect_mesh(const SDL_FColor color) -> std::vector<SDL_Vertex> {
        constexpr SDL_FPoint center {260.0F, 220.0F};
        constexpr float radius = 32.0F;
        constexpr std::array<SDL_FPoint, 4> corners {
            {{380.0F, 140.0F}, {380.0F, 300.0F}, {140.0F, 300.0F}, {140.0F, 140.0F}}
        };
        std::vector<SDL_Vertex> mesh;
        mesh.reserve(4 * 8 * 3);
        for (int corner = 0; corner < 4; ++corner) {
            const float base = static_cast<float>(corner) * 1.57079632679F;
            for (int step = 0; step < 8; ++step) {
                const float a0 = base + static_cast<float>(step) * 1.57079632679F / 8.0F;
                const float a1 = base + static_cast<float>(step + 1) * 1.57079632679F / 8.0F;
                const auto corner_center = corners[static_cast<std::size_t>(corner)];
                const SDL_FPoint p0 {
                    corner_center.x
                        + (corner == 0 || corner == 3 ? -radius : radius) * SDL_cosf(a0),
                    corner_center.y + (corner < 2 ? -radius : radius) * SDL_sinf(a0)
                };
                const SDL_FPoint p1 {
                    corner_center.x
                        + (corner == 0 || corner == 3 ? -radius : radius) * SDL_cosf(a1),
                    corner_center.y + (corner < 2 ? -radius : radius) * SDL_sinf(a1)
                };
                append_triangle(mesh, center, p0, p1, color);
            }
        }
        return mesh;
    }

    auto make_atlas() -> std::array<Uint32, 16 * 16> {
        std::array<Uint32, 16 * 16> pixels {};
        constexpr std::array<Uint8, 7>
            glyph {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001};
        for (std::size_t y = 0; y < glyph.size(); ++y) {
            for (int x = 0; x < 5; ++x) {
                if ((glyph[y] & (1U << (4 - x))) != 0) {
                    pixels[(y + 4) * 16 + static_cast<std::size_t>(x + 5)] = 0xffffffffU;
                }
            }
        }
        return pixels;
    }
} // namespace

SDL_AppResult SDL_AppInit(void** appstate, int, char**) {
    auto* state = new AppState;
    *appstate = state;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        report_to_server("init-failed", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    state->window = SDL_CreateWindow(
        "Nandina SDL WebGL feasibility probe",
        640,
        480,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    if (state->window == nullptr) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    state->renderer = SDL_CreateRenderer(state->window, nullptr);
    if (state->renderer == nullptr) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        report_to_server("renderer-failed", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    report_to_server("renderer-created", SDL_GetRendererName(state->renderer));
    SDL_SetRenderVSync(state->renderer, 1);
    state->atlas = SDL_CreateTexture(
        state->renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        16,
        16
    );
    if (state->atlas == nullptr) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    const auto pixels = make_atlas();
    if (!SDL_UpdateTexture(state->atlas, nullptr, pixels.data(), 16 * sizeof(Uint32))) {
        SDL_Log("SDL_UpdateTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetTextureBlendMode(state->atlas, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSizeInPixels(state->window, &state->pixel_width, &state->pixel_height);
    SDL_Log(
        "web-init renderer=%s pixel-size=%dx%d",
        SDL_GetRendererName(state->renderer),
        state->pixel_width,
        state->pixel_height
    );
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    auto* state = static_cast<AppState*>(appstate);
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        if (event->key.key == SDLK_SPACE) {
            state->alternate = !state->alternate;
        }
        SDL_Log(
            "web-key keycode=%d scancode=%d",
            event->key.key,
            static_cast<int>(event->key.scancode)
        );
    }
    if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        SDL_GetWindowSizeInPixels(state->window, &state->pixel_width, &state->pixel_height);
        SDL_Log("web-resize pixel-size=%dx%d", state->pixel_width, state->pixel_height);
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    auto* state = static_cast<AppState*>(appstate);
    if (!SDL_SetRenderDrawColor(state->renderer, 245, 246, 248, 255)
        || !SDL_RenderClear(state->renderer))
    {
        SDL_Log("clear failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const SDL_Rect clip {150, 130, 220, 180};
    SDL_SetRenderClipRect(state->renderer, &clip);
    const auto color = state->alternate ? SDL_FColor {0.91F, 0.35F, 0.25F, 1.0F}
                                        : SDL_FColor {0.20F, 0.72F, 0.55F, 1.0F};
    const auto mesh = rounded_rect_mesh(color);
    if (!SDL_RenderGeometry(
            state->renderer,
            nullptr,
            mesh.data(),
            static_cast<int>(mesh.size()),
            nullptr,
            0
        ))
    {
        SDL_Log("SDL_RenderGeometry failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderClipRect(state->renderer, nullptr);

    const SDL_FRect glyph_rect {430.0F, 150.0F, 160.0F, 160.0F};
    if (!SDL_RenderTexture(state->renderer, state->atlas, nullptr, &glyph_rect)) {
        SDL_Log("SDL_RenderTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_RenderDebugTextFormat(
        state->renderer,
        24.0F,
        360.0F,
        "SDL WebGL probe: SPACE changes color; resize logs pixel size"
    );

    // ---- 自证：读回帧缓冲，统计非背景像素与目标色的出现 ----
    ++state->frames;
    if (state->frames >= kFirstReportFrame
        && (state->reports == 0
            || state->frames >= kFirstReportFrame + state->reports * kRepeatReportFrames))
    {
        ++state->reports;
        int ink = 0;
        int teal = 0;
        int total = 0;
        if (auto* shot = SDL_RenderReadPixels(state->renderer, nullptr); shot != nullptr) {
            total = shot->w * shot->h;
            if (auto* pixels = static_cast<const std::uint8_t*>(shot->pixels);
                pixels != nullptr && shot->format == SDL_PIXELFORMAT_RGBA32)
            {
                for (int index = 0; index < total; ++index) {
                    const auto r = pixels[index * 4 + 0];
                    const auto g = pixels[index * 4 + 1];
                    const auto b = pixels[index * 4 + 2];
                    // 背景是 245,246,248；任何明显偏离它的像素都是「画出来的东西」。
                    if (std::abs(int(r) - 245) > 8 || std::abs(int(g) - 246) > 8
                        || std::abs(int(b) - 248) > 8)
                    {
                        ++ink;
                    }
                    // 圆角矩形的主色（teal 0.20,0.72,0.55 → 约 51,184,140）。
                    if (r > 30 && r < 90 && g > 150 && g < 220 && b > 110 && b < 180) {
                        ++teal;
                    }
                }
            }
            SDL_RenderDebugTextFormat(
                state->renderer,
                24.0F,
                376.0F,
                "frame %d ink %d/%d teal %d",
                state->frames,
                ink,
                total,
                teal
            );
            SDL_DestroySurface(shot);
        }
#ifdef __EMSCRIPTEN__
        // 服务端能看到这一行请求（查询串即证据）；返回什么不影响探测。
        EM_ASM(
            {
                fetch(
                    '/probe-report?stage=rendered&frames=' + $0 + '&ink=' + $1 + '&teal=' + $2
                    + '&total=' + $3
                )
                    .catch(function() {});
            },
            state->frames,
            ink,
            teal,
            total
        );
#endif
        SDL_Log(
            "web probe report: frames=%d ink=%d teal=%d total=%d",
            state->frames,
            ink,
            teal,
            total
        );
    }

    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult) {
    auto* state = static_cast<AppState*>(appstate);
    if (state != nullptr) {
        if (state->atlas != nullptr)
            SDL_DestroyTexture(state->atlas);
        if (state->renderer != nullptr)
            SDL_DestroyRenderer(state->renderer);
        if (state->window != nullptr)
            SDL_DestroyWindow(state->window);
        delete state;
    }
}
