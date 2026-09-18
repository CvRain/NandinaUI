//
// widget/select - single-choice dropdown with a popup option list.
//

#include "select.hpp"

#include "key_codes.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "internal/anchored_positioner.hpp"
#include "internal/dismiss_layer.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

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

        /// Build a text style from a resolved type style plus the inherited style
        /// context. Shared by the in-tree option texts and the portal popup so both
        /// render the same field/value/option typography.
        [[nodiscard]] auto make_text_style(
            const theme::ResolvedStyleContext& context,
            const theme::ResolvedTypeStyle& type,
            const text::FontRequest& fallback_font
        ) -> primitives::TextStyle {
            return primitives::TextStyle {
                .color = context.text_color_from_context ? context.text_color : type.color,
                .font_size = context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : fallback_font,
                .overflow = primitives::TextOverflow::clip,
                .max_lines = 1,
            };
        }

        class SelectPopup final: public scene::NanControl {
        public:
            SelectPopup(
                float width,
                std::vector<std::string> options,
                theme::ResolvedSelectStyle style,
                primitives::TextPipeline pipeline,
                primitives::TextStyle option_style,
                primitives::TextStyle option_selected_style,
                int selected_index,
                std::weak_ptr<Select> owner,
                std::function<void(int)> on_select
            ):
                width_(width),
                options_(std::move(options)),
                style_(std::move(style)),
                option_style_(std::move(option_style)),
                option_selected_style_(std::move(option_selected_style)),
                owner_(std::move(owner)),
                on_select_(std::move(on_select)) {
                option_texts_.reserve(options_.size());
                for (const auto& option: options_) {
                    auto text = std::make_shared<primitives::Text>(option);
                    text->set_text_pipeline(pipeline);
                    option_texts_.push_back(std::move(text));
                }
                set_selected(selected_index);
            }

            /// Move the highlight only. Arrow-key navigation must not rebuild the
            /// popup (and every option text) on each key press.
            void set_selected(const int index) {
                selected_index_ = index;
                for (std::size_t i = 0; i < option_texts_.size(); ++i) {
                    const auto& next = static_cast<int>(i) == selected_index_
                        ? option_selected_style_
                        : option_style_;
                    if (!same_text_style(option_texts_[i]->style(), next)) {
                        option_texts_[i]->set_style(next);
                    }
                }
                mark_paint_dirty();
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

            /// The popup sits in the overlay layer while its select sits in the
            /// content layer. A click on the popup belongs to the select, so focus
            /// must stay on the field instead of being cleared.
            [[nodiscard]] auto focus_delegate() const -> scene::NanNode2D* override {
                return owner_.lock().get();
            }

            auto on_input(scene::InputEvent& event) -> bool override {
                if (event.type() != scene::EventType::mouse_button) {
                    return false;
                }
                auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
                if (!mouse.is_pressed() || mouse.button() != scene::MouseButtonEvent::Button::left) {
                    return false;
                }
                const auto local = to_local(mouse.screen_pos());
                const auto row = style_.metrics.min_height;
                const auto index = row > 0.0F
                    ? static_cast<int>(local.get_y() / row)
                    : -1;
                if (index >= 0 && index < static_cast<int>(options_.size()) && on_select_) {
                    on_select_(index);
                    event.accept();
                    return true;
                }
                return false;
            }

        protected:
            [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto height = style_.metrics.min_height * static_cast<float>(options_.size());
                return constraints.constrain(foundation::NanSize(width_, height));
            }

            void on_draw(render::DrawContext& context) override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                primitives::BoxPainter::paint(context, world, style_.popup, context.opacity());
                const float row_height = context.logical_to_screen(style_.metrics.min_height);
                for (std::size_t index = 0; index < option_texts_.size(); ++index) {
                    auto& text = *option_texts_[index];
                    (void)text.measure_layout(scene::LayoutConstraints::loose());
                    const float text_height =
                        context.logical_to_screen(text.measured_text_height());
                    text.draw_at(
                        context,
                        foundation::NanPoint(
                            world.get_left() + context.logical_to_screen(style_.metrics.padding_x),
                            world.get_top() + row_height * static_cast<float>(index)
                                + (row_height - text_height) * 0.5F
                        )
                    );
                }
            }

        private:
            float width_ = 0.0F;
            std::vector<std::string> options_;
            theme::ResolvedSelectStyle style_;
            primitives::TextStyle option_style_;
            primitives::TextStyle option_selected_style_;
            std::vector<std::shared_ptr<primitives::Text>> option_texts_;
            std::weak_ptr<Select> owner_;
            int selected_index_ = 0;
            std::function<void(int)> on_select_;
        };
    } // namespace

    Select::~Select() = default;

    Select::Select(std::vector<std::string> options, theme::NanTheme theme):
        options_(std::move(options)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        rebuild_texts();
        apply_text_styles();
        const auto style = resolved_style();
        set_size(foundation::NanSize(style.metrics.preferred_width, style.metrics.height));
    }

    auto Select::create(std::vector<std::string> options, theme::NanTheme theme)
        -> std::shared_ptr<Select> {
        return std::make_shared<Select>(std::move(options), theme);
    }

    void Select::set_options(std::vector<std::string> options) {
        options_ = std::move(options);
        if (selected_index_ >= static_cast<int>(options_.size())) {
            selected_index_ = options_.empty() ? 0 : static_cast<int>(options_.size()) - 1;
        }
        rebuild_texts();
        apply_text_styles();
        mark_layout_dirty();
        mark_semantics_dirty();
        refresh_portal();
    }

    auto Select::option_count() const -> std::size_t {
        return options_.size();
    }

    auto Select::option(const std::size_t index) const -> std::string_view {
        return index < options_.size() ? options_[index] : std::string_view {};
    }

    void Select::set_selected_index(const int index) {
        if (options_.empty()) {
            selected_index_ = 0;
            return;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(options_.size()) - 1);
        if (selected_index_ == clamped) {
            return;
        }
        selected_index_ = clamped;
        apply_text_styles();
        // The portal popup owns its own option texts, so move its highlight in place
        // instead of rebuilding it (arrow-key navigation changes the index often).
        if (portal_set_selected_) {
            portal_set_selected_(clamped);
        }
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Select::selected_index() const -> int {
        return selected_index_;
    }

    auto Select::selected_label() const -> std::string_view {
        return option(static_cast<std::size_t>(selected_index_));
    }

    void Select::select(const int index) {
        if (disabled_) {
            return;
        }
        const int before = selected_index_;
        // Close before touching the index: `set_selected_index()` refreshes the
        // portal, and doing that while still open would tear the popup down and
        // rebuild it only to close it again — and it would destroy the popup from
        // inside its own input handler.
        close();
        set_selected_index(index);
        if (before != selected_index_) {
            if (on_change_) {
                on_change_(selected_index_);
            }
            selection_changed_.emit(selected_index_);
        }
    }

    void Select::open() {
        if (disabled_ || open_) {
            return;
        }
        open_ = true;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void Select::close() {
        if (!open_) {
            return;
        }
        open_ = false;
        close_portal();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Select::is_open() const -> bool {
        return open_;
    }

    void Select::set_disabled(const bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        if (disabled_) {
            open_ = false;
            close_portal();
            focused_ = false;
            if (is_inside_tree() && get_tree()->focused_node() == this) {
                get_tree()->set_focus(nullptr);
            }
        }
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Select::disabled() const -> bool {
        return disabled_;
    }

    void Select::set_on_change(std::function<void(int)> callback) {
        on_change_ = std::move(callback);
    }

    auto Select::selection_changed() const -> const reactive::Event<int>& {
        return selection_changed_;
    }

    void Select::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_styles();
        refresh_portal();
        mark_layout_dirty();
    }

    auto Select::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Select::set_override(theme::SelectRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_styles();
        refresh_portal();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Select::visual_state() const -> theme::SelectVisualState {
        if (disabled_) {
            return theme::SelectVisualState::disabled;
        }
        if (focused_) {
            return theme::SelectVisualState::focused;
        }
        return theme::SelectVisualState::normal;
    }

    auto Select::resolved_style() const -> theme::ResolvedSelectStyle {
        auto style = theme::resolve_select(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Select::set_text_pipeline(primitives::TextPipeline pipeline) {
        value_text_.set_text_pipeline(pipeline);
        for (auto& text: option_texts_) {
            text->set_text_pipeline(pipeline);
        }
        refresh_portal();
        mark_layout_dirty();
    }

    void Select::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        value_text_.apply_default_text_pipeline(pipeline);
        for (auto& text: option_texts_) {
            text->apply_default_text_pipeline(pipeline);
        }
        refresh_portal();
        mark_layout_dirty();
    }

    void Select::apply_font_context(text::FontPipelineCache& context) {
        value_text_.apply_font_context(context);
        for (auto& text: option_texts_) {
            text->apply_font_context(context);
        }
        refresh_portal();
        mark_layout_dirty();
    }

    void Select::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_styles();
        refresh_portal();
        mark_layout_dirty();
    }

    void Select::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_styles();
        refresh_portal();
        mark_layout_dirty();
    }

    auto Select::z_index_hint() const -> int {
        return open_ ? 1 : 0;
    }

    auto Select::global_bounds() const -> foundation::NanRect {
        if (!open_ || portal_handle_ != nullptr) {
            const auto style = resolved_style();
            return render::world_bounds_from_local(
                global_transform(),
                foundation::NanRect::from_xywh(0.0F, 0.0F, width(), style.metrics.height)
            );
        }
        const auto style = resolved_style();
        const float popup_height =
            style.metrics.min_height * static_cast<float>(options_.size());
        const auto extended = foundation::NanRect::from_xywh(
            0.0F,
            0.0F,
            width(),
            style.metrics.height + style.metrics.gap + popup_height
        );
        return render::world_bounds_from_local(global_transform(), extended);
    }

    auto Select::contains_point(const foundation::NanPoint local_point) const -> bool {
        const auto style = resolved_style();
        if (local_point.get_x() >= 0.0F && local_point.get_x() <= width()
            && local_point.get_y() >= 0.0F && local_point.get_y() <= style.metrics.height)
        {
            return true;
        }
        if (!open_ || portal_handle_ != nullptr) {
            return false;
        }
        const float popup_top = style.metrics.height + style.metrics.gap;
        const float popup_bottom =
            popup_top + style.metrics.min_height * static_cast<float>(options_.size());
        return local_point.get_x() >= 0.0F && local_point.get_x() <= width()
            && local_point.get_y() >= popup_top && local_point.get_y() <= popup_bottom;
    }

    auto Select::is_focusable() const -> bool {
        return !disabled_ && !options_.empty();
    }

    void Select::sync_roving() {
        focus_.set_movement(RovingMovement::selection_only);
        focus_.set_orientation(RovingOrientation::vertical);
        const auto& options = options_;
        focus_.sync(
            options.size(),
            {},
            [&options](std::size_t index) -> std::string_view { return options[index]; }
        );
        focus_.set_active_index(selected_index_);
    }

    auto Select::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::focus_enter) {
            focused_ = !disabled_;
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            return false;
        }
        if (event.type() == scene::EventType::focus_leave) {
            focused_ = false;
            close();
            mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            return false;
        }
        if (disabled_) {
            return false;
        }
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            const auto local = to_local(mouse.screen_pos());
            if (open_) {
                if (portal_handle_ != nullptr) {
                    close();
                }
                else {
                    const int hit = hit_option(local.get_y());
                    if (hit >= 0) {
                        select(hit);
                    }
                    else {
                        close();
                    }
                }
            }
            else if (local.get_y() >= 0.0F && local.get_y() <= height()) {
                open();
            }
            event.accept();
            return true;
        }
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (!key.is_pressed()) {
                return false;
            }
            if (key.keycode() == keys::escape && open_) {
                close();
                event.accept();
                return true;
            }
            if (key.keycode() == keys::enter || key.keycode() == keys::space) {
                if (open_) {
                    select(selected_index_);
                }
                else {
                    open();
                }
                event.accept();
                return true;
            }
            if (open_ && !options_.empty()) {
                // 弹出列表的漫游：selection_only（焦点留在触发字段上），垂直方向，
                // 方向键 / Home / End / PageUp / PageDown 统一交给共享设施。
                sync_roving();
                const auto intent = focus_.handle_key(key);
                if (intent.has_value()) {
                    set_selected_index(intent->index);
                    event.accept();
                    return true;
                }
            }
            return false;
        }
        if (event.type() == scene::EventType::text_input && open_ && !options_.empty()) {
            // 弹出列表打开时按字母跳转（typeahead）。
            sync_roving();
            const auto intent = focus_.handle_text(static_cast<scene::TextInputEvent&>(event));
            if (intent.has_value()) {
                set_selected_index(intent->index);
                event.accept();
                return true;
            }
            return false;
        }
        return false;
    }

    void Select::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        const float opacity = context.opacity();

        apply_text_styles();

        // 触发字段。
        const auto field = foundation::NanRect::from_xywh(
            world.get_left(),
            world.get_top(),
            world.get_width(),
            context.logical_to_screen(style.metrics.height)
        );
        primitives::BoxPainter::paint(context, field, style.container, opacity);

        (void)value_text_.measure_layout(scene::LayoutConstraints::loose());
        const float value_height =
            context.logical_to_screen(value_text_.measured_text_height());
        const auto value_pos = foundation::NanPoint(
            field.get_left() + context.logical_to_screen(style.metrics.padding_x),
            field.get_top() + (field.get_height() - value_height) * 0.5F
        );
        value_text_.draw_at(context, value_pos);

        // 折叠箭头（右端）。
        const float cx = field.get_right() - context.logical_to_screen(style.metrics.padding_x);
        const float cy = field.get_top() + field.get_height() * 0.5F;
        const float arm = context.logical_to_screen(4.0F);
        const auto arrow_color = style.value.color.with_alpha(style.value.color.alpha() * opacity);
        context.device().draw_line(
            foundation::NanPoint(cx - arm, cy - arm * 0.5F),
            foundation::NanPoint(cx, cy + arm * 0.5F),
            context.logical_to_screen(1.5F),
            arrow_color
        );
        context.device().draw_line(
            foundation::NanPoint(cx, cy + arm * 0.5F),
            foundation::NanPoint(cx + arm, cy - arm * 0.5F),
            context.logical_to_screen(1.5F),
            arrow_color
        );

        // 弹出列表。
        if (open_ && portal_handle_ == nullptr) {
            const float row_h = context.logical_to_screen(style.metrics.min_height);
            const float gap = context.logical_to_screen(style.metrics.gap);
            const float popup_top = field.get_bottom() + gap;
            const float popup_h = row_h * static_cast<float>(options_.size());
            const auto popup = foundation::NanRect::from_xywh(
                field.get_left(),
                popup_top,
                field.get_width(),
                popup_h
            );
            primitives::BoxPainter::paint(context, popup, style.popup, opacity);

            for (std::size_t i = 0; i < option_texts_.size(); ++i) {
                (void)option_texts_[i]->measure_layout(scene::LayoutConstraints::loose());
                const float option_height =
                    context.logical_to_screen(option_texts_[i]->measured_text_height());
                const auto pos = foundation::NanPoint(
                    popup.get_left() + context.logical_to_screen(style.metrics.padding_x),
                    popup.get_top() + row_h * static_cast<float>(i)
                        + (row_h - option_height) * 0.5F
                );
                option_texts_[i]->draw_at(context, pos);
            }
        }

        if (focused_ && !disabled_ && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(context, field, style.focus, opacity);
        }
    }

    void Select::on_process(const float dt) {
        // typeahead 缓冲按时间衰减。
        focus_.advance_time(dt);
        if (open_) {
            sync_portal();
        }
    }

    void Select::on_exit_tree() {
        open_ = false;
        close_portal();
    }

    void Select::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ =
            host != nullptr ? host->weak_self() : std::weak_ptr<scene::OverlayHost> {};
    }

    auto Select::resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost> {
        if (auto injected = overlay_service_.lock()) {
            return injected;
        }
        for (auto* node = parent(); node != nullptr; node = node->parent()) {
            auto* node_2d = node->as_node2d();
            auto* stack = node_2d != nullptr ? node_2d->as_layer_stack() : nullptr;
            if (stack == nullptr) {
                continue;
            }
            if (auto* host = stack->as_overlay_host(); host != nullptr) {
                return host->weak_self().lock();
            }
        }
        return nullptr;
    }

    void Select::sync_portal() {
        if (!open_) {
            close_portal();
            return;
        }
        auto host = resolve_overlay_host();
        if (host == nullptr || options_.empty()) {
            close_portal();
            return;
        }
        const auto viewport_size = host->viewport_size();
        const auto anchor = scene::NanControl::global_bounds();
        if (!viewport_size.is_valid() || !anchor.is_valid() || width() <= 0.0F) {
            return;
        }
        if (portal_handle_ != nullptr && !portal_handle_->mounted()) {
            close_portal();
        }
        if (portal_handle_ != nullptr && anchor == portal_anchor_
            && viewport_size == portal_viewport_)
        {
            return;
        }

        const auto style = resolved_style();
        const auto viewport =
            foundation::NanRect::from_origin_size(foundation::NanPoint::zero(), viewport_size);
        const internal::AnchoredPositionOptions options {
            .placement = internal::OverlayPlacement::bottom,
            .alignment = internal::OverlayAlignment::start,
            .gap = style.metrics.gap,
            .viewport_padding = style.metrics.padding_x,
        };

        // Move the existing popup instead of rebuilding it: while the page scrolls
        // the anchor changes every frame, and a rebuild would re-create every option
        // text object each time. Only opening or a content change recreates the popup.
        if (auto popup = portal_popup_.lock(); popup != nullptr && portal_handle_ != nullptr) {
            const auto popup_size = popup->measure_layout(scene::LayoutConstraints::loose());
            if (!popup_size.is_valid()) {
                return;
            }
            const auto position =
                internal::position_anchored_overlay(anchor, popup_size, viewport, options);
            popup->set_position(position.rect.get_top_left());
            portal_anchor_ = anchor;
            portal_viewport_ = viewport_size;
            return;
        }

        close_portal();
        const auto& context = resolved_style_context();
        const auto& option_font =
            option_texts_.empty() ? value_text_.font() : option_texts_.front()->font();
        auto self = std::static_pointer_cast<Select>(shared_from_this());
        auto popup = std::make_shared<SelectPopup>(
            width(),
            options_,
            style,
            value_text_.text_pipeline(),
            make_text_style(context, style.option, option_font),
            make_text_style(context, style.option_selected, option_font),
            selected_index_,
            std::weak_ptr<Select>(self),
            [weak = std::weak_ptr<Select>(self)](const int index) {
                if (auto select = weak.lock()) {
                    select->select(index);
                }
            }
        );
        const auto popup_size = popup->measure_layout(scene::LayoutConstraints::loose());
        if (!popup_size.is_valid()) {
            return;
        }
        const auto position =
            internal::position_anchored_overlay(anchor, popup_size, viewport, options);
        popup->set_position(position.rect.get_top_left());
        popup->layout_to(
            foundation::NanRect::from_origin_size(position.rect.get_top_left(), popup_size)
        );

        auto dismiss = std::make_shared<internal::DismissLayer>(
            [weak = std::weak_ptr<Select>(self)](const internal::DismissReason) {
                if (auto select = weak.lock()) {
                    select->close();
                }
            }
        );
        dismiss->set_content(popup);
        portal_popup_ = popup;
        portal_set_selected_ = [weak = std::weak_ptr<SelectPopup>(popup)](const int index) {
            if (auto popup_ptr = weak.lock()) {
                popup_ptr->set_selected(index);
            }
        };
        // 模态内容里再展开的下拉必须压过模态遮罩，否则会被埋掉且点不到。
        const auto level = host->hosts_node(*this) ? scene::OverlayLevel::nested_popup
                                                   : scene::OverlayLevel::popup;
        portal_handle_ =
            std::make_unique<scene::OverlayHandle>(host->present(std::move(dismiss), {.level = level}));
        portal_anchor_ = anchor;
        portal_viewport_ = viewport_size;
    }

    void Select::refresh_portal() {
        if (open_) {
            close_portal();
            sync_portal();
        }
    }

    void Select::close_portal() {
        if (portal_handle_ != nullptr) {
            portal_handle_->close();
            portal_handle_.reset();
        }
        portal_popup_.reset();
        portal_set_selected_ = nullptr;
        portal_anchor_ = foundation::NanRect {};
        portal_viewport_ = foundation::NanSize {};
    }

    auto Select::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        apply_text_styles();
        float max_width = style.metrics.preferred_width;
        for (auto& text: option_texts_) {
            (void)text->measure_layout(scene::LayoutConstraints::loose());
            max_width = std::max(
                max_width,
                text->measured_text_width() + style.metrics.padding_x * 2.0F
            );
        }
        return constraints.constrain(foundation::NanSize(max_width, style.metrics.height));
    }

    auto Select::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::combobox,
            .label = std::string(selected_label()),
            .value = std::to_string(selected_index_),
            .state =
                {
                    .focusable = !disabled_ && !options_.empty(),
                    .focused = focused_,
                    .disabled = disabled_,
                },
            .actions = disabled_ ? semantics::Action::none : semantics::Action::focus,
        };
    }

    void Select::rebuild_texts() {
        value_text_.set_text(std::string(selected_label()));
        option_texts_.clear();
        option_texts_.reserve(options_.size());
        for (const auto& option: options_) {
            option_texts_.push_back(std::make_shared<primitives::Text>(option));
        }
    }

    void Select::apply_text_styles() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        value_text_.set_text(std::string(selected_label()));
        const auto value_style = make_text_style(context, style.value, value_text_.font());
        if (!same_text_style(value_text_.style(), value_style)) {
            value_text_.set_style(value_style);
        }
        for (std::size_t i = 0; i < option_texts_.size(); ++i) {
            const auto& type =
                static_cast<int>(i) == selected_index_ ? style.option_selected : style.option;
            const auto option_style = make_text_style(context, type, option_texts_[i]->font());
            if (!same_text_style(option_texts_[i]->style(), option_style)) {
                option_texts_[i]->set_style(option_style);
            }
        }
    }

    auto Select::hit_option(const float local_y) const -> int {
        const auto style = resolved_style();
        const float popup_top = style.metrics.height + style.metrics.gap;
        const float row_h = style.metrics.min_height;
        const int index = static_cast<int>((local_y - popup_top) / row_h);
        if (index < 0 || index >= static_cast<int>(options_.size())) {
            return -1;
        }
        return index;
    }
} // namespace nandina::widget
