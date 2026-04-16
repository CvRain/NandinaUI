module;

#include <SDL3/SDL.h>
#include <format>
#include <stdexcept>
#include <thorvg-1/thorvg.h>

export module nandina.app.application;

export namespace nandina {
    class NanApplication {
    public:
        NanApplication();

        NanApplication(const NanApplication &) = delete;

        NanApplication(NanApplication &&) = delete;

        NanApplication& operator=(const NanApplication &) = delete;

        NanApplication& operator=(NanApplication &&) = delete;

        ~NanApplication();
    };
} // namespace nandina


namespace nandina {
    NanApplication::NanApplication() {
        const auto error_label{"[Nandina] NanApplication::NanApplication"};

        // 初始化SDL
        if (const auto &sdl_init_result = SDL_Init(SDL_INIT_VIDEO); not sdl_init_result) {
            const auto &sdl_init_error = SDL_GetError();
            throw std::runtime_error(std::format("{}: SDL_INIT_FAILED: {}", error_label, sdl_init_error));
        }

        // 初始化ThorVG
        if (const auto &tvg_init_result = tvg::Initializer::init(4); tvg_init_result != tvg::Result::Success) {
            throw std::runtime_error(std::format("{}: TVG_INIT_FAILED", error_label));
        }
    }

    NanApplication::~NanApplication() {
        tvg::Initializer::term();
        SDL_Quit();
    }
}; // namespace nandina
