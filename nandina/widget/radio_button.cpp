//
// widget/radio_button - semantic single-choice control (radio group member).
//

#include "radio_button.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        constexpr int key_right = 262;
        constexpr int key_left = 263;
        constexpr int key_down = 264;
        constexpr int key_up = 265;

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

    RadioButton::RadioButton(
        std::string label,
        std::shared_ptr<RadioGroup> group,
        theme::NanTheme theme
    ):
        text_(std::move(label)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        set_group(std::move(group));
        apply_metrics();
    }

    RadioButton::~RadioButton() {
        if (group_) {
            group_->unregister_radio(this);
        }
    }

    auto RadioButton::create(
        std::string label,
        std::shared_ptr<RadioGroup> group,
        theme::NanTheme theme
    ) -> std::shared_ptr<RadioButton> {
        return std::make_shared<RadioButton>(std::move(label), std::move(group), theme);
    }

    void RadioButton::set_label(std::string label) {
        text_.set_text(std::move(label));
        apply_metrics();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto RadioButton::label() const -> std::string_view {
        return text_.text();
    }

    void RadioButton::set_checked(const bool checked) {
        if (checked_ == checked) {
            return;
        }
        checked_ = checked;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto RadioButton::checked() const -> bool {
        return checked_;
    }

    void RadioButton::select() {
        if (disabled()) {
            return;
        }
        const bool previous = checked_;
        if (group_) {
            group_->select(this);
        }
        else if (!checked_) {
            set_checked(true);
        }
        if (checked_ && !previous) {
            if (on_change_) {
                on_change_(true);
            }
            checked_changed_.emit(true);
        }
    }

    void RadioButton::set_group(std::shared_ptr<RadioGroup> group) {
        if (group_ == group) {
            return;
        }
        if (group_) {
            group_->unregister_radio(this);
        }
        group_ = std::move(group);
        if (group_) {
            group_->register_radio(this);
        }
    }

    auto RadioButton::group() const -> const std::shared_ptr<RadioGroup>& {
        return group_;
    }

    void RadioButton::set_on_change(std::function<void(bool)> callback) {
        on_change_ = std::move(callback);
    }

    auto RadioButton::checked_changed() const -> const reactive::Event<bool>& {
        return checked_changed_;
    }

    void RadioButton::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_metrics();
        mark_layout_dirty();
    }

    auto RadioButton::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void RadioButton::set_override(theme::RadioButtonRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto RadioButton::visual_state() const -> theme::RadioButtonVisualState {
        if (disabled()) {
            return theme::RadioButtonVisualState::disabled;
        }
        if (pressed()) {
            return theme::RadioButtonVisualState::pressed;
        }
        if (hovered()) {
            return theme::RadioButtonVisualState::hovered;
        }
        if (focused()) {
            return theme::RadioButtonVisualState::focused;
        }
        return theme::RadioButtonVisualState::normal;
    }

    auto RadioButton::resolved_style() const -> theme::ResolvedRadioButtonStyle {
        auto style = theme::resolve_radio_button(*system_, appearance_, checked_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void RadioButton::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        apply_metrics();
        mark_layout_dirty();
    }

    auto RadioButton::text_pipeline() const -> primitives::TextPipeline {
        return text_.text_pipeline();
    }

    void RadioButton::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        apply_metrics();
        mark_layout_dirty();
    }

    void RadioButton::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        apply_metrics();
        mark_layout_dirty();
    }

    void RadioButton::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_metrics();
        mark_layout_dirty();
    }

    void RadioButton::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_metrics();
        mark_layout_dirty();
    }

    auto RadioButton::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::key && group_) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (key.is_pressed()) {
                int direction = 0;
                if (key.keycode() == key_left || key.keycode() == key_up) {
                    direction = -1;
                }
                else if (key.keycode() == key_right || key.keycode() == key_down) {
                    direction = 1;
                }
                if (direction != 0 && group_->move_focus(this, direction)) {
                    event.accept();
                    return true;
                }
            }
        }
        return primitives::Pressable::on_input(event);
    }

    void RadioButton::on_draw(render::DrawContext& context) {
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
            const auto dot = style.dot.with_alpha(style.dot.alpha() * opacity);
            context.device().draw_circle(
                foundation::NanPoint(
                    box.get_left() + box_size * 0.5F,
                    box.get_top() + box_size * 0.5F
                ),
                context.logical_to_screen(style.metrics.box_size * 0.25F),
                dot
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

    auto RadioButton::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
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

    void RadioButton::on_click() {
        select();
    }

    void RadioButton::on_pressable_state_changed() {
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto RadioButton::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::radio,
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

    auto RadioButton::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::activate || disabled()) {
            return false;
        }
        activate();
        return true;
    }

    void RadioButton::apply_metrics() {
        apply_text_style();
        const auto style = resolved_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(
            style.metrics.box_size + style.metrics.gap + text_.width(),
            std::max(style.metrics.min_height, text_.height())
        ));
    }

    void RadioButton::apply_text_style() {
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
