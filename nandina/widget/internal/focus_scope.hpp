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

        /// 内容没有可聚焦控件时，作用域自身作为焦点兜底。Escape 与 Tab 都由沿祖先链冒泡的
        /// on_input_capture 处理，只有焦点位于作用域内部时它们才可达；内容存在可聚焦控件时
        /// 作用域不参与 Tab 循环，避免焦点停在不可见的容器上。
        [[nodiscard]] auto is_focusable() const -> bool override;

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
