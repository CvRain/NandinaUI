// A Surface reuses Padding layout, BoxPainter drawing and BoxPresentation overrides.
#include "surface.hpp"

#include "../theme/theme_manager.hpp"
#include "internal/inset_validation.hpp"
#include "primitives/box_painter.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        auto resolve_box(const theme::NanTheme& theme, const theme::BoxStyle& style)
            -> theme::ResolvedBoxStyle {
            const float border = theme::resolve_theme_scalar(theme, style.border_width);
            const float radius = theme::resolve_theme_scalar(theme, style.radius);
            if (!std::isfinite(border) || border < 0.0F || !std::isfinite(radius) || radius < 0.0F)
            {
                throw std::invalid_argument(
                    "Surface border width and radius must be finite and non-negative"
                );
            }
            return {
                .fill = theme::resolve_theme_color(theme, style.fill),
                .border = theme::resolve_theme_color(theme, style.border),
                .border_width = border,
                .radius = radius,
            };
        }
    } // namespace

    Surface::Surface(theme::NanTheme theme): theme_(std::move(theme)), presentation_(*this) {}

    auto Surface::create(theme::NanTheme theme) -> std::shared_ptr<Surface> {
        return std::make_shared<Surface>(std::move(theme));
    }

    auto Surface::set_padding(const foundation::NanInsets insets) -> Surface& {
        internal::require_content_insets(insets);
        if (padding() != insets) {
            Padding::set_padding(insets);
        }
        return *this;
    }

    void Surface::set_box_style(theme::BoxStyle style) {
        (void)resolve_box(theme_, style); // Validate before replacing the author's configuration.
        box_ = std::move(style);
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Surface::box_style() const noexcept -> const theme::BoxStyle& {
        return box_;
    }

    auto Surface::resolved_style() const -> theme::ResolvedBoxStyle {
        auto result = resolve_box(theme_, box_);
        presentation_.apply(result);
        return result;
    }

    auto Surface::visual_part(visual::container_t) noexcept -> primitives::BoxPresentation& {
        return presentation_;
    }

    void Surface::on_theme_changed(const theme::ThemeManager& manager) {
        theme_ = manager.theme();
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Surface::on_draw(render::DrawContext& context) {
        primitives::BoxPainter::paint(
            context,
            render::world_bounds_from_local(context.world_transform(), local_rect()),
            resolved_style(),
            context.opacity()
        );
    }
} // namespace nandina::widget
