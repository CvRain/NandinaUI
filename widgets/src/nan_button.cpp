//
// nan_button.cpp — Button 组件实现
//

module;

#include <string>
#include <string_view>
#include <functional>
#include <optional>

#include <thorvg-1/thorvg.h>

module nandina.widgets.button;

import nandina.widgets.focus_ring;
import nandina.widgets.text;
import nandina.widgets.icon;
import nandina.widgets.surface;
import nandina.layout.container;      // Row
import nandina.layout.flex_widgets;   // Center, Spacer
import nandina.foundation.nan_insets;
import nandina.foundation.color;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.text.nan_font;
import nandina.foundation.nan_types;
import nandina.theme.nan_style;  // ButtonVariant/ButtonSize

namespace nandina::widgets {

    namespace {
        auto preset_for_legacy(ButtonVariant v) -> const theme::NanButtonStyle::PresetStyle& {
            const auto& style = theme::NanStylePrimitives::current().button;
            switch (v) {
            case ButtonVariant::default_variant:
                return style.filled;
            case ButtonVariant::secondary:
                return style.tonal;
            case ButtonVariant::outline:
                return style.outlined;
            case ButtonVariant::ghost:
                return style.ghost;
            case ButtonVariant::destructive:
                return style.destructive;
            case ButtonVariant::link:
                return style.link;
            }
            return style.filled;
        }

        auto resolved_color_variant(ButtonVariant variant, ColorVariant explicit_variant) noexcept -> ColorVariant {
            if (explicit_variant != ColorVariant::inherit) {
                return explicit_variant;
            }

            switch (variant) {
            case ButtonVariant::secondary:
                return ColorVariant::secondary;
            case ButtonVariant::destructive:
                return ColorVariant::destructive;
            default:
                return ColorVariant::primary;
            }
        }

        auto family_for(ColorVariant color_variant) -> const theme::NanButtonStyle::ColorFamilyStyle& {
            const auto& style = theme::NanStylePrimitives::current().button;
            switch (color_variant) {
            case ColorVariant::primary:
            case ColorVariant::inherit:
                return style.primary_family;
            case ColorVariant::secondary:
                return style.secondary_family;
            case ColorVariant::neutral:
                return style.neutral_family;
            case ColorVariant::destructive:
                return style.destructive_family;
            }
            return style.primary_family;
        }

        auto preset_for(ButtonVariant variant, ColorVariant color_variant) -> const theme::NanButtonStyle::PresetStyle& {
            if (color_variant == ColorVariant::inherit) {
                return preset_for_legacy(variant);
            }

            const auto& family = family_for(color_variant);
            switch (variant) {
            case ButtonVariant::default_variant:
                return family.filled;
            case ButtonVariant::secondary:
                return family.tonal;
            case ButtonVariant::outline:
                return family.outlined;
            case ButtonVariant::ghost:
                return family.ghost;
            case ButtonVariant::destructive:
                return family.destructive;
            case ButtonVariant::link:
                return family.link;
            }
            return family.filled;
        }

        auto focus_color_for(const theme::NanButtonStyle::PresetStyle& preset) -> NanColor {
            if (preset.border.to<NanRgb>().alpha() > 0 && preset.border_width > 0.0f) {
                return preset.border;
            }
            if (preset.bg.to<NanRgb>().alpha() > 0) {
                return preset.bg;
            }
            return preset.text;
        }

        auto size_style_for(ButtonSize s) -> const theme::NanButtonStyle::SizeStyle& {
            const auto& style = theme::NanStylePrimitives::current().button;
            switch (s) {
            case ButtonSize::xs:
                return style.xs;
            case ButtonSize::sm:
                return style.sm;
            case ButtonSize::md:
                return style.md;
            case ButtonSize::lg:
                return style.lg;
            case ButtonSize::icon:
                return style.icon;
            }
            return style.md;
        }
    } // namespace

    // ═══════════════════════════════════════════════════════════
    // 工厂 / 构造
    // ═══════════════════════════════════════════════════════════

    auto Button::create() -> Ptr {
        return Ptr{new Button()};
    }

    Button::Button() {
        const auto& button_style = theme::NanStylePrimitives::current().button;
        m_color_variant = button_style.color_variant;
        m_variant = button_style.variant;
        m_size = button_style.size;

        // 默认：不设 bg（由 variant 控制），no border
        set_border_width(0.0f);

        // 构建内容行
        auto row  = layout::Row::Create();
        m_content_row = row.get();
        m_content_row->align_items(layout::LayoutAlignment::center);
        m_content_row->justify_content(layout::LayoutAlignment::center);
        m_content_row->gap(8.0f);
        add_child(std::move(row));

        auto focus_ring = FocusRing::create();
        m_focus_ring = focus_ring.get();
        m_focus_ring->set_active(false);
        add_child(std::move(focus_ring));

        // Text — 按钮文本应始终单行 + ellipsis
        auto label = Text::create();
        m_label    = label.get();
        m_label->set_font(text::NanFont{}
            .weight(button_style.font_weight)
            .color(button_style.font_color)
            .single_line(button_style.single_line)
            .overflow(button_style.overflow));
        m_label->set_vertical_align(TextVerticalAlign::Center);
        m_content_row->add(std::move(label));

        // 应用默认 variant + size
        apply_variant();
        apply_size();
        update_visual_state(); // 初始化文本颜色
    }

