//
// widget/spinner - indeterminate progress indicator (pure display, no interaction).
//

#include "spinner.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/arc_painter.hpp"

#include <cmath>
#include <utility>

namespace nandina::widget
{
    Spinner::Spinner(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        const auto style = resolved_style();
        set_size(foundation::NanSize(style.metrics.diameter, style.metrics.diameter));
    }

    auto Spinner::create(theme::NanTheme theme) -> std::shared_ptr<Spinner> {
        return std::make_shared<Spinner>(theme);
    }

    void Spinner::set_label(std::string label) {
        label_ = std::move(label);
        mark_semantics_dirty();
    }

    auto Spinner::label() const -> std::string_view {
        return label_;
    }

    void Spinner::set_disabled(const bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Spinner::disabled() const -> bool {
        return disabled_;
    }

    void Spinner::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
    }

    auto Spinner::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Spinner::set_override(theme::SpinnerRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Spinner::visual_state() const -> theme::SpinnerVisualState {
        return disabled_ ? theme::SpinnerVisualState::disabled
                         : theme::SpinnerVisualState::normal;
    }

    auto Spinner::resolved_style() const -> theme::ResolvedSpinnerStyle {
        auto style = theme::resolve_spinner(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    auto Spinner::rotation() const -> float {
        return rotation_;
    }

    void Spinner::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
    }

    void Spinner::on_process(const float dt) {
        // Disabled 时不推进，也不请求重绘：指示环冻结在当前角度。
        if (disabled_) {
            return;
        }
        const float speed = resolved_style().metrics.rotation_speed;
        if (std::isfinite(dt) && std::isfinite(speed)) {
            rotation_ += dt * speed;
        }
        // 持续请求下一帧，动画才会继续。
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Spinner::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        primitives::ArcPainter::paint(
            context,
            world,
            primitives::ArcStyle {
                .radius = style.metrics.diameter * 0.5F,
                .thickness = style.metrics.thickness,
                .start_radians = rotation_,
                .sweep_radians = style.metrics.arc_radians,
            },
            style.indicator,
            context.opacity()
        );
    }

    auto Spinner::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const float diameter = resolved_style().metrics.diameter;
        return constraints.constrain(foundation::NanSize(diameter, diameter));
    }

    auto Spinner::semantics_properties() const -> semantics::Properties {
        // 不定量进度：与 ProgressBar 同一角色，但不给 value（无百分比可播报）。
        return {
            .role = semantics::Role::progress_bar,
            .label = label_,
            .state = {.disabled = disabled_},
        };
    }
} // namespace nandina::widget
