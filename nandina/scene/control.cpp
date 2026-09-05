//
// Created by cvrain on 2026/7/3.
//

#include "control.hpp"
#include "../render/draw_context.hpp"
#include "scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace nandina::scene
{

    namespace
    {
        [[nodiscard]] auto finite_or(float value, float fallback) -> float {
            return std::isfinite(value) ? value : fallback;
        }

        void require_non_negative_finite(float value, const char* name) {
            if (!std::isfinite(value) || value < 0.0F) {
                throw std::invalid_argument(std::string(name) + " must be finite and non-negative");
            }
        }

        [[nodiscard]] auto resolved_length(const LayoutLength& length, float available)
            -> std::optional<float> {
            return std::visit(
                [available](const auto& value) -> std::optional<float> {
                    using Value = std::decay_t<decltype(value)>;
                    if constexpr (std::same_as<Value, LogicalLength>) {
                        return value.value;
                    }
                    else if constexpr (std::same_as<Value, PercentLength>) {
                        return std::isfinite(available)
                            ? std::optional(available * value.value * 0.01F)
                            : std::nullopt;
                    }
                    else if constexpr (std::same_as<Value, FillLength>) {
                        return std::isfinite(available) ? std::optional(available) : std::nullopt;
                    }
                    else {
                        return std::nullopt;
                    }
                },
                length
            );
        }

        [[nodiscard]] auto constrained_axis(
            float parent_min,
            float parent_max,
            const std::optional<LayoutLength>& own_min,
            const std::optional<LayoutLength>& own_max
        ) -> std::pair<float, float> {
            const auto resolve = [parent_max](
                                     const std::optional<LayoutLength>& length,
                                     const float fallback
                                 ) -> float {
                if (!length.has_value()) {
                    return fallback;
                }
                return resolved_length(*length, parent_max).value_or(fallback);
            };
            const float minimum = std::max(parent_min, resolve(own_min, 0.0F));
            const float maximum = std::max(
                minimum,
                std::min(
                    parent_max,
                    resolve(own_max, std::numeric_limits<float>::infinity())
                )
            );
            return {minimum, maximum};
        }
    } // namespace

    auto percent(float value) -> PercentLength {
        require_non_negative_finite(value, "percentage");
        return {.value = value};
    }

    auto LayoutConstraints::loose() -> LayoutConstraints {
        return {};
    }

    auto LayoutConstraints::tight(foundation::NanSize size) -> LayoutConstraints {
        return {
            .min_width = size.get_width(),
            .max_width = size.get_width(),
            .min_height = size.get_height(),
            .max_height = size.get_height(),
        };
    }

    auto LayoutConstraints::constrain(foundation::NanSize size) const -> foundation::NanSize {
        const float max_w = finite_or(max_width, std::max(size.get_width(), min_width));
        const float max_h = finite_or(max_height, std::max(size.get_height(), min_height));
        return foundation::NanSize(
            std::clamp(size.get_width(), min_width, std::max(min_width, max_w)),
            std::clamp(size.get_height(), min_height, std::max(min_height, max_h))
        );
    }

    auto LayoutConstraints::deflated(foundation::NanInsets insets) const -> LayoutConstraints {
        const float horizontal = insets.horizontal_sum();
        const float vertical = insets.vertical_sum();
        return {
            .min_width = std::max(0.0F, min_width - horizontal),
            .max_width =
                std::isfinite(max_width) ? std::max(0.0F, max_width - horizontal) : max_width,
            .min_height = std::max(0.0F, min_height - vertical),
            .max_height =
                std::isfinite(max_height) ? std::max(0.0F, max_height - vertical) : max_height,
        };
    }

    NanControl::NanControl(const foundation::NanSize& size): size_(size) {}

    auto NanControl::size() const -> foundation::NanSize {
        return size_;
    }

    void NanControl::set_size(foundation::NanSize size) {
        if (size_ == size) {
            return;
        }
        size_ = size;
        measured_size_ = size;
        mark_semantics_dirty();
        if (auto* control_parent = parent() != nullptr ? parent()->as_control() : nullptr) {
            control_parent->mark_layout_dirty();
        }
    }

    auto NanControl::width() const -> float {
        return size_.get_width();
    }

    auto NanControl::height() const -> float {
        return size_.get_height();
    }

    auto NanControl::set_width(float width) -> NanControl& {
        require_non_negative_finite(width, "width");
        size_spec_.width = LogicalLength {width};
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_width(PercentLength width) -> NanControl& {
        require_non_negative_finite(width.value, "width percentage");
        size_spec_.width = width;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_width(FillLength width) -> NanControl& {
        size_spec_.width = width;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_width(ContentLength width) -> NanControl& {
        size_spec_.width = width;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_height(float height) -> NanControl& {
        require_non_negative_finite(height, "height");
        size_spec_.height = LogicalLength {height};
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_height(PercentLength height) -> NanControl& {
        require_non_negative_finite(height.value, "height percentage");
        size_spec_.height = height;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_height(FillLength height) -> NanControl& {
        size_spec_.height = height;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_height(ContentLength height) -> NanControl& {
        size_spec_.height = height;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_min_width(float width) -> NanControl& {
        require_non_negative_finite(width, "minimum width");
        size_spec_.min_width = LogicalLength {width};
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_min_width(PercentLength width) -> NanControl& {
        require_non_negative_finite(width.value, "minimum width percentage");
        size_spec_.min_width = width;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_max_width(float width) -> NanControl& {
        require_non_negative_finite(width, "maximum width");
        size_spec_.max_width = LogicalLength {width};
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_max_width(PercentLength width) -> NanControl& {
        require_non_negative_finite(width.value, "maximum width percentage");
        size_spec_.max_width = width;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_min_height(float height) -> NanControl& {
        require_non_negative_finite(height, "minimum height");
        size_spec_.min_height = LogicalLength {height};
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_min_height(PercentLength height) -> NanControl& {
        require_non_negative_finite(height.value, "minimum height percentage");
        size_spec_.min_height = height;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_max_height(float height) -> NanControl& {
        require_non_negative_finite(height, "maximum height");
        size_spec_.max_height = LogicalLength {height};
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_max_height(PercentLength height) -> NanControl& {
        require_non_negative_finite(height.value, "maximum height percentage");
        size_spec_.max_height = height;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::set_aspect_ratio(float ratio) -> NanControl& {
        if (!std::isfinite(ratio) || ratio <= 0.0F) {
            throw std::invalid_argument("aspect ratio must be finite and positive");
        }
        size_spec_.aspect_ratio = ratio;
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::clear_aspect_ratio() -> NanControl& {
        size_spec_.aspect_ratio.reset();
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::size_spec() const -> const ControlSizeSpec& {
        return size_spec_;
    }

    auto NanControl::local_rect() const -> foundation::NanRect {
        return foundation::NanRect::from_xywh(0.0F, 0.0F, size_.get_width(), size_.get_height());
    }

    auto NanControl::measured_size() const -> foundation::NanSize {
        return measured_size_;
    }

    auto NanControl::last_layout_constraints() const -> LayoutConstraints {
        return last_layout_constraints_;
    }

    auto NanControl::layout_dirty() const -> bool {
        return has_any(dirty_flags_, layout_dirty_flags);
    }

    auto NanControl::dirty_flags() const -> DirtyFlags {
        return dirty_flags_;
    }

    auto NanControl::is_dirty(const DirtyFlags flags) const -> bool {
        return has_any(dirty_flags_, flags);
    }

    auto NanControl::mark_dirty(const DirtyFlags flags) -> void {
        if (has_any(flags, DirtyFlags::semantics)) {
            mark_semantics_dirty();
        }
        const auto newly_dirty = static_cast<DirtyFlags>(
            static_cast<std::uint8_t>(flags)
            & ~static_cast<std::uint8_t>(dirty_flags_)
        );
        dirty_flags_ |= flags;
        if (!has_any(newly_dirty, layout_dirty_flags)) {
            return;
        }
        for (auto* ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent()) {
            if (auto* control = ancestor->as_control(); control != nullptr) {
                control->dirty_flags_ |= layout_dirty_flags;
            }
        }
    }

    auto NanControl::clear_dirty(const DirtyFlags flags) -> void {
        dirty_flags_ = static_cast<DirtyFlags>(
            static_cast<std::uint8_t>(dirty_flags_)
            & ~static_cast<std::uint8_t>(flags)
        );
    }

    auto NanControl::mark_layout_dirty() -> void {
        mark_dirty(layout_dirty_flags | DirtyFlags::paint);
    }

    auto NanControl::clear_layout_dirty() -> void {
        clear_dirty(layout_dirty_flags);
    }

    auto NanControl::layout_flex_factor() const -> int {
        return 0;
    }

    auto NanControl::layout_flex_policy() const -> LayoutFlexPolicy {
        const auto factor = layout_flex_factor();
        return factor > 0 ? LayoutFlexPolicy {.basis = 0.0F, .grow = static_cast<float>(factor)}
                          : LayoutFlexPolicy {};
    }

    auto NanControl::measure_layout(LayoutConstraints constraints) -> foundation::NanSize {
        last_layout_constraints_ = constraints;
        auto [min_width, max_width] = constrained_axis(
            constraints.min_width,
            constraints.max_width,
            size_spec_.min_width,
            size_spec_.max_width
        );
        auto [min_height, max_height] = constrained_axis(
            constraints.min_height,
            constraints.max_height,
            size_spec_.min_height,
            size_spec_.max_height
        );

        auto width = resolved_length(size_spec_.width, constraints.max_width);
        auto height = resolved_length(size_spec_.height, constraints.max_height);
        if (width.has_value()) {
            width = std::clamp(*width, min_width, max_width);
        }
        if (height.has_value()) {
            height = std::clamp(*height, min_height, max_height);
        }

        if (size_spec_.aspect_ratio.has_value()) {
            const float ratio = *size_spec_.aspect_ratio;
            if (width.has_value() && !height.has_value()) {
                height = std::clamp(*width / ratio, min_height, max_height);
            }
            else if (height.has_value() && !width.has_value()) {
                width = std::clamp(*height * ratio, min_width, max_width);
            }
        }

        LayoutConstraints effective {
            .min_width = width.value_or(min_width),
            .max_width = width.value_or(max_width),
            .min_height = height.value_or(min_height),
            .max_height = height.value_or(max_height),
        };
        auto measured = effective.constrain(on_measure(effective));

        if (size_spec_.aspect_ratio.has_value() && !width.has_value() && !height.has_value()) {
            const float ratio = *size_spec_.aspect_ratio;
            float result_width = measured.get_width();
            float result_height = result_width / ratio;
            if (result_height < min_height || result_height > max_height) {
                result_height = std::clamp(measured.get_height(), min_height, max_height);
                result_width = result_height * ratio;
            }
            measured = effective.constrain(foundation::NanSize(result_width, result_height));
        }

        measured_size_ = constraints.constrain(measured);
        return measured_size_;
    }

    auto NanControl::layout_to(foundation::NanRect rect) -> void {
        clear_dirty(layout_dirty_flags);
        set_position(rect.get_top_left());
        set_size(rect.get_size());
        on_layout();
        clear_dirty(DirtyFlags::paint);
    }

    void NanControl::set_background(foundation::NanColor color) {
        background_ = color;
        mark_dirty(DirtyFlags::paint);
    }

    void NanControl::clear_background() {
        if (!background_.has_value()) {
            return;
        }
        background_.reset();
        mark_dirty(DirtyFlags::paint);
    }

    auto NanControl::background() const -> const std::optional<foundation::NanColor>& {
        return background_;
    }

    void NanControl::set_overflow(const ControlOverflow overflow) {
        if (overflow_ == overflow) {
            return;
        }
        overflow_ = overflow;
        mark_dirty(DirtyFlags::paint | DirtyFlags::semantics);
    }

    auto NanControl::overflow() const -> ControlOverflow {
        return overflow_;
    }

    bool NanControl::contains_point(foundation::NanPoint local_point) const {
        return local_point.get_x() >= 0.0F && local_point.get_x() <= size_.get_width()
            && local_point.get_y() >= 0.0F && local_point.get_y() <= size_.get_height();
    }
    auto NanControl::global_bounds() const -> foundation::NanRect {
        return render::world_bounds_from_local(global_transform(), local_rect());
    }

    void NanControl::on_draw(render::DrawContext& ctx) {
        if (!background_.has_value()) {
            return;
        }
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const auto color = background_->with_alpha(background_->alpha() * ctx.opacity());
        ctx.device().draw_rect(world, color);
    }

    auto NanControl::on_measure(LayoutConstraints constraints) -> foundation::NanSize {
        return constraints.constrain(size_);
    }

    auto NanControl::on_layout() -> void {
        std::vector<NanControl*> visible_children;
        for (std::size_t i = 0; i < child_count(); ++i) {
            auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
            if (!child || !child->visible()) {
                continue;
            }
            visible_children.push_back(child);
        }

        if (visible_children.size() == 1) {
            auto* child = visible_children.front();
            (void)child->measure_layout(LayoutConstraints::tight(size()));
            child->layout_to(local_rect());
            return;
        }

        for (auto* child: visible_children) {
            const auto measured = child->measure_layout(
                LayoutConstraints {
                    .min_width = 0.0F,
                    .max_width = width(),
                    .min_height = 0.0F,
                    .max_height = height(),
                }
            );
            child->layout_to(foundation::NanRect::from_origin_size(child->position(), measured));
        }
    }

    auto NanControl::_push_child_clip(render::DrawContext& ctx) -> render::ClipStack::Guard {
        if (overflow_ != ControlOverflow::clip) {
            return {nullptr, false};
        }
        return ctx.clip().push(
            render::world_bounds_from_local(ctx.world_transform(), local_rect())
        );
    }

} // namespace nandina::scene
