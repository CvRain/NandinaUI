#include "dismiss_layer.hpp"

#include "../../scene/input_event.hpp"

#include <stdexcept>
#include <utility>

namespace nandina::widget::internal
{
    auto DismissLayer::set_content(std::shared_ptr<scene::NanControl> content)
        -> scene::NanControl& {
        if (!content) {
            throw std::invalid_argument("DismissLayer::set_content: content is null");
        }
        if (content->parent() != nullptr) {
            throw std::logic_error("DismissLayer::set_content: content must be detached");
        }
        if (auto current = content_.lock()) {
            remove_and_delete(*current);
        }
        auto* result = content.get();
        content_ = content;
        add_child(std::move(content));
        return *result;
    }

    auto DismissLayer::content() const -> scene::NanControl* {
        return content_.lock().get();
    }

    auto DismissLayer::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_button) {
            auto& pointer = static_cast<scene::MouseButtonEvent&>(event);
            if (pointer.is_pressed() && pointer.button() == scene::MouseButtonEvent::Button::left) {
                if (auto current = content_.lock(); current != nullptr
                    && current->global_bounds().contains_point(pointer.screen_pos())) {
                    return false;
                }
                if (callback_) {
                    callback_(DismissReason::pointer);
                }
                event.accept();
                return true;
            }
        }
        else if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (key.is_pressed() && key.keycode() == 256) {
                if (callback_) {
                    callback_(DismissReason::escape);
                }
                event.accept();
                return true;
            }
        }
        return false;
    }

    void DismissLayer::on_layout() {
        auto current = content_.lock();
        if (current == nullptr) {
            return;
        }
        const scene::LayoutConstraints constraints {
            .min_width = 0.0F,
            .max_width = width(),
            .min_height = 0.0F,
            .max_height = height(),
        };
        const auto measured = current->measure_layout(constraints);
        current->layout_to(foundation::NanRect::from_origin_size(current->position(), measured));
    }
}
