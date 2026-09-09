#ifndef NANDINA_EXPERIMENT_WIDGET_POINTER_AREA_HPP
#define NANDINA_EXPERIMENT_WIDGET_POINTER_AREA_HPP

#include "../scene/control.hpp"
#include "../scene/input_event.hpp"

#include <functional>
#include <memory>

namespace nandina::widget
{
    class PointerArea: public scene::NanControl {
    public:
        PointerArea() = default;
        [[nodiscard]] static auto create() -> std::shared_ptr<PointerArea>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> PointerArea&;
        void set_on_pointer_down(std::function<void(const scene::MouseButtonEvent&)> callback);
        void set_on_pointer_up(std::function<void(const scene::MouseButtonEvent&)> callback);
        void set_on_pointer_move(std::function<void(const scene::MouseMoveEvent&)> callback);
        void set_on_pointer_enter(std::function<void(const scene::MouseEnterEvent&)> callback);
        void set_on_pointer_leave(std::function<void(const scene::MouseLeaveEvent&)> callback);

        auto on_input_capture(scene::InputEvent& event) -> bool override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;

    private:
        std::weak_ptr<scene::NanControl> child_;
        std::function<void(const scene::MouseButtonEvent&)> on_pointer_down_;
        std::function<void(const scene::MouseButtonEvent&)> on_pointer_up_;
        std::function<void(const scene::MouseMoveEvent&)> on_pointer_move_;
        std::function<void(const scene::MouseEnterEvent&)> on_pointer_enter_;
        std::function<void(const scene::MouseLeaveEvent&)> on_pointer_leave_;
    };
}

#endif
