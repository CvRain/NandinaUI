#include "sdl_text_input_service.hpp"

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include <SDL3/SDL.h>

    #include <cmath>
    #include <limits>

namespace nandina::app::detail
{
    SdlTextInputService::SdlTextInputService(
        SDL_Window* window,
        const StartTextInput start_text_input
    ) noexcept:
        window_(window),
        start_text_input_(start_text_input) {}

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
        if (start_text_input_ == nullptr) {
            return scene::TextInputRequestResult::unavailable;
        }
        SDL_TextInputType type = SDL_TEXTINPUT_TYPE_TEXT;
        switch (request.purpose) {
            case scene::TextInputPurpose::text:
                type = SDL_TEXTINPUT_TYPE_TEXT;
                break;
            case scene::TextInputPurpose::password:
                type = SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_HIDDEN;
                break;
            case scene::TextInputPurpose::email:
                type = SDL_TEXTINPUT_TYPE_TEXT_EMAIL;
                break;
            case scene::TextInputPurpose::number:
                type = SDL_TEXTINPUT_TYPE_NUMBER;
                break;
            default:
                return scene::TextInputRequestResult::rejected;
        }
        const auto properties = SDL_CreateProperties();
        if (properties == 0) {
            return scene::TextInputRequestResult::unavailable;
        }
        // SDL consumes/copies the properties during start; this group never outlives the call.
        const bool configured =
            SDL_SetNumberProperty(properties, SDL_PROP_TEXTINPUT_TYPE_NUMBER, type)
            && SDL_SetBooleanProperty(
                properties,
                SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN,
                request.multiline
            );
        if (!configured) {
            SDL_DestroyProperties(properties);
            return scene::TextInputRequestResult::unavailable;
        }
        // The host enables legacy committed text before any session exists. SDL's Wayland
        // backend applies properties only when transitioning from stopped to started. Take
        // ownership of that bootstrap input before starting the first typed session.
        bool restore_bootstrap = false;
        if (!active_session_ && SDL_TextInputActive(window_)) {
            if (!SDL_StopTextInput(window_)) {
                SDL_DestroyProperties(properties);
                return scene::TextInputRequestResult::unavailable;
            }
            restore_bootstrap = true;
        }
        const bool started = start_text_input_(window_, properties);
        SDL_DestroyProperties(properties);
        if (!started) {
            // A failed typed start must not leave the window without any text input: the
            // scene falls back to legacy TextInputEvent when no session is published.
            if (restore_bootstrap) {
                (void)SDL_StartTextInput(window_);
            }
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
        // The scene protocol is in logical (author) window coordinates; SDL_SetTextInputArea
        // takes native window coordinates. On a 175% Windows window that is 1.75x; 1x mode
        // passes 1.0. Fallback to 1.0 for a missing platform query.
        const double scale =
            std::isfinite(window_scale_) && window_scale_ > 0.0F ? window_scale_ : 1.0;
        // Round outward so a fractional caret is never truncated to an empty SDL rectangle.
        const double left = std::floor(area.caret.get_left() * scale);
        const double top = std::floor(area.caret.get_top() * scale);
        const double right = std::ceil(area.caret.get_right() * scale);
        const double bottom = std::ceil(area.caret.get_bottom() * scale);
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
        (void)SDL_SetTextInputArea(window_, nullptr, 0);
        active_session_ = {};
    }

    void SdlTextInputService::set_sink(scene::ITextInputSessionSink* sink) noexcept {
        sink_ = sink;
    }

    void SdlTextInputService::set_window_scale(const float window_scale) noexcept {
        window_scale_ = std::isfinite(window_scale) && window_scale > 0.0F ? window_scale : 1.0F;
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