    // ═══════════════════════════════════════════════════════════
    // 文本
    // ═══════════════════════════════════════════════════════════

    auto Button::set_text(std::string_view text) -> Button& {
        m_label->set_text(text);
        mark_layout_dirty();
        return *this;
    }

    auto Button::text() const noexcept -> const std::string& {
        return m_label->text();
    }

    // ═══════════════════════════════════════════════════════════
    // Variant / Size
    // ═══════════════════════════════════════════════════════════

    auto Button::variant(ButtonVariant v) -> Button& {
        m_variant = v;
        apply_variant();
        update_visual_state();
        return *this;
    }

    auto Button::variant() const noexcept -> ButtonVariant {
        return m_variant;
    }

    auto Button::size(ButtonSize s) -> Button& {
        m_size = s;
        apply_size();
        mark_layout_dirty();
        return *this;
    }

    auto Button::size() const noexcept -> ButtonSize {
        return m_size;
    }

    auto Button::color_variant(ColorVariant value) -> Button& {
        if (m_color_variant == value) {
            return *this;
        }

        m_color_variant = value;
        apply_variant();
        update_visual_state();
        return *this;
    }

    auto Button::color_variant() const noexcept -> ColorVariant {
        return m_color_variant;
    }

    auto Button::apply_variant() -> void {
        const auto& preset = preset_for(m_variant, m_color_variant);
        set_border_color(preset.border);
        set_border_width(preset.border_width);
    }

    auto Button::resolved_preset_style() const -> theme::NanButtonStyle::PresetStyle {
        return preset_for(m_variant, m_color_variant);
    }

    auto Button::apply_resolved_foreground_color(const nandina::NanColor& color) -> bool {
        bool changed = false;
        if (m_label->color() != color) {
            m_label->set_color(color);
            changed = true;
        }
        if (m_icon_left && m_icon_left->color() != color) {
            m_icon_left->set_color(color);
            changed = true;
        }
        if (m_icon_right && m_icon_right->color() != color) {
            m_icon_right->set_color(color);
            changed = true;
        }
        return changed;
    }

    auto Button::apply_size() -> void {
        const auto& button_style = theme::NanStylePrimitives::current().button;
        const auto& size_style = size_style_for(m_size);
        set_padding(geometry::NanInsets{size_style.padding_h, size_style.padding_v, size_style.padding_h, size_style.padding_v});
        set_corner_radius(button_style.corner_radius);
        m_content_row->gap(size_style.gap);

        m_label->set_font_size(size_style.font_size);
        // icon-only: hide text, set square
        if (m_icon_left) {
            m_icon_left->set_size(size_style.icon_size);
        }
        if (m_icon_right) {
            m_icon_right->set_size(size_style.icon_size);
        }

        if (size_style.square) {
            // square icon button — center the icon
            m_label->set_text("");
            m_content_row->justify_content(layout::LayoutAlignment::center);
        } else {
            m_content_row->justify_content(layout::LayoutAlignment::center);
        }
    }

    // ═══════════════════════════════════════════════════════════
    // 图标
    // ═══════════════════════════════════════════════════════════

    auto Button::icon(IconType type) -> Button& {
        return icon_left(type);
    }

    auto Button::icon_left(IconType type) -> Button& {
        if (!m_icon_left) {
            auto icon_widget = Icon::create();
            m_icon_left = icon_widget.get();
            const auto& size_style = size_style_for(m_size);
            m_icon_left->set_size(size_style.icon_size);
            // 插入到 row 的第一个位置
            auto& children = m_content_row->children();
            children.insert(children.begin(), std::move(icon_widget));
        }
        if (m_icon_left) {
            m_icon_left->set_type(type);
        }
        m_icon_left_type = type;
        update_visual_state();
        mark_layout_dirty();
        return *this;
    }

    auto Button::icon_right(IconType type) -> Button& {
        if (!m_icon_right) {
            auto icon_widget = Icon::create();
            const auto& size_style = size_style_for(m_size);
            icon_widget->set_size(size_style.icon_size);
            m_icon_right = icon_widget.get();
            m_content_row->add(std::move(icon_widget));
        }
        if (m_icon_right) {
            m_icon_right->set_type(type);
        }
        m_icon_right_type = type;
        update_visual_state();
        mark_layout_dirty();
        return *this;
    }

