#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/widget/label.hpp>

#include <SDL3/SDL.h>

#include <cstdio>

int main() {
    nandina::app::NanApplication application;
    nandina::app::NanWindow window(application, {});
    window.set_content(
        nandina::widget::Label::create(application.graph(), "SDK consumer", application.theme())
    );
    const auto backend = window.platform_name();
    const auto decoder = application.image_decoder();
    const auto version = SDL_GetVersion();
    std::printf(
        "backend=%.*s sdl=%d.%d.%d decoder=%d\n",
        static_cast<int>(backend.size()),
        backend.data(),
        SDL_VERSIONNUM_MAJOR(version),
        SDL_VERSIONNUM_MINOR(version),
        SDL_VERSIONNUM_MICRO(version),
        decoder != nullptr
    );
    return backend == "sdl3" && decoder != nullptr ? 0 : 1;
}
