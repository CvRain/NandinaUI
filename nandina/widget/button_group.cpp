//
// widget/button_group - horizontal/vertical row of related buttons with one shared rhythm.
//

#include "button_group.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        /// 子控件测量约束：保留父级的可用上界，但清零最小约束（与 Row 的做法一致）。
        [[nodiscard]] auto child_constraints(const scene::LayoutConstraints constraints)
            -> scene::LayoutConstraints {
            return {
                .min_width = 0.0F,
                .max_width = constraints.max_width,
                .min_height = 0.0F,
                .max_height = constraints.max_height,
            };
        }

        [[nodiscard]] auto main_extent(const foundation::NanSize size, const LayoutAxis axis)
            -> float {
            return axis == LayoutAxis::horizontal ? size.get_width() : size.get_height();
        }

        [[nodiscard]] auto cross_extent(const foundation::NanSize size, const LayoutAxis axis)
            -> float {
            return axis == LayoutAxis::horizontal ? size.get_height() : size.get_width();
        }

        [[nodiscard]] auto
        size_from_extents(const float main, const float cross, const LayoutAxis axis)
            -> foundation::NanSize {
            if (axis == LayoutAxis::horizontal) {
                return foundation::NanSize(main, cross);
            }
            return foundation::NanSize(cross, main);
        }
    } // namespace

    ButtonGroup::ButtonGroup(const LayoutAxis axis, theme::NanTheme theme): axis_(axis) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
    }

    auto ButtonGroup::create(const LayoutAxis axis, theme::NanTheme theme)
        -> std::shared_ptr<ButtonGroup> {
        return std::make_shared<ButtonGroup>(axis, theme);
    }

    auto ButtonGroup::add_button(std::shared_ptr<scene::NanControl> button) -> ButtonGroup& {
        if (!button) {
            throw std::runtime_error("ButtonGroup::add_button: button is null");
        }
        add_child(std::move(button));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    void ButtonGroup::clear() {
        // 逆序删除：remove_and_delete 会移动剩余子节点。
        for (std::size_t index = child_count(); index > 0; --index) {
            if (auto* node = get_child(index - 1); node != nullptr) {
                remove_and_delete(*node);
            }
        }
        mark_layout_dirty();
        relayout();
    }

    auto ButtonGroup::button_count() const -> std::size_t {
        return child_count();
    }

    auto ButtonGroup::set_orientation(const LayoutAxis axis) -> ButtonGroup& {
        if (axis_ == axis) {
            return *this;
        }
        axis_ = axis;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto ButtonGroup::orientation() const -> LayoutAxis {
        return axis_;
    }

    auto ButtonGroup::set_gap(const float gap) -> ButtonGroup& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    void ButtonGroup::clear_gap() {
        if (!gap_) {
            return;
        }
        gap_.reset();
        mark_layout_dirty();
        relayout();
    }

    auto ButtonGroup::gap() const -> float {
        return resolved_gap();
    }

    void ButtonGroup::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
        relayout();
    }

    auto ButtonGroup::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void ButtonGroup::set_override(theme::ButtonGroupRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
        relayout();
    }

    auto ButtonGroup::visual_state() const -> theme::ButtonGroupVisualState {
        // 组只做布局协调，自身不接受输入，没有交互状态。
        return theme::ButtonGroupVisualState::normal;
    }

    auto ButtonGroup::resolved_style() const -> theme::ResolvedButtonGroupStyle {
        auto style = theme::resolve_button_group(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void ButtonGroup::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
        relayout();
    }

    auto ButtonGroup::resolved_gap() const -> float {
        if (gap_) {
            return *gap_;
        }
        return resolved_style().metrics.gap;
    }

    auto ButtonGroup::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        const auto style = resolved_style();
        const float gap = resolved_gap();
        const auto inner = child_constraints(constraints);

        float main = style.metrics.padding_x * 2.0F;
        float cross = 0.0F;
        std::size_t visible = 0;
        for (std::size_t index = 0; index < child_count(); ++index) {
            auto* node = get_child(index);
            auto* child = node != nullptr ? node->as_control() : nullptr;
            if (child == nullptr || !child->visible()) {
                continue;
            }
            if (visible > 0) {
                main += gap;
            }
            const auto measured = child->measure_layout(inner);
            main += main_extent(measured, axis_);
            cross = std::max(cross, cross_extent(measured, axis_));
            ++visible;
        }

        const auto content = size_from_extents(main, cross, axis_);
        return constraints.constrain(
            foundation::NanSize(content.get_width(), std::max(style.metrics.min_height, content.get_height()))
        );
    }

    void ButtonGroup::on_layout() {
        const auto style = resolved_style();
        const float gap = resolved_gap();
        const float padding = style.metrics.padding_x;
        const auto inner = child_constraints(scene::LayoutConstraints::loose());

        const float group_cross = axis_ == LayoutAxis::horizontal ? height() : width();

        // 先量一遍求交叉轴内容尺寸，再按居中落位（单轴对齐由组自己决定，不引入
        // alignment API）。
        float content_cross = 0.0F;
        for (std::size_t index = 0; index < child_count(); ++index) {
            auto* node = get_child(index);
            auto* child = node != nullptr ? node->as_control() : nullptr;
            if (child == nullptr || !child->visible()) {
                continue;
            }
            const auto measured = child->measure_layout(inner);
            content_cross = std::max(content_cross, cross_extent(measured, axis_));
        }

        float main = padding;
        for (std::size_t index = 0; index < child_count(); ++index) {
            auto* node = get_child(index);
            auto* child = node != nullptr ? node->as_control() : nullptr;
            if (child == nullptr || !child->visible()) {
                continue;
            }
            const auto measured = child->measure_layout(inner);
            const float child_main = main_extent(measured, axis_);
            const float child_cross = cross_extent(measured, axis_);
            const float cross_pos = std::max(0.0F, (group_cross - content_cross) * 0.5F);

            const auto rect = axis_ == LayoutAxis::horizontal
                ? foundation::NanRect::from_xywh(main, cross_pos, child_main, child_cross)
                : foundation::NanRect::from_xywh(cross_pos, main, child_cross, child_main);
            child->layout_to(rect);
            main += child_main + gap;
        }
    }

    void ButtonGroup::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto ButtonGroup::on_draw(render::DrawContext& context) -> void {
        const auto style = resolved_style();
        // 默认容器完全透明（BoxPainter 对 alpha 0 是 no-op）；主题作者可打开分组底色。
        primitives::BoxPainter::paint(
            context,
            render::world_bounds_from_local(context.world_transform(), local_rect()),
            style.container,
            context.opacity()
        );
    }

    void ButtonGroup::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }
} // namespace nandina::widget
