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
            hovered_ = false;
            pressed_ = false;
            focused_ = false;
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
                set_pressed(false);
                return false;
            case scene::EventType::focus_enter:
                set_focused(true);
                return false;
            case scene::EventType::focus_leave:
                set_focused(false);
                set_pressed(false);
                return false;
            case scene::EventType::mouse_button: {
                auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
                if (mouse.button() != scene::MouseButtonEvent::Button::left) {
                    return false;
                }
                if (mouse.is_pressed()) {
                    set_pressed(true);
                    event.accept();
                    return true;
                }
                const bool should_click = pressed_ && hovered_;
                set_pressed(false);
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
    }

} // namespace nandina::widget::primitives
