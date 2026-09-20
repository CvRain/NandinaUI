//
// widget/internal/alert_dismiss_button - Alert 内部的关闭小按钮。
//

#include "alert_dismiss_button.hpp"

#include "../../render/draw_context.hpp"
#include "../primitives/box_painter.hpp"
#include "../primitives/focus_ring_painter.hpp"

#include <utility>

namespace nandina::widget::internal
{
    namespace
    {
        /// × 字形的描边宽度（逻辑单位）：组件几何，无语义标量 token。
        constexpr float kGlyphStroke = 1.5F;
        /// 焦点环宽度（逻辑单位）：组件几何。
        constexpr float kFocusRingWidth = 2.0F;
        /// hover / pressed 状态底色的 alpha 强度（对 tone 图标色取 alpha）。
        constexpr float kHoverAlpha = 0.12F;
        constexpr float kPressedAlpha = 0.20F;
        /// × 端点相对方形边长的内缩比例，使字形居中且不贴边。
        constexpr float kGlyphInsetRatio = 0.28F;
    } // namespace

    AlertDismissButton::AlertDismissButton() = default;

    void AlertDismissButton::set_glyph_color(foundation::NanColor color) {
        glyph_color_ = color;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto AlertDismissButton::glyph_color() const -> const foundation::NanColor& {
        return glyph_color_;
    }

    void AlertDismissButton::set_focus_ring_color(foundation::NanColor color) {
        focus_ring_color_ = color;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto AlertDismissButton::focus_ring_color() const -> const foundation::NanColor& {
        return focus_ring_color_;
    }

    void AlertDismissButton::set_affordance_size(const float size) {
        size_ = size;
        mark_layout_dirty();
    }

    auto AlertDismissButton::affordance_size() const -> float {
        return size_;
    }

    void AlertDismissButton::set_accessible_label(std::string label) {
        label_ = std::move(label);
        mark_semantics_dirty();
    }

    auto AlertDismissButton::accessible_label() const -> std::string_view {
        return label_;
    }

    auto AlertDismissButton::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        return constraints.constrain(foundation::NanSize(size_, size_));
    }

    void AlertDismissButton::on_pressable_state_changed() {
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto AlertDismissButton::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::button,
            .label = label_,
            .state = {.focusable = !disabled(), .focused = focused(), .disabled = disabled()},
            .actions = disabled() ? semantics::Action::none : semantics::Action::activate,
        };
    }

    void AlertDismissButton::on_draw(render::DrawContext& context) {
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());

        // hover / pressed 反馈：用 tone 图标色本身按低 alpha 铺一层底，不引入新颜色。
        if ((hovered() || pressed()) && glyph_color_.alpha() > 0.0F) {
            const float alpha = pressed() ? kPressedAlpha : kHoverAlpha;
            primitives::BoxPainter::paint_fill(
                context,
                world,
                theme::ResolvedBoxStyle {
                    .fill = glyph_color_.with_alpha(glyph_color_.alpha() * alpha),
                    .border = glyph_color_.with_alpha(0.0F),
                    .border_width = 0.0F,
                    .radius = size_ * 0.5F,
                },
                context.opacity()
            );
        }

        if (focused()) {
            primitives::FocusRingPainter::paint(
                context,
                world,
                theme::ResolvedFocusRing {
                    .color = focus_ring_color_,
                    .width = kFocusRingWidth,
                },
                context.opacity()
            );
        }

        // × 字形：两条对角线，避免依赖字体里是否存在 U+00D7。
        const float inset = size_ * kGlyphInsetRatio;
        const auto color =
            glyph_color_.with_alpha(glyph_color_.alpha() * context.opacity());
        context.device().draw_line(
            foundation::NanPoint(world.get_left() + inset, world.get_top() + inset),
            foundation::NanPoint(world.get_right() - inset, world.get_bottom() - inset),
            context.logical_to_screen(kGlyphStroke),
            color
        );
        context.device().draw_line(
            foundation::NanPoint(world.get_right() - inset, world.get_top() + inset),
            foundation::NanPoint(world.get_left() + inset, world.get_bottom() - inset),
            context.logical_to_screen(kGlyphStroke),
            color
        );
    }
} // namespace nandina::widget::internal
