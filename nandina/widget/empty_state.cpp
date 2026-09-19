//
// widget/empty_state - empty collection placeholder (title + description + optional slots).
//

#include "empty_state.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        /// 描述文本最大行数：非配方字段，仅用于防止超长描述把空状态撑出容器。
        constexpr int kDescriptionMaxLines = 4;

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

        /// 替换槽位内容：旧内容立即销毁，新内容必须处于游离状态（与 Dialog 命名槽位一致）。
        void install_slot(
            scene::NanControl& host,
            std::weak_ptr<scene::NanControl>& slot,
            std::shared_ptr<scene::NanControl> next,
            const char* what
        ) {
            const std::string label = std::string("EmptyState::") + what;
            if (!next) {
                throw std::invalid_argument(label + ": content is null");
            }
            if (next->parent() != nullptr) {
                throw std::logic_error(label + ": content must be detached");
            }
            if (auto current = slot.lock()) {
                host.remove_and_delete(*current);
            }
            slot = next;
            host.add_child(std::move(next));
        }
    } // namespace

    EmptyState::EmptyState(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        relayout();
    }

    auto EmptyState::create(theme::NanTheme theme) -> std::shared_ptr<EmptyState> {
        return std::make_shared<EmptyState>(theme);
    }

    void EmptyState::set_title(std::string title) {
        if (title_text_.text() == title) {
            return;
        }
        title_text_.set_text(std::move(title));
        mark_layout_dirty();
        mark_semantics_dirty();
        relayout();
    }

    auto EmptyState::title() const -> std::string_view {
        return title_text_.text();
    }

    void EmptyState::set_description(std::string description) {
        if (description_text_.text() == description) {
            return;
        }
        description_text_.set_text(std::move(description));
        mark_layout_dirty();
        relayout();
    }

    auto EmptyState::description() const -> std::string_view {
        return description_text_.text();
    }

    void EmptyState::set_icon(std::shared_ptr<scene::NanControl> icon) {
        install_slot(*this, icon_, std::move(icon), "set_icon");
        mark_layout_dirty();
        relayout();
    }

    void EmptyState::set_action(std::shared_ptr<scene::NanControl> action) {
        install_slot(*this, action_, std::move(action), "set_action");
        mark_layout_dirty();
        relayout();
    }

    auto EmptyState::icon() const -> scene::NanControl* {
        return icon_.lock().get();
    }

    auto EmptyState::action() const -> scene::NanControl* {
        return action_.lock().get();
    }

    void EmptyState::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_styles();
        mark_layout_dirty();
        relayout();
    }

    auto EmptyState::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void EmptyState::set_override(theme::EmptyStateRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_styles();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
        relayout();
    }

    auto EmptyState::visual_state() const -> theme::EmptyStateVisualState {
        return theme::EmptyStateVisualState::normal;
    }

    auto EmptyState::resolved_style() const -> theme::ResolvedEmptyStateStyle {
        auto style = theme::resolve_empty_state(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void EmptyState::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        title_text_.apply_default_text_pipeline(pipeline);
        description_text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void EmptyState::apply_font_context(text::FontPipelineCache& context) {
        title_text_.apply_font_context(context);
        description_text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void EmptyState::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_styles();
        mark_layout_dirty();
    }

    void EmptyState::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_styles();
        mark_layout_dirty();
        relayout();
    }

    auto EmptyState::is_empty() const -> bool {
        return title_text_.text().empty() && icon_.lock() == nullptr;
    }

    auto EmptyState::placement(const float available_width) -> Placement {
        Placement result;
        const auto style = resolved_style();
        result.inner_width = std::max(0.0F, available_width - style.metrics.padding_x * 2.0F);
        if (is_empty()) {
            return result;
        }

        apply_text_styles();

        const float padding_y = style.metrics.padding_y;
        float top = padding_y;
        bool first = true;
        // 纵向堆叠，内容水平居中；每个块高度由各自测量结果决定。
        const auto stack = [&](const float item_width, const float item_height) {
            if (!first) {
                top += style.metrics.gap;
            }
            first = false;
            const float left = style.metrics.padding_x
                + std::max(0.0F, (result.inner_width - item_width) * 0.5F);
            const auto rect =
                foundation::NanRect::from_xywh(left, top, item_width, item_height);
            top += item_height;
            return rect;
        };

        const scene::LayoutConstraints inner {.max_width = result.inner_width};

        if (auto icon = icon_.lock()) {
            const auto size = icon->measure_layout(inner);
            result.icon = stack(size.get_width(), size.get_height());
        }
        if (!title_text_.text().empty()) {
            (void)title_text_.measure_layout(inner);
            result.title = stack(title_text_.width(), title_text_.height());
        }
        if (!description_text_.text().empty()) {
            (void)description_text_.measure_layout(inner);
            result.description = stack(description_text_.width(), description_text_.height());
        }
        if (auto action = action_.lock()) {
            const auto size = action->measure_layout(inner);
            result.action = stack(size.get_width(), size.get_height());
        }

        result.total_height = top + padding_y;
        return result;
    }

    auto EmptyState::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        if (is_empty()) {
            // 条件挂载：没有标题也没有图标时不占位，避免在列表里留下空隙。
            return constraints.constrain(foundation::NanSize(0.0F, 0.0F));
        }
        const float width = std::isfinite(constraints.max_width)
            ? constraints.max_width
            : style.metrics.preferred_width;
        const auto placed = placement(width);
        return constraints.constrain(foundation::NanSize(
            width,
            std::max(style.metrics.min_height, placed.total_height)
        ));
    }

    void EmptyState::on_layout() {
        const auto placed = placement(width());
        const auto zero = foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
        if (auto icon = icon_.lock()) {
            icon->layout_to(placed.icon.value_or(zero));
        }
        if (auto action = action_.lock()) {
            action->layout_to(placed.action.value_or(zero));
        }
    }

    auto EmptyState::on_draw(render::DrawContext& context) -> void {
        if (is_empty()) {
            return;
        }
        const auto style = resolved_style();
        // placement() 同时会刷新文本样式与换行布局，draw 前必须调用一次。
        const auto placed = placement(width());
        const auto& transform = context.world_transform();
        primitives::BoxPainter::paint(
            context,
            render::world_bounds_from_local(transform, local_rect()),
            style.container,
            context.opacity()
        );
        if (placed.title) {
            title_text_.draw_at(
                context,
                render::world_bounds_from_local(transform, *placed.title).get_top_left()
            );
        }
        if (placed.description) {
            description_text_.draw_at(
                context,
                render::world_bounds_from_local(transform, *placed.description).get_top_left()
            );
        }
    }

    auto EmptyState::semantics_properties() const -> semantics::Properties {
        if (title_text_.text().empty()) {
            return {};
        }
        return {
            .role = semantics::Role::generic,
            .label = std::string(title_text_.text()),
            .hint = std::string(description_text_.text()),
        };
    }

    void EmptyState::apply_text_styles() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const auto apply = [&](
                               primitives::Text& text,
                               const theme::ResolvedTypeStyle& type,
                               const primitives::TextOverflow overflow,
                               const int max_lines
                           ) {
            const primitives::TextStyle text_style {
                .color = context.text_color_from_context ? context.text_color : type.color,
                .font_size =
                    context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : text.font(),
                .overflow = overflow,
                .max_lines = max_lines,
            };
            if (!same_text_style(text.style(), text_style)) {
                text.set_style(text_style);
            }
        };
        apply(title_text_, style.title, primitives::TextOverflow::clip, 1);
        apply(
            description_text_,
            style.description,
            primitives::TextOverflow::wrap,
            kDescriptionMaxLines
        );
    }

    void EmptyState::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }
} // namespace nandina::widget
