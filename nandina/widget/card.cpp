//
// widget/card - semantic surface container for one child (pure container, no interaction).
//

#include "card.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"
#include "primitives/shadow_painter.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    Card::Card(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        relayout();
    }

    auto Card::create(theme::NanTheme theme) -> std::shared_ptr<Card> {
        return std::make_shared<Card>(theme);
    }

    auto Card::set_child(std::shared_ptr<scene::NanControl> child) -> Card& {
        if (!child) {
            throw std::runtime_error("Card::set_child: child is null");
        }
        auto current = child_.lock();
        child_ = child;
        replace_child(current.get(), std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    void Card::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
        relayout();
    }

    auto Card::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Card::set_override(theme::CardRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
        relayout();
    }

    auto Card::resolved_style() const -> theme::ResolvedCardStyle {
        auto style = theme::resolve_card(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Card::on_style_context_changed(const theme::ResolvedStyleContext&) {
        mark_layout_dirty();
        relayout();
    }

    void Card::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
        relayout();
    }

    void Card::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        primitives::ShadowPainter::paint(
            context,
            world,
            style.container.radius,
            style.shadow,
            context.opacity()
        );
        primitives::BoxPainter::paint(context, world, style.container, context.opacity());
    }

    auto Card::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        const float horizontal = style.metrics.padding_x * 2.0F;
        const float vertical = style.metrics.padding_y * 2.0F;
        auto child = child_.lock();
        if (!child) {
            return constraints.constrain(foundation::NanSize(
                horizontal,
                std::max(style.metrics.min_height, vertical)
            ));
        }

        const auto measured = child->measure_layout(
            constraints.deflated(foundation::NanInsets::symmetric(
                style.metrics.padding_x,
                style.metrics.padding_y
            ))
        );
        return constraints.constrain(foundation::NanSize(
            measured.get_width() + horizontal,
            std::max(style.metrics.min_height, measured.get_height() + vertical)
        ));
    }

    auto Card::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }

        const auto style = resolved_style();
        const float content_width =
            std::max(0.0F, width() - style.metrics.padding_x * 2.0F);
        const float content_height =
            std::max(0.0F, height() - style.metrics.padding_y * 2.0F);
        const auto child_size = child->measured_size();
        child->layout_to(
            foundation::NanRect::from_xywh(
                style.metrics.padding_x,
                style.metrics.padding_y,
                std::min(child_size.get_width(), content_width),
                std::min(child_size.get_height(), content_height)
            )
        );
    }

    void Card::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }
} // namespace nandina::widget