    auto Button::icon_type() const noexcept -> std::optional<IconType> {
        if (m_icon_left_type.has_value()) {
            return m_icon_left_type;
        }
        return m_icon_right_type;
    }

    auto Button::left_icon_type() const noexcept -> std::optional<IconType> {
        return m_icon_left_type;
    }

    auto Button::right_icon_type() const noexcept -> std::optional<IconType> {
        return m_icon_right_type;
    }

    // ═══════════════════════════════════════════════════════════
    // 字体（委托）
    // ═══════════════════════════════════════════════════════════

    auto Button::font_size(float pt) -> Button& {
        m_label->set_font_size(pt);
        mark_layout_dirty();
        return *this;
    }

    auto Button::font_size() const noexcept -> float {
        return m_label->font_size();
    }

    auto Button::font_color(const nandina::NanColor& color) -> Button& {
        m_user_font_color = color;      // 持久化，hover/leave 后仍保持
        m_label->set_color(color);
        return *this;
    }

    auto Button::font_color() const noexcept -> const nandina::NanColor& {
        return m_label->color();
    }

    auto Button::font_weight(text::NanFontWeight w) -> Button& {
        m_label->set_font_weight(w);
        mark_layout_dirty();
        return *this;
    }

    auto Button::font_weight() const noexcept -> text::NanFontWeight {
        return m_label->font_weight();
    }

    auto Button::font_family(std::string family) -> Button& {
        m_label->set_font_family(std::move(family));
        mark_layout_dirty();
        return *this;
    }

    auto Button::font_family() const noexcept -> const std::string& {
        return m_label->font_family();
    }

    auto Button::set_font(text::NanFont font) -> Button& {
        font.single_line(true).overflow(text::TextOverflow::ellipsis);
        // 若 font 显式指定了颜色，同步到 m_user_font_color（update_visual_state 会以此为准）
        if (font.has_explicit_color()) {
            m_user_font_color = font.color();
        }
        m_label->set_font(std::move(font));
        // 重新应用 m_user_font_color，防止 font 携带的默认色覆盖开发者设置的颜色
        if (m_user_font_color.has_value()) {
            m_label->set_color(*m_user_font_color);
        }
        mark_layout_dirty();
        return *this;
    }

    auto Button::font() const noexcept -> const text::NanFont& {
        return m_label->font();
    }

    // ═══════════════════════════════════════════════════════════
    // 回调
    // ═══════════════════════════════════════════════════════════

    auto Button::on_click(Callback cb) -> Button& {
        m_clicked_signal.connect(std::move(cb));
        return *this;
    }

    auto Button::on_press(Callback cb) -> Button& {
        m_pressed_signal.connect(std::move(cb));
        return *this;
    }

    auto Button::on_release(Callback cb) -> Button& {
        m_released_signal.connect(std::move(cb));
        return *this;
    }

    auto Button::on_hover(Callback cb) -> Button& {
        m_hovered_signal.connect(std::move(cb));
        return *this;
    }

    auto Button::on_leave(Callback cb) -> Button& {
        m_left_signal.connect(std::move(cb));
        return *this;
    }

    // ═══════════════════════════════════════════════════════════
    // 信号
    // ═══════════════════════════════════════════════════════════

    auto Button::clicked() -> reactive::EventSignal<>& {
        return m_clicked_signal;
    }

    auto Button::hovered() -> reactive::EventSignal<>& {
        return m_hovered_signal;
    }

    auto Button::left() -> reactive::EventSignal<>& {
        return m_left_signal;
    }

    auto Button::pressed() -> reactive::EventSignal<>& {
        return m_pressed_signal;
    }

    auto Button::released() -> reactive::EventSignal<>& {
        return m_released_signal;
    }

    // ═══════════════════════════════════════════════════════════
    // 禁用 / 加载
    // ═══════════════════════════════════════════════════════════

    auto Button::set_disabled(bool disabled) -> Button& {
        if (m_disabled == disabled) return *this;
        m_disabled = disabled;
        if (disabled) {
            m_hovered = false;
            m_pressed = false;
            m_focused = false;
            if (m_focus_ring) {
                m_focus_ring->set_active(false);
            }
        }
        update_visual_state();
        return *this;
    }

    auto Button::is_disabled() const noexcept -> bool {
        return m_disabled;
    }

    auto Button::set_loading(bool loading) -> Button& {
        m_loading = loading;
        update_visual_state();
        return *this;
    }

    auto Button::is_loading() const noexcept -> bool {
        return m_loading;
    }

    // ═══════════════════════════════════════════════════════════
    // 布局
    // ═══════════════════════════════════════════════════════════

