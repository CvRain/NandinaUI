//
// widget/alert - inline status / feedback banner (presentational, named slots).
//

#include "alert.hpp"

#include "internal/alert_dismiss_button.hpp"

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
        /// 描述文本最大行数：非配方字段，仅用于防止超长描述把消息条撑出容器
        /// （与 EmptyState 保持同一上限）。
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
            const std::string label = std::string("Alert::") + what;
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

    Alert::Alert(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        relayout();
    }

    auto Alert::create(theme::NanTheme theme) -> std::shared_ptr<Alert> {
        return std::make_shared<Alert>(theme);
    }

    void Alert::set_tone(const theme::AlertTone tone) {
        if (tone_ == tone) {
            return;
        }
        tone_ = tone;
        sync_dismiss_appearance();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
        relayout();
    }

    auto Alert::tone() const -> theme::AlertTone {
        return tone_;
    }

    void Alert::set_title(std::string title) {
        if (title_text_.text() == title) {
            return;
        }
        title_text_.set_text(std::move(title));
        mark_layout_dirty();
        mark_semantics_dirty();
        relayout();
    }

    auto Alert::title() const -> std::string_view {
        return title_text_.text();
    }

    void Alert::set_description(std::string description) {
        if (description_text_.text() == description) {
            return;
        }
        description_text_.set_text(std::move(description));
        mark_layout_dirty();
        relayout();
    }

    auto Alert::description() const -> std::string_view {
        return description_text_.text();
    }

    void Alert::set_icon(std::shared_ptr<scene::NanControl> icon) {
        install_slot(*this, icon_, std::move(icon), "set_icon");
        mark_layout_dirty();
        relayout();
    }

    void Alert::set_action(std::shared_ptr<scene::NanControl> action) {
        install_slot(*this, action_, std::move(action), "set_action");
        mark_layout_dirty();
        relayout();
    }

    auto Alert::icon() const -> scene::NanControl* {
        return icon_.lock().get();
    }

    auto Alert::action() const -> scene::NanControl* {
        return action_.lock().get();
    }

    void Alert::set_dismissible(const bool on) {
        if (dismissible_ == on) {
            return;
        }
        dismissible_ = on;
        if (on) {
            dismiss_ = std::make_shared<internal::AlertDismissButton>();
            // 子控件由 Alert 独占，生命周期不超过本控件，捕获 this 安全。
            dismiss_->set_on_click([this] {
                if (on_dismiss_) {
                    on_dismiss_();
                }
            });
            add_child(dismiss_);
        }
        else if (dismiss_) {
            remove_and_delete(*dismiss_);
            dismiss_.reset();
        }
        sync_dismiss_appearance();
        mark_layout_dirty();
        mark_semantics_dirty();
        relayout();
    }

    auto Alert::dismissible() const -> bool {
        return dismissible_;
    }

    void Alert::set_on_dismiss(std::function<void()> callback) {
        on_dismiss_ = std::move(callback);
    }

    void Alert::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_styles();
        sync_dismiss_appearance();
        mark_layout_dirty();
        relayout();
    }

    auto Alert::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Alert::set_override(theme::AlertRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_styles();
        sync_dismiss_appearance();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
        relayout();
    }

    auto Alert::visual_state() const -> theme::AlertVisualState {
        return theme::AlertVisualState::normal;
    }

    auto Alert::resolved_style() const -> theme::ResolvedAlertStyle {
        auto style = theme::resolve_alert(*system_, appearance_, tone_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Alert::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        title_text_.apply_default_text_pipeline(pipeline);
        description_text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Alert::apply_font_context(text::FontPipelineCache& context) {
        title_text_.apply_font_context(context);
        description_text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Alert::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_styles();
        mark_layout_dirty();
    }

    void Alert::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_styles();
        sync_dismiss_appearance();
        mark_layout_dirty();
        relayout();
    }

    auto Alert::is_empty() const -> bool {
        return title_text_.text().empty() && description_text_.text().empty()
            && icon_.lock() == nullptr;
    }

    auto Alert::placement(const float available_width) -> Placement {
        Placement result;
        const auto style = resolved_style();
        if (is_empty()) {
            return result;
        }

        apply_text_styles();

        const float padding_x = style.metrics.padding_x;
        const float padding_y = style.metrics.padding_y;
        const float gap = style.metrics.gap;
        const float inner_width = std::max(0.0F, available_width - padding_x * 2.0F);

        // ── 固定列宽度：icon / action 由各自测量，dismiss 由配方边长决定 ──────────
        foundation::NanSize icon_size {};
        bool has_icon = false;
        if (auto icon = icon_.lock()) {
            icon_size = icon->measure_layout(scene::LayoutConstraints {.max_width = inner_width});
            has_icon = true;
        }
        foundation::NanSize action_size {};
        bool has_action = false;
        if (auto action = action_.lock()) {
            action_size = action->measure_layout(scene::LayoutConstraints {.max_width = inner_width});
            has_action = true;
        }
        const bool has_dismiss = dismissible_ && dismiss_ != nullptr;
        const float dismiss_size = has_dismiss ? style.metrics.box_size : 0.0F;

        const bool has_title = !title_text_.text().empty();
        const bool has_description = !description_text_.text().empty();
        const bool has_text = has_title || has_description;

        const int columns = (has_icon ? 1 : 0) + (has_text ? 1 : 0) + (has_action ? 1 : 0)
            + (has_dismiss ? 1 : 0);
        const float fixed_width = (has_icon ? icon_size.get_width() : 0.0F)
            + (has_action ? action_size.get_width() : 0.0F) + dismiss_size;
        const float gaps = columns > 1 ? gap * static_cast<float>(columns - 1) : 0.0F;
        const float text_budget = std::max(0.0F, inner_width - fixed_width - gaps);

        // ── 文本列：标题裁剪一行，描述按列宽换行 ─────────────────────────────────
        float text_width = 0.0F;
        float title_height = 0.0F;
        float description_height = 0.0F;
        if (has_text) {
            const scene::LayoutConstraints text_constraints {.max_width = text_budget};
            if (has_title) {
                (void)title_text_.measure_layout(text_constraints);
                title_height = title_text_.height();
                text_width = std::max(text_width, title_text_.width());
            }
            if (has_description) {
                (void)description_text_.measure_layout(text_constraints);
                description_height = description_text_.height();
                text_width = std::max(text_width, description_text_.width());
            }
        }
        const bool text_stacked = has_title && has_description;
        const float text_height =
            title_height + description_height + (text_stacked ? gap : 0.0F);

        const float content_height = std::max(
            std::max(has_icon ? icon_size.get_height() : 0.0F, text_height),
            std::max(has_action ? action_size.get_height() : 0.0F, dismiss_size)
        );
        result.content_height = content_height;

        // ── 从左到右落位，各列垂直居中 ───────────────────────────────────────────
        float x = padding_x;
        const auto place_column = [&](const float width, const float height) {
            const float y = padding_y + (content_height - height) * 0.5F;
            const auto rect = foundation::NanRect::from_xywh(x, y, width, height);
            x += width + gap;
            return rect;
        };

        if (has_icon) {
            result.icon = place_column(icon_size.get_width(), icon_size.get_height());
        }
        if (has_text) {
            const auto column = place_column(text_width, text_height);
            if (has_title) {
                result.title = foundation::NanRect::from_xywh(
                    column.get_left(), column.get_top(), title_text_.width(), title_height
                );
            }
            if (has_description) {
                const float top = has_title ? column.get_top() + title_height + gap : column.get_top();
                result.description = foundation::NanRect::from_xywh(
                    column.get_left(), top, description_text_.width(), description_height
                );
            }
        }
        if (has_action) {
            result.action = place_column(action_size.get_width(), action_size.get_height());
        }
        if (has_dismiss) {
            result.dismiss = place_column(dismiss_size, dismiss_size);
        }

        result.total_height = padding_y * 2.0F + content_height;
        return result;
    }

    auto Alert::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        if (is_empty()) {
            // 条件挂载：没有标题、描述与图标时不占位，避免在列表里留下空隙。
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

    void Alert::on_layout() {
        const auto placed = placement(width());
        const auto zero = foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
        if (auto icon = icon_.lock()) {
            icon->layout_to(placed.icon.value_or(zero));
        }
        if (auto action = action_.lock()) {
            action->layout_to(placed.action.value_or(zero));
        }
        if (dismiss_) {
            // 空状态时整控件不绘制，关闭按钮也必须收起（否则会留下一个 0 尺寸但可聚焦的节点）。
            dismiss_->set_visible(!is_empty());
            dismiss_->layout_to(placed.dismiss.value_or(zero));
        }
    }

    auto Alert::on_draw(render::DrawContext& context) -> void {
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

    auto Alert::semantics_properties() const -> semantics::Properties {
        if (title_text_.text().empty()) {
            return {};
        }
        return {
            .role = semantics::Role::generic,
            .label = std::string(title_text_.text()),
            .hint = std::string(description_text_.text()),
        };
    }

    void Alert::apply_text_styles() {
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

    void Alert::sync_dismiss_appearance() {
        if (!dismiss_) {
            return;
        }
        const auto style = resolved_style();
        dismiss_->set_glyph_color(style.icon);
        // 焦点环取语义 ring token；关闭按钮不是配方的造型字段，这里直接解析系统快照。
        dismiss_->set_focus_ring_color(theme::resolve_color(
            *system_,
            appearance_,
            theme::ThemeColor::token(theme::ColorToken::ring)
        ));
        dismiss_->set_affordance_size(style.metrics.box_size);
    }

    void Alert::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }
} // namespace nandina::widget
