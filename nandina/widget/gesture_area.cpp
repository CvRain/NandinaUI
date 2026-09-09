#include "gesture_area.hpp"

#include <algorithm>
#include <cmath>

namespace nandina::widget
{
    auto GestureArea::create() -> std::shared_ptr<GestureArea> {
        return std::make_shared<GestureArea>();
    }

    void GestureArea::set_on_click(std::function<void(const scene::MouseButtonEvent&)> callback) {
        on_click_ = std::move(callback);
    }
    void GestureArea::set_on_double_click(std::function<void(const scene::MouseButtonEvent&)> callback) {
        on_double_click_ = std::move(callback);
    }
    void GestureArea::set_on_long_press(std::function<void(const scene::MouseButtonEvent&)> callback) {
        on_long_press_ = std::move(callback);
    }
    void GestureArea::set_on_drag_start(std::function<void(const scene::MouseButtonEvent&)> callback) {
        on_drag_start_ = std::move(callback);
    }
    void GestureArea::set_on_drag_move(std::function<void(const scene::MouseMoveEvent&)> callback) {
        on_drag_move_ = std::move(callback);
    }
    void GestureArea::set_on_drag_end(std::function<void(const scene::MouseButtonEvent&)> callback) {
        on_drag_end_ = std::move(callback);
    }
    void GestureArea::set_double_click_interval(float seconds) { double_click_interval_ = std::max(0.0F, seconds); }
    void GestureArea::set_long_press_interval(float seconds) { long_press_interval_ = std::max(0.0F, seconds); }
    void GestureArea::set_drag_threshold(float pixels) { drag_threshold_ = std::max(0.0F, pixels); }
    void GestureArea::set_double_click_distance(float pixels) { double_click_distance_ = std::max(0.0F, pixels); }

    auto GestureArea::on_input_capture(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_button) {
            auto& pointer = static_cast<scene::MouseButtonEvent&>(event);
            if (pointer.button() != scene::MouseButtonEvent::Button::left) return PointerArea::on_input_capture(event);
            if (pointer.is_pressed()) {
                press_position_ = pointer.screen_pos();
                press_event_ = pointer;
                pointer_down_ = true;
                long_press_triggered_ = false;
                press_elapsed_ = 0.0F;
            } else {
                const bool was_dragging = dragging_;
                if (was_dragging && on_drag_end_) on_drag_end_(pointer);
                dragging_ = false;
                pointer_down_ = false;
                press_event_.reset();
                if (was_dragging || long_press_triggered_) {
                    pending_click_.reset();
                } else if (pending_click_ && pending_click_elapsed_ <= double_click_interval_
                           && pending_click_->screen_pos().distance(pointer.screen_pos())
                               <= double_click_distance_) {
                    pending_click_.reset();
                    if (on_double_click_) on_double_click_(pointer);
                } else if (on_double_click_) {
                    if (pending_click_ && on_click_) on_click_(*pending_click_);
                    pending_click_ = pointer;
                    pending_click_elapsed_ = 0.0F;
                } else if (on_click_) {
                    on_click_(pointer);
                }
            }
        } else if (event.type() == scene::EventType::mouse_move && pointer_down_) {
            auto& move = static_cast<scene::MouseMoveEvent&>(event);
            const auto dx = move.screen_pos().get_x() - press_position_.get_x();
            const auto dy = move.screen_pos().get_y() - press_position_.get_y();
            if (!dragging_ && std::hypot(dx, dy) >= drag_threshold_) {
                dragging_ = true;
                pending_click_.reset();
                if (on_drag_start_) {
                    on_drag_start_(*press_event_);
                }
            }
            if (dragging_ && on_drag_move_) on_drag_move_(move);
        }
        return PointerArea::on_input_capture(event);
    }

    void GestureArea::on_process(float dt) {
        const float elapsed = std::max(0.0F, dt);
        if (pointer_down_ && !dragging_ && !long_press_triggered_) {
            press_elapsed_ += elapsed;
            if (press_elapsed_ >= long_press_interval_) {
                long_press_triggered_ = true;
                pending_click_.reset();
                if (on_long_press_ && press_event_) on_long_press_(*press_event_);
            }
        }
        if (pending_click_) {
            pending_click_elapsed_ += elapsed;
            if (pending_click_elapsed_ > double_click_interval_) {
                auto click = *pending_click_;
                pending_click_.reset();
                if (on_click_) on_click_(click);
            }
        }
        PointerArea::on_process(dt);
    }
}
