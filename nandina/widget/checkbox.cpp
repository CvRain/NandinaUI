//
// widget/checkbox - semantic boolean selection control.
//

#include "checkbox.hpp"

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

    Checkbox::Checkbox(std::string label, const bool checked, theme::NanTheme theme):
        text_(std::move(label)),
        checked_(checked) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        apply_metrics();
    }

    auto Checkbox::create(std::string label, const bool checked, theme::NanTheme theme)
        -> std::shared_ptr<Checkbox> {
        return std::make_shared<Checkbox>(std::move(label), checked, theme);
    }

    void Checkbox::set_label(std::string label) {
        text_.set_text(std::move(label));
        apply_metrics();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Checkbox::label() const -> std::string_view {
        return text_.text();
    }

    void Checkbox::set_checked(const bool checked) {
        if (checked_ == checked) {
            return;
        }
        checked_ = checked;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Checkbox::checked() const -> bool {
        return checked_;
    }

    void Checkbox::toggle() {
        if (disabled()) {
            return;
        }
        set_checked(!checked_);
        if (on_change_) {
            on_change_(checked_);
        }
        checked_changed_.emit(checked_);
    }

    void Checkbox::set_on_change(std::function<void(bool)> callback) {
        on_change_ = std::move(callback);
    }

    auto Checkbox::checked_changed() const -> const reactive::Event<bool>& {
        return checked_changed_;
    }

    void Checkbox::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_metrics();
        mark_layout_dirty();
    }

    auto Checkbox::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Checkbox::set_override(theme::CheckboxRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Checkbox::visual_state() const -> theme::CheckboxVisualState {
        if (disabled()) {
            return theme::CheckboxVisualState::disabled;
        }
        if (pressed()) {
            return theme::CheckboxVisualState::pressed;
        }
        if (hovered()) {
            return theme::CheckboxVisualState::hovered;
        }
        if (focused()) {
            return theme::CheckboxVisualState::focused;
        }
        return theme::CheckboxVisualState::normal;
    }

    auto Checkbox::resolved_style() const -> theme::ResolvedCheckboxStyle {
        auto style = theme::resolve_checkbox(*system_, appearance_, checked_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Checkbox::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        apply_metrics();
        mark_layout_dirty();
    }

    auto Checkbox::text_pipeline() const -> primitives::TextPipeline {
        return text_.text_pipeline();
    }

    void Checkbox::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        apply_metrics();
        mark_layout_dirty();
    }

    void Checkbox::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        apply_metrics();
        mark_layout_dirty();
    }

    void Checkbox::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_metrics();
        mark_layout_dirty();
    }

    void Checkbox::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_metrics();
        mark_layout_dirty();
    }

    void Checkbox::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        const float box_size = context.logical_to_screen(style.metrics.box_size);
        const float box_top = world.get_top() + (world.get_height() - box_size) * 0.5F;
        const auto box = foundation::NanRect::from_xywh(
            world.get_left(),
            box_top,
            box_size,
            box_size
        );
        const float opacity = context.opacity();

        primitives::BoxPainter::paint(context, box, style.indicator, opacity);
        if (checked_) {
            const auto check = style.check.with_alpha(style.check.alpha() * opacity);
            context.device().draw_line(
                foundation::NanPoint(
                    box.get_left() + box_size * 0.22F,
                    box.get_top() + box_size * 0.52F
                ),
                foundation::NanPoint(
                    box.get_left() + box_size * 0.43F,
                    box.get_top() + box_size * 0.72F
                ),
                context.logical_to_screen(2.0F),
                check
            );
            context.device().draw_line(
                foundation::NanPoint(
                    box.get_left() + box_size * 0.43F,
                    box.get_top() + box_size * 0.72F
                ),
                foundation::NanPoint(
                    box.get_left() + box_size * 0.80F,
                    box.get_top() + box_size * 0.30F
                ),
                context.logical_to_screen(2.0F),
                check
            );
        }

        apply_text_style();
        const float text_height = context.logical_to_screen(text_.measured_text_height());
        const auto text_position = foundation::NanPoint(
            box.get_right() + context.logical_to_screen(style.metrics.gap),
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(context, text_position);

        if (focused() && !disabled() && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(context, world, style.focus, opacity);
        }
    }

    auto Checkbox::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        apply_text_style();
        const float text_width = std::isfinite(constraints.max_width)
            ? std::max(0.0F, constraints.max_width - style.metrics.box_size - style.metrics.gap)
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
            style.metrics.box_size + style.metrics.gap + text_size.get_width(),
            std::max(style.metrics.min_height, text_size.get_height())
        ));
    }

    void Checkbox::on_click() {
        toggle();
    }

    void Checkbox::on_pressable_state_changed() {
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Checkbox::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::checkbox,
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

    auto Checkbox::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::activate || disabled()) {
            return false;
        }
        activate();
        return true;
    }

    void Checkbox::apply_metrics() {
        apply_text_style();
        const auto style = resolved_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(
            style.metrics.box_size + style.metrics.gap + text_.width(),
            std::max(style.metrics.min_height, text_.height())
        ));
    }

    void Checkbox::apply_text_style() {
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
