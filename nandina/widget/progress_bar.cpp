//
// widget/progress_bar - determinate progress bar (pure display, no interaction).
//

#include "progress_bar.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        [[nodiscard]] auto percent_text(const float fraction) -> std::string {
            const int percent = std::clamp(
                static_cast<int>(std::lround(fraction * 100.0F)),
                0,
                100
            );
            return std::to_string(percent) + "%";
        }
    } // namespace

    ProgressBar::ProgressBar(const float value, theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        set_value(value);
        const auto style = resolved_style();
        set_size(foundation::NanSize(style.metrics.preferred_width, style.metrics.height));
    }

    auto ProgressBar::create(const float value, theme::NanTheme theme) -> std::shared_ptr<ProgressBar> {
        return std::make_shared<ProgressBar>(value, theme);
    }

    void ProgressBar::set_label(std::string label) {
        label_ = std::move(label);
        mark_semantics_dirty();
    }

    auto ProgressBar::label() const -> std::string_view {
        return label_;
    }

    void ProgressBar::set_value(const float value) {
        const float clamped = std::isfinite(value) ? std::clamp(value, 0.0F, 1.0F) : 0.0F;
        if (std::abs(value_ - clamped) <= foundation::nan_epsilon) {
            return;
        }
        value_ = clamped;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto ProgressBar::value() const -> float {
        return value_;
    }

    void ProgressBar::set_disabled(const bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto ProgressBar::disabled() const -> bool {
        return disabled_;
    }

    void ProgressBar::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
    }

    auto ProgressBar::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void ProgressBar::set_override(theme::ProgressBarRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto ProgressBar::visual_state() const -> theme::ProgressBarVisualState {
        return disabled_ ? theme::ProgressBarVisualState::disabled
                         : theme::ProgressBarVisualState::normal;
    }

    auto ProgressBar::resolved_style() const -> theme::ResolvedProgressBarStyle {
        auto style = theme::resolve_progress_bar(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void ProgressBar::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
    }

    void ProgressBar::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        const float opacity = context.opacity();
        primitives::BoxPainter::paint_fill(context, world, style.track, opacity);

        const float fill_width = std::clamp(value_, 0.0F, 1.0F) * world.get_width();
        if (fill_width > 0.0F) {
            const auto fill_rect = foundation::NanRect::from_xywh(
                world.get_left(),
                world.get_top(),
                fill_width,
                world.get_height()
            );
            primitives::BoxPainter::paint_fill(context, fill_rect, style.fill, opacity);
        }
    }

    auto ProgressBar::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        const float width = std::isfinite(constraints.max_width)
            ? constraints.max_width
            : style.metrics.preferred_width;
        return constraints.constrain(foundation::NanSize(width, style.metrics.height));
    }

    auto ProgressBar::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::progress_bar,
            .label = label_,
            .value = percent_text(value_),
            .state = {.disabled = disabled_},
        };
    }
} // namespace nandina::widget
