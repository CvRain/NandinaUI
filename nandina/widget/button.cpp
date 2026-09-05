//
// widget/button — first semantic control built from tone/treatment tokens.
//

#include "button.hpp"
#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "primitives/ripple_painter.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{

    namespace
    {
        [[nodiscard]] auto near(float lhs, float rhs) -> bool {
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

    Button::Button(std::string text, theme::NanTheme theme):
        text_(std::move(text)),
        label_presentation_(*this),
        container_presentation_(*this) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        apply_metrics();
    }

    auto Button::create(std::string text, theme::NanTheme theme) -> std::shared_ptr<Button> {
        return std::make_shared<Button>(std::move(text), theme);
    }

    void Button::set_text(std::string text) {
        text_.set_text(std::move(text));
        mark_layout_dirty();
        apply_metrics();
        mark_semantics_dirty();
    }

    auto Button::text() const -> std::string_view {
        return text_.text();
    }

    auto Button::text_node() -> primitives::Text& {
        return text_;
    }

    auto Button::text_node() const -> const primitives::Text& {
        return text_;
    }

    auto Button::visual_part(visual::label_t) noexcept -> primitives::TextPresentation& {
        return label_presentation_;
    }

    auto Button::visual_part(visual::container_t) noexcept -> primitives::BoxPresentation& {
        return container_presentation_;
    }

    void Button::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(pipeline);
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::text_pipeline() const -> primitives::TextPipeline {
        return text_.text_pipeline();
    }

    void Button::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Button::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Button::on_style_context_changed(const theme::ResolvedStyleContext& /*context*/) {
        mark_layout_dirty();
        apply_text_style(visual_state(), height());
    }

    void Button::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        reduced_motion_ = manager.reduced_motion();
        if (reduced_motion_) {
            ripple_origin_local_.reset();
            ripple_progress_ = 1.0F;
        }
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
        apply_metrics();
    }

    void Button::set_font(text::FontRequest request) {
        font_explicit_ = true;
        text_.set_font(std::move(request));
        mark_layout_dirty();
    }

    void Button::set_font_family(resource::ResourceKey family) {
        font_explicit_ = true;
        text_.set_font_family(std::move(family));
        mark_layout_dirty();
    }

    void Button::set_font_weight(const int weight) {
        font_explicit_ = true;
        text_.set_font_weight(weight);
        mark_layout_dirty();
    }

    void Button::set_font_slant(const text::FontSlant slant) {
        font_explicit_ = true;
        text_.set_font_slant(slant);
        mark_layout_dirty();
    }

    void Button::set_font_size(const float size) {
        if (!std::isfinite(size) || size <= 0.0F) {
            throw std::invalid_argument("font size must be finite and positive");
        }
        property::write(*this, visual::label.font_size, size);
        font_size_percent_.reset();
        apply_metrics();
    }

    void Button::set_font_size(const scene::PercentLength size) {
        if (!std::isfinite(size.value) || size.value < 0.0F) {
            throw std::invalid_argument("font size percentage must be finite and non-negative");
        }
        font_size_percent_ = size;
        label_presentation_.clear_font_size();
        mark_layout_dirty();
        apply_metrics();
    }

    void Button::clear_font_size() {
        if (!font_size_percent_.has_value()) {
            label_presentation_.clear_font_size();
            apply_metrics();
            return;
        }
        font_size_percent_.reset();
        label_presentation_.clear_font_size();
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::font_size() const -> float {
        return text_.font_size();
    }

    void Button::set_text_overflow(primitives::TextOverflow overflow) {
        text_overflow_ = overflow;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::text_overflow() const -> primitives::TextOverflow {
        return text_overflow_;
    }

    void Button::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Button::set_override(theme::ButtonRecipeRule rule) {
        override_ = std::move(rule);
        apply_metrics();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    void Button::set_tone(theme::ButtonTone tone) {
        tone_ = tone;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::tone() const -> theme::ButtonTone {
        return tone_;
    }

    void Button::set_treatment(theme::ButtonTreatment treatment) {
        treatment_ = treatment;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::treatment() const -> theme::ButtonTreatment {
        return treatment_;
    }

    void Button::set_button_size(theme::ButtonSize size) {
        size_ = size;
        mark_layout_dirty();
        apply_metrics();
    }

    auto Button::button_size() const -> theme::ButtonSize {
        return size_;
    }

    auto Button::visual_state() const -> theme::ButtonVisualState {
        if (disabled()) {
            return theme::ButtonVisualState::disabled;
        }
        if (pressed()) {
            return theme::ButtonVisualState::pressed;
        }
        if (hovered()) {
            return theme::ButtonVisualState::hovered;
        }
        if (focused()) {
            return theme::ButtonVisualState::focused;
        }
        return theme::ButtonVisualState::normal;
    }

    auto Button::resolved_style() const -> theme::ResolvedButtonStyle {
        auto style = resolved_recipe_style(visual_state());
        label_presentation_.apply(style.label);
        container_presentation_.apply(style.container);
        return style;
    }

    auto Button::ripple_active() const noexcept -> bool {
        return ripple_origin_local_.has_value() && ripple_progress_ < 1.0F;
    }

    auto Button::ripple_progress() const noexcept -> float {
        return ripple_progress_;
    }

    auto Button::on_input(scene::InputEvent& event) -> bool {
        const bool handled = primitives::Pressable::on_input(event);
        if (!handled || event.type() != scene::EventType::mouse_button || disabled()) {
            return handled;
        }
        const auto& mouse = static_cast<const scene::MouseButtonEvent&>(event);
        if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
            return handled;
        }

        const auto style = resolved_style();
        if (reduced_motion_ || style.ripple.duration <= foundation::nan_epsilon) {
            ripple_origin_local_.reset();
            ripple_progress_ = 1.0F;
            return handled;
        }
        ripple_origin_local_ = to_local(mouse.screen_pos());
        ripple_progress_ = 0.0F;
        mark_dirty(scene::DirtyFlags::paint);
        return handled;
    }

    void Button::on_draw(render::DrawContext& ctx) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const float opacity = ctx.opacity();

        // 基础容器、状态层和 ripple 保持为独立图层；border 覆盖在其上，避免
        // 半透明反馈改写描边颜色。
        primitives::BoxPainter::paint_fill(ctx, world, style.container, opacity);
        auto state_layer = style.container;
        state_layer.fill = theme::button_state_layer_color(style, visual_state());
        primitives::BoxPainter::paint_fill(ctx, world, state_layer, opacity);
        if (ripple_active()) {
            primitives::RipplePainter::paint(
                ctx,
                world,
                ctx.world_transform().transform_point(*ripple_origin_local_),
                style.container.radius,
                style.ripple,
                ripple_progress_,
                opacity
            );
        }
        primitives::BoxPainter::paint_outline(ctx, world, style.container, opacity);

        apply_text_style(visual_state(), height());
        // 文本布局始终使用逻辑尺寸；world 已包含视口变换，不能再作为 measure 输入，
        // 否则缩放后的文本宽度会污染下一次逻辑布局并造成组件尺寸振荡。
        const float content_width =
            std::max(0.0F, width() - style.metrics.padding_x * 2.0F);
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = content_width,
                .min_height = 0.0F,
                .max_height = height(),
            }
        );
        const float text_width = ctx.logical_to_screen(text_.measured_text_width());
        const float text_height = ctx.logical_to_screen(text_.measured_text_height());
        const auto text_pos = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(ctx, text_pos);

        if (focused() && !disabled() && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(ctx, world, style.focus, opacity);
        }
    }

    void Button::on_pressable_state_changed() {
        if (disabled()) {
            ripple_origin_local_.reset();
            ripple_progress_ = 1.0F;
        }
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout
            | scene::DirtyFlags::semantics
        );
    }

    void Button::on_process(const float dt) {
        if (!ripple_active()) {
            return;
        }
        const float duration = resolved_style().ripple.duration;
        if (reduced_motion_ || duration <= foundation::nan_epsilon) {
            ripple_origin_local_.reset();
            ripple_progress_ = 1.0F;
        }
        else {
            ripple_progress_ = std::clamp(
                ripple_progress_ + std::max(dt, 0.0F) / duration,
                0.0F,
                1.0F
            );
            if (ripple_progress_ >= 1.0F) {
                ripple_origin_local_.reset();
            }
        }
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Button::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::button,
            .label = std::string(text()),
            .state = {
                .focusable = !disabled(),
                .focused = focused(),
                .disabled = disabled(),
            },
            .actions = disabled() ? semantics::Action::none
                                  : semantics::Action::activate | semantics::Action::focus,
        };
    }

    auto Button::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::activate || disabled()) {
            return false;
        }
        activate();
        return true;
    }

    auto Button::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto state = visual_state();
        const auto style = resolved_style();
        // 百分比字号的基准 = 本次布局后按钮的最终高度（高度由 size_spec/配方决定）。
        const float reference_height =
            constraints.constrain(foundation::NanSize(0.0F, style.metrics.height)).get_height();
        apply_text_style(state, reference_height);
        const float max_text_width = std::isfinite(constraints.max_width)
            ? std::max(0.0F, constraints.max_width - style.metrics.padding_x * 2.0F)
            : constraints.max_width;
        (void)text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = max_text_width,
                .min_height = 0.0F,
                .max_height = style.metrics.height,
            }
        );
        return constraints.constrain(foundation::NanSize(
            text_.width() + style.metrics.padding_x * 2.0F,
            style.metrics.height
        ));
    }

    void Button::apply_metrics() {
        const auto state = visual_state();
        auto style = resolved_recipe_style(state);
        label_presentation_.apply(style.label);
        container_presentation_.apply(style.container);
        apply_text_style(state, height());
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(
            text_.width() + style.metrics.padding_x * 2.0F,
            style.metrics.height
        ));
    }

    auto Button::resolved_recipe_style(const theme::ButtonVisualState state) const
        -> theme::ResolvedButtonStyle {
        auto style =
            theme::resolve_button(*system_, appearance_, tone_, treatment_, size_, state);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_, tone_);
        }
        return style;
    }

    auto Button::resolved_font_size(const float reference_height, const float fallback) const
        -> float {
        if (font_size_percent_.has_value() && reference_height > 0.0F) {
            return std::max(1.0F, reference_height * font_size_percent_->value * 0.01F);
        }
        return fallback;
    }

    void Button::apply_text_style(
        const theme::ButtonVisualState state,
        const float reference_height
    ) {
        const auto style = resolved_recipe_style(state);
        const auto& context = resolved_style_context();
        primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.label.color,
            .font_size = resolved_font_size(
                reference_height,
                context.font_size_from_context ? context.font_size : style.label.font_size
            ),
            .font = context.font_from_context && !font_explicit_ ? context.font : text_.font(),
            .overflow = text_overflow_,
            .max_lines = 1,
        };
        label_presentation_.apply(text_style);
        if (same_text_style(text_.style(), text_style)) {
            return;
        }
        text_.set_style(text_style);
    }

} // namespace nandina::widget
