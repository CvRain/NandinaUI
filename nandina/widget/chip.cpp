//
// widget/chip - removable pill tag with optional dismiss action.
//

#include "chip.hpp"

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

    Chip::Chip(std::string text, const bool removable, theme::NanTheme theme):
        text_string_(std::move(text)),
        removable_(removable) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        text_.set_text(text_string_);
        apply_text_style();
    }

    auto Chip::create(std::string text, const bool removable, theme::NanTheme theme)
        -> std::shared_ptr<Chip> {
        return std::make_shared<Chip>(std::move(text), removable, theme);
    }

    void Chip::set_text(std::string text) {
        text_string_ = std::move(text);
        text_.set_text(text_string_);
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Chip::text() const -> std::string_view {
        return text_string_;
    }

    void Chip::set_removable(const bool removable) {
        removable_ = removable;
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Chip::removable() const -> bool {
        return removable_;
    }

    void Chip::set_on_remove(std::function<void()> callback) {
        on_remove_ = std::move(callback);
    }

    auto Chip::removed() const -> const reactive::Event<>& {
        return removed_;
    }

    void Chip::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_style();
        mark_layout_dirty();
    }

    auto Chip::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Chip::set_override(theme::ChipRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_style();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Chip::resolved_style() const -> theme::ResolvedChipStyle {
        auto style = theme::resolve_chip(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Chip::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        mark_layout_dirty();
    }

    void Chip::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Chip::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Chip::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_style();
        mark_layout_dirty();
    }

    void Chip::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_style();
        mark_layout_dirty();
    }

    auto Chip::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::focus_enter) {
            focused_ = removable_;
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            return false;
        }
        if (event.type() == scene::EventType::focus_leave) {
            focused_ = false;
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            return false;
        }
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (!key.is_pressed() || !removable_) {
                return false;
            }
            constexpr int key_enter = 257;
            constexpr int key_space = 32;
            constexpr int key_backspace = 259;
            constexpr int key_delete = 261;
            if (key.keycode() == key_enter || key.keycode() == key_space
                || key.keycode() == key_backspace || key.keycode() == key_delete)
            {
                remove();
                event.accept();
                return true;
            }
            return false;
        }
        if (event.type() != scene::EventType::mouse_button) {
            return false;
        }
        if (!removable_) {
            return false;
        }
        auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
        if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
            return false;
        }
        const auto local = to_local(mouse.screen_pos());
        if (remove_rect().contains_point(local)) {
            remove();
            event.accept();
            return true;
        }
        return false;
    }

    auto Chip::is_focusable() const -> bool {
        return removable_;
    }

    void Chip::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        primitives::BoxPainter::paint(context, world, style.container, context.opacity());
        if (focused_ && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(context, world, style.focus, context.opacity());
        }

        apply_text_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        const float text_height = context.logical_to_screen(text_.measured_text_height());
        const auto text_position = foundation::NanPoint(
            world.get_left() + context.logical_to_screen(style.metrics.padding_x),
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(context, text_position);

        if (removable_) {
            const auto rect = render::world_bounds_from_local(
                context.world_transform(),
                remove_rect()
            );
            const float arm = rect.get_height() * 0.18F;
            const auto color =
                style.remove_color.with_alpha(style.remove_color.alpha() * context.opacity());
            context.device().draw_line(
                foundation::NanPoint(rect.get_left() + arm, rect.get_top() + arm),
                foundation::NanPoint(rect.get_right() - arm, rect.get_bottom() - arm),
                context.logical_to_screen(1.5F),
                color
            );
            context.device().draw_line(
                foundation::NanPoint(rect.get_right() - arm, rect.get_top() + arm),
                foundation::NanPoint(rect.get_left() + arm, rect.get_bottom() - arm),
                context.logical_to_screen(1.5F),
                color
            );
        }
    }

    auto Chip::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        apply_text_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        float width = style.metrics.padding_x * 2.0F + text_.measured_text_width();
        if (removable_) {
            width += style.metrics.gap + style.metrics.height * 0.4F;
        }
        return constraints.constrain(
            foundation::NanSize(width, std::max(style.metrics.min_height, style.metrics.height))
        );
    }

    auto Chip::semantics_properties() const -> semantics::Properties {
        return {
            .role = removable_ ? semantics::Role::button : semantics::Role::generic,
            .label = text_string_,
            .state = {.focusable = removable_, .focused = focused_},
            .actions = removable_ ? semantics::Action::activate : semantics::Action::none,
        };
    }

    void Chip::apply_text_style() {
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

    void Chip::remove() {
        if (on_remove_) {
            on_remove_();
        }
        removed_.emit();
    }

    auto Chip::remove_rect() const -> foundation::NanRect {
        const auto style = resolved_style();
        const float size = style.metrics.height * 0.4F;
        const float right = width() - style.metrics.padding_x;
        return foundation::NanRect::from_xywh(
            right - size,
            (height() - size) * 0.5F,
            size,
            size
        );
    }
} // namespace nandina::widget
