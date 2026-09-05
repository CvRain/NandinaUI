//
// widget/divider - horizontal/vertical separator line (pure display).
//

#include "divider.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    Divider::Divider(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
    }

    auto Divider::create(theme::NanTheme theme) -> std::shared_ptr<Divider> {
        return std::make_shared<Divider>(theme);
    }

    void Divider::set_orientation(const Orientation orientation) {
        orientation_ = orientation;
        mark_layout_dirty();
    }

    auto Divider::orientation() const -> Orientation {
        return orientation_;
    }

    void Divider::set_pattern(const Pattern pattern) {
        pattern_ = pattern;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Divider::pattern() const -> Pattern {
        return pattern_;
    }

    void Divider::set_dash_length(const float length) {
        if (!std::isfinite(length)) {
            throw std::invalid_argument("divider dash length must be finite");
        }
        dash_length_ = length;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Divider::dash_length() const -> float {
        return dash_length_;
    }

    void Divider::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
    }

    auto Divider::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Divider::set_override(theme::DividerRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Divider::resolved_style() const -> theme::ResolvedDividerStyle {
        auto style = theme::resolve_divider(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Divider::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
    }

    void Divider::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        const float thickness = context.logical_to_screen(style.thickness);
        const float dash = dash_length_ > 0.0F
            ? context.logical_to_screen(dash_length_)
            : thickness * 4.0F;
        const auto color = style.color.with_alpha(style.color.alpha() * context.opacity());

        const bool horizontal = orientation_ == Orientation::horizontal;
        const bool solid = pattern_ == Pattern::solid || pattern_ == Pattern::double_line;
        const int line_count = (pattern_ == Pattern::double_line || pattern_ == Pattern::double_dashed)
            ? 2
            : 1;

        // 沿主轴画一条线；offset 为垂直于主轴的偏移（双线的第二根）。
        const auto paint_line = [&](const float offset) {
            if (horizontal) {
                const float y = world.get_top() + (world.get_height() - thickness) * 0.5F + offset;
                if (solid) {
                    context.device().draw_rect(
                        foundation::NanRect::from_xywh(world.get_left(), y, world.get_width(), thickness),
                        color
                    );
                }
                else {
                    float x = world.get_left();
                    while (x < world.get_right()) {
                        const float w = std::min(dash, world.get_right() - x);
                        context.device().draw_rect(
                            foundation::NanRect::from_xywh(x, y, w, thickness),
                            color
                        );
                        x += dash * 2.0F; // 段 + 间隙（各占 dash 长度）
                    }
                }
            }
            else {
                const float x = world.get_left() + (world.get_width() - thickness) * 0.5F + offset;
                if (solid) {
                    context.device().draw_rect(
                        foundation::NanRect::from_xywh(x, world.get_top(), thickness, world.get_height()),
                        color
                    );
                }
                else {
                    float y = world.get_top();
                    while (y < world.get_bottom()) {
                        const float h = std::min(dash, world.get_bottom() - y);
                        context.device().draw_rect(
                            foundation::NanRect::from_xywh(x, y, thickness, h),
                            color
                        );
                        y += dash * 2.0F;
                    }
                }
            }
        };

        if (line_count == 1) {
            paint_line(0.0F);
        }
        else {
            paint_line(-thickness);
            paint_line(thickness);
        }
    }

    auto Divider::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        const auto style = resolved_style();
        if (orientation_ == Orientation::horizontal) {
            const float width = std::isfinite(constraints.max_width)
                ? constraints.max_width
                : style.preferred_length;
            return constraints.constrain(foundation::NanSize(width, style.thickness));
        }
        const float height = std::isfinite(constraints.max_height)
            ? constraints.max_height
            : style.preferred_length;
        return constraints.constrain(foundation::NanSize(style.thickness, height));
    }

    auto Divider::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::separator,
            .state =
                {
                    .focusable = false,
                },
        };
    }
} // namespace nandina::widget