    auto Button::preferred_size() const noexcept -> geometry::NanSize {
        // Icon-only: square
        const auto& size_style = size_style_for(m_size);
        if (size_style.square) {
            const auto& pad = padding();
            return {size_style.height + pad.left() + pad.right(),
                    size_style.height + pad.top() + pad.bottom()};
        }
        // Text + icon: height from size config, width from content
        const float h = size_style.height + padding().top() + padding().bottom();
        const auto label_pref = m_label->preferred_size();
        float w = label_pref.width() + padding().left() + padding().right();
        if (m_icon_left) {
            w += size_style.icon_size + size_style.gap;
        }
        if (m_icon_right) {
            w += size_style.icon_size + size_style.gap;
        }
        return {w, h};
    }

    auto Button::layout() -> void {
        const auto content = content_bounds();

        if (m_content_row) {
            m_content_row->set_bounds(content.x(), content.y(), content.width(), content.height());
            m_content_row->layout();
        }

        if (m_focus_ring) {
            m_focus_ring->set_bounds(x(), y(), width(), height());
            m_focus_ring->set_corner_radius(corner_radius() + m_focus_ring->offset());
            m_focus_ring->layout();
        }

        clear_layout_dirty();
    }

    // ═══════════════════════════════════════════════════════════
    // 事件（Button 直接处理）
    // ═══════════════════════════════════════════════════════════

    auto Button::on_pointer_move(const runtime::PointerMoveEvent& /*event*/) -> bool {
        if (m_disabled || m_loading) return false;

        const bool was_hovered = m_hovered;
        m_hovered             = true;
        if (!was_hovered) {
            update_visual_state();
            m_hovered_signal.emit();
        }
        return true;
    }

    auto Button::on_pointer_enter(const runtime::PointerMoveEvent& event) -> bool {
        return on_pointer_move(event);
    }

    auto Button::on_pointer_leave(const runtime::PointerMoveEvent& /*event*/) -> bool {
        if (!m_hovered && !m_pressed) {
            return false;
        }

        m_hovered = false;
        m_pressed = false;
        update_visual_state();
        m_left_signal.emit();
        return true;
    }

    auto Button::on_pointer_down(const runtime::PointerButtonEvent& event) -> bool {
        if (m_disabled || m_loading) return false;
        if (event.button != nandina::types::PointerButton::Left) return false;

        m_pressed = true;
        update_visual_state();
        m_pressed_signal.emit();
        return true;
    }

    auto Button::on_pointer_up(const runtime::PointerButtonEvent& event) -> bool {
        if (m_disabled || m_loading) return false;
        if (event.button != nandina::types::PointerButton::Left) return false;

        const bool was_pressed = m_pressed;
        m_pressed              = false;
        update_visual_state();

        if (was_pressed) {
            m_released_signal.emit();
            if (m_hovered) {
                m_clicked_signal.emit();
            }
        }
        return true;
    }

    auto Button::on_focus_in() -> bool {
        if (m_disabled) {
            return false;
        }

        m_focused = true;
        if (m_focus_ring) {
            m_focus_ring->set_active(true);
        }
        return true;
    }

    auto Button::on_focus_out() -> bool {
        m_focused = false;
        if (m_focus_ring) {
            m_focus_ring->set_active(false);
        }
        return true;
    }

    // ═══════════════════════════════════════════════════════════
    // 绘制
    // ═══════════════════════════════════════════════════════════

    void Button::on_draw(tvg::SwCanvas& canvas) {
        Surface::on_draw(canvas);
        // 子节点由基类 draw() 遍历
    }

    // ═══════════════════════════════════════════════════════════
    // 视觉状态
    // ═══════════════════════════════════════════════════════════

    auto Button::update_visual_state() -> void {
        const auto& preset = preset_for(m_variant, m_color_variant);

        NanColor target_bg;
        NanColor target_text;

        if (m_disabled) {
            target_bg   = preset.bg_disabled;
            target_text = preset.text_disabled;
        } else if (m_pressed) {
            target_bg   = preset.bg_pressed;
            target_text = m_user_font_color.value_or(preset.text);
        } else if (m_hovered) {
            target_bg   = preset.bg_hover;
            target_text = m_user_font_color.value_or(preset.text);
        } else {
            target_bg   = preset.bg;
            target_text = m_user_font_color.value_or(preset.text);
        }

        if (m_loading) {
            target_text = preset.text_disabled;
        }

        bool changed = false;
        if (bg_color() != target_bg) {
            set_bg_color(target_bg);
            changed = true;
        }
        changed = apply_resolved_foreground_color(target_text) || changed;
        if (m_focus_ring) {
            m_focus_ring->set_color(focus_color_for(preset));
            m_focus_ring->set_active(m_focused && !m_disabled);
        }
        if (changed) {
            mark_dirty();
        }
    }
} // namespace nandina::widgets
