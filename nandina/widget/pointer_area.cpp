#include "pointer_area.hpp"
#include "../scene/scene_tree.hpp"

#include <stdexcept>

namespace nandina::widget
{
    auto PointerArea::create() -> std::shared_ptr<PointerArea> {
        return std::make_shared<PointerArea>();
    }

    auto PointerArea::set_child(std::shared_ptr<scene::NanControl> child) -> PointerArea& {
        if (!child) {
            throw std::invalid_argument("PointerArea::set_child: child is null");
        }
        auto current = child_.lock();
        child_ = child;
        replace_child(current.get(), std::move(child));
        mark_layout_dirty();
        return *this;
    }

    void PointerArea::set_on_pointer_down(std::function<void(const scene::MouseButtonEvent&)> callback) {
        on_pointer_down_ = std::move(callback);
    }
    void PointerArea::set_on_pointer_up(std::function<void(const scene::MouseButtonEvent&)> callback) {
        on_pointer_up_ = std::move(callback);
    }
    void PointerArea::set_on_pointer_move(std::function<void(const scene::MouseMoveEvent&)> callback) {
        on_pointer_move_ = std::move(callback);
    }
    void PointerArea::set_on_pointer_enter(std::function<void(const scene::MouseEnterEvent&)> callback) {
        on_pointer_enter_ = std::move(callback);
    }
    void PointerArea::set_on_pointer_leave(std::function<void(const scene::MouseLeaveEvent&)> callback) {
        on_pointer_leave_ = std::move(callback);
    }

    auto PointerArea::on_input_capture(scene::InputEvent& event) -> bool {
        switch (event.type()) {
            case scene::EventType::mouse_button: {
                auto& pointer = static_cast<scene::MouseButtonEvent&>(event);
                if (pointer.is_pressed()) {
                    if (on_pointer_down_) on_pointer_down_(pointer);
                    if (is_inside_tree()) {
                        auto child = child_.lock();
                        get_tree()->set_pointer_capture(child ? child.get() : this);
                    }
                } else if (on_pointer_up_) {
                    on_pointer_up_(pointer);
                }
                return false;
            }
            case scene::EventType::mouse_move:
                if (on_pointer_move_) on_pointer_move_(static_cast<scene::MouseMoveEvent&>(event));
                return false;
            case scene::EventType::mouse_enter:
                if (on_pointer_enter_) on_pointer_enter_(static_cast<scene::MouseEnterEvent&>(event));
                return false;
            case scene::EventType::mouse_leave:
                if (on_pointer_leave_) on_pointer_leave_(static_cast<scene::MouseLeaveEvent&>(event));
                return false;
            default:
                return false;
        }
    }

    auto PointerArea::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        return child ? constraints.constrain(child->measure_layout(constraints))
                     : constraints.constrain(foundation::NanSize {});
    }

    auto PointerArea::on_layout() -> void {
        auto child = child_.lock();
        if (!child) return;
        (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
        child->layout_to(local_rect());
    }
}
