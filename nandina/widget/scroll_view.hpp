//
// widget/scroll_view — clipped single-child scrolling viewport.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_SCROLL_VIEW_HPP
#define NANDINA_EXPERIMENT_WIDGET_SCROLL_VIEW_HPP

#include "../scene/control.hpp"

#include <cstdint>
#include <memory>

namespace nandina::widget
{
    enum class ScrollAxis { vertical, horizontal, both };

    class ScrollView final: public scene::NanControl {
    public:
        explicit ScrollView(ScrollAxis axis = ScrollAxis::vertical);
        [[nodiscard]] static auto create(ScrollAxis axis = ScrollAxis::vertical)
            -> std::shared_ptr<ScrollView>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> ScrollView&;
        auto set_axis(ScrollAxis axis) -> ScrollView&;
        auto set_scroll_offset(foundation::NanPoint offset) -> ScrollView&;
        auto scroll_by(foundation::NanPoint delta) -> ScrollView&;
        auto set_wheel_step(float step) -> ScrollView&;

        /// Scroll to the current content end after the next completed layout.
        /// Calls made while detached are remembered until the view enters a tree.
        void request_scroll_to_end();

        [[nodiscard]] auto child() const -> scene::NanControl*;
        [[nodiscard]] auto axis() const -> ScrollAxis;
        [[nodiscard]] auto scroll_offset() const -> foundation::NanPoint;
        [[nodiscard]] auto maximum_scroll_offset() const -> foundation::NanPoint;
        [[nodiscard]] auto content_size() const -> foundation::NanSize;
        [[nodiscard]] auto wheel_step() const -> float;

        auto on_input(scene::InputEvent& event) -> bool override;

    protected:
        void on_enter_tree() override;
        void on_exit_tree() override;
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;

    private:
        void clamp_offset();
        void apply_child_position();
        void schedule_scroll_to_end();

        ScrollAxis axis_ = ScrollAxis::vertical;
        std::weak_ptr<scene::NanControl> child_;
        foundation::NanPoint offset_ {};
        foundation::NanSize content_size_ {};
        float wheel_step_ = 40.0F;
        bool scroll_to_end_requested_ = false;
        bool scroll_to_end_scheduled_ = false;
        std::uint64_t scroll_request_generation_ = 0;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_SCROLL_VIEW_HPP
