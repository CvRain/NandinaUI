module;

#include <SDL3/SDL.h>
#include <format>
#include <stdexcept>
#include <thorvg-1/thorvg.h>

export module nandina.app.application;

import nandina.log;

export namespace nandina {
    class NanApplication {
    public:
        NanApplication();

        NanApplication(const NanApplication &) = delete;

        NanApplication(NanApplication &&) = delete;

        NanApplication &operator=(const NanApplication &) = delete;

        NanApplication &operator=(NanApplication &&) = delete;

        ~NanApplication();
    };
} // namespace nandina


namespace nandina {
    NanApplication::NanApplication() {
        nandina::log::init("nandina", nandina::log::Level::Debug);

        auto log = nandina::log::get("NanApplication::NanApplication");

        log.debug("Initializing application...");

        // 初始化SDL
        if (const auto &sdl_init_result = SDL_Init(SDL_INIT_VIDEO); not sdl_init_result) {
            const auto &sdl_init_error = SDL_GetError();
            log.error("SDL initialization failed: {}", sdl_init_error);
            throw std::runtime_error(sdl_init_error);
        }

        // 初始化ThorVG
        if (const auto &tvg_init_result = tvg::Initializer::init(4); tvg_init_result != tvg::Result::Success) {
            log.error("TVG initialization failed");
            throw std::runtime_error("TVG initialization failed");
        }

        log.debug("Finished initialization");
    }

    NanApplication::~NanApplication() {
        tvg::Initializer::term();
        SDL_Quit();
        log::shutdown();
    }
}; // namespace nandina
