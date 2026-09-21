//
// widget/internal/breadcrumb_link - Breadcrumb 内部的可聚焦链接条目。
//

#include "breadcrumb_link.hpp"

#include "../../render/draw_context.hpp"
#include "../primitives/box_painter.hpp"
#include "../primitives/focus_ring_painter.hpp"

#include <algorithm>
#include <utility>

namespace nandina::widget::internal
{
    namespace
    {
        /// pressed 状态底色强度（对 hover 文字色取 alpha）：组件几何常量，
        /// 无语义标量 token，与 AlertDismissButton 的状态层同款处理。
        constexpr float kPressedAlpha = 0.20F;
    } // namespace

    BreadcrumbLink::BreadcrumbLink(std::string label): text_(std::move(label)) {}

    void BreadcrumbLink::set_label(std::string label) {
        if (text_.text() == label) {
            return;
        }
        text_.set_text(std::move(label));
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto BreadcrumbLink::label() const -> std::string_view {
        return text_.text();
    }

    void BreadcrumbLink::set_text_style(primitives::TextStyle style) {
        text_.set_style(std::move(style));
        mark_layout_dirty();
    }

    auto BreadcrumbLink::text_style() const -> const primitives::TextStyle& {
        return text_.style();
    }

    auto BreadcrumbLink::font() const -> const text::FontRequest& {
        return text_.font();
    }

    void BreadcrumbLink::set_hover_color(foundation::NanColor color) {
        hover_color_ = color;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void BreadcrumbLink::set_focus_ring(theme::ResolvedFocusRing ring) {
        focus_ = ring;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void BreadcrumbLink::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void BreadcrumbLink::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    auto BreadcrumbLink::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        (void)text_.measure_layout(constraints);
        return constraints.constrain(foundation::NanSize(
            std::max(1.0F, text_.measured_text_width()),
            std::max(1.0F, text_.measured_text_height())
        ));
    }

    void BreadcrumbLink::on_pressable_state_changed() {
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto BreadcrumbLink::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::button,
            .label = std::string(text_.text()),
            .state = {.focusable = !disabled(), .focused = focused(), .disabled = disabled()},
            .actions = disabled() ? semantics::Action::none : semantics::Action::activate,
        };
    }

    void BreadcrumbLink::on_draw(render::DrawContext& context) {
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());

        // hover / pressed 只改文字色（与 shadcn BreadcrumbLink 的 hover:text-foreground
        // 一致）；pressed 再叠一层低透明度底色，保证 pressed 与 hover 可区分。
        const bool active = hovered() || pressed();
        if (pressed() && hover_color_.alpha() > 0.0F) {
            primitives::BoxPainter::paint_fill(
                context,
                world,
                theme::ResolvedBoxStyle {
                    .fill = hover_color_.with_alpha(hover_color_.alpha() * kPressedAlpha),
                    .border = hover_color_.with_alpha(0.0F),
                    .border_width = 0.0F,
                    .radius = 0.0F,
                },
                context.opacity()
            );
        }
        if (active) {
            text_.set_color(hover_color_);
        }
        text_.draw_at(context, world.get_top_left());

        if (focused()) {
            primitives::FocusRingPainter::paint(context, world, focus_, context.opacity());
        }
    }
} // namespace nandina::widget::internal
