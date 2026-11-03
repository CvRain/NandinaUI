//
// app/detail/sdl_text_input_service — SDL3 implementation of the scene text input protocol.
//
// SDL3 desktop backends deliver IME composition through SDL_EVENT_TEXT_EDITING; this service
// owns the session identity SDL does not carry, forwards composition/commit/cancel to the
// scene sink, and reports only the capabilities the desktop SDL path actually provides.
//

#ifndef NANDINA_EXPERIMENT_APP_DETAIL_SDL_TEXT_INPUT_SERVICE_HPP
#define NANDINA_EXPERIMENT_APP_DETAIL_SDL_TEXT_INPUT_SERVICE_HPP

#include "../../scene/text_input_service.hpp"

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include <SDL3/SDL_keyboard.h>

namespace nandina::app::detail
{
    class SdlTextInputService final: public scene::ITextInputService {
    public:
        using StartTextInput = decltype(&SDL_StartTextInputWithProperties);

        /// The internal call seam lets tests observe properties consumed by SDL without
        /// depending on private SDL window fields or a particular desktop input method.
        explicit SdlTextInputService(
            SDL_Window* window,
            StartTextInput start_text_input = SDL_StartTextInputWithProperties
        ) noexcept;
        ~SdlTextInputService() override;

        SdlTextInputService(const SdlTextInputService&) = delete;
        auto operator=(const SdlTextInputService&) -> SdlTextInputService& = delete;

        [[nodiscard]] auto capabilities() const -> scene::TextInputCapabilities override;

        auto begin_session(const scene::TextInputBeginRequest& request)
            -> scene::TextInputRequestResult override;
        auto update_caret_area(scene::TextInputSession session, const scene::TextInputArea& area)
            -> scene::TextInputRequestResult override;
        auto request_keyboard_visibility(scene::TextInputSession session, bool visible)
            -> scene::TextInputRequestResult override;
        void
        end_session(scene::TextInputSession session, scene::TextInputEndReason reason) override;

        void set_sink(scene::ITextInputSessionSink* sink) noexcept override;
        void dispatch(const scene::TextInputPlatformEvent& event) override;

    private:
        SDL_Window* window_ = nullptr;
        StartTextInput start_text_input_;
        scene::ITextInputSessionSink* sink_ = nullptr;
        scene::TextInputSession active_session_ {};
    };
} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND

#endif // NANDINA_EXPERIMENT_APP_DETAIL_SDL_TEXT_INPUT_SERVICE_HPP
