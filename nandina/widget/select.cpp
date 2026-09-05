//
// widget/select - single-choice dropdown with a popup option list.
//

#include "select.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        constexpr int key_up = 265;
        constexpr int key_down = 264;
        constexpr int key_enter = 257;
        constexpr int key_space = 32;
        constexpr int key_escape = 256;

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
        set_selected_index(index);
        close();
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
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void Select::close() {
        if (!open_) {
            return;
        }
        open_ = false;
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
        mark_layout_dirty();
    }

    auto Select::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Select::set_override(theme::SelectRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_styles();
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
        mark_layout_dirty();
    }

    void Select::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        value_text_.apply_default_text_pipeline(pipeline);
        for (auto& text: option_texts_) {
            text->apply_default_text_pipeline(pipeline);
        }
        mark_layout_dirty();
    }

    void Select::apply_font_context(text::FontPipelineCache& context) {
        value_text_.apply_font_context(context);
        for (auto& text: option_texts_) {
            text->apply_font_context(context);
        }
        mark_layout_dirty();
    }

    void Select::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_styles();
        mark_layout_dirty();
    }

    void Select::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_styles();
        mark_layout_dirty();
    }

    auto Select::z_index_hint() const -> int {
        return open_ ? 1 : 0;
    }

    auto Select::global_bounds() const -> foundation::NanRect {
        if (!open_) {
            return scene::NanControl::global_bounds();
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
        if (scene::NanControl::contains_point(local_point)) {
            return true;
        }
        if (!open_) {
            return false;
        }
        const auto style = resolved_style();
        const float popup_top = style.metrics.height + style.metrics.gap;
        const float popup_bottom =
            popup_top + style.metrics.min_height * static_cast<float>(options_.size());
        return local_point.get_x() >= 0.0F && local_point.get_x() <= width()
            && local_point.get_y() >= popup_top && local_point.get_y() <= popup_bottom;
    }

    auto Select::is_focusable() const -> bool {
        return !disabled_ && !options_.empty();
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
                const int hit = hit_option(local.get_y());
                if (hit >= 0) {
                    select(hit);
                }
                else {
                    close();
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
            if (key.keycode() == key_escape && open_) {
                close();
                event.accept();
                return true;
            }
            if (key.keycode() == key_enter || key.keycode() == key_space) {
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
                int direction = 0;
                if (key.keycode() == key_up) {
                    direction = -1;
                }
                else if (key.keycode() == key_down) {
                    direction = 1;
                }
                if (direction != 0) {
                    const int size = static_cast<int>(options_.size());
                    set_selected_index((selected_index_ + direction + size) % size);
                    event.accept();
                    return true;
                }
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
        if (open_) {
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
        const primitives::TextStyle value_style {
            .color = context.text_color_from_context ? context.text_color : style.value.color,
            .font_size = context.font_size_from_context ? context.font_size : style.value.font_size,
            .font = context.font_from_context ? context.font : value_text_.font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        if (!same_text_style(value_text_.style(), value_style)) {
            value_text_.set_style(value_style);
        }
        for (std::size_t i = 0; i < option_texts_.size(); ++i) {
            const auto& type =
                static_cast<int>(i) == selected_index_ ? style.option_selected : style.option;
            const primitives::TextStyle option_style {
                .color = context.text_color_from_context ? context.text_color : type.color,
                .font_size =
                    context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : option_texts_[i]->font(),
                .overflow = primitives::TextOverflow::clip,
                .max_lines = 1,
            };
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
