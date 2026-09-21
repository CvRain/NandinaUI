//
// widget/breadcrumb - horizontal trail of links showing the current location.
//

#include "breadcrumb.hpp"

#include "internal/breadcrumb_link.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
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

    Breadcrumb::Breadcrumb(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        apply_text_styles();
        relayout();
    }

    auto Breadcrumb::create(theme::NanTheme theme) -> std::shared_ptr<Breadcrumb> {
        return std::make_shared<Breadcrumb>(theme);
    }

    void Breadcrumb::add_item(std::string label, std::function<void()> on_click) {
        Item item;
        item.label = std::move(label);
        if (on_click) {
            auto link = std::make_shared<internal::BreadcrumbLink>(item.label);
            link->set_on_click(std::move(on_click));
            item.link = link;
            add_child(link);
        }
        else {
            item.text = std::make_shared<primitives::Text>(item.label);
        }
        items_.push_back(std::move(item));
        mark_layout_dirty();
        mark_semantics_dirty();
        relayout();
    }

    void Breadcrumb::clear() {
        for (auto& item: items_) {
            if (item.link) {
                remove_and_delete(*item.link);
            }
        }
        items_.clear();
        separator_rects_.clear();
        mark_layout_dirty();
        mark_semantics_dirty();
        relayout();
    }

    auto Breadcrumb::item_count() const -> std::size_t {
        return items_.size();
    }

    auto Breadcrumb::item_label(const std::size_t index) const -> std::string_view {
        return index < items_.size() ? items_[index].label : std::string_view {};
    }

    auto Breadcrumb::item_clickable(const std::size_t index) const -> bool {
        return index < items_.size() && items_[index].link != nullptr;
    }

    void Breadcrumb::set_separator(std::string separator) {
        if (separator_ == separator) {
            return;
        }
        separator_ = std::move(separator);
        separator_text_.set_text(separator_);
        mark_layout_dirty();
        relayout();
    }

    auto Breadcrumb::separator() const -> std::string_view {
        return separator_;
    }

    void Breadcrumb::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_styles();
        mark_layout_dirty();
        relayout();
    }

    auto Breadcrumb::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Breadcrumb::set_override(theme::BreadcrumbRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_styles();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
        relayout();
    }

    auto Breadcrumb::visual_state() const -> theme::BreadcrumbVisualState {
        // 容器只做排布与绘制；交互状态属于内部链接条目。
        return theme::BreadcrumbVisualState::normal;
    }

    auto Breadcrumb::resolved_style() const -> theme::ResolvedBreadcrumbStyle {
        auto style = theme::resolve_breadcrumb(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Breadcrumb::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        // 只作用于容器自绘文本；链接子控件由场景树各自传播。
        separator_text_.apply_default_text_pipeline(pipeline);
        for (auto& item: items_) {
            if (item.text) {
                item.text->apply_default_text_pipeline(pipeline);
            }
        }
        mark_layout_dirty();
    }

    void Breadcrumb::apply_font_context(text::FontPipelineCache& context) {
        separator_text_.apply_font_context(context);
        for (auto& item: items_) {
            if (item.text) {
                item.text->apply_font_context(context);
            }
        }
        mark_layout_dirty();
    }

    void Breadcrumb::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_styles();
        mark_layout_dirty();
    }

    void Breadcrumb::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_styles();
        mark_layout_dirty();
        relayout();
    }

    auto Breadcrumb::measure_trail(const float available_width) -> TrailLayout {
        const auto style = resolved_style();
        apply_text_styles();

        const scene::LayoutConstraints text_constraints {
            .max_width = available_width,
        };
        (void)separator_text_.measure_layout(text_constraints);
        const float separator_width = separator_text_.measured_text_width();
        const float separator_height = separator_text_.measured_text_height();

        float content_height = 0.0F;
        for (auto& item: items_) {
            if (item.link) {
                const auto measured = item.link->measure_layout(text_constraints);
                item.width = measured.get_width();
                item.height = measured.get_height();
            }
            else if (item.text) {
                (void)item.text->measure_layout(text_constraints);
                item.width = item.text->measured_text_width();
                item.height = item.text->measured_text_height();
            }
            else {
                item.width = 0.0F;
                item.height = 0.0F;
            }
            content_height = std::max(content_height, item.height);
        }

        const float height = std::max(
            style.metrics.min_height,
            std::max(content_height, separator_height)
        );

        separator_rects_.assign(items_.empty() ? 0 : items_.size() - 1, foundation::NanRect {});
        float x = style.metrics.padding_x;
        for (std::size_t index = 0; index < items_.size(); ++index) {
            auto& item = items_[index];
            if (index > 0) {
                x += style.metrics.gap;
                separator_rects_[index - 1] = foundation::NanRect::from_xywh(
                    x,
                    (height - separator_height) * 0.5F,
                    separator_width,
                    separator_height
                );
                x += separator_width + style.metrics.gap;
            }
            item.rect = foundation::NanRect::from_xywh(
                x,
                (height - item.height) * 0.5F,
                item.width,
                item.height
            );
            x += item.width;
        }

        return {.width = x + style.metrics.padding_x, .height = height};
    }

    auto Breadcrumb::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        const float available = std::isfinite(constraints.max_width)
            ? constraints.max_width
            : std::numeric_limits<float>::infinity();
        const auto trail = measure_trail(available);
        return constraints.constrain(foundation::NanSize(trail.width, trail.height));
    }

    void Breadcrumb::on_layout() {
        (void)measure_trail(width());
        for (auto& item: items_) {
            if (item.link) {
                item.link->layout_to(item.rect);
            }
        }
    }

    void Breadcrumb::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Breadcrumb::on_draw(render::DrawContext& context) -> void {
        const auto style = resolved_style();
        // measure_trail() 同时刷新文本样式并重算条目矩形，draw 前必须调用一次。
        (void)measure_trail(width());
        const auto& transform = context.world_transform();

        primitives::BoxPainter::paint(
            context,
            render::world_bounds_from_local(transform, local_rect()),
            style.container,
            context.opacity()
        );

        for (const auto& rect: separator_rects_) {
            separator_text_.draw_at(
                context,
                render::world_bounds_from_local(transform, rect).get_top_left()
            );
        }
        for (const auto& item: items_) {
            if (item.text) {
                item.text->draw_at(
                    context,
                    render::world_bounds_from_local(transform, item.rect).get_top_left()
                );
            }
        }
    }

    auto Breadcrumb::semantics_properties() const -> semantics::Properties {
        if (items_.empty()) {
            return {};
        }
        // 平台语义树没有 nav / breadcrumb role：容器用 generic，label 给出整条路径；
        // 可点击条目由各自的 BreadcrumbLink 暴露为 button。
        std::string trail;
        for (std::size_t index = 0; index < items_.size(); ++index) {
            if (index > 0) {
                trail += " " + separator_ + " ";
            }
            trail += items_[index].label;
        }
        return {
            .role = semantics::Role::generic,
            .label = std::move(trail),
        };
    }

    void Breadcrumb::apply_text_styles() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();

        const auto make_style = [&](
                                    const theme::ResolvedTypeStyle& type,
                                    const text::FontRequest& font,
                                    const foundation::NanColor color
                                ) {
            return primitives::TextStyle {
                .color = context.text_color_from_context ? context.text_color : color,
                .font_size = context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : font,
                .overflow = primitives::TextOverflow::clip,
                .max_lines = 1,
            };
        };

        // 分隔符复用当前页字号，只换颜色。
        const auto separator_style =
            make_style(style.current, separator_text_.font(), style.separator);
        if (!same_text_style(separator_text_.style(), separator_style)) {
            separator_text_.set_style(separator_style);
        }

        for (auto& item: items_) {
            if (item.link) {
                const auto link_style = make_style(style.link, item.link->font(), style.link.color);
                if (!same_text_style(item.link->text_style(), link_style)) {
                    item.link->set_text_style(link_style);
                }
                item.link->set_hover_color(style.link_hover);
                item.link->set_focus_ring(style.link_focus);
            }
            else if (item.text) {
                const auto text_style =
                    make_style(style.current, item.text->font(), style.current.color);
                if (!same_text_style(item.text->style(), text_style)) {
                    item.text->set_style(text_style);
                }
            }
        }
    }

    void Breadcrumb::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }
} // namespace nandina::widget
