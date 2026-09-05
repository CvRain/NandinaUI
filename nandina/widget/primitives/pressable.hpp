//
// widget/primitives/pressable — interaction-state primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_PRESSABLE_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_PRESSABLE_HPP

#include "../../scene/control.hpp"

#include <functional>

namespace nandina::widget::primitives
{

    class Pressable: public scene::NanControl {
    public:
        Pressable() = default;
        explicit Pressable(foundation::NanSize size): scene::NanControl(size) {}

        void set_disabled(bool disabled);
        [[nodiscard]] auto disabled() const -> bool;
        [[nodiscard]] auto hovered() const -> bool;
        [[nodiscard]] auto pressed() const -> bool;
        [[nodiscard]] auto focused() const -> bool;

        void set_on_click(std::function<void()> callback);

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;

    protected:
        virtual void on_pressable_state_changed() {}
        virtual void on_click() {}
        void activate();

    private:
        void emit_click();
        void set_hovered(bool hovered);
        void set_pressed(bool pressed);
        void set_focused(bool focused);

        bool disabled_ = false;
        bool hovered_ = false;
        bool pressed_ = false;
        bool focused_ = false;
        std::function<void()> on_click_;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_PRESSABLE_HPP
