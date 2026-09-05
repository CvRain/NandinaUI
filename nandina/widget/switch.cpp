//
// widget/switch - semantic boolean toggle control (track + thumb).
//

#include "switch.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        [[nodiscard]] auto near(const float lhs, const float rhs) -> bool {
            return std::abs(lhs - rhs) <= foundation::nan_epsilon;
        }

        [[nodiscard]] auto
        same_text_style(const primitives::TextStyle& lhs, const primitives::TextStyle& rhs)
            -> bool {
            return lhs.color.approx_equals(rhs.color) && near(lhs.font_size, rhs.font_size)
                && lhs.font == rhs.font && lhs.overflow == rhs.overflow
                && lhs.max_lines == rhs.max_lines;
        }
    } // namespace

    Switch::Switch(std::string label, const bool checked, theme::NanTheme theme):
        text_(std::move(label)),
        checked_(checked) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        apply_metrics();
    }

    auto Switch::create(std::string label, const bool checked, theme::NanTheme theme)
        -> std::shared_ptr<Switch> {
        return std::make_shared<Switch>(std::move(label), checked, theme);
    }

    void Switch::set_label(std::string label) {
        text_.set_text(std::move(label));
        apply_metrics();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Switch::label() const -> std::string_view {
        return text_.text();
    }

    void Switch::set_checked(const bool checked) {
        if (checked_ == checked) {
            return;
        }
        checked_ = checked;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Switch::checked() const -> bool {
        return checked_;
    }

    void Switch::toggle() {
        if (disabled()) {
            return;
        }
        set_checked(!checked_);
        if (on_change_) {
            on_change_(checked_);
        }
        checked_changed_.emit(checked_);
    }

    void Switch::set_on_change(std::function<void(bool)> callback) {
        on_change_ = std::move(callback);
    }

    auto Switch::checked_changed() const -> const reactive::Event<bool>& {
        return checked_changed_;
    }

    void Switch::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_metrics();
        mark_layout_dirty();
    }

    auto Switch::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Switch::set_override(theme::SwitchRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Switch::visual_state() const -> theme::SwitchVisualState {
        if (disabled()) {
            return theme::SwitchVisualState::disabled;
        }
        if (pressed()) {
            return theme::SwitchVisualState::pressed;
        }
        if (hovered()) {
            return theme::SwitchVisualState::hovered;
        }
        if (focused()) {
            return theme::SwitchVisualState::focused;
        }
        return theme::SwitchVisualState::normal;
    }

    auto Switch::resolved_style() const -> theme::ResolvedSwitchStyle {
        auto style = theme::resolve_switch(*system_, appearance_, checked_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Switch::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        apply_metrics();
        mark_layout_dirty();
    }

    auto Switch::text_pipeline() const -> primitives::TextPipeline {
        return text_.text_pipeline();
    }

    void Switch::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        apply_metrics();
        mark_layout_dirty();
    }

    void Switch::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        apply_metrics();
        mark_layout_dirty();
    }

    void Switch::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_metrics();
        mark_layout_dirty();
    }

    void Switch::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_metrics();
        mark_layout_dirty();
    }

    void Switch::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        const float opacity = context.opacity();
        const float track_width = context.logical_to_screen(style.metrics.track_width);
        const float track_height = context.logical_to_screen(style.metrics.track_height);
        const float thumb_size = context.logical_to_screen(style.metrics.thumb_size);
        const float track_top =
            world.get_top() + (world.get_height() - track_height) * 0.5F;
        const auto track = foundation::NanRect::from_xywh(
            world.get_left(),
            track_top,
            track_width,
            track_height
        );

        primitives::BoxPainter::paint(context, track, style.track, opacity);

        // 拇指：贴轨道内壁，未勾选靠左、勾选靠右。
        const float padding = (track_height - thumb_size) * 0.5F;
        const float thumb_left = checked_
            ? track.get_right() - padding - thumb_size
            : track.get_left() + padding;
        const auto thumb = foundation::NanRect::from_xywh(
            thumb_left,
            track_top + padding,
            thumb_size,
            thumb_size
        );
        primitives::BoxPainter::paint(context, thumb, style.thumb, opacity);

        apply_text_style();
        const float text_height = context.logical_to_screen(text_.measured_text_height());
        const auto text_position = foundation::NanPoint(
            track.get_right() + context.logical_to_screen(style.metrics.gap),
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(context, text_position);

        if (focused() && !disabled() && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(context, track, style.focus, opacity);
        }
    }

    auto Switch::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        apply_text_style();
        const float text_width = std::isfinite(constraints.max_width)
            ? std::max(0.0F, constraints.max_width - style.metrics.track_width - style.metrics.gap)
            : constraints.max_width;
        const auto text_size = text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = text_width,
                .min_height = 0.0F,
                .max_height = constraints.max_height,
            }
        );
        return constraints.constrain(foundation::NanSize(
            style.metrics.track_width + style.metrics.gap + text_size.get_width(),
            std::max(style.metrics.min_height, text_size.get_height())
        ));
    }

    void Switch::on_click() {
        toggle();
    }

    void Switch::on_pressable_state_changed() {
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Switch::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::switch_control,
            .label = std::string(label()),
            .state =
                {
                    .focusable = !disabled(),
                    .focused = focused(),
                    .disabled = disabled(),
                    .checked = checked_,
                },
            .actions = disabled() ? semantics::Action::none
                                  : semantics::Action::activate | semantics::Action::focus,
        };
    }

    auto Switch::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::activate || disabled()) {
            return false;
        }
        activate();
        return true;
    }

    void Switch::apply_metrics() {
        apply_text_style();
        const auto style = resolved_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(
            style.metrics.track_width + style.metrics.gap + text_.width(),
            std::max(style.metrics.min_height, text_.height())
        ));
    }

    void Switch::apply_text_style() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.label.color,
            .font_size =
                context.font_size_from_context ? context.font_size : style.label.font_size,
            .font = context.font_from_context ? context.font : text_.font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        if (!same_text_style(text_.style(), text_style)) {
            text_.set_style(text_style);
        }
    }
} // namespace nandina::widget
