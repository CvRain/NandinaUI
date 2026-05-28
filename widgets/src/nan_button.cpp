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

import nandina.widgets.label;
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
        // ── Variant 颜色表 ────────────────────────────────
        struct VariantColors {
            NanColor bg;
            NanColor bg_hover;
            NanColor bg_pressed;
            NanColor bg_disabled;
            NanColor text;
            NanColor text_disabled;
            NanColor border;
            bool     has_border{true};
            bool     has_bg{true};
        };

        auto colors_for(ButtonVariant v) -> VariantColors {
            switch (v) {
            case ButtonVariant::default_variant:
                return {
                    .bg          = NanColor::from(NanRgb{99, 102, 241}),
                    .bg_hover    = NanColor::from(NanRgb{120, 123, 255}),
                    .bg_pressed  = NanColor::from(NanRgb{80, 82, 200}),
                    .bg_disabled = NanColor::from(NanRgb{60, 62, 80}),
                    .text        = NanColor::from(NanRgb{255, 255, 255}),
                    .text_disabled = NanColor::from(NanRgb{110, 112, 130}),
                    .border      = {},
                    .has_border  = false,
                };
            case ButtonVariant::secondary:
                return {
                    .bg          = NanColor::from(NanRgb{230, 232, 250}),
                    .bg_hover    = NanColor::from(NanRgb{210, 213, 242}),
                    .bg_pressed  = NanColor::from(NanRgb{190, 193, 230}),
                    .bg_disabled = NanColor::from(NanRgb{230, 232, 250}),
                    .text        = NanColor::from(NanRgb{69, 72, 200}),
                    .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
                    .border      = {},
                    .has_border  = false,
                };
            case ButtonVariant::outline:
                return {
                    .bg          = NanColor::from(NanRgb{0, 0, 0, 0}),  // transparent
                    .bg_hover    = NanColor::from(NanRgb{230, 232, 250}),
                    .bg_pressed  = NanColor::from(NanRgb{210, 213, 242}),
                    .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                    .text        = NanColor::from(NanRgb{69, 72, 200}),
                    .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
                    .border      = NanColor::from(NanRgb{180, 183, 220}),
                    .has_border  = true,
                    .has_bg      = false,  // normal state = transparent
                };
            case ButtonVariant::ghost:
                return {
                    .bg          = NanColor::from(NanRgb{0, 0, 0, 0}),
                    .bg_hover    = NanColor::from(NanRgb{230, 232, 250}),
                    .bg_pressed  = NanColor::from(NanRgb{210, 213, 242}),
                    .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                    .text        = NanColor::from(NanRgb{69, 72, 200}),
                    .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
                    .border      = {},
                    .has_border  = false,
                    .has_bg      = false,
                };
            case ButtonVariant::destructive:
                return {
                    .bg          = NanColor::from(NanRgb{230, 69, 83}),
                    .bg_hover    = NanColor::from(NanRgb{245, 90, 100}),
                    .bg_pressed  = NanColor::from(NanRgb{200, 50, 65}),
                    .bg_disabled = NanColor::from(NanRgb{100, 60, 70}),
                    .text        = NanColor::from(NanRgb{255, 255, 255}),
                    .text_disabled = NanColor::from(NanRgb{180, 160, 165}),
                    .border      = {},
                    .has_border  = false,
                };
            case ButtonVariant::link:
                return {
                    .bg          = NanColor::from(NanRgb{0, 0, 0, 0}),
                    .bg_hover    = NanColor::from(NanRgb{0, 0, 0, 0}),
                    .bg_pressed  = NanColor::from(NanRgb{0, 0, 0, 0}),
                    .bg_disabled = NanColor::from(NanRgb{0, 0, 0, 0}),
                    .text        = NanColor::from(NanRgb{69, 72, 200}),
                    .text_disabled = NanColor::from(NanRgb{160, 163, 200}),
                    .border      = {},
                    .has_border  = false,
                    .has_bg      = false,
                };
            }
            return {};
        }

        // ── Size 对照表 ───────────────────────────────────
        struct SizeConfig {
            float height;
            float font_size;
            float padding_h;   // horizontal
            float padding_v;   // vertical
            float gap;         // icon <-> text spacing
            float icon_size;
            bool  square;      // icon-only → width = height
        };

        auto config_for(ButtonSize s) -> SizeConfig {
            switch (s) {
            case ButtonSize::xs:
                return {24.0f, 11.0f, 8.0f, 2.0f, 4.0f, 14.0f, false};
            case ButtonSize::sm:
                return {32.0f, 12.0f, 12.0f, 4.0f, 6.0f, 16.0f, false};
            case ButtonSize::md:
                return {40.0f, 14.0f, 16.0f, 8.0f, 8.0f, 18.0f, false};
            case ButtonSize::lg:
                return {48.0f, 16.0f, 24.0f, 12.0f, 10.0f, 22.0f, false};
            case ButtonSize::icon:
                return {40.0f, 0.0f, 0.0f, 0.0f, 0.0f, 20.0f, true};
            }
            return {};
        }
    } // namespace

    // ═══════════════════════════════════════════════════════════
    // 工厂 / 构造
    // ═══════════════════════════════════════════════════════════

    auto Button::create() -> Ptr {
        return Ptr{new Button()};
    }

    Button::Button() {
        // 默认：不设 bg（由 variant 控制），no border
        set_border_width(0.0f);

        // 构建内容行
        auto row  = layout::Row::Create();
        m_content_row = row.get();
        m_content_row->align_items(layout::LayoutAlignment::center);
        m_content_row->justify_content(layout::LayoutAlignment::center);
        m_content_row->gap(8.0f);
        add_child(std::move(row));

        // Label — 按钮文本应始终单行 + ellipsis
        auto label = Label::create();
        m_label    = label.get();
        // 设置默认字体行为：单行 + 溢出省略号
        m_label->set_font(text::NanFont{}
            .single_line(true)
            .overflow(text::TextOverflow::ellipsis));
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

    auto Button::apply_variant() -> void {
        const auto c = colors_for(m_variant);
        if (c.has_border) {
            set_border_color(c.border);
            set_border_width(1.0f);
        } else {
            set_border_width(0.0f);
        }
    }

    auto Button::apply_size() -> void {
        const auto c = config_for(m_size);
        set_padding(geometry::NanInsets{c.padding_h, c.padding_v, c.padding_h, c.padding_v});
        set_corner_radius(6.0f);  // shadcn default rounding

        m_label->set_font_size(c.font_size);
        // icon-only: hide text, set square
        if (m_icon_left) {
            m_icon_left->set_size(c.icon_size);
        }

        if (c.square) {
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
            const auto c = config_for(m_size);
            m_icon_left->set_size(c.icon_size);
            // 插入到 row 的第一个位置
            auto& children = m_content_row->children();
            children.insert(children.begin(), std::move(icon_widget));
        }
        if (m_icon_left) {
            m_icon_left->set_type(type);
        }
        m_icon_type = type;
        mark_layout_dirty();
        return *this;
    }

    auto Button::icon_right(IconType type) -> Button& {
        // 在 label 之后添加 icon
        if (!m_icon_left) {
            auto icon_widget = Icon::create();
            const auto c = config_for(m_size);
            icon_widget->set_size(c.icon_size);
            icon_widget->set_type(type);
            m_icon_left = icon_widget.get();  // reuse the left slot for simplicity
            m_content_row->add(std::move(icon_widget));
        } else {
            // icon already exists as left; for now just reuse it
            m_icon_left->set_type(type);
        }
        m_icon_type = type;
        mark_layout_dirty();
        return *this;
    }

    auto Button::icon_type() const noexcept -> std::optional<IconType> {
        return m_icon_type;
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
        const auto sc = config_for(m_size);
        if (sc.square) {
            const auto& pad = padding();
            return {sc.height + pad.left() + pad.right(),
                    sc.height + pad.top() + pad.bottom()};
        }
        // Text + icon: height from size config, width from content
        const float h = sc.height + padding().top() + padding().bottom();
        const auto label_pref = m_label->preferred_size();
        float w = label_pref.width() + padding().left() + padding().right();
        if (m_icon_left) {
            w += sc.icon_size + sc.gap;
        }
        return {w, h};
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
        const auto c = colors_for(m_variant);

        NanColor target_bg;
        NanColor target_text;

        if (m_disabled) {
            target_bg   = c.has_bg ? c.bg_disabled : c.bg;
            target_text = c.text_disabled;
        } else if (m_pressed) {
            target_bg   = c.has_bg ? c.bg_pressed : c.bg_hover;
            target_text = m_user_font_color.value_or(c.text);
        } else if (m_hovered) {
            target_bg   = c.bg_hover;
            target_text = m_user_font_color.value_or(c.text);
        } else {
            target_bg   = c.has_bg ? c.bg : c.bg;
            target_text = m_user_font_color.value_or(c.text);
        }

        if (m_loading) {
            target_text = c.text_disabled;
        }

        bool changed = false;
        if (bg_color() != target_bg) {
            set_bg_color(target_bg);
            changed = true;
        }
        if (m_label->color() != target_text) {
            m_label->set_color(target_text);
            changed = true;
        }
        if (changed) {
            mark_dirty();
        }
    }
} // namespace nandina::widgets
