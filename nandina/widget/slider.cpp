//
// widget/slider - semantic continuous numeric input control.
//

#include "slider.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace nandina::widget
{
    namespace
    {
        constexpr int key_right = 262;
        constexpr int key_left = 263;
        constexpr int key_down = 264;
        constexpr int key_up = 265;
        constexpr int key_home = 268;
        constexpr int key_end = 269;

        [[nodiscard]] auto numeric_text(const float value) -> std::string {
            char buffer[32] {};
            const auto [end, error] = std::to_chars(
                std::begin(buffer),
                std::end(buffer),
                value,
                std::chars_format::general,
                6
            );
            return error == std::errc {} ? std::string(buffer, end) : std::string {};
        }
    } // namespace

    Slider::Slider(
        std::string label,
        const float value,
        const float minimum,
        const float maximum,
        const float step,
        theme::NanTheme theme
    ):
        label_(std::move(label)),
        minimum_(minimum),
        maximum_(maximum),
        step_(step) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        if (!(minimum_ < maximum_)) {
            throw std::invalid_argument("Slider minimum must be less than maximum");
        }
        if (!(step_ > 0.0F) || !std::isfinite(step_)) {
            throw std::invalid_argument("Slider step must be finite and positive");
        }
        value_ = normalized(value);
        value_text_.set_text(numeric_text(value_));
        const auto style = resolved_style();
        set_size(foundation::NanSize(style.metrics.preferred_width, style.metrics.min_height));
    }

    auto Slider::create(
        std::string label,
        const float value,
        const float minimum,
        const float maximum,
        const float step,
        theme::NanTheme theme
    ) -> std::shared_ptr<Slider> {
        return std::make_shared<Slider>(std::move(label), value, minimum, maximum, step, theme);
    }

    void Slider::set_label(std::string label) {
        label_ = std::move(label);
        mark_semantics_dirty();
    }

    auto Slider::label() const -> std::string_view {
        return label_;
    }

    void Slider::set_value(const float value) {
        const float next = normalized(value);
        if (std::abs(value_ - next) <= foundation::nan_epsilon) {
            return;
        }
        value_ = next;
        value_text_.set_text(numeric_text(value_));
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Slider::value() const -> float {
        return value_;
    }

    void Slider::set_range(const float minimum, const float maximum) {
        if (!(minimum < maximum)) {
            throw std::invalid_argument("Slider minimum must be less than maximum");
        }
        minimum_ = minimum;
        maximum_ = maximum;
        value_ = normalized(value_);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Slider::minimum() const -> float {
        return minimum_;
    }

    auto Slider::maximum() const -> float {
        return maximum_;
    }

    void Slider::set_step(const float step) {
        if (!(step > 0.0F) || !std::isfinite(step)) {
            throw std::invalid_argument("Slider step must be finite and positive");
        }
        step_ = step;
        value_ = normalized(value_);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Slider::step() const -> float {
        return step_;
    }

    void Slider::set_disabled(const bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        if (disabled_) {
            hovered_ = false;
            dragging_ = false;
            focused_ = false;
            if (is_inside_tree()) {
                get_tree()->release_pointer_capture(this);
                if (get_tree()->focused_node() == this) {
                    get_tree()->set_focus(nullptr);
                }
            }
        }
        update_visual_state();
    }

    auto Slider::disabled() const -> bool {
        return disabled_;
    }

    void Slider::set_on_change(std::function<void(float)> callback) {
        on_change_ = std::move(callback);
    }

    auto Slider::value_changed() const -> const reactive::Event<float>& {
        return value_changed_;
    }

    void Slider::set_show_value_label(const bool show) {
        if (show_value_label_ == show) {
            return;
        }
        show_value_label_ = show;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Slider::show_value_label() const -> bool {
        return show_value_label_;
    }

    auto Slider::value_label_text() const -> std::string_view {
        return value_text_.text();
    }

    void Slider::set_text_pipeline(primitives::TextPipeline pipeline) {
        value_text_.set_text_pipeline(std::move(pipeline));
        if (show_value_label_) {
            mark_layout_dirty();
        }
    }

    auto Slider::text_pipeline() const -> primitives::TextPipeline {
        return value_text_.text_pipeline();
    }

    void Slider::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        value_text_.apply_default_text_pipeline(pipeline);
        if (show_value_label_) {
            mark_layout_dirty();
        }
    }

    void Slider::apply_font_context(text::FontPipelineCache& context) {
        value_text_.apply_font_context(context);
        if (show_value_label_) {
            mark_layout_dirty();
        }
    }

    void Slider::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_value_label_style();
        if (show_value_label_) {
            mark_layout_dirty();
        }
    }

    void Slider::apply_value_label_style() {
        const auto& context = resolved_style_context();
        const primitives::TextStyle style {
            .color = context.text_color_from_context
                ? context.text_color
                : system_->palette(appearance_).on_surface_variant,
            .font_size = 12.0F,
            .font = context.font_from_context ? context.font : value_text_.font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        const auto& current = value_text_.style();
        if (current.color.approx_equals(style.color)
            && std::abs(current.font_size - style.font_size) <= foundation::nan_epsilon
            && current.font == style.font)
        {
            return;
        }
        value_text_.set_style(style);
    }

    void Slider::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
    }

    auto Slider::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Slider::set_override(theme::SliderRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Slider::visual_state() const -> theme::SliderVisualState {
        if (disabled_) {
            return theme::SliderVisualState::disabled;
        }
        if (dragging_) {
            return theme::SliderVisualState::dragging;
        }
        if (hovered_) {
            return theme::SliderVisualState::hovered;
        }
        if (focused_) {
            return theme::SliderVisualState::focused;
        }
        return theme::SliderVisualState::normal;
    }

    auto Slider::resolved_style() const -> theme::ResolvedSliderStyle {
        auto style = theme::resolve_slider(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Slider::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
    }

    auto Slider::is_focusable() const -> bool {
        return !disabled_;
    }

    auto Slider::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::focus_enter) {
            focused_ = !disabled_;
            update_visual_state();
            return false;
        }
        if (event.type() == scene::EventType::focus_leave) {
            focused_ = false;
            dragging_ = false;
            if (is_inside_tree()) {
                get_tree()->release_pointer_capture(this);
            }
            update_visual_state();
            return false;
        }
        if (disabled_) {
            return false;
        }

        switch (event.type()) {
            case scene::EventType::mouse_enter:
                hovered_ = true;
                update_visual_state();
                return false;
            case scene::EventType::mouse_leave:
                hovered_ = false;
                update_visual_state();
                return false;
            case scene::EventType::mouse_button: {
                auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
                if (mouse.button() != scene::MouseButtonEvent::Button::left) {
                    return false;
                }
                update_from_pointer(mouse.screen_pos());
                dragging_ = mouse.is_pressed();
                if (dragging_) {
                    get_tree()->set_pointer_capture(this);
                }
                else {
                    get_tree()->release_pointer_capture(this);
                }
                update_visual_state();
                event.accept();
                return true;
            }
            case scene::EventType::mouse_move:
                if (dragging_) {
                    update_from_pointer(static_cast<scene::MouseMoveEvent&>(event).screen_pos());
                    event.accept();
                    return true;
                }
                return false;
            case scene::EventType::key: {
                const auto& key = static_cast<scene::KeyEvent&>(event);
                if (!key.is_pressed()) {
                    return false;
                }
                if (key.keycode() == key_left || key.keycode() == key_down) {
                    set_user_value(value_ - step_);
                }
                else if (key.keycode() == key_right || key.keycode() == key_up) {
                    set_user_value(value_ + step_);
                }
                else if (key.keycode() == key_home) {
                    set_user_value(minimum_);
                }
                else if (key.keycode() == key_end) {
                    set_user_value(maximum_);
                }
                else {
                    return false;
                }
                event.accept();
                return true;
            }
            case scene::EventType::mouse_wheel:
            case scene::EventType::text_input:
            case scene::EventType::focus_enter:
            case scene::EventType::focus_leave:
                return false;
        }
        return false;
    }

    void Slider::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        float track_inset = 0.0F;
        if (show_value_label_ && !value_text_.text().empty()) {
            apply_value_label_style();
            (void)value_text_.measure_layout(scene::LayoutConstraints::loose());
            const float label_height = context.logical_to_screen(value_text_.measured_text_height());
            value_text_.draw_at(context, foundation::NanPoint(world.get_left(), world.get_top()));
            track_inset = label_height + context.logical_to_screen(4.0F);
        }
        const float center_y = world.get_top() + track_inset
            + std::max(0.0F, world.get_height() - track_inset) * 0.5F;
        const float thumb_radius = context.logical_to_screen(style.thumb.box.radius);
        const float track_height = context.logical_to_screen(style.inactive_track.thickness);
        const float left = world.get_left() + thumb_radius;
        const float right = world.get_right() - thumb_radius;
        const float thumb_x = left + (right - left) * fraction();
        const float opacity = context.opacity();
        const auto inactive = foundation::NanRect::from_xywh(
            left,
            center_y - track_height * 0.5F,
            std::max(0.0F, right - left),
            track_height
        );
        const auto active = foundation::NanRect::from_xywh(
            left,
            center_y - track_height * 0.5F,
            std::max(0.0F, thumb_x - left),
            track_height
        );
        primitives::BoxPainter::paint_fill(context, inactive, style.inactive_track.box, opacity);
        primitives::BoxPainter::paint_fill(context, active, style.active_track.box, opacity);
        context.device().draw_circle(
            foundation::NanPoint(thumb_x, center_y),
            thumb_radius,
            style.thumb.box.fill.with_alpha(style.thumb.box.fill.alpha() * opacity)
        );
        if (focused_ && !disabled_ && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(context, world, style.focus, opacity);
        }
    }

    auto Slider::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        const float width = std::isfinite(constraints.max_width)
            ? constraints.max_width
            : style.metrics.preferred_width;
        float extra_height = 0.0F;
        if (show_value_label_ && !value_text_.text().empty()) {
            apply_value_label_style();
            (void)value_text_.measure_layout(scene::LayoutConstraints::loose());
            extra_height = value_text_.measured_text_height() + 4.0F;
        }
        return constraints.constrain(foundation::NanSize(width, style.metrics.min_height + extra_height));
    }

    auto Slider::semantics_properties() const -> semantics::Properties {
        auto actions = semantics::Action::none;
        if (!disabled_) {
            actions = semantics::Action::focus | semantics::Action::set_value
                | semantics::Action::increment | semantics::Action::decrement;
        }
        return {
            .role = semantics::Role::slider,
            .label = label_,
            .value = numeric_text(value_),
            .hint = numeric_text(minimum_) + " to " + numeric_text(maximum_),
            .state =
                {
                    .focusable = !disabled_,
                    .focused = focused_,
                    .disabled = disabled_,
                },
            .actions = actions,
        };
    }

    auto Slider::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (disabled_) {
            return false;
        }
        if (request.action == semantics::Action::increment) {
            set_user_value(value_ + step_);
            return true;
        }
        if (request.action == semantics::Action::decrement) {
            set_user_value(value_ - step_);
            return true;
        }
        if (request.action != semantics::Action::set_value) {
            return false;
        }
        float requested = 0.0F;
        const char* begin = request.value.data();
        const char* end = begin + request.value.size();
        const auto [parsed, error] = std::from_chars(begin, end, requested);
        if (error != std::errc {} || parsed != end || !std::isfinite(requested)) {
            return false;
        }
        set_user_value(requested);
        return true;
    }

    auto Slider::normalized(const float value) const -> float {
        const float finite = std::isfinite(value) ? value : minimum_;
        const float clamped = std::clamp(finite, minimum_, maximum_);
        const float steps = std::round((clamped - minimum_) / step_);
        return std::clamp(minimum_ + steps * step_, minimum_, maximum_);
    }

    auto Slider::fraction() const -> float {
        return (value_ - minimum_) / (maximum_ - minimum_);
    }

    void Slider::set_user_value(const float value) {
        const float before = value_;
        set_value(value);
        if (std::abs(before - value_) <= foundation::nan_epsilon) {
            return;
        }
        if (on_change_) {
            on_change_(value_);
        }
        value_changed_.emit(value_);
    }

    void Slider::update_from_pointer(const foundation::NanPoint screen_position) {
        const auto style = resolved_style();
        const auto local = to_local(screen_position);
        const float thumb_radius = style.thumb.box.radius;
        const float width = std::max(0.0F, size().get_width() - thumb_radius * 2.0F);
        const float current = width <= foundation::nan_epsilon
            ? minimum_
            : minimum_ + std::clamp((local.get_x() - thumb_radius) / width, 0.0F, 1.0F)
                  * (maximum_ - minimum_);
        set_user_value(current);
    }

    void Slider::update_visual_state() {
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }
} // namespace nandina::widget
