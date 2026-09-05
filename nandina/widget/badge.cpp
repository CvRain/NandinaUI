//
// widget/badge - static pill badge for short labels (pure display, no interaction).
//

#include "badge.hpp"

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

    Badge::Badge(std::string text, theme::NanTheme theme): text_(std::move(text)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        apply_metrics();
    }

    auto Badge::create(std::string text, theme::NanTheme theme) -> std::shared_ptr<Badge> {
        return std::make_shared<Badge>(std::move(text), theme);
    }

    void Badge::set_text(std::string text) {
        text_.set_text(std::move(text));
        apply_metrics();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Badge::text() const -> std::string_view {
        return text_.text();
    }

    void Badge::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_metrics();
        mark_layout_dirty();
    }

    auto Badge::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Badge::set_override(theme::BadgeRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Badge::resolved_style() const -> theme::ResolvedBadgeStyle {
        auto style = theme::resolve_badge(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Badge::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        apply_metrics();
        mark_layout_dirty();
    }

    auto Badge::text_pipeline() const -> primitives::TextPipeline {
        return text_.text_pipeline();
    }

    void Badge::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        apply_metrics();
        mark_layout_dirty();
    }

    void Badge::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        apply_metrics();
        mark_layout_dirty();
    }

    void Badge::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_metrics();
        mark_layout_dirty();
    }

    void Badge::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_metrics();
        mark_layout_dirty();
    }

    void Badge::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        primitives::BoxPainter::paint(context, world, style.container, context.opacity());

        apply_text_style();
        const float text_height = context.logical_to_screen(text_.measured_text_height());
        const float text_width = context.logical_to_screen(text_.measured_text_width());
        const auto text_position = foundation::NanPoint(
            world.get_left() + (world.get_width() - text_width) * 0.5F,
            world.get_top() + (world.get_height() - text_height) * 0.5F
        );
        text_.draw_at(context, text_position);
    }

    auto Badge::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        apply_text_style();
        const auto text_size = text_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = constraints.max_width,
                .min_height = 0.0F,
                .max_height = constraints.max_height,
            }
        );
        return constraints.constrain(foundation::NanSize(
            style.metrics.padding_x * 2.0F + text_size.get_width(),
            std::max(style.metrics.height, text_size.get_height())
        ));
    }

    auto Badge::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::static_text,
            .label = std::string(text()),
        };
    }

    void Badge::apply_metrics() {
        apply_text_style();
        const auto style = resolved_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());
        set_size(foundation::NanSize(
            style.metrics.padding_x * 2.0F + text_.width(),
            std::max(style.metrics.height, text_.height())
        ));
    }

    void Badge::apply_text_style() {
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
