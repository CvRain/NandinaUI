//
// widget/aspect_ratio — single-child container preserving a fixed width/height ratio.
//

#include "aspect_ratio.hpp"

#include "../scene/scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        void validate_ratio(const float ratio) {
            if (!std::isfinite(ratio) || ratio <= 0.0F) {
                throw std::invalid_argument("AspectRatio: ratio must be finite and positive");
            }
        }

        void validate_alignment(const LayoutAlignment alignment) {
            switch (alignment) {
                case LayoutAlignment::start:
                case LayoutAlignment::center:
                case LayoutAlignment::end:
                    return;
                default:
                    break;
            }
            throw std::invalid_argument("AspectRatio: invalid LayoutAlignment");
        }
    } // namespace

    AspectRatio::AspectRatio(const float ratio, const LayoutAlignment alignment) {
        validate_ratio(ratio);
        validate_alignment(alignment);
        ratio_ = ratio;
        alignment_ = alignment;
    }

    auto AspectRatio::create(const float ratio, const LayoutAlignment alignment)
        -> std::shared_ptr<AspectRatio> {
        return std::make_shared<AspectRatio>(ratio, alignment);
    }

    auto AspectRatio::set_ratio(const float ratio) -> AspectRatio& {
        validate_ratio(ratio);
        if (std::abs(ratio_ - ratio) <= foundation::nan_epsilon) {
            return *this;
        }
        ratio_ = ratio;
        if (!is_inside_tree() && (width() > 0.0F || height() > 0.0F)) {
            scene::NanControl::ProvisionalRelayoutScope provisional;
            on_layout();
        }
        mark_layout_dirty();
        return *this;
    }

    auto AspectRatio::ratio() const noexcept -> float {
        return ratio_;
    }

    auto AspectRatio::set_alignment(const LayoutAlignment alignment) -> AspectRatio& {
        validate_alignment(alignment);
        if (alignment_ == alignment) {
            return *this;
        }
        alignment_ = alignment;
        if (!is_inside_tree() && (width() > 0.0F || height() > 0.0F)) {
            scene::NanControl::ProvisionalRelayoutScope provisional;
            on_layout();
        }
        mark_layout_dirty();
        return *this;
    }

    auto AspectRatio::alignment() const noexcept -> LayoutAlignment {
        return alignment_;
    }

    auto AspectRatio::set_child(std::shared_ptr<scene::NanControl> child) -> AspectRatio& {
        if (!child) {
            throw std::invalid_argument("AspectRatio::set_child: child must not be null");
        }
        auto current = child_.lock();
        if (current == child) {
            return *this;
        }
        if (child.get() == this || child->is_ancestor_of(*this) || child->parent() != nullptr
            || child->is_inside_tree())
        {
            throw std::invalid_argument(
                "AspectRatio::set_child: child must be a detached control not forming a cycle"
            );
        }
        child->validate_anchor_parent(*this);
        if (current) {
            release_child_input(*current);
        }
        replace_child(current.get(), child);
        child_ = std::move(child);
        if (!is_inside_tree() && (width() > 0.0F || height() > 0.0F)) {
            scene::NanControl::ProvisionalRelayoutScope provisional;
            on_layout();
        }
        mark_layout_dirty();
        return *this;
    }

    auto AspectRatio::clear_child() -> AspectRatio& {
        if (const auto current = child_.lock()) {
            release_child_input(*current);
            remove_and_delete(*current);
            child_.reset();
            content_rect_ = foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
            mark_layout_dirty();
        }
        return *this;
    }

    auto AspectRatio::child() const noexcept -> scene::NanControl* {
        return child_.lock().get();
    }

    auto AspectRatio::content_rect() const noexcept -> foundation::NanRect {
        return content_rect_;
    }

    auto AspectRatio::
        child_axis_assignment(const scene::NanControl& target, const bool /*horizontal*/) const
        -> scene::ChildAxisAssignment {
        if (child_.lock().get() != &target) {
            return scene::ChildAxisAssignment::none;
        }
        const auto constraints = last_layout_constraints();
        const bool any_finite =
            (std::isfinite(constraints.max_width) && constraints.max_width >= 0.0F)
            || (std::isfinite(constraints.max_height) && constraints.max_height >= 0.0F);
        return any_finite ? scene::ChildAxisAssignment::assigned
                          : scene::ChildAxisAssignment::pass_through;
    }

    void AspectRatio::relayout() {
        scene::NanControl::ProvisionalRelayoutScope provisional;
        (void)measure_layout(foundation::NanLayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto
    AspectRatio::resolve_box_for_constraints(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const bool finite_w = std::isfinite(constraints.max_width);
        const bool finite_h = std::isfinite(constraints.max_height);

        float width = 0.0F;
        float height = 0.0F;

        if (finite_w && finite_h) {
            width = std::max(0.0F, constraints.max_width);
            height = width / ratio_;
            if (height > constraints.max_height) {
                height = std::max(0.0F, constraints.max_height);
                width = height * ratio_;
            }
            if (width < constraints.min_width) {
                width = constraints.min_width;
                height = std::clamp(width / ratio_, constraints.min_height, constraints.max_height);
            }
            if (height < constraints.min_height) {
                height = constraints.min_height;
                width = std::clamp(height * ratio_, constraints.min_width, constraints.max_width);
            }
        }
        else if (finite_w) {
            width = std::max(constraints.min_width, constraints.max_width);
            height = width / ratio_;
            if (height < constraints.min_height) {
                height = constraints.min_height;
                width = std::min(constraints.max_width, height * ratio_);
            }
        }
        else if (finite_h) {
            height = std::max(constraints.min_height, constraints.max_height);
            width = height * ratio_;
            if (width < constraints.min_width) {
                width = constraints.min_width;
                height = std::min(constraints.max_height, width / ratio_);
            }
        }
        else {
            float child_w = 0.0F;
            float child_h = 0.0F;
            if (const auto current = child_.lock(); current && current->visible()) {
                const auto measured =
                    current->measure_layout(foundation::NanLayoutConstraints::loose());
                child_w = measured.get_width();
                child_h = measured.get_height();
            }
            const float base_w = std::max(constraints.min_width, child_w);
            const float base_h = std::max(constraints.min_height, child_h);
            if (base_w > 0.0F || base_h > 0.0F) {
                width = std::max(base_w, base_h * ratio_);
                height = width / ratio_;
            }
        }

        return constraints.constrain(foundation::NanSize(width, height));
    }

    auto AspectRatio::resolve_child_rect(const foundation::NanSize available) const
        -> foundation::NanRect {
        const float avail_w = std::max(0.0F, available.get_width());
        const float avail_h = std::max(0.0F, available.get_height());
        if (avail_w <= 0.0F || avail_h <= 0.0F) {
            return foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
        }

        float target_w = avail_w;
        float target_h = target_w / ratio_;
        if (target_h > avail_h) {
            target_h = avail_h;
            target_w = target_h * ratio_;
        }

        float offset_x = 0.0F;
        float offset_y = 0.0F;
        switch (alignment_) {
            case LayoutAlignment::start:
                offset_x = 0.0F;
                offset_y = 0.0F;
                break;
            case LayoutAlignment::end:
                offset_x = std::max(0.0F, avail_w - target_w);
                offset_y = std::max(0.0F, avail_h - target_h);
                break;
            case LayoutAlignment::center:
            case LayoutAlignment::stretch:
            case LayoutAlignment::space_between:
                offset_x = std::max(0.0F, (avail_w - target_w) * 0.5F);
                offset_y = std::max(0.0F, (avail_h - target_h) * 0.5F);
                break;
        }

        return foundation::NanRect::from_xywh(offset_x, offset_y, target_w, target_h);
    }

    auto AspectRatio::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const auto box = resolve_box_for_constraints(constraints);
        if (const auto current = child_.lock(); current && current->visible()) {
            (void)current->measure_layout(foundation::NanLayoutConstraints::tight(box));
        }
        return box;
    }

    auto AspectRatio::on_layout() -> void {
        content_rect_ = resolve_child_rect(size());
        if (const auto current = child_.lock(); current && current->visible()) {
            (void)current->measure_layout(
                foundation::NanLayoutConstraints::tight(content_rect_.get_size())
            );
            current->layout_to(content_rect_);
        }
    }

    void AspectRatio::on_ready() {
        scene::NanControl::on_ready();
    }

    void AspectRatio::release_child_input(const scene::NanControl& target) {
        auto* tree = get_tree();
        if (tree == nullptr) {
            return;
        }
        if (auto* captured = tree->pointer_capture();
            captured != nullptr && (captured == &target || target.is_ancestor_of(*captured)))
        {
            tree->release_pointer_capture(captured);
        }
        if (auto* focused = tree->focused_node();
            focused != nullptr && (focused == &target || target.is_ancestor_of(*focused)))
        {
            tree->set_focus(nullptr);
        }
    }
} // namespace nandina::widget
