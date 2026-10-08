//
// Created by cvrain on 2026/7/3.
//

#include "control.hpp"
#include "../render/draw_context.hpp"
#include "scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace nandina::scene
{

    namespace
    {
        void require_non_negative_finite(float value, const char* name) {
            if (!std::isfinite(value) || value < 0.0F) {
                throw std::invalid_argument(std::string(name) + " must be finite and non-negative");
            }
        }

    } // namespace

    auto resolve_layout_length(const LayoutLength& length, float available)
        -> std::optional<float> {
        return std::visit(
            [available](const auto& value) -> std::optional<float> {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::same_as<Value, LogicalLength>) {
                    return value.value;
                }
                else if constexpr (std::same_as<Value, PercentLength>) {
                    return std::isfinite(available) ? std::optional(available * value.value * 0.01F)
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

    namespace
    {
        [[nodiscard]] auto constrained_axis(
            float parent_min,
            float parent_max,
            float percentage_basis,
            const std::optional<LayoutLength>& own_min,
            const std::optional<LayoutLength>& own_max
        ) -> std::pair<float, float> {
            const auto resolve = [percentage_basis](
                                     const std::optional<LayoutLength>& length,
                                     const float fallback
                                 ) -> float {
                if (!length.has_value()) {
                    return fallback;
                }
                return resolve_layout_length(*length, percentage_basis).value_or(fallback);
            };
            const float minimum = std::max(parent_min, resolve(own_min, 0.0F));
            const float maximum = std::max(
                minimum,
                std::min(parent_max, resolve(own_max, std::numeric_limits<float>::infinity()))
            );
            return {minimum, maximum};
        }
    } // namespace

    auto percent(float value) -> PercentLength {
        require_non_negative_finite(value, "percentage");
        return {.value = value};
    }

    NanControl::NanControl(const foundation::NanSize& size): size_(size) {}

    auto NanControl::anchors() const -> const AnchorSpec& {
        static const AnchorSpec empty;
        return anchors_ ? *anchors_ : empty;
    }

    void NanControl::validate_anchor_parent(const NanNode& prospective_parent) const {
        const auto* control = prospective_parent.as_control();
        if (!anchors().empty() && (!control || !control->is_anchor_canvas())) {
            throw std::logic_error(
                "anchors: child '" + std::string(name())
                + "' requires an AnchorCanvas parent; clear anchors before moving to a linear layout"
            );
        }
    }

    auto NanControl::set_anchors(AnchorSpec spec) -> NanControl& {
        spec.validate();
        if (!spec.empty() && parent()) {
            const auto* owner = parent()->as_control();
            if (!owner || !owner->is_anchor_canvas()) {
                throw std::logic_error(
                    "anchors: child '" + std::string(name()) + "' requires an AnchorCanvas parent"
                );
            }
        }
        anchors_ = spec.empty() ? nullptr : std::make_unique<AnchorSpec>(std::move(spec));
        mark_layout_dirty();
        return *this;
    }

    auto NanControl::size() const -> foundation::NanSize {
        return size_;
    }

    void NanControl::set_size(foundation::NanSize size) {
        if (size_ == size) {
            return;
        }
        size_ = size;
        measured_size_ = size;
        // 缩放中心是按**当前布局尺寸**解析的，所以尺寸变化本身就是几何变化：必须让
        // 几何缓存失效并重建语义 bounds。只标 paint 会让缓存永远不失效 —— 缩放中心
        // 会停在旧尺寸上。
        mark_dirty(DirtyFlags::paint | DirtyFlags::transform);
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

    auto NanControl::last_layout_constraints() const -> foundation::NanLayoutConstraints {
        return last_layout_constraints_;
    }

    auto NanControl::layout_dirty() const -> bool {
        return is_dirty(layout_dirty_flags);
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

    auto NanControl::measure_layout(foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        if (!anchors().empty() && is_inside_tree()) {
            if (!parent()) {
                throw std::logic_error(
                    "anchors: a detached node requires an AnchorCanvas parent before layout"
                );
            }
            validate_anchor_parent(*parent());
        }
        return measure_layout_with_basis(
            constraints,
            foundation::NanSize(constraints.max_width, constraints.max_height)
        );
    }

    auto NanControl::measure_layout_with_basis(
        foundation::NanLayoutConstraints constraints,
        foundation::NanSize percentage_basis
    ) -> foundation::NanSize {
        last_layout_constraints_ = constraints;
        auto [min_width, max_width] = constrained_axis(
            constraints.min_width,
            constraints.max_width,
            percentage_basis.get_width(),
            size_spec_.min_width,
            size_spec_.max_width
        );
        auto [min_height, max_height] = constrained_axis(
            constraints.min_height,
            constraints.max_height,
            percentage_basis.get_height(),
            size_spec_.min_height,
            size_spec_.max_height
        );

        auto width = resolve_layout_length(size_spec_.width, percentage_basis.get_width());
        auto height = resolve_layout_length(size_spec_.height, percentage_basis.get_height());
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

        foundation::NanLayoutConstraints effective {
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
        if (!anchors().empty() && is_inside_tree()) {
            if (!parent()) {
                throw std::logic_error(
                    "anchors: a detached node requires an AnchorCanvas parent before layout"
                );
            }
            validate_anchor_parent(*parent());
        }
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

    auto NanControl::on_measure(foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
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
            (void)child->measure_layout(foundation::NanLayoutConstraints::tight(size()));
            child->layout_to(local_rect());
            return;
        }

        for (auto* child: visible_children) {
            const auto measured = child->measure_layout(
                foundation::NanLayoutConstraints {
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
