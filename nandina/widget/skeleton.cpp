//
// widget/skeleton - loading placeholder (pure display, no interaction).
//

#include "skeleton.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    Skeleton::Skeleton(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        (void)measure_layout(scene::LayoutConstraints::loose());
        set_size(measured_size());
    }

    auto Skeleton::create(theme::NanTheme theme) -> std::shared_ptr<Skeleton> {
        return std::make_shared<Skeleton>(theme);
    }

    void Skeleton::set_variant(const SkeletonVariant variant) {
        if (variant_ == variant) {
            return;
        }
        variant_ = variant;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Skeleton::variant() const -> SkeletonVariant {
        return variant_;
    }

    void Skeleton::set_lines(const int lines) {
        const int clamped = std::max(1, lines);
        if (lines_ == clamped) {
            return;
        }
        lines_ = clamped;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Skeleton::lines() const -> int {
        return lines_;
    }

    void Skeleton::set_label(std::string label) {
        label_ = std::move(label);
        mark_semantics_dirty();
    }

    auto Skeleton::label() const -> std::string_view {
        return label_;
    }

    void Skeleton::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        mark_layout_dirty();
    }

    auto Skeleton::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Skeleton::set_override(theme::SkeletonRecipeRule rule) {
        override_ = std::move(rule);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Skeleton::visual_state() const -> theme::SkeletonVisualState {
        return theme::SkeletonVisualState::normal;
    }

    auto Skeleton::resolved_style() const -> theme::ResolvedSkeletonStyle {
        auto style = theme::resolve_skeleton(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Skeleton::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        mark_layout_dirty();
    }

    auto Skeleton::line_width_ratios() const -> std::vector<float> {
        const auto style = resolved_style();
        const float last_ratio = std::clamp(style.metrics.last_line_ratio, 0.0F, 1.0F);
        std::vector<float> ratios;
        ratios.reserve(static_cast<std::size_t>(std::max(0, lines_)));
        for (int index = 0; index < lines_; ++index) {
            // 末行收窄是多行占位的观感约定；只有一行时整条占满，
            // 否则一个"单行文本占位"会莫名只剩 60% 宽。
            const bool narrow_last = lines_ > 1 && index == lines_ - 1;
            ratios.push_back(narrow_last ? last_ratio : 1.0F);
        }
        return ratios;
    }

    auto Skeleton::on_draw(render::DrawContext& context) -> void {
        const auto style = resolved_style();
        const auto& transform = context.world_transform();
        const auto local = local_rect();
        const float opacity = context.opacity();

        if (variant_ == SkeletonVariant::rectangle) {
            primitives::BoxPainter::paint(
                context,
                render::world_bounds_from_local(transform, local),
                style.surface,
                opacity
            );
            return;
        }

        const float bar_height = std::max(0.0F, style.metrics.height);
        const float line_gap = std::max(0.0F, style.metrics.line_gap);
        const float last_ratio = std::clamp(style.metrics.last_line_ratio, 0.0F, 1.0F);
        float top = local.get_top();
        for (int index = 0; index < lines_; ++index) {
            // 末行收窄是多行占位的观感约定；只有一行时整条占满，
            // 否则一个"单行文本占位"会莫名只剩 60% 宽。
            const bool narrow_last = lines_ > 1 && index == lines_ - 1;
            const float bar_width =
                local.get_width() * (narrow_last ? last_ratio : 1.0F);
            const auto line_local = foundation::NanRect::from_xywh(
                local.get_left(),
                top,
                std::max(0.0F, bar_width),
                bar_height
            );
            primitives::BoxPainter::paint(
                context,
                render::world_bounds_from_local(transform, line_local),
                style.surface,
                opacity
            );
            top += bar_height + line_gap;
        }
    }

    auto Skeleton::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        const float width = std::isfinite(constraints.max_width)
            ? constraints.max_width
            : style.metrics.preferred_width;

        if (variant_ == SkeletonVariant::rectangle) {
            // 显式尺寸经 size_spec 变成 tight 约束，因此取上界即"尊重显式尺寸"。
            const float height = std::isfinite(constraints.max_height)
                ? constraints.max_height
                : style.metrics.height;
            return constraints.constrain(foundation::NanSize(width, height));
        }

        const int lines = std::max(1, lines_);
        const float height = static_cast<float>(lines) * style.metrics.height
            + static_cast<float>(lines - 1) * style.metrics.line_gap;
        return constraints.constrain(foundation::NanSize(width, height));
    }

    auto Skeleton::semantics_properties() const -> semantics::Properties {
        // 骨架屏就是"内容未就绪"的进度指示：复用 progress_bar 角色（无 value = 不确定进度），
        // 而不是发明新角色。label 为空时辅助技术只会读到"忙碌"，不会读到百分比。
        return {
            .role = semantics::Role::progress_bar,
            .label = label_,
        };
    }
} // namespace nandina::widget
