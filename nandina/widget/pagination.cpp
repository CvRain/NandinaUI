//
// widget/pagination - page navigation with prev/next, numbered pages and ellipsis.
//

#include "pagination.hpp"

#include "key_codes.hpp"

#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"

#include <algorithm>
#include <cmath>
#include <string>
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

        /// 上一页 / 下一页字形：用 ASCII 尖括号，避免字体缺少 ‹ › 时出现豆腐块。
        constexpr const char* kPreviousGlyph = "<";
        constexpr const char* kNextGlyph = ">";
        constexpr const char* kEllipsisGlyph = "\xE2\x80\xA6"; // U+2026 …
    } // namespace

    Pagination::Pagination(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        ensure_model();
        relayout();
    }

    auto Pagination::create(theme::NanTheme theme) -> std::shared_ptr<Pagination> {
        return std::make_shared<Pagination>(theme);
    }

    auto Pagination::clamp_page(const int page) const -> int {
        if (page_count_ < 1) {
            return 0;
        }
        return std::clamp(page, 1, page_count_);
    }

    void Pagination::set_page_count(const int count) {
        const int next = std::max(0, count);
        if (next == page_count_) {
            return;
        }
        page_count_ = next;
        current_page_ = clamp_page(current_page_);
        model_dirty_ = true;
        mark_layout_dirty();
        mark_semantics_dirty();
        relayout();
    }

    auto Pagination::page_count() const -> int {
        return page_count_;
    }

    void Pagination::set_current_page(const int page) {
        const int next = clamp_page(page);
        if (next == current_page_) {
            return;
        }
        current_page_ = next;
        model_dirty_ = true;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics);
        relayout();
    }

    auto Pagination::current_page() const -> int {
        return current_page_;
    }

    void Pagination::set_sibling_count(const int siblings) {
        const int next = std::max(0, siblings);
        if (next == sibling_count_) {
            return;
        }
        sibling_count_ = next;
        model_dirty_ = true;
        mark_layout_dirty();
        relayout();
    }

    auto Pagination::sibling_count() const -> int {
        return sibling_count_;
    }

    void Pagination::set_disabled(const bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        if (disabled_) {
            hover_index_ = -1;
            focused_ = false;
            if (is_inside_tree() && get_tree()->focused_node() == this) {
                get_tree()->set_focus(nullptr);
            }
        }
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Pagination::disabled() const -> bool {
        return disabled_;
    }

    void Pagination::set_on_page_change(std::function<void(int)> callback) {
        on_page_change_ = std::move(callback);
    }

    auto Pagination::page_changed() const -> const reactive::Event<int>& {
        return page_changed_;
    }

    void Pagination::go_to_page(const int page) {
        if (disabled_ || page_count_ < 1) {
            return;
        }
        const int next = clamp_page(page);
        if (next == current_page_ || next < 1) {
            return;
        }
        current_page_ = next;
        model_dirty_ = true;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics);
        if (on_page_change_) {
            on_page_change_(current_page_);
        }
        page_changed_.emit(current_page_);
        relayout();
    }

    auto Pagination::visible_item_count() const -> std::size_t {
        return items_.size();
    }

    auto Pagination::roving_member_count() const -> std::size_t {
        return focus_.member_count();
    }

    auto Pagination::focused_item_index() const -> int {
        return focus_index_;
    }

    auto Pagination::item_is_page(const std::size_t index) const -> bool {
        return index < items_.size() && items_[index].kind == ItemKind::page;
    }

    auto Pagination::item_page(const std::size_t index) const -> int {
        return index < items_.size() ? items_[index].page : 0;
    }

    void Pagination::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_styles();
        mark_layout_dirty();
        relayout();
    }

    auto Pagination::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Pagination::set_override(theme::PaginationRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_styles();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
        relayout();
    }

    auto Pagination::visual_state() const -> theme::PaginationVisualState {
        if (disabled_) {
            return theme::PaginationVisualState::disabled;
        }
        if (focused_) {
            return theme::PaginationVisualState::focused;
        }
        return theme::PaginationVisualState::normal;
    }

    auto Pagination::resolved_style() const -> theme::ResolvedPaginationStyle {
        auto style = theme::resolve_pagination(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Pagination::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        for (auto& item: items_) {
            if (item.text) {
                item.text->apply_default_text_pipeline(pipeline);
            }
        }
        mark_layout_dirty();
    }

    void Pagination::apply_font_context(text::FontPipelineCache& context) {
        for (auto& item: items_) {
            if (item.text) {
                item.text->apply_font_context(context);
            }
        }
        mark_layout_dirty();
    }

    void Pagination::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_styles();
        mark_layout_dirty();
    }

    void Pagination::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_styles();
        mark_layout_dirty();
        relayout();
    }

    auto Pagination::visible_pages() const -> std::vector<int> {
        std::vector<int> pages;
        if (page_count_ < 1) {
            return pages;
        }
        const int siblings = std::max(0, sibling_count_);
        // 总量小的时候直接全展开，避免出现 "1 … 3" 这种只有一个间隙的省略号。
        if (page_count_ <= siblings * 2 + 5) {
            pages.reserve(static_cast<std::size_t>(page_count_));
            for (int page = 1; page <= page_count_; ++page) {
                pages.push_back(page);
            }
            return pages;
        }

        std::vector<int> wanted;
        wanted.push_back(1);
        wanted.push_back(page_count_);
        for (int page = current_page_ - siblings; page <= current_page_ + siblings; ++page) {
            if (page >= 1 && page <= page_count_) {
                wanted.push_back(page);
            }
        }
        std::ranges::sort(wanted);
        const auto unique_end = std::ranges::unique(wanted).begin();
        wanted.erase(unique_end, wanted.end());
        return wanted;
    }

    void Pagination::ensure_model() {
        if (model_dirty_) {
            rebuild_model();
        }
    }

    void Pagination::rebuild_model() {
        items_.clear();
        if (page_count_ >= 1) {
            Item previous;
            previous.kind = ItemKind::previous;
            previous.enabled = current_page_ > 1;
            items_.push_back(std::move(previous));

            const auto pages = visible_pages();
            for (std::size_t index = 0; index < pages.size(); ++index) {
                if (index > 0 && pages[index] - pages[index - 1] > 1) {
                    Item ellipsis;
                    ellipsis.kind = ItemKind::ellipsis;
                    ellipsis.enabled = false;
                    items_.push_back(std::move(ellipsis));
                }
                Item page;
                page.kind = ItemKind::page;
                page.page = pages[index];
                page.enabled = true;
                items_.push_back(std::move(page));
            }

            Item next;
            next.kind = ItemKind::next;
            next.enabled = current_page_ < page_count_;
            items_.push_back(std::move(next));
        }

        for (auto& item: items_) {
            std::string glyph;
            switch (item.kind) {
                case ItemKind::previous:
                    glyph = kPreviousGlyph;
                    break;
                case ItemKind::next:
                    glyph = kNextGlyph;
                    break;
                case ItemKind::ellipsis:
                    glyph = kEllipsisGlyph;
                    break;
                case ItemKind::page:
                    glyph = std::to_string(item.page);
                    break;
            }
            item.text = std::make_shared<primitives::Text>(std::move(glyph));
        }

        model_dirty_ = false;
        sync_roving();
    }

    void Pagination::sync_roving() {
        // focus_only：方向键只移动"哪个槽位带焦点"，翻页是 Enter / Space / 点击的显式动作。
        focus_.set_movement(RovingMovement::focus_only);
        focus_.set_orientation(RovingOrientation::horizontal);
        focus_.set_loop(true);
        // 成员 = 可见槽位（含省略号与边界处被禁用的上一页 / 下一页）；不可聚焦的槽位
        // 由 RovingFocus 统一跳过，容器不自己写环绕 / Home / End。
        focus_.sync(items_.size(), [this](const std::size_t index) {
            return index < items_.size() && items_[index].enabled;
        });

        const int slot = slot_of_current_page();
        focus_index_ = slot >= 0 ? slot : focus_.active_index();
        focus_.set_active_index(focus_index_);
    }

    auto Pagination::slot_of_current_page() const -> int {
        for (std::size_t index = 0; index < items_.size(); ++index) {
            if (items_[index].kind == ItemKind::page && items_[index].page == current_page_) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void Pagination::layout_items() {
        ensure_model();
        const auto style = resolved_style();
        const float box = style.metrics.box_size;
        const float gap = style.metrics.gap;
        const float content_height = std::max(style.metrics.min_height, box);
        const float container_height = height() > 0.0F ? height() : content_height;
        const float y = std::max(0.0F, (container_height - box) * 0.5F);

        float x = style.metrics.padding_x;
        for (auto& item: items_) {
            item.rect = foundation::NanRect::from_xywh(x, y, box, box);
            x += box + gap;
        }
    }

    auto Pagination::hit_item(const float local_x, const float local_y) const -> int {
        for (std::size_t index = 0; index < items_.size(); ++index) {
            const auto& rect = items_[index].rect;
            if (local_x >= rect.get_left() && local_x < rect.get_right()
                && local_y >= rect.get_top() && local_y < rect.get_bottom())
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void Pagination::activate_item(const int index) {
        if (index < 0 || static_cast<std::size_t>(index) >= items_.size()) {
            return;
        }
        // go_to_page() 会重建 items_，先拷出需要的字段再调用。
        const ItemKind kind = items_[static_cast<std::size_t>(index)].kind;
        const int page = items_[static_cast<std::size_t>(index)].page;
        const bool enabled = items_[static_cast<std::size_t>(index)].enabled;
        if (!enabled) {
            return;
        }
        switch (kind) {
            case ItemKind::previous:
                go_to_page(current_page_ - 1);
                break;
            case ItemKind::next:
                go_to_page(current_page_ + 1);
                break;
            case ItemKind::page:
                go_to_page(page);
                break;
            case ItemKind::ellipsis:
                break;
        }
    }

    auto Pagination::is_focusable() const -> bool {
        return !disabled_ && page_count_ >= 1;
    }

    auto Pagination::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::focus_enter) {
            focused_ = !disabled_;
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            return false;
        }
        if (event.type() == scene::EventType::focus_leave) {
            focused_ = false;
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            return false;
        }
        if (event.type() == scene::EventType::mouse_leave) {
            if (hover_index_ >= 0) {
                hover_index_ = -1;
                mark_dirty(scene::DirtyFlags::paint);
            }
            return false;
        }
        if (disabled_) {
            return false;
        }
        if (event.type() == scene::EventType::mouse_move) {
            // 槽位是绘制出来的，hover 需要自己按位置命中。
            layout_items();
            const auto local = to_local(static_cast<scene::MouseMoveEvent&>(event).screen_pos());
            const int index = hit_item(local.get_x(), local.get_y());
            if (index != hover_index_) {
                hover_index_ = index;
                mark_dirty(scene::DirtyFlags::paint);
            }
            return false;
        }
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            ensure_model();
            layout_items();
            const auto local = to_local(mouse.screen_pos());
            const int index = hit_item(local.get_x(), local.get_y());
            if (index < 0) {
                return false;
            }
            // 点击把漫游焦点移到被点槽位（漫游组成员身份由容器映射）。
            focus_index_ = index;
            focus_.set_active_index(index);
            activate_item(index);
            event.accept();
            return true;
        }
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (!key.is_pressed()) {
                return false;
            }
            ensure_model();
            if (key.keycode() == keys::enter || key.keycode() == keys::space) {
                if (focus_index_ < 0) {
                    return false;
                }
                activate_item(focus_index_);
                event.accept();
                return true;
            }
            // 方向键 / Home / End / PageUp / PageDown 统一交给共享漫游设施；容器只把
            // Intent 的 index 落到"哪个槽位带视觉焦点"，不自己实现环绕 / 边界逻辑。
            const auto intent = focus_.handle_key(key);
            if (!intent.has_value() || intent->index < 0) {
                return false;
            }
            focus_index_ = intent->index;
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            event.accept();
            return true;
        }
        return false;
    }

    auto Pagination::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        ensure_model();
        const auto style = resolved_style();
        if (items_.empty()) {
            // 没有可翻页的内容：不占位（与 EmptyState / Alert 的条件挂载一致）。
            return constraints.constrain(foundation::NanSize(0.0F, 0.0F));
        }
        const float box = style.metrics.box_size;
        const float gap = style.metrics.gap;
        const float count = static_cast<float>(items_.size());
        const float width = style.metrics.padding_x * 2.0F + count * box
            + std::max(0.0F, count - 1.0F) * gap;
        const float height = std::max(style.metrics.min_height, box);
        return constraints.constrain(foundation::NanSize(width, height));
    }

    void Pagination::on_layout() {
        layout_items();
    }

    void Pagination::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    void Pagination::on_process(const float dt) {
        // 漫游设施内部的 typeahead 缓冲按时间衰减（本组件未接文本输入，但保持一致）。
        focus_.advance_time(dt);
    }

    auto Pagination::on_draw(render::DrawContext& context) -> void {
        ensure_model();
        layout_items();
        apply_text_styles();
        const auto style = resolved_style();
        const auto& transform = context.world_transform();
        const float opacity = context.opacity();

        primitives::BoxPainter::paint(
            context,
            render::world_bounds_from_local(transform, local_rect()),
            style.container,
            opacity
        );

        for (std::size_t index = 0; index < items_.size(); ++index) {
            auto& item = items_[index];
            const bool is_current = item.kind == ItemKind::page && item.page == current_page_;
            const auto& face = is_current ? style.item_active : style.item;
            const auto world = render::world_bounds_from_local(transform, item.rect);
            primitives::BoxPainter::paint(context, world, face, opacity);

            if (static_cast<int>(index) == hover_index_ && item.enabled
                && style.item_hover.alpha() > 0.0F)
            {
                primitives::BoxPainter::paint_fill(
                    context,
                    world,
                    theme::ResolvedBoxStyle {
                        .fill = style.item_hover,
                        .border = style.item_hover.with_alpha(0.0F),
                        .border_width = 0.0F,
                        .radius = face.radius,
                    },
                    opacity
                );
            }

            if (item.text) {
                const float text_width = context.logical_to_screen(item.text->measured_text_width());
                const float text_height =
                    context.logical_to_screen(item.text->measured_text_height());
                const auto position = foundation::NanPoint(
                    world.get_left() + (world.get_width() - text_width) * 0.5F,
                    world.get_top() + (world.get_height() - text_height) * 0.5F
                );
                item.text->draw_at(context, position);
            }
        }

        const bool focus_slot_valid = focus_index_ >= 0
            && static_cast<std::size_t>(focus_index_) < items_.size();
        if (focused_ && focus_slot_valid && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(
                context,
                render::world_bounds_from_local(
                    transform,
                    items_[static_cast<std::size_t>(focus_index_)].rect
                ),
                style.focus,
                opacity
            );
        }
    }

    auto Pagination::semantics_properties() const -> semantics::Properties {
        if (page_count_ < 1) {
            return {};
        }
        // 页码槽位是绘制出来的，无法各自成为语义节点（见文档"已知空白"）；容器用
        // generic + "Page X of Y" 暴露当前位置。
        return {
            .role = semantics::Role::generic,
            .label = "Page " + std::to_string(current_page_) + " of "
                + std::to_string(page_count_),
            .value = std::to_string(current_page_),
            .state =
                {
                    .focusable = !disabled_,
                    .focused = focused_,
                    .disabled = disabled_,
                },
            .actions =
                disabled_ ? semantics::Action::none
                          : (semantics::Action::focus | semantics::Action::activate),
        };
    }

    void Pagination::apply_text_styles() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const float disabled_alpha = system_->tokens.opacity.disabled;

        for (std::size_t index = 0; index < items_.size(); ++index) {
            auto& item = items_[index];
            if (!item.text) {
                continue;
            }
            const bool is_current = item.kind == ItemKind::page && item.page == current_page_;
            const auto& type = is_current ? style.label_active : style.label;
            foundation::NanColor color =
                item.kind == ItemKind::ellipsis ? style.ellipsis : type.color;
            if (!item.enabled) {
                // 边界处不可用的上一页 / 下一页弱化（与 disabled 同一不透明度 token）。
                color = color.with_alpha(color.alpha() * disabled_alpha);
            }
            const primitives::TextStyle text_style {
                .color = context.text_color_from_context ? context.text_color : color,
                .font_size = context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : item.text->font(),
                .overflow = primitives::TextOverflow::clip,
                .max_lines = 1,
            };
            if (!same_text_style(item.text->style(), text_style)) {
                item.text->set_style(text_style);
            }
            (void)item.text->measure_layout(scene::LayoutConstraints::loose());
        }
    }

    void Pagination::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }
} // namespace nandina::widget
