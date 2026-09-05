//
// widget/tabs - horizontal tab bar with single selection and roving keyboard focus.
//

#include "tabs.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../animation/animation_host.hpp"
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
        constexpr int key_right = 262;
        constexpr int key_left = 263;
        constexpr int key_down = 264;
        constexpr int key_up = 265;

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

    Tabs::Tabs(std::vector<std::string> labels, theme::NanTheme theme):
        labels_(std::move(labels)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        rebuild_texts();
        apply_text_styles();
        measure_labels();
    }

    auto Tabs::create(std::vector<std::string> labels, theme::NanTheme theme)
        -> std::shared_ptr<Tabs> {
        return std::make_shared<Tabs>(std::move(labels), theme);
    }

    void Tabs::set_labels(std::vector<std::string> labels) {
        labels_ = std::move(labels);
        if (selected_index_ >= static_cast<int>(labels_.size())) {
            selected_index_ = labels_.empty() ? 0 : static_cast<int>(labels_.size()) - 1;
        }
        rebuild_texts();
        apply_text_styles();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Tabs::label_count() const -> std::size_t {
        return labels_.size();
    }

    auto Tabs::label(const std::size_t index) const -> std::string_view {
        return index < labels_.size() ? labels_[index] : std::string_view {};
    }

    void Tabs::set_selected_index(const int index) {
        if (labels_.empty()) {
            selected_index_ = 0;
            return;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(labels_.size()) - 1);
        if (selected_index_ == clamped) {
            return;
        }
        selected_index_ = clamped;
        apply_text_styles();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Tabs::selected_index() const -> int {
        return selected_index_;
    }

    auto Tabs::selected_label() const -> std::string_view {
        return label(static_cast<std::size_t>(selected_index_));
    }

    void Tabs::select(const int index) {
        if (disabled_) {
            return;
        }
        const int before = selected_index_;
        set_selected_index(index);
        if (before == selected_index_) {
            return;
        }
        sync_indicator(true);
        if (on_change_) {
            on_change_(selected_index_);
        }
        selection_changed_.emit(selected_index_);
    }

    void Tabs::set_disabled(const bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        if (disabled_) {
            hovered_ = false;
            focused_ = false;
            if (is_inside_tree() && get_tree()->focused_node() == this) {
                get_tree()->set_focus(nullptr);
            }
        }
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Tabs::disabled() const -> bool {
        return disabled_;
    }

    void Tabs::set_on_change(std::function<void(int)> callback) {
        on_change_ = std::move(callback);
    }

    auto Tabs::selection_changed() const -> const reactive::Event<int>& {
        return selection_changed_;
    }

    void Tabs::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_styles();
        mark_layout_dirty();
    }

    auto Tabs::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Tabs::set_override(theme::TabsRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_styles();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Tabs::visual_state() const -> theme::TabsVisualState {
        if (disabled_) {
            return theme::TabsVisualState::disabled;
        }
        if (focused_) {
            return theme::TabsVisualState::focused;
        }
        return theme::TabsVisualState::normal;
    }

    auto Tabs::resolved_style() const -> theme::ResolvedTabsStyle {
        auto style = theme::resolve_tabs(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Tabs::set_text_pipeline(primitives::TextPipeline pipeline) {
        for (auto& text: label_texts_) {
            text->set_text_pipeline(pipeline);
        }
        mark_layout_dirty();
    }

    void Tabs::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        for (auto& text: label_texts_) {
            text->apply_default_text_pipeline(pipeline);
        }
        mark_layout_dirty();
    }

    void Tabs::apply_font_context(text::FontPipelineCache& context) {
        for (auto& text: label_texts_) {
            text->apply_font_context(context);
        }
        mark_layout_dirty();
    }

    void Tabs::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_styles();
        mark_layout_dirty();
    }

    void Tabs::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_styles();
        mark_layout_dirty();
    }

    auto Tabs::is_focusable() const -> bool {
        return !disabled_ && !labels_.empty();
    }

    auto Tabs::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_enter) {
            hovered_ = !disabled_;
            return false;
        }
        if (event.type() == scene::EventType::mouse_leave) {
            hovered_ = false;
            return false;
        }
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
        if (disabled_) {
            return false;
        }
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            const int index = hit_index(to_local(mouse.screen_pos()).get_x());
            if (index >= 0) {
                select(index);
                event.accept();
                return true;
            }
            return false;
        }
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (!key.is_pressed()) {
                return false;
            }
            int direction = 0;
            if (key.keycode() == key_left || key.keycode() == key_up) {
                direction = -1;
            }
            else if (key.keycode() == key_right || key.keycode() == key_down) {
                direction = 1;
            }
            if (direction != 0 && !labels_.empty()) {
                const int size = static_cast<int>(labels_.size());
                select((selected_index_ + direction + size) % size);
                event.accept();
                return true;
            }
            return false;
        }
        return false;
    }

    void Tabs::on_draw(render::DrawContext& context) {
        const auto style = resolved_style();
        const auto world = render::world_bounds_from_local(context.world_transform(), local_rect());
        const float opacity = context.opacity();

        apply_text_styles();
        measure_labels();

        // 1. 列表容器背景/边框（透明则 no-op）。
        primitives::BoxPainter::paint(context, world, style.container, opacity);

        const bool has_selected =
            selected_index_ >= 0 && static_cast<std::size_t>(selected_index_) < labels_.size();
        const float padding = context.logical_to_screen(style.metrics.padding_x);

        // 2. 选中标签 pill 背景（shadcn 风格，透明则跳过）。
        if (has_selected && style.selected_background.fill.alpha() > 0.0F) {
            const std::size_t selected = static_cast<std::size_t>(selected_index_);
            const float selected_x = context.logical_to_screen(tab_offsets_[selected]);
            const float selected_width =
                context.logical_to_screen(label_texts_[selected]->measured_text_width());
            const auto pill = foundation::NanRect::from_xywh(
                world.get_left() + selected_x - padding,
                world.get_top() + padding,
                selected_width + padding * 2.0F,
                std::max(0.0F, world.get_height() - padding * 2.0F)
            );
            primitives::BoxPainter::paint(context, pill, style.selected_background, opacity);
        }

        // 3. 标签。
        for (std::size_t i = 0; i < label_texts_.size(); ++i) {
            const float text_height = context.logical_to_screen(label_texts_[i]->measured_text_height());
            const auto position = foundation::NanPoint(
                world.get_left() + context.logical_to_screen(tab_offsets_[i]),
                world.get_top() + (world.get_height() - text_height) * 0.5F
            );
            label_texts_[i]->draw_at(context, position);
        }

        // 4. 下划线指示条（透明则跳过；x/宽由 tween 驱动，切换时平滑过渡）。
        if (has_selected && style.indicator.alpha() > 0.0F && style.indicator_thickness > 0.0F) {
            const float thickness = context.logical_to_screen(style.indicator_thickness);
            const float selected_x = context.logical_to_screen(indicator_x_.value());
            const float selected_width = context.logical_to_screen(indicator_width_.value());
            const auto indicator = foundation::NanRect::from_xywh(
                world.get_left() + selected_x,
                world.get_bottom() - thickness,
                std::max(0.0F, selected_width),
                thickness
            );
            context.device().draw_rounded_rect(
                indicator,
                context.logical_to_screen(thickness * 0.5F),
                style.indicator.with_alpha(style.indicator.alpha() * opacity)
            );
        }

        if (focused_ && !disabled_ && style.focus.width > 0.0F) {
            primitives::FocusRingPainter::paint(context, world, style.focus, opacity);
        }
    }

    auto Tabs::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto style = resolved_style();
        apply_text_styles();
        measure_labels();

        float total_width = style.metrics.padding_x * 2.0F;
        float max_height = 0.0F;
        for (std::size_t i = 0; i < label_texts_.size(); ++i) {
            total_width += label_texts_[i]->measured_text_width();
            if (i > 0) {
                total_width += style.metrics.gap;
            }
            max_height = std::max(max_height, label_texts_[i]->laid_out_font_size());
        }
        return constraints.constrain(foundation::NanSize(
            total_width,
            std::max(style.metrics.min_height, max_height)
        ));
    }

    auto Tabs::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::tab,
            .label = std::string(selected_label()),
            .value = std::to_string(selected_index_),
            .state =
                {
                    .focusable = !disabled_ && !labels_.empty(),
                    .focused = focused_,
                    .disabled = disabled_,
                    .selected = true,
                },
            .actions = disabled_ ? semantics::Action::none : semantics::Action::focus,
        };
    }

    void Tabs::rebuild_texts() {
        label_texts_.clear();
        label_texts_.reserve(labels_.size());
        for (const auto& label: labels_) {
            label_texts_.push_back(std::make_shared<primitives::Text>(label));
        }
        tab_offsets_.assign(labels_.size(), 0.0F);
    }

    void Tabs::apply_text_styles() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        for (std::size_t i = 0; i < label_texts_.size(); ++i) {
            const auto& type =
                static_cast<int>(i) == selected_index_ ? style.label_selected : style.label;
            const primitives::TextStyle text_style {
                .color = context.text_color_from_context ? context.text_color : type.color,
                .font_size =
                    context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : label_texts_[i]->font(),
                .overflow = primitives::TextOverflow::clip,
                .max_lines = 1,
            };
            if (!same_text_style(label_texts_[i]->style(), text_style)) {
                label_texts_[i]->set_style(text_style);
            }
        }
    }

    void Tabs::measure_labels() {
        float x = resolved_style().metrics.padding_x;
        const float gap = resolved_style().metrics.gap;
        for (std::size_t i = 0; i < label_texts_.size(); ++i) {
            (void)label_texts_[i]->measure_layout(scene::LayoutConstraints::loose());
            tab_offsets_[i] = x;
            x += label_texts_[i]->measured_text_width() + gap;
        }
        // 空闲时吸附指示条到当前选中项（首次绘制 / 程序化选中 / 主题变化）。
        const bool has = selected_index_ >= 0
            && static_cast<std::size_t>(selected_index_) < label_texts_.size();
        if (has && !indicator_x_.is_animating() && !indicator_width_.is_animating()) {
            const auto selected = static_cast<std::size_t>(selected_index_);
            indicator_x_.clear_behavior();
            indicator_width_.clear_behavior();
            indicator_x_.set_target(tab_offsets_[selected]);
            indicator_width_.set_target(label_texts_[selected]->measured_text_width());
        }
    }

    void Tabs::sync_indicator(const bool animate) {
        // 注意：不能在选中后立即 measure_labels()——那会把指示条吸附到新目标，
        // 导致动画变成 new→new 的空操作。这里直接用上一次 measure 的偏移，
        // 从「当前（旧）值」动画到新目标。
        const bool has = selected_index_ >= 0
            && static_cast<std::size_t>(selected_index_) < label_texts_.size();
        if (!has) {
            return;
        }
        const auto selected = static_cast<std::size_t>(selected_index_);
        const float target_x = tab_offsets_[selected];
        const float target_width = label_texts_[selected]->measured_text_width();

        if (animate && labels_.size() > 1) {
            const float duration = system_->tokens.motion.medium_duration;
            indicator_x_.set_behavior(animation::Behavior<float>(duration));
            indicator_width_.set_behavior(animation::Behavior<float>(duration));
        }
        else {
            indicator_x_.clear_behavior();
            indicator_width_.clear_behavior();
        }

        if (auto* tree = get_tree(); tree != nullptr) {
            tree->animation_host().set_target(
                *this, indicator_x_, target_x, scene::DirtyFlags::paint
            );
            tree->animation_host().set_target(
                *this, indicator_width_, target_width, scene::DirtyFlags::paint
            );
        }
        else {
            // 未挂载：无 Host 推进，直接吸附。
            indicator_x_.clear_behavior();
            indicator_width_.clear_behavior();
            indicator_x_.set_target(target_x);
            indicator_width_.set_target(target_width);
        }
    }

    auto Tabs::hit_index(const float local_x) const -> int {
        for (std::size_t i = 0; i < label_texts_.size(); ++i) {
            const float left = tab_offsets_[i];
            const float right = left + label_texts_[i]->measured_text_width();
            if (local_x >= left && local_x < right) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
} // namespace nandina::widget
