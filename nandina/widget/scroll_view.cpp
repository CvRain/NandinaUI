//
// widget/scroll_view — clipped single-child scrolling viewport.
//

#include "scroll_view.hpp"

#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    ScrollView::ScrollView(const ScrollAxis axis): axis_(axis) {
        set_overflow(scene::ControlOverflow::clip);
    }

    auto ScrollView::create(const ScrollAxis axis) -> std::shared_ptr<ScrollView> {
        return std::make_shared<ScrollView>(axis);
    }

    auto ScrollView::set_child(std::shared_ptr<scene::NanControl> child) -> ScrollView& {
        if (!child) {
            throw std::runtime_error("ScrollView::set_child: child is null");
        }
        auto current = child_.lock();
        child_ = child;
        replace_child(current.get(), std::move(child));
        mark_layout_dirty();
        return *this;
    }

    auto ScrollView::set_axis(const ScrollAxis axis) -> ScrollView& {
        axis_ = axis;
        mark_layout_dirty();
        clamp_offset();
        return *this;
    }

    auto ScrollView::set_scroll_offset(const foundation::NanPoint offset) -> ScrollView& {
        offset_ = offset;
        clamp_offset();
        apply_child_position();
        return *this;
    }

    auto ScrollView::scroll_by(const foundation::NanPoint delta) -> ScrollView& {
        return set_scroll_offset(offset_ + delta);
    }

    auto ScrollView::set_wheel_step(const float step) -> ScrollView& {
        if (!std::isfinite(step) || step < 0.0F) {
            throw std::invalid_argument("ScrollView wheel step must be finite and non-negative");
        }
        wheel_step_ = step;
        return *this;
    }

    void ScrollView::request_scroll_to_end() {
        scroll_to_end_requested_ = true;
        schedule_scroll_to_end();
    }

    auto ScrollView::child() const -> scene::NanControl* {
        return child_.lock().get();
    }
    auto ScrollView::axis() const -> ScrollAxis {
        return axis_;
    }
    auto ScrollView::scroll_offset() const -> foundation::NanPoint {
        return offset_;
    }
    auto ScrollView::content_size() const -> foundation::NanSize {
        return content_size_;
    }
    auto ScrollView::wheel_step() const -> float {
        return wheel_step_;
    }

    auto ScrollView::maximum_scroll_offset() const -> foundation::NanPoint {
        return foundation::NanPoint(
            axis_ == ScrollAxis::vertical ? 0.0F
                                          : std::max(0.0F, content_size_.get_width() - width()),
            axis_ == ScrollAxis::horizontal ? 0.0F
                                            : std::max(0.0F, content_size_.get_height() - height())
        );
    }

    auto ScrollView::on_input(scene::InputEvent& event) -> bool {
        if (event.type() != scene::EventType::mouse_wheel) {
            return false;
        }
        const auto& wheel = static_cast<const scene::MouseWheelEvent&>(event);
        auto delta = wheel.delta();
        float dx = -delta.get_x() * wheel_step_;
        float dy = -delta.get_y() * wheel_step_;
        if (axis_ == ScrollAxis::horizontal && dx == 0.0F) {
            dx = dy;
        }
        if (axis_ == ScrollAxis::vertical) {
            dx = 0.0F;
        }
        if (axis_ == ScrollAxis::horizontal) {
            dy = 0.0F;
        }
        const auto before = offset_;
        scroll_by(foundation::NanPoint(dx, dy));
        if (offset_ != before) {
            event.accept();
            return true;
        }
        return false;
    }

    void ScrollView::on_enter_tree() {
        scene::NanControl::on_enter_tree();
        schedule_scroll_to_end();
    }

    void ScrollView::on_exit_tree() {
        scroll_to_end_requested_ = false;
        scroll_to_end_scheduled_ = false;
        ++scroll_request_generation_;
        scene::NanControl::on_exit_tree();
    }

    auto ScrollView::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto current = child_.lock();
        if (!current) {
            return constraints.constrain(foundation::NanSize {});
        }
        auto child_limits = scene::LayoutConstraints::loose();
        if (axis_ != ScrollAxis::horizontal) {
            child_limits.max_width = constraints.max_width;
        }
        if (axis_ != ScrollAxis::vertical) {
            child_limits.max_height = constraints.max_height;
        }
        content_size_ = current->measure_layout(child_limits);
        return constraints.constrain(content_size_);
    }

    auto ScrollView::on_layout() -> void {
        auto current = child_.lock();
        if (!current) {
            content_size_ = foundation::NanSize {};
            offset_ = foundation::NanPoint {};
            return;
        }
        scene::LayoutConstraints limits = scene::LayoutConstraints::loose();
        if (axis_ != ScrollAxis::horizontal) {
            limits.min_width = width();
            limits.max_width = width();
        }
        if (axis_ != ScrollAxis::vertical) {
            limits.min_height = height();
            limits.max_height = height();
        }
        content_size_ = current->measure_layout(limits);
        clamp_offset();
        current->layout_to(
            foundation::NanRect::from_xywh(
                -offset_.get_x(),
                -offset_.get_y(),
                content_size_.get_width(),
                content_size_.get_height()
            )
        );
    }

    void ScrollView::clamp_offset() {
        const auto maximum = maximum_scroll_offset();
        offset_.set_x(std::clamp(offset_.get_x(), 0.0F, maximum.get_x()));
        offset_.set_y(std::clamp(offset_.get_y(), 0.0F, maximum.get_y()));
    }

    void ScrollView::apply_child_position() {
        if (auto current = child_.lock()) {
            current->set_position(foundation::NanPoint(-offset_.get_x(), -offset_.get_y()));
        }
    }

    void ScrollView::schedule_scroll_to_end() {
        if (!scroll_to_end_requested_ || scroll_to_end_scheduled_ || !is_inside_tree()) {
            return;
        }
        auto* tree = get_tree();
        auto weak =
            std::weak_ptr<ScrollView>(std::static_pointer_cast<ScrollView>(shared_from_this()));
        const auto generation = scroll_request_generation_;
        scroll_to_end_scheduled_ = true;
        tree->post_layout([weak = std::move(weak), tree, generation] {
            const auto view = weak.lock();
            if (!view || view->scroll_request_generation_ != generation) {
                return;
            }
            view->scroll_to_end_scheduled_ = false;
            if (!view->scroll_to_end_requested_ || view->get_tree() != tree) {
                return;
            }
            view->scroll_to_end_requested_ = false;
            view->set_scroll_offset(view->maximum_scroll_offset());
        });
    }
} // namespace nandina::widget
