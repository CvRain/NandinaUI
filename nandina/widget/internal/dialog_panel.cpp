#include "dialog_panel.hpp"

#include "../../render/draw_context.hpp"
#include "../primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nandina::widget::internal
{
    namespace
    {
        [[nodiscard]] auto same_text_style(
            const primitives::TextStyle& lhs,
            const primitives::TextStyle& rhs
        ) -> bool {
            return lhs.color.approx_equals(rhs.color)
                && std::abs(lhs.font_size - rhs.font_size) <= foundation::nan_epsilon
                && lhs.font == rhs.font && lhs.overflow == rhs.overflow
                && lhs.max_lines == rhs.max_lines;
        }

        /// 替换槽位内容：旧内容立即销毁，新内容必须处于游离状态。
        void install_slot(
            scene::NanControl& panel,
            std::weak_ptr<scene::NanControl>& slot,
            std::shared_ptr<scene::NanControl> next,
            const char* what
        ) {
            const std::string label = std::string("DialogPanel::") + what;
            if (!next) {
                throw std::invalid_argument(label + ": content is null");
            }
            if (next->parent() != nullptr) {
                throw std::logic_error(label + ": content must be detached");
            }
            if (auto current = slot.lock()) {
                panel.remove_and_delete(*current);
            }
            slot = next;
            panel.add_child(std::move(next));
        }
    } // namespace

    DialogPanel::DialogPanel() {
        style_ =
            theme::resolve_dialog(theme::default_design_system(), theme::ColorAppearance::light);
    }

    auto DialogPanel::set_header(std::shared_ptr<scene::NanControl> header) -> scene::NanControl& {
        // 自定义 header 取代标题文本；标题对象仍由本面板持有，之后 set_title() 可以再取回。
        install_slot(*this, header_, std::move(header), "set_header");
        mark_layout_dirty();
        return *header_.lock();
    }

    auto DialogPanel::set_content(std::shared_ptr<scene::NanControl> content) -> scene::NanControl& {
        install_slot(*this, content_, std::move(content), "set_content");
        mark_layout_dirty();
        return *content_.lock();
    }

    auto DialogPanel::set_footer(std::shared_ptr<scene::NanControl> footer) -> scene::NanControl& {
        install_slot(*this, footer_, std::move(footer), "set_footer");
        mark_layout_dirty();
        return *footer_.lock();
    }

    void DialogPanel::set_title(std::string title) {
        if (title_ == nullptr) {
            title_ = std::make_shared<primitives::Text>("");
        }
        // 标题就是 header 槽位的内容：必须登记到 header_，否则 slot_nodes() 看不到它，
        // 它既不会参与测量也不参与布局，会停在面板原点与 content 重叠。
        if (auto current = header_.lock(); current != nullptr && current != title_) {
            remove_and_delete(*current);
        }
        title_->set_text(std::move(title));
        header_ = title_;
        if (title_->parent() == nullptr) {
            add_child(title_);
        }
        apply_title_style();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto DialogPanel::title() const -> std::string_view {
        return title_ != nullptr ? title_->text() : std::string_view {};
    }

    void DialogPanel::set_style(theme::ResolvedDialogStyle style) {
        style_ = std::move(style);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    void DialogPanel::set_text_pipeline(primitives::TextPipeline pipeline) {
        if (title_ != nullptr) {
            title_->set_text_pipeline(std::move(pipeline));
            mark_layout_dirty();
        }
    }

    void DialogPanel::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        if (title_ != nullptr) {
            title_->apply_default_text_pipeline(pipeline);
            mark_layout_dirty();
        }
    }

    void DialogPanel::apply_font_context(text::FontPipelineCache& context) {
        if (title_ != nullptr) {
            title_->apply_font_context(context);
            mark_layout_dirty();
        }
    }

    void DialogPanel::apply_title_style() {
        if (title_ == nullptr) {
            return;
        }
        const auto& context = resolved_style_context();
        const primitives::TextStyle style {
            .color = context.text_color_from_context ? context.text_color : style_.title.color,
            .font_size =
                context.font_size_from_context ? context.font_size : style_.title.font_size,
            .font = context.font_from_context ? context.font : title_->font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        if (!same_text_style(title_->style(), style)) {
            title_->set_style(style);
            mark_layout_dirty();
        }
    }

    auto DialogPanel::style() const noexcept -> const theme::ResolvedDialogStyle& {
        return style_;
    }

    auto DialogPanel::slot_nodes() const -> std::vector<scene::NanControl*> {
        std::vector<scene::NanControl*> nodes;
        nodes.reserve(3);
        for (const auto& slot: {header_, content_, footer_}) {
            if (auto node = slot.lock()) {
                nodes.push_back(node.get());
            }
        }
        return nodes;
    }

    auto DialogPanel::inner_constraints(const float panel_width, const float max_height) const
        -> scene::LayoutConstraints {
        return scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = std::max(0.0F, panel_width - style_.metrics.padding_x * 2.0F),
            .min_height = 0.0F,
            .max_height = std::max(0.0F, max_height),
        };
    }

    auto DialogPanel::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        apply_title_style();
        const float available = std::max(0.0F, constraints.max_width - kViewportMargin * 2.0F);
        const float panel_width = std::min(style_.metrics.panel_width, available);
        const auto inner = inner_constraints(panel_width, constraints.max_height);

        float height = style_.metrics.padding_y * 2.0F;
        for (auto* slot: slot_nodes()) {
            if (height > style_.metrics.padding_y * 2.0F) {
                height += style_.metrics.gap;
            }
            height += slot->measure_layout(inner).get_height();
        }
        height = std::max(height, style_.metrics.min_height);
        return constraints.constrain(foundation::NanSize(panel_width, height));
    }

    void DialogPanel::on_layout() {
        apply_title_style();
        const auto inner = inner_constraints(width(), height());
        float y = style_.metrics.padding_y;
        for (auto* slot: slot_nodes()) {
            if (y > style_.metrics.padding_y) {
                y += style_.metrics.gap;
            }
            const auto measured = slot->measure_layout(inner);
            slot->layout_to(foundation::NanRect::from_xywh(
                style_.metrics.padding_x,
                y,
                measured.get_width(),
                measured.get_height()
            ));
            y += measured.get_height();
        }
    }

    auto DialogPanel::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::dialog,
            .label = std::string(title()),
        };
    }

    void DialogPanel::on_draw(render::DrawContext& context) {
        apply_title_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        primitives::BoxPainter::paint(context, world, style_.panel, context.opacity());
    }
} // namespace nandina::widget::internal
