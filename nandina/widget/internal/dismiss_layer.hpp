#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_DISMISS_LAYER_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_DISMISS_LAYER_HPP

#include "../../scene/control.hpp"

#include <functional>
#include <memory>
#include <utility>

namespace nandina::widget::internal
{
    enum class DismissReason {
        pointer,
        escape,
    };

    class DismissLayer final: public scene::NanControl {
    public:
        using Callback = std::function<void(DismissReason)>;

        explicit DismissLayer(Callback callback = {}): callback_(std::move(callback)) {}

        auto set_content(std::shared_ptr<scene::NanControl> content) -> scene::NanControl&;
        [[nodiscard]] auto content() const -> scene::NanControl*;

        void set_callback(Callback callback) {
            callback_ = std::move(callback);
        }

        [[nodiscard]] auto on_input(scene::InputEvent& event) -> bool override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override {
            return constraints.constrain(foundation::NanSize(constraints.max_width, constraints.max_height));
        }
        void on_layout() override;

    private:
        Callback callback_;
        std::weak_ptr<scene::NanControl> content_;
    };
}

#endif
