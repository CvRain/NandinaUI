#include "sdl_text_input_service.hpp"

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include <SDL3/SDL.h>

    #include <cmath>
    #include <limits>

namespace nandina::app::detail
{
    SdlTextInputService::SdlTextInputService(SDL_Window* window) noexcept: window_(window) {}

    SdlTextInputService::~SdlTextInputService() {
        if (window_ != nullptr && active_session_) {
            SDL_StopTextInput(window_);
        }
    }

    auto SdlTextInputService::capabilities() const -> scene::TextInputCapabilities {
        // SDL accepts caret areas in window coordinates. Desktop keyboard visibility and
        // occlusion feedback remain unsupported independently of this capability.
        return scene::TextInputCapabilities::composition | scene::TextInputCapabilities::input_area;
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
        const scene::TextInputSession session,
        const scene::TextInputArea& area
    ) -> scene::TextInputRequestResult {
        if (window_ == nullptr || !active_session_ || session != active_session_) {
            return scene::TextInputRequestResult::unavailable;
        }
        // Round outward so a fractional caret is never truncated to an empty SDL rectangle.
        const double left = std::floor(area.caret.get_left());
        const double top = std::floor(area.caret.get_top());
        const double right = std::ceil(area.caret.get_right());
        const double bottom = std::ceil(area.caret.get_bottom());
        constexpr double min = std::numeric_limits<int>::min();
        constexpr double max = std::numeric_limits<int>::max();
        if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right)
            || !std::isfinite(bottom) || area.caret.get_width() <= 0.0F
            || area.caret.get_height() <= 0.0F || left < min || top < min || right > max
            || bottom > max || right - left > max || bottom - top > max)
        {
            return scene::TextInputRequestResult::rejected;
        }
        const SDL_Rect rect {
            static_cast<int>(left),
            static_cast<int>(top),
            static_cast<int>(right - left),
            static_cast<int>(bottom - top)
        };
        return SDL_SetTextInputArea(window_, &rect, 0) ? scene::TextInputRequestResult::accepted
                                                       : scene::TextInputRequestResult::rejected;
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
