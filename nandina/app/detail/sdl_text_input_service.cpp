#include "sdl_text_input_service.hpp"

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include <SDL3/SDL.h>

namespace nandina::app::detail
{
    SdlTextInputService::SdlTextInputService(SDL_Window* window) noexcept: window_(window) {}

    SdlTextInputService::~SdlTextInputService() {
        if (window_ != nullptr && active_session_) {
            SDL_StopTextInput(window_);
        }
    }

    auto SdlTextInputService::capabilities() const -> scene::TextInputCapabilities {
        // Desktop SDL exposes IME composition; soft keyboard, caret-area and occlusion
        // feedback are not available on the desktop paths this framework ships.
        return scene::TextInputCapabilities::composition;
    }

    auto SdlTextInputService::begin_session(const scene::TextInputBeginRequest& request)
        -> scene::TextInputRequestResult {
        if (window_ == nullptr) {
            return scene::TextInputRequestResult::unavailable;
        }
        if (!request.session) {
            return scene::TextInputRequestResult::rejected;
        }
        if (!SDL_StartTextInput(window_)) {
            return scene::TextInputRequestResult::unavailable;
        }
        active_session_ = request.session;
        return scene::TextInputRequestResult::accepted;
    }

    auto SdlTextInputService::update_caret_area(
        const scene::TextInputSession /*session*/,
        const scene::TextInputArea& /*area*/
    ) -> scene::TextInputRequestResult {
        return scene::TextInputRequestResult::unsupported;
    }

    auto SdlTextInputService::request_keyboard_visibility(
        const scene::TextInputSession /*session*/,
        const bool /*visible*/
    ) -> scene::TextInputRequestResult {
        return scene::TextInputRequestResult::unsupported;
    }

    void SdlTextInputService::end_session(
        const scene::TextInputSession session,
        const scene::TextInputEndReason /*reason*/
    ) {
        if (window_ == nullptr || !active_session_ || session != active_session_) {
            return;
        }
        SDL_StopTextInput(window_);
        active_session_ = {};
    }

    void SdlTextInputService::set_sink(scene::ITextInputSessionSink* sink) noexcept {
        sink_ = sink;
    }

    void SdlTextInputService::dispatch(const scene::TextInputPlatformEvent& event) {
        if (sink_ == nullptr || !active_session_) {
            return;
        }
        switch (event.type) {
            case scene::TextInputEventType::composition_updated:
                sink_->on_composition_updated(
                    active_session_,
                    event.text,
                    event.selection_start,
                    event.selection_end
                );
                break;
            case scene::TextInputEventType::composition_committed:
                sink_->on_text_committed(active_session_, event.text);
                break;
            case scene::TextInputEventType::composition_cancelled:
                sink_->on_composition_cancelled(active_session_);
                break;
            case scene::TextInputEventType::keyboard_visibility_changed:
                sink_->on_keyboard_visibility_changed(active_session_, event.visibility);
                break;
        }
    }
} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND
