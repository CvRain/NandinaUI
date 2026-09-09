#ifndef NANDINA_EXPERIMENT_WIDGET_GESTURE_AREA_HPP
#define NANDINA_EXPERIMENT_WIDGET_GESTURE_AREA_HPP

#include "pointer_area.hpp"

#include <chrono>
#include <optional>

namespace nandina::widget
{
    class GestureArea: public PointerArea {
    public:
        GestureArea() = default;
        [[nodiscard]] static auto create() -> std::shared_ptr<GestureArea>;

        void set_on_click(std::function<void(const scene::MouseButtonEvent&)> callback);
        void set_on_double_click(std::function<void(const scene::MouseButtonEvent&)> callback);
        void set_on_long_press(std::function<void(const scene::MouseButtonEvent&)> callback);
        void set_on_drag_start(std::function<void(const scene::MouseButtonEvent&)> callback);
        void set_on_drag_move(std::function<void(const scene::MouseMoveEvent&)> callback);
        void set_on_drag_end(std::function<void(const scene::MouseButtonEvent&)> callback);
        void set_double_click_interval(float seconds);
        void set_long_press_interval(float seconds);
        void set_drag_threshold(float pixels);
        void set_double_click_distance(float pixels);

        auto on_input_capture(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;

    private:
        std::function<void(const scene::MouseButtonEvent&)> on_click_;
        std::function<void(const scene::MouseButtonEvent&)> on_double_click_;
        std::function<void(const scene::MouseButtonEvent&)> on_long_press_;
        std::function<void(const scene::MouseButtonEvent&)> on_drag_start_;
        std::function<void(const scene::MouseMoveEvent&)> on_drag_move_;
        std::function<void(const scene::MouseButtonEvent&)> on_drag_end_;
        foundation::NanPoint press_position_ {};
        std::optional<scene::MouseButtonEvent> press_event_;
        std::optional<scene::MouseButtonEvent> pending_click_;
        bool pointer_down_ = false;
        bool dragging_ = false;
        bool long_press_triggered_ = false;
        float press_elapsed_ = 0.0F;
        float pending_click_elapsed_ = 0.0F;
        float double_click_interval_ = 0.30F;
        float long_press_interval_ = 0.50F;
        float drag_threshold_ = 5.0F;
        float double_click_distance_ = 5.0F;
    };
}

#endif
