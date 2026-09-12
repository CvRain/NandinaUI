#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_FOCUS_SCOPE_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_FOCUS_SCOPE_HPP

#include "../../scene/control.hpp"

#include <memory>

namespace nandina::widget::internal
{
    class FocusScope final: public scene::NanControl {
    public:
        auto set_content(std::shared_ptr<scene::NanControl> content) -> scene::NanControl&;
        [[nodiscard]] auto content() const -> scene::NanControl*;

        [[nodiscard]] auto on_input_capture(scene::InputEvent& event) -> bool override;
        void on_ready() override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;

    private:
        std::weak_ptr<scene::NanNode> previous_focus_;
        std::weak_ptr<scene::NanControl> content_;
    };
}

#endif
