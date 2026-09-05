//
// Created by cvrain on 2026/7/1.
//

#ifndef NANDINA_EXPERIMENT_INPUT_EVENT_HPP
#define NANDINA_EXPERIMENT_INPUT_EVENT_HPP

#include "../foundation/geometry.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace nandina::scene
{

    /// Discriminator for InputEvent subtypes. Enables switch-based dispatch in
    /// handlers without dynamic_cast / RTTI (matches the library's no-RTTI policy).
    enum class EventType : std::uint8_t {
        mouse_button,
        mouse_move,
        mouse_enter,
        mouse_leave,
        mouse_wheel,
        key,
        text_input,
        focus_enter,
        focus_leave,
    };

    /// Keyboard / pointer modifier state, shared by button and key events.
    struct KeyModifiers {
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
        bool super = false; // Windows / Command key.

        [[nodiscard]] auto any() const -> bool {
            return shift || ctrl || alt || super;
        }
    };

    /// Base class for all input events.
    class InputEvent {
    public:
        explicit InputEvent(EventType type): type_(type) {}
        virtual ~InputEvent() = default;

        /// Runtime discriminator; use with static_cast for RTTI-free dispatch.
        [[nodiscard]] auto type() const -> EventType {
            return type_;
        }

        /// True if a node has consumed this event (stops bubbling).
        [[nodiscard]] auto is_accepted() const -> bool {
            return accepted_;
        }

        /// Mark this event as consumed (no further propagation).
        void accept() {
            accepted_ = true;
        }

    private:
        EventType type_;
        bool accepted_ = false;
    };

    /// Base class for pointer events that carry a screen-space position.
    class MouseEvent: public InputEvent {
    public:
        MouseEvent(EventType type, foundation::NanPoint screen_pos):
            InputEvent(type),
            screen_pos_(screen_pos) {}

        [[nodiscard]] auto screen_pos() const -> foundation::NanPoint {
            return screen_pos_;
        }

    private:
        foundation::NanPoint screen_pos_;
    };

    /// Mouse button press or release.
    class MouseButtonEvent final: public MouseEvent {
    public:
        enum class Button { left, middle, right };
        enum class Action { press, release };

        MouseButtonEvent(
            Button button,
            Action action,
            foundation::NanPoint screen_pos,
            KeyModifiers mods = {}
        ):
            MouseEvent(EventType::mouse_button, screen_pos),
            button_(button),
            action_(action),
            mods_(mods) {}

        [[nodiscard]] auto button() const -> Button {
            return button_;
        }
        [[nodiscard]] auto action() const -> Action {
            return action_;
        }
        [[nodiscard]] auto is_pressed() const -> bool {
            return action_ == Action::press;
        }
        [[nodiscard]] auto modifiers() const -> KeyModifiers {
            return mods_;
        }

    private:
        Button button_;
        Action action_;
        KeyModifiers mods_;
    };

    /// Mouse movement while remaining over the active hover target.
    class MouseMoveEvent final: public MouseEvent {
    public:
        MouseMoveEvent(foundation::NanPoint screen_pos, foundation::NanPoint delta):
            MouseEvent(EventType::mouse_move, screen_pos),
            delta_(delta) {}

        [[nodiscard]] auto delta() const -> foundation::NanPoint {
            return delta_;
        }

    private:
        foundation::NanPoint delta_;
    };

    /// Mouse entered a node's hover chain.
    class MouseEnterEvent final: public MouseEvent {
    public:
        explicit MouseEnterEvent(foundation::NanPoint screen_pos):
            MouseEvent(EventType::mouse_enter, screen_pos) {}
    };

    /// Mouse left a node's hover chain.
    class MouseLeaveEvent final: public MouseEvent {
    public:
        explicit MouseLeaveEvent(foundation::NanPoint screen_pos):
            MouseEvent(EventType::mouse_leave, screen_pos) {}
    };

    /// Scroll wheel / trackpad scroll. delta.y > 0 conventionally scrolls up.
    class MouseWheelEvent final: public MouseEvent {
    public:
        MouseWheelEvent(
            foundation::NanPoint screen_pos,
            foundation::NanPoint delta,
            KeyModifiers mods = {}
        ):
            MouseEvent(EventType::mouse_wheel, screen_pos),
            delta_(delta),
            mods_(mods) {}

        /// Scroll amount; x for horizontal, y for vertical.
        [[nodiscard]] auto delta() const -> foundation::NanPoint {
            return delta_;
        }
        [[nodiscard]] auto modifiers() const -> KeyModifiers {
            return mods_;
        }

    private:
        foundation::NanPoint delta_;
        KeyModifiers mods_;
    };

    /// Physical / logical key press or release.
    class KeyEvent final: public InputEvent {
    public:
        enum class Action { press, release };

        KeyEvent(int keycode, Action action, KeyModifiers mods = {}):
            InputEvent(EventType::key),
            keycode_(keycode),
            action_(action),
            mods_(mods) {}

        [[nodiscard]] auto keycode() const -> int {
            return keycode_;
        }
        [[nodiscard]] auto action() const -> Action {
            return action_;
        }
        [[nodiscard]] auto is_pressed() const -> bool {
            return action_ == Action::press;
        }
        [[nodiscard]] auto modifiers() const -> KeyModifiers {
            return mods_;
        }

    private:
        int keycode_;
        Action action_;
        KeyModifiers mods_;
    };

    /// Committed text input (already composed; may be multiple UTF-8 bytes).
    /// Distinct from KeyEvent: this carries the resulting characters, not keycodes.
    class TextInputEvent final: public InputEvent {
    public:
        explicit TextInputEvent(std::string text):
            InputEvent(EventType::text_input),
            text_(std::move(text)) {}

        [[nodiscard]] auto text() const -> std::string_view {
            return text_;
        }

    private:
        std::string text_;
    };

    /// Focus entered a node's focus chain.
    class FocusEnterEvent final: public InputEvent {
    public:
        FocusEnterEvent(): InputEvent(EventType::focus_enter) {}
    };

    /// Focus left a node's focus chain.
    class FocusLeaveEvent final: public InputEvent {
    public:
        FocusLeaveEvent(): InputEvent(EventType::focus_leave) {}
    };

} // namespace nandina::scene

#endif // NANDINA_EXPERIMENT_INPUT_EVENT_HPP
