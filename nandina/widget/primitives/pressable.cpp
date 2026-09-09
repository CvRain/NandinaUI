//
// widget/primitives/pressable — interaction-state primitive.
//

#include "pressable.hpp"
#include "../../scene/input_event.hpp"
#include "../../scene/scene_tree.hpp"

namespace nandina::widget::primitives
{

    void Pressable::set_disabled(bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        if (disabled_) {
            cancel_press();
            set_hovered(false);
            set_focused(false);
            if (is_inside_tree() && get_tree()->focused_node() == this) {
                get_tree()->set_focus(nullptr);
            }
        }
        on_pressable_state_changed();
        mark_semantics_dirty();
    }

    auto Pressable::disabled() const -> bool {
        return disabled_;
    }

    auto Pressable::hovered() const -> bool {
        return hovered_;
    }

    auto Pressable::pressed() const -> bool {
        return pressed_;
    }

    auto Pressable::focused() const -> bool {
        return focused_;
    }

    void Pressable::set_on_click(std::function<void()> callback) {
        on_click_ = std::move(callback);
    }

    void Pressable::set_on_press(std::function<void()> callback) { on_press_ = std::move(callback); }
    void Pressable::set_on_release(std::function<void()> callback) { on_release_ = std::move(callback); }
    void Pressable::set_on_cancel(std::function<void()> callback) { on_cancel_ = std::move(callback); }
    void Pressable::set_on_hover_changed(std::function<void(bool)> callback) { on_hover_changed_ = std::move(callback); }
    void Pressable::set_on_focus_changed(std::function<void(bool)> callback) { on_focus_changed_ = std::move(callback); }

    auto Pressable::is_focusable() const -> bool {
        return !disabled_;
    }

    auto Pressable::on_input(scene::InputEvent& event) -> bool {
        if (disabled_) {
            return false;
        }

        switch (event.type()) {
            case scene::EventType::mouse_enter:
                set_hovered(true);
                return false;
            case scene::EventType::mouse_leave:
                set_hovered(false);
                cancel_press();
                return false;
            case scene::EventType::focus_enter:
                set_focused(true);
                return false;
            case scene::EventType::focus_leave:
                set_focused(false);
                cancel_press();
                return false;
            case scene::EventType::mouse_button: {
                auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
                if (mouse.button() != scene::MouseButtonEvent::Button::left) {
                    return false;
                }
                if (mouse.is_pressed()) {
                    set_pressed(true);
                    if (is_inside_tree()) get_tree()->set_pointer_capture(this);
                    if (on_press_) on_press_();
                    event.accept();
                    return true;
                }
                const bool should_click = pressed_ && hovered_;
                const bool was_pressed = pressed_;
                set_pressed(false);
                if (was_pressed && on_release_) on_release_();
                if (should_click) {
                    emit_click();
                    event.accept();
                    return true;
                }
                return false;
            }
            case scene::EventType::key: {
                auto& key = static_cast<scene::KeyEvent&>(event);
                constexpr int key_enter = 257;
                constexpr int key_space = 32;
                if (key.is_pressed() && (key.keycode() == key_enter || key.keycode() == key_space))
                {
                    emit_click();
                    event.accept();
                    return true;
                }
                return false;
            }
            case scene::EventType::mouse_move:
            case scene::EventType::mouse_wheel:
            case scene::EventType::text_input:
                return false;
        }
        return false;
    }

    void Pressable::emit_click() {
        on_click();
        if (on_click_) {
            on_click_();
        }
    }

    void Pressable::cancel_press() {
        if (!pressed_) return;
        set_pressed(false);
        if (on_cancel_) on_cancel_();
    }

    void Pressable::activate() {
        if (!disabled_) {
            emit_click();
        }
    }

    void Pressable::set_hovered(bool hovered) {
        if (hovered_ == hovered) {
            return;
        }
        hovered_ = hovered;
        on_pressable_state_changed();
        if (on_hover_changed_) on_hover_changed_(hovered_);
    }

    void Pressable::set_pressed(bool pressed) {
        if (pressed_ == pressed) {
            return;
        }
        pressed_ = pressed;
        on_pressable_state_changed();
    }

    void Pressable::set_focused(bool focused) {
        if (focused_ == focused) {
            return;
        }
        focused_ = focused;
        on_pressable_state_changed();
        mark_semantics_dirty();
        if (on_focus_changed_) on_focus_changed_(focused_);
    }

} // namespace nandina::widget::primitives
