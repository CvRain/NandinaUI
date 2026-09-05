//
// widget/tooltip - hover-triggered floating hint over a trigger control.
//

#include "tooltip.hpp"

#include "primitives/box_painter.hpp"
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

    Tooltip::Tooltip(
        std::string text,
        std::shared_ptr<scene::NanControl> trigger,
        theme::NanTheme theme
    ):
        text_(std::move(text)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        if (trigger) {
            set_trigger(std::move(trigger));
        }
        apply_text_style();
    }

    auto Tooltip::create(
        std::string text,
        std::shared_ptr<scene::NanControl> trigger,
        theme::NanTheme theme
    ) -> std::shared_ptr<Tooltip> {
        return std::make_shared<Tooltip>(std::move(text), std::move(trigger), theme);
    }

    void Tooltip::set_text(std::string text) {
        text_.set_text(std::move(text));
        apply_text_style();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Tooltip::text() const -> std::string_view {
        return text_.text();
    }

    auto Tooltip::set_trigger(std::shared_ptr<scene::NanControl> trigger) -> Tooltip& {
        if (!trigger) {
            throw std::runtime_error("Tooltip::set_trigger: trigger is null");
        }
        auto current = trigger_.lock();
        trigger_ = trigger;
        replace_child(current.get(), std::move(trigger));
        mark_layout_dirty();
        return *this;
    }

    void Tooltip::set_placement(const Placement placement) {
        placement_ = placement;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Tooltip::placement() const -> Placement {
        return placement_;
    }

    void Tooltip::set_delay(const float seconds) {
        if (!std::isfinite(seconds) || seconds < 0.0F) {
            throw std::invalid_argument("tooltip delay must be finite and non-negative");
        }
        delay_ = seconds;
    }

    auto Tooltip::delay() const -> float {
        return delay_;
    }

    auto Tooltip::visible() const -> bool {
        return visible_;
    }

    void Tooltip::show() {
        if (visible_) {
            return;
        }
        visible_ = true;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void Tooltip::hide() {
        if (!visible_) {
            return;
        }
        visible_ = false;
        hover_time_ = 0.0F;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void Tooltip::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_style();
        mark_layout_dirty();
    }

    auto Tooltip::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Tooltip::set_override(theme::TooltipRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_style();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Tooltip::resolved_style() const -> theme::ResolvedTooltipStyle {
        auto style = theme::resolve_tooltip(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Tooltip::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        mark_layout_dirty();
    }

    void Tooltip::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Tooltip::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Tooltip::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_style();
        mark_layout_dirty();
    }

    void Tooltip::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_style();
        mark_layout_dirty();
    }

    auto Tooltip::on_input(scene::InputEvent& event) -> bool {
        // 只观察悬停，不消费输入——触发控件仍需接收点击/键盘。
        if (event.type() == scene::EventType::mouse_enter) {
            hovered_ = true;
            hover_time_ = 0.0F;
            return false;
        }
        if (event.type() == scene::EventType::mouse_leave) {
            hovered_ = false;
            hide();
            return false;
        }
        return false;
    }

    void Tooltip::on_process(const float dt) {
        if (!hovered_ || visible_ || delay_ <= 0.0F) {
            if (hovered_ && delay_ <= 0.0F) {
                show();
            }
            return;
        }
        hover_time_ += std::max(dt, 0.0F);
        if (hover_time_ >= delay_) {
            show();
        }
    }

    void Tooltip::on_draw(render::DrawContext& context) {
        if (!visible_) {
            return;
        }
        const auto style = resolved_style();
        apply_text_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());

        const float padding_x = style.metrics.padding_x;
        const float bubble_w = text_.measured_text_width() + padding_x * 2.0F;
        const float bubble_h = style.metrics.min_height;
        const float bubble_x = (width() - bubble_w) * 0.5F;
        const float bubble_y = placement_ == Placement::top
            ? -(style.metrics.gap + bubble_h)
            : height() + style.metrics.gap;

        const auto local_bubble = foundation::NanRect::from_xywh(
            bubble_x,
            bubble_y,
            bubble_w,
            bubble_h
        );
        const auto world =
            render::world_bounds_from_local(context.world_transform(), local_bubble);
        primitives::BoxPainter::paint(context, world, style.container, context.opacity());

        const float text_height = context.logical_to_screen(text_.measured_text_height());
        const float text_width = context.logical_to_screen(text_.measured_text_width());
        const auto text_position = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(context, text_position);
    }

    auto Tooltip::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        auto trigger = trigger_.lock();
        if (!trigger) {
            return constraints.constrain(foundation::NanSize(0.0F, 0.0F));
        }
        return constraints.constrain(trigger->measure_layout(constraints));
    }

    auto Tooltip::on_layout() -> void {
        auto trigger = trigger_.lock();
        if (!trigger) {
            return;
        }
        trigger->layout_to(local_rect());
    }

    auto Tooltip::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::tooltip,
            .label = std::string(text()),
        };
    }

    void Tooltip::apply_text_style() {
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
