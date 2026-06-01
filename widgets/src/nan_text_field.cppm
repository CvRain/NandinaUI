module;

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <string>
#include <string_view>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.text_field;

import nandina.foundation.color;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_size;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.text.nan_font;
import nandina.theme.nan_style;
import nandina.widgets.focus_ring;
import nandina.widgets.text;
import nandina.widgets.surface;

export namespace nandina::widgets {

    using ColorVariant = nandina::theme::ColorVariant;

    class TextField final : public Surface {
    public:
        using Ptr = std::unique_ptr<TextField>;

        ~TextField() override = default;

        static auto create() -> Ptr {
            return Ptr{new TextField()};
        }

        auto set_value(std::string value) -> TextField& {
            if (m_value == value) {
                return *this;
            }

            m_value = std::move(value);
            m_caret_index = std::min(m_caret_index, m_value.size());
            clear_preedit();
            clear_selection();
            sync_display_text();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto value() const noexcept -> const std::string& {
            return m_value;
        }

        auto set_placeholder(std::string value) -> TextField& {
            if (m_placeholder == value) {
                return *this;
            }

            m_placeholder = std::move(value);
            sync_display_text();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto placeholder() const noexcept -> const std::string& {
            return m_placeholder;
        }

        auto set_disabled(const bool value) -> TextField& {
            if (m_disabled == value) {
                return *this;
            }

            m_disabled = value;
            if (m_disabled) {
                blur();
            }
            sync_visual_state();
            sync_display_text();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto disabled() const noexcept -> bool {
            return m_disabled;
        }

        auto set_read_only(const bool value) -> TextField& {
            if (m_read_only == value) {
                return *this;
            }

            m_read_only = value;
            sync_visual_state();
            sync_display_text();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto read_only() const noexcept -> bool {
            return m_read_only;
        }

        auto set_invalid(const bool value) -> TextField& {
            if (m_invalid == value) {
                return *this;
            }

            m_invalid = value;
            sync_visual_state();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto invalid() const noexcept -> bool {
            return m_invalid;
        }

        auto color_variant(const ColorVariant value) -> TextField& {
            if (m_color_variant == value) {
                return *this;
            }

            m_color_variant = value;
            sync_visual_state();
            sync_display_text();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto color_variant() const noexcept -> ColorVariant {
            return m_color_variant;
        }

        [[nodiscard]] auto focused() const noexcept -> bool {
            return m_focused;
        }

        [[nodiscard]] auto text_input_area() const noexcept -> std::optional<runtime::TextInputArea> override {
            if (!m_focused || m_disabled) {
                return std::nullopt;
            }

            const auto content = content_bounds();
            const auto& font = m_label->font();
            const float line_height = font.line_height() > 0.0f ? font.line_height() : min_content_line_height();
            const float area_y = content.y() + std::max(0.0f, (content.height() - line_height) * 0.5f);
            const int cursor = static_cast<int>(std::lround(caret_visual_x() - content.x()));

            return runtime::TextInputArea{
                .rect = geometry::NanRect{
                    geometry::NanPoint{content.x(), area_y},
                    geometry::NanSize{std::max(1.0f, content.width()), std::max(1.0f, line_height)},
                },
                .cursor = std::max(0, cursor),
            };
        }

        auto on_change(std::function<void(std::string_view)> cb) -> TextField& {
            m_change_signal.connect(std::move(cb));
            return *this;
        }

        auto on_submit(std::function<void(std::string_view)> cb) -> TextField& {
            m_submit_signal.connect(std::move(cb));
            return *this;
        }

        auto focus() -> void {
            if (m_disabled || m_focused) {
                return;
            }

            m_focused = true;
            m_caret_index = m_value.size();
            clear_selection();
            sync_visual_state();
            mark_dirty();
        }

        auto blur() -> void {
            if (!m_focused) {
                return;
            }

            m_focused = false;
            m_selecting = false;
            clear_preedit();
            clear_selection();
            sync_visual_state();
            mark_dirty();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto& pad = padding();
            const auto label_pref = m_label ? m_label->preferred_size() : geometry::NanSize{};
            const float line_height = min_content_line_height();
            return geometry::NanSize{
                label_pref.width() + pad.left() + pad.right(),
                std::max(label_pref.height(), line_height) + pad.top() + pad.bottom()
            };
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const auto& pad = padding();
            const geometry::NanConstraints child_constraints{
                std::max(0.0f, constraints.min_width() - pad.left() - pad.right()),
                constraints.max_width() == geometry::NanConstraints::k_infinity
                    ? geometry::NanConstraints::k_infinity
                    : std::max(0.0f, constraints.max_width() - pad.left() - pad.right()),
                0.0f,
                constraints.max_height() == geometry::NanConstraints::k_infinity
                    ? geometry::NanConstraints::k_infinity
                    : std::max(0.0f, constraints.max_height() - pad.top() - pad.bottom()),
            };

            geometry::NanSize label_measured{};
            if (m_label) {
                m_label->measure(child_constraints.is_tight() ? child_constraints : child_constraints.loosen());
                label_measured = m_label->measured_size();
                if (label_measured.width() <= 0.0f && label_measured.height() <= 0.0f) {
                    label_measured = m_label->preferred_size();
                }
            }

            set_measured_layout_state(
                constraints,
                constraints.constrain(geometry::NanSize{
                    label_measured.width() + pad.left() + pad.right(),
                    std::max(label_measured.height(), min_content_line_height()) + pad.top() + pad.bottom()
                }));
        }

        auto layout() -> void override {
            const auto content = content_bounds();

            if (m_label) {
                m_label->set_bounds(content.x(), content.y(), content.width(), content.height());
                m_label->layout();
            }

            if (m_focus_ring) {
                m_focus_ring->set_bounds(x(), y(), width(), height());
                m_focus_ring->set_corner_radius(corner_radius() + m_focus_ring->offset());
                m_focus_ring->layout();
            }

            clear_layout_dirty();
        }

        void draw(tvg::SwCanvas& canvas) override {
            if (!visible()) {
                return;
            }

            Surface::on_draw(canvas);

            draw_selection(canvas);

            draw_text(canvas);

            draw_preedit(canvas);

            draw_caret(canvas);

            if (m_focus_ring) {
                m_focus_ring->draw(canvas);
            }
        }

        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return true;
        }

    protected:
        auto on_pointer_down(const runtime::PointerButtonEvent& event) -> bool override {
            if (m_disabled || event.button != nandina::types::PointerButton::Left) {
                return false;
            }

            focus();
            const auto caret_index = caret_index_from_position(static_cast<float>(event.x));

            if (event.click_count == 2) {
                if (const auto word_hit = word_hit_index_from_position(static_cast<float>(event.x)); word_hit.has_value()) {
                    select_word_at(*word_hit);
                    m_selecting = false;
                    return true;
                }
            }

            clear_preedit();
            m_caret_index = caret_index;
            m_selection_anchor = caret_index;
            m_selection_extent = caret_index;
            m_selecting = true;
            mark_dirty();
            return true;
        }

        auto on_pointer_move(const runtime::PointerMoveEvent& event) -> bool override {
            if (m_disabled || !m_selecting || !m_focused) {
                return false;
            }

            const auto content = content_bounds();
            const float pointer_x = static_cast<float>(event.x);
            scroll_view_for_pointer(pointer_x);
            m_caret_index = caret_index_from_position(std::clamp(
                pointer_x,
                content.x(),
                content.x() + content.width()));
            m_selection_extent = m_caret_index;
            mark_dirty();
            return true;
        }

        auto on_pointer_leave(const runtime::PointerMoveEvent& /*event*/) -> bool override {
            return true;
        }

        auto on_pointer_up(const runtime::PointerButtonEvent& event) -> bool override {
            if (m_disabled || event.button != nandina::types::PointerButton::Left || !m_selecting) {
                return false;
            }

            m_caret_index = caret_index_from_position(static_cast<float>(event.x));
            m_selection_extent = m_caret_index;
            m_selecting = false;
            mark_dirty();
            return true;
        }

        auto on_key_down(const runtime::KeyEvent& event) -> bool override {
            if (m_disabled || !m_focused) {
                return false;
            }

            if (event.shortcut_modifier()) {
                switch (static_cast<SDL_Keycode>(event.key_code)) {
                case SDLK_A:
                    select_all();
                    return true;

                case SDLK_C:
                    static_cast<void>(copy_selection_to_clipboard());
                    return true;

                case SDLK_X:
                    if (m_read_only) {
                        static_cast<void>(copy_selection_to_clipboard());
                        return true;
                    }
                    if (copy_selection_to_clipboard() && delete_selection()) {
                        notify_value_changed();
                    }
                    return true;

                case SDLK_V:
                    if (m_read_only) {
                        return true;
                    }
                    if (paste_from_clipboard()) {
                        notify_value_changed();
                    }
                    return true;

                default:
                    break;
                }
            }

            const bool extend_selection = event.shift();

            switch (static_cast<SDL_Keycode>(event.key_code)) {
            case SDLK_LEFT:
                if (!extend_selection && has_selection()) {
                    move_caret_to(selection_range().first, false);
                    return true;
                }
                if (m_caret_index > 0) {
                    move_caret_to(previous_codepoint_boundary(m_caret_index), extend_selection);
                }
                return true;

            case SDLK_RIGHT:
                if (!extend_selection && has_selection()) {
                    move_caret_to(selection_range().second, false);
                    return true;
                }
                if (m_caret_index < m_value.size()) {
                    move_caret_to(next_codepoint_boundary(m_caret_index), extend_selection);
                }
                return true;

            case SDLK_HOME:
                move_caret_to(0, extend_selection);
                return true;

            case SDLK_END:
                move_caret_to(m_value.size(), extend_selection);
                return true;

            case SDLK_BACKSPACE:
            {
                if (m_read_only) {
                    return true;
                }
                if (delete_selection()) {
                    notify_value_changed();
                    return true;
                }
                if (m_caret_index == 0 || m_value.empty()) {
                    return true;
                }
                const auto erase_from = previous_codepoint_boundary(m_caret_index);
                m_value.erase(erase_from, m_caret_index - erase_from);
                m_caret_index = erase_from;
                sync_display_text();
                notify_value_changed();
                return true;
            }

            case SDLK_DELETE:
            {
                if (m_read_only) {
                    return true;
                }
                if (delete_selection()) {
                    notify_value_changed();
                    return true;
                }
                if (m_caret_index >= m_value.size() || m_value.empty()) {
                    return true;
                }
                const auto erase_to = next_codepoint_boundary(m_caret_index);
                m_value.erase(m_caret_index, erase_to - m_caret_index);
                sync_display_text();
                notify_value_changed();
                return true;
            }

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (!m_read_only) {
                    m_submit_signal.emit(std::string_view{m_value});
                }
                return true;

            default:
                return false;
            }
        }

        auto on_text_input(const runtime::TextInputEvent& event) -> bool override {
            if (m_disabled || m_read_only || !m_focused || event.text.empty()) {
                return false;
            }

            clear_preedit();

            if (has_selection()) {
                const auto [start, end] = selection_range();
                m_value.replace(start, end - start, event.text);
                m_caret_index = start + event.text.size();
                clear_selection();
            } else {
                m_value.insert(m_caret_index, event.text);
                m_caret_index += event.text.size();
            }
            sync_display_text();
            notify_value_changed();
            return true;
        }

        auto on_text_editing(const runtime::TextEditingEvent& event) -> bool override {
            if (m_disabled || m_read_only || !m_focused) {
                return false;
            }

            m_preedit_text = event.text;
            m_preedit_selection_start = std::clamp(
                static_cast<std::size_t>(std::max(0, event.start)),
                std::size_t{0},
                m_preedit_text.size());
            m_preedit_selection_length = std::clamp(
                static_cast<std::size_t>(std::max(0, event.length)),
                std::size_t{0},
                m_preedit_text.size() - m_preedit_selection_start);
            m_preedit_cursor = m_preedit_selection_start;
            sync_display_text();
            mark_layout_dirty();
            return true;
        }

        auto on_focus_in() -> bool override {
            if (m_disabled) {
                return false;
            }

            focus();
            return true;
        }

        auto on_focus_out() -> bool override {
            if (!m_focused) {
                return false;
            }

            blur();
            return true;
        }

    private:
        struct ResolvedInputColors {
            NanColor font_color;
            NanColor placeholder_font_color;
            NanColor bg;
            NanColor border;
            NanColor border_focus;
        };

        TextField() {
            const auto& style = theme::NanStylePrimitives::current().input;
            m_color_variant = style.color_variant;

            const auto colors = resolved_input_colors();
            set_bg_color(colors.bg);
            set_border_color(colors.border);
            set_border_width(style.border_width);
            set_corner_radius(style.corner_radius);
            set_padding(style.padding);

            auto label = Text::create();
            m_label = label.get();
            m_label->set_align(TextAlign::Start);
            m_label->set_vertical_align(TextVerticalAlign::Center);
            m_label->set_align_to_ink_bounds(false);
            m_label->set_font(text::NanFont{}
                .size(style.font_size)
                .weight(style.font_weight)
                .color(colors.font_color)
                .single_line(true)
                .overflow(text::TextOverflow::clip));
            add_child(std::move(label));

            auto focus_ring = FocusRing::create();
            m_focus_ring = focus_ring.get();
            m_focus_ring->set_active(false);
            add_child(std::move(focus_ring));

            sync_display_text();
            sync_visual_state();
        }

        [[nodiscard]] auto resolved_input_colors() const -> ResolvedInputColors {
            const auto& input_style = theme::NanStylePrimitives::current().input;
            const auto resolved_variant = m_color_variant == ColorVariant::inherit
                ? ColorVariant::primary
                : m_color_variant;

            switch (resolved_variant) {
            case ColorVariant::secondary:
                return {
                    .font_color = input_style.secondary_family.font_color,
                    .placeholder_font_color = input_style.secondary_family.placeholder_font_color,
                    .bg = input_style.secondary_family.bg,
                    .border = input_style.secondary_family.border,
                    .border_focus = input_style.secondary_family.border_focus,
                };
            case ColorVariant::neutral:
                return {
                    .font_color = input_style.neutral_family.font_color,
                    .placeholder_font_color = input_style.neutral_family.placeholder_font_color,
                    .bg = input_style.neutral_family.bg,
                    .border = input_style.neutral_family.border,
                    .border_focus = input_style.neutral_family.border_focus,
                };
            case ColorVariant::destructive:
                return {
                    .font_color = input_style.destructive_family.font_color,
                    .placeholder_font_color = input_style.destructive_family.placeholder_font_color,
                    .bg = input_style.destructive_family.bg,
                    .border = input_style.destructive_family.border,
                    .border_focus = input_style.destructive_family.border_focus,
                };
            case ColorVariant::primary:
            case ColorVariant::inherit:
                return {
                    .font_color = input_style.font_color,
                    .placeholder_font_color = input_style.placeholder_font_color,
                    .bg = input_style.bg,
                    .border = input_style.border,
                    .border_focus = input_style.border_focus,
                };
            }

            return {
                .font_color = input_style.font_color,
                .placeholder_font_color = input_style.placeholder_font_color,
                .bg = input_style.bg,
                .border = input_style.border,
                .border_focus = input_style.border_focus,
            };
        }

        [[nodiscard]] auto resolved_focus_color() const -> NanColor {
            const auto colors = resolved_input_colors();
            if (m_invalid) {
                return theme::NanStylePrimitives::current().label.error_font_color;
            }
            return colors.border_focus;
        }

        [[nodiscard]] auto min_content_line_height() const noexcept -> float {
            const auto& style = theme::NanStylePrimitives::current().input;
            return std::max(style.font_size, style.placeholder_font_size) * 1.4f;
        }

        [[nodiscard]] auto display_text() const -> std::string_view {
            return m_value.empty() ? std::string_view{m_placeholder} : std::string_view{m_value};
        }

        [[nodiscard]] auto showing_placeholder() const noexcept -> bool {
            return m_value.empty() && m_preedit_text.empty();
        }

        [[nodiscard]] auto display_text_for_render() const -> std::string {
            return showing_placeholder() ? m_placeholder : composed_text();
        }

        [[nodiscard]] auto composed_text() const -> std::string {
            if (showing_placeholder()) {
                return m_placeholder;
            }

            if (m_preedit_text.empty()) {
                return m_value;
            }

            std::string composed;
            composed.reserve(m_value.size() + m_preedit_text.size());
            composed.append(m_value.substr(0, m_caret_index));
            composed.append(m_preedit_text);
            composed.append(m_value.substr(m_caret_index));
            return composed;
        }

        [[nodiscard]] auto resolved_border_color() const -> NanColor {
            const auto colors = resolved_input_colors();
            if (m_invalid) {
                return theme::NanStylePrimitives::current().label.error_font_color;
            }
            if (m_focused) {
                return colors.border_focus;
            }
            return colors.border;
        }

        [[nodiscard]] auto resolved_text_color() const -> NanColor {
            const auto& label_style = theme::NanStylePrimitives::current().label;
            const auto colors = resolved_input_colors();
            if (m_disabled) {
                return label_style.disabled_font_color;
            }
            if (showing_placeholder()) {
                return colors.placeholder_font_color;
            }
            return colors.font_color;
        }

        [[nodiscard]] auto has_selection() const noexcept -> bool {
            return m_selection_anchor != m_selection_extent;
        }

        [[nodiscard]] auto selection_range() const noexcept -> std::pair<std::size_t, std::size_t> {
            return {
                std::min(m_selection_anchor, m_selection_extent),
                std::max(m_selection_anchor, m_selection_extent),
            };
        }

        [[nodiscard]] auto selected_text() const -> std::string {
            if (!has_selection()) {
                return {};
            }
            const auto [start, end] = selection_range();
            return m_value.substr(start, end - start);
        }

        auto clear_selection() noexcept -> void {
            m_selection_anchor = m_caret_index;
            m_selection_extent = m_caret_index;
        }

        auto clear_preedit() noexcept -> void {
            m_preedit_text.clear();
            m_preedit_cursor = 0;
            m_preedit_selection_start = 0;
            m_preedit_selection_length = 0;
        }

        auto move_caret_to(std::size_t index, const bool extend_selection) -> void {
            const auto clamped = std::min(index, m_value.size());
            if (extend_selection) {
                if (!has_selection()) {
                    m_selection_anchor = m_caret_index;
                }
                m_caret_index = clamped;
                m_selection_extent = clamped;
            } else {
                m_caret_index = clamped;
                clear_selection();
            }
            mark_dirty();
        }

        auto select_all() -> void {
            m_selection_anchor = 0;
            m_selection_extent = m_value.size();
            m_caret_index = m_value.size();
            mark_dirty();
        }

        enum class TextUnitKind : std::uint8_t {
            whitespace,
            word,
            punctuation,
        };

        [[nodiscard]] static auto classify_text_unit(const std::string& text, const std::size_t index) noexcept -> TextUnitKind {
            if (index >= text.size()) {
                return TextUnitKind::whitespace;
            }

            const auto byte = static_cast<unsigned char>(text[index]);
            if (byte >= 0x80u) {
                return TextUnitKind::word;
            }
            if (std::isspace(byte) != 0) {
                return TextUnitKind::whitespace;
            }
            if (std::isalnum(byte) != 0 || byte == static_cast<unsigned char>('_')) {
                return TextUnitKind::word;
            }
            return TextUnitKind::punctuation;
        }

        [[nodiscard]] auto word_range_at(std::size_t index) const noexcept -> std::pair<std::size_t, std::size_t> {
            if (m_value.empty()) {
                return {0, 0};
            }

            index = std::min(index, m_value.size());
            if (index == m_value.size() && index > 0) {
                index = previous_codepoint_boundary(index);
            }
            if (index < m_value.size() && classify_text_unit(m_value, index) == TextUnitKind::whitespace && index > 0) {
                const auto previous = previous_codepoint_boundary(index);
                if (classify_text_unit(m_value, previous) != TextUnitKind::whitespace) {
                    index = previous;
                }
            }

            const auto kind = classify_text_unit(m_value, index);
            auto start = index;
            while (start > 0) {
                const auto previous = previous_codepoint_boundary(start);
                if (classify_text_unit(m_value, previous) != kind) {
                    break;
                }
                start = previous;
            }

            auto end = next_codepoint_boundary(index);
            while (end < m_value.size()) {
                if (classify_text_unit(m_value, end) != kind) {
                    break;
                }
                end = next_codepoint_boundary(end);
            }

            return {start, end};
        }

        auto select_word_at(const std::size_t index) -> void {
            const auto [start, end] = word_range_at(index);
            m_selection_anchor = start;
            m_selection_extent = end;
            m_caret_index = end;
            mark_dirty();
        }

        [[nodiscard]] auto word_hit_index_from_position(const float position_x) const noexcept -> std::optional<std::size_t> {
            if (!m_label || m_value.empty()) {
                return std::nullopt;
            }

            sync_view_start(m_value);
            const auto content = content_bounds();
            const auto& font = m_label->font();
            const float local_x = position_x - content.x();
            if (local_x <= 0.0f) {
                return std::nullopt;
            }

            std::size_t boundary = std::min(m_view_start, m_value.size());
            float previous_width = 0.0f;

            while (boundary < m_value.size()) {
                const auto next_boundary = next_codepoint_boundary(boundary);
                const float next_width = font.estimate_text_width(
                    std::string_view{m_value}.substr(m_view_start, next_boundary - m_view_start));

                if (local_x > previous_width && local_x < next_width) {
                    if (classify_text_unit(m_value, boundary) == TextUnitKind::whitespace) {
                        return std::nullopt;
                    }
                    return boundary;
                }

                boundary = next_boundary;
                previous_width = next_width;
            }

            return std::nullopt;
        }

        [[nodiscard]] static auto is_utf8_continuation_byte(const unsigned char byte) noexcept -> bool {
            return (byte & 0xC0u) == 0x80u;
        }

        [[nodiscard]] static auto previous_codepoint_boundary_in(
            const std::string_view text,
            std::size_t index) noexcept -> std::size_t {
            index = std::min(index, text.size());
            if (index == 0) {
                return 0;
            }

            --index;
            while (index > 0 && is_utf8_continuation_byte(static_cast<unsigned char>(text[index]))) {
                --index;
            }
            return index;
        }

        [[nodiscard]] static auto next_codepoint_boundary_in(
            const std::string_view text,
            std::size_t index) noexcept -> std::size_t {
            index = std::min(index, text.size());
            if (index >= text.size()) {
                return text.size();
            }

            ++index;
            while (index < text.size() && is_utf8_continuation_byte(static_cast<unsigned char>(text[index]))) {
                ++index;
            }
            return index;
        }

        [[nodiscard]] auto previous_codepoint_boundary(std::size_t index) const noexcept -> std::size_t {
            return previous_codepoint_boundary_in(m_value, index);
        }

        [[nodiscard]] auto next_codepoint_boundary(std::size_t index) const noexcept -> std::size_t {
            return next_codepoint_boundary_in(m_value, index);
        }

        [[nodiscard]] auto composed_caret_index() const noexcept -> std::size_t {
            return std::min(m_caret_index + std::min(m_preedit_cursor, m_preedit_text.size()), composed_text().size());
        }

        [[nodiscard]] auto composed_index_from_committed(std::size_t index) const noexcept -> std::size_t {
            index = std::min(index, m_value.size());
            if (m_preedit_text.empty() || index <= m_caret_index) {
                return index;
            }
            return index + m_preedit_text.size();
        }

        [[nodiscard]] auto committed_index_from_composed(std::size_t index) const noexcept -> std::size_t {
            index = std::min(index, composed_text().size());
            if (m_preedit_text.empty() || index <= m_caret_index) {
                return std::min(index, m_value.size());
            }

            const auto preedit_end = m_caret_index + m_preedit_text.size();
            if (index <= preedit_end) {
                return m_caret_index;
            }
            return std::min(index - m_preedit_text.size(), m_value.size());
        }

        [[nodiscard]] auto measured_text_width(
            const std::string_view text,
            const std::size_t start,
            const std::size_t end) const noexcept -> float {
            if (!m_label || start >= end || start >= text.size()) {
                return 0.0f;
            }

            return m_label->font().estimate_text_width(text.substr(start, std::min(end, text.size()) - start));
        }

        auto sync_view_start(const std::string_view text) const noexcept -> void {
            if (showing_placeholder() || !m_focused || !m_label) {
                m_view_start = 0;
                return;
            }

            const auto content = content_bounds();
            if (content.width() <= 0.0f || text.empty()) {
                m_view_start = 0;
                return;
            }

            auto start = std::min(m_view_start, text.size());
            const auto caret = std::min(composed_caret_index(), text.size());

            if (caret < start) {
                start = caret;
            }

            while (start < caret && measured_text_width(text, start, caret) > content.width()) {
                start = next_codepoint_boundary_in(text, start);
            }

            while (start > 0) {
                const auto previous = previous_codepoint_boundary_in(text, start);
                if (measured_text_width(text, previous, caret) > content.width()) {
                    break;
                }
                start = previous;
            }

            m_view_start = std::min(start, text.size());
        }

        auto scroll_view_for_pointer(const float position_x) const noexcept -> void {
            if (!m_selecting || !m_focused || !m_label) {
                return;
            }

            const auto text = composed_text();
            if (text.empty()) {
                m_view_start = 0;
                return;
            }

            const auto content = content_bounds();
            sync_view_start(text);

            if (position_x < content.x() && m_view_start > 0) {
                m_view_start = previous_codepoint_boundary_in(text, m_view_start);
                return;
            }

            if (position_x > content.x() + content.width()) {
                const auto visible_end = visible_text_end(text);
                if (visible_end < text.size()) {
                    m_view_start = next_codepoint_boundary_in(text, m_view_start);
                }
            }
        }

        [[nodiscard]] auto visible_text_end(const std::string_view text) const noexcept -> std::size_t {
            if (!m_label || text.empty()) {
                return 0;
            }

            const auto content = content_bounds();
            if (content.width() <= 0.0f) {
                return text.size();
            }

            auto end = std::min(m_view_start, text.size());
            while (end < text.size()) {
                const auto next = next_codepoint_boundary_in(text, end);
                if (measured_text_width(text, m_view_start, next) > content.width()) {
                    break;
                }
                end = next;
            }

            return end;
        }

        [[nodiscard]] auto visible_x_for_composed_index(const std::size_t index) const noexcept -> float {
            const auto content = content_bounds();
            const auto text = display_text_for_render();
            sync_view_start(text);

            const auto clamped = std::min(index, text.size());
            if (clamped <= m_view_start) {
                return content.x();
            }

            return content.x() + measured_text_width(text, m_view_start, clamped);
        }

        [[nodiscard]] auto caret_index_from_position(const float position_x) const noexcept -> std::size_t {
            if (!m_label || m_value.empty()) {
                return 0;
            }

            const auto text = composed_text();
            sync_view_start(text);
            const auto content = content_bounds();
            const auto& font = m_label->font();
            const float local_x = std::max(0.0f, position_x - content.x());

            std::size_t boundary = std::min(m_view_start, text.size());
            float previous_width = 0.0f;

            while (boundary < text.size()) {
                const auto next_boundary = next_codepoint_boundary_in(text, boundary);
                const float next_width = font.estimate_text_width(text.substr(m_view_start, next_boundary - m_view_start));
                const float midpoint = previous_width + (next_width - previous_width) * 0.5f;
                if (local_x <= midpoint) {
                    return committed_index_from_composed(boundary);
                }
                boundary = next_boundary;
                previous_width = next_width;
            }

            return committed_index_from_composed(text.size());
        }

        [[nodiscard]] auto delete_selection() -> bool {
            if (!has_selection()) {
                return false;
            }

            const auto [start, end] = selection_range();
            m_value.erase(start, end - start);
            m_caret_index = start;
            clear_selection();
            sync_display_text();
            return true;
        }

        [[nodiscard]] auto copy_selection_to_clipboard() const -> bool {
            if (!has_selection()) {
                return false;
            }
            const auto selection = selected_text();
            return SDL_SetClipboardText(selection.c_str());
        }

        [[nodiscard]] auto paste_from_clipboard() -> bool {
            if (!SDL_HasClipboardText()) {
                return false;
            }

            char* raw_text = SDL_GetClipboardText();
            if (raw_text == nullptr) {
                return false;
            }

            const std::string clipboard_text{raw_text};
            SDL_free(raw_text);
            if (clipboard_text.empty()) {
                return false;
            }

            if (has_selection()) {
                const auto [start, end] = selection_range();
                m_value.replace(start, end - start, clipboard_text);
                m_caret_index = start + clipboard_text.size();
                clear_selection();
            } else {
                m_value.insert(m_caret_index, clipboard_text);
                m_caret_index += clipboard_text.size();
            }
            sync_display_text();
            return true;
        }

        auto notify_value_changed() -> void {
            m_change_signal.emit(std::string_view{m_value});
            mark_layout_dirty();
        }

        [[nodiscard]] auto caret_visual_x() const noexcept -> float {
            return visible_x_for_composed_index(composed_caret_index());
        }

        auto sync_display_text() -> void {
            if (!m_label) {
                return;
            }

            const auto& input_style = theme::NanStylePrimitives::current().input;
            m_label->set_text(composed_text());
            m_label->set_font_size(showing_placeholder() ? input_style.placeholder_font_size : input_style.font_size);
            m_label->set_color(resolved_text_color());
        }

        auto draw_text(tvg::SwCanvas& canvas) -> void {
            if (!m_label) {
                return;
            }

            const auto text = display_text_for_render();
            if (text.empty()) {
                return;
            }

            sync_view_start(text);
            const auto end = visible_text_end(text);
            const auto visible = text.substr(m_view_start, end - m_view_start);
            if (visible.empty()) {
                return;
            }

            const auto content = content_bounds();
            const auto& font = m_label->font();
            const auto layout = font.shape(visible, 0.0f);
            const float text_y = content.y() + std::max(0.0f, (content.height() - layout.total_height) * 0.5f);
            font.paint(canvas, layout, content.x(), text_y);
        }

        auto sync_visual_state() -> void {
            const auto& style = theme::NanStylePrimitives::current().input;
            const auto colors = resolved_input_colors();
            set_bg_color(colors.bg);
            set_border_width(style.border_width);
            set_corner_radius(style.corner_radius);
            set_padding(style.padding);
            set_border_color(resolved_border_color());

            if (m_focus_ring) {
                m_focus_ring->set_active(m_focused && !m_disabled);
                m_focus_ring->set_color(resolved_focus_color());
            }
        }

        auto draw_caret(tvg::SwCanvas& canvas) -> void {
            if (m_disabled || !m_focused || !m_label || m_read_only) {
                return;
            }

            const auto content = content_bounds();
            const auto& font = m_label->font();
            const float line_height = font.line_height() > 0.0f ? font.line_height() : min_content_line_height();
            const float caret_y = content.y() + std::max(0.0f, (content.height() - line_height) * 0.5f);
            const auto caret_color = resolved_border_color().to<NanRgb>();

            auto* caret = tvg::Shape::gen();
            caret->moveTo(caret_visual_x(), caret_y);
            caret->lineTo(caret_visual_x(), caret_y + line_height);
            caret->strokeWidth(1.5f);
            caret->strokeFill(caret_color.red(), caret_color.green(), caret_color.blue(), caret_color.alpha());
            canvas.add(caret);
        }

        auto draw_preedit(tvg::SwCanvas& canvas) -> void {
            if (m_disabled || !m_focused || !m_label || m_preedit_text.empty()) {
                return;
            }

            const auto content = content_bounds();
            const auto& font = m_label->font();
            const float start_x = visible_x_for_composed_index(m_caret_index);
            const float preedit_width = measured_text_width(m_preedit_text, 0, m_preedit_text.size());
            const float line_height = font.line_height() > 0.0f ? font.line_height() : min_content_line_height();
            const float preedit_y = content.y() + std::max(0.0f, (content.height() - line_height) * 0.5f);
            const float underline_y = content.y() + std::max(0.0f, (content.height() - line_height) * 0.5f) + line_height;
            const auto underline_color = resolved_border_color().to<NanRgb>();

            if (m_preedit_selection_length > 0) {
                const float selection_start_x = start_x + measured_text_width(
                    m_preedit_text,
                    0,
                    std::min(m_preedit_selection_start, m_preedit_text.size()));
                const float selection_end_x = start_x + measured_text_width(
                    m_preedit_text,
                    0,
                    std::min(m_preedit_selection_start + m_preedit_selection_length, m_preedit_text.size()));

                const float clipped_start_x = std::max(content.x(), selection_start_x);
                const float clipped_end_x = std::min(content.x() + content.width(), selection_end_x);

                if (clipped_end_x > clipped_start_x) {
                    auto* highlight = tvg::Shape::gen();
                    highlight->appendRect(
                        clipped_start_x,
                        preedit_y,
                        clipped_end_x - clipped_start_x,
                        line_height,
                        2.0f,
                        2.0f);
                    highlight->fill(underline_color.red(), underline_color.green(), underline_color.blue(), 40);
                    canvas.add(highlight);
                }
            }

            const float clipped_start_x = std::max(content.x(), start_x);
            const float clipped_end_x = std::min(content.x() + content.width(), start_x + preedit_width);
            if (clipped_end_x <= clipped_start_x) {
                return;
            }

            auto* underline = tvg::Shape::gen();
            underline->moveTo(clipped_start_x, underline_y);
            underline->lineTo(clipped_end_x, underline_y);
            underline->strokeWidth(1.0f);
            underline->strokeFill(
                underline_color.red(),
                underline_color.green(),
                underline_color.blue(),
                underline_color.alpha());
            canvas.add(underline);
        }

        auto draw_selection(tvg::SwCanvas& canvas) -> void {
            if (m_disabled || !m_focused || !m_label || !has_selection() || m_value.empty()) {
                return;
            }

            const auto [start, end] = selection_range();
            if (start == end) {
                return;
            }

            const auto content = content_bounds();
            const auto& font = m_label->font();
            const float start_x = visible_x_for_composed_index(composed_index_from_committed(start));
            const float end_x = visible_x_for_composed_index(composed_index_from_committed(end));
            const float line_height = font.line_height() > 0.0f ? font.line_height() : min_content_line_height();
            const float selection_y = content.y() + std::max(0.0f, (content.height() - line_height) * 0.5f);
            const auto selection_color = resolved_focus_color().to<NanRgb>();

            const float clipped_start_x = std::max(content.x(), start_x);
            const float clipped_end_x = std::min(content.x() + content.width(), end_x);
            if (clipped_end_x <= clipped_start_x) {
                return;
            }

            auto* highlight = tvg::Shape::gen();
            highlight->appendRect(clipped_start_x, selection_y, clipped_end_x - clipped_start_x, line_height, 4.0f, 4.0f);
            highlight->fill(selection_color.red(), selection_color.green(), selection_color.blue(), 56);
            canvas.add(highlight);
        }

        Text* m_label{nullptr};
        FocusRing* m_focus_ring{nullptr};

        std::string m_value;
        std::string m_placeholder;
        std::string m_preedit_text;
        std::size_t m_caret_index{0};
        std::size_t m_preedit_cursor{0};
        std::size_t m_preedit_selection_start{0};
        std::size_t m_preedit_selection_length{0};
        std::size_t m_selection_anchor{0};
        std::size_t m_selection_extent{0};
        mutable std::size_t m_view_start{0};

        bool m_disabled{false};
        bool m_read_only{false};
        bool m_invalid{false};
        bool m_focused{false};
        bool m_selecting{false};
        ColorVariant m_color_variant{ColorVariant::inherit};

        reactive::EventSignal<std::string_view> m_change_signal;
        reactive::EventSignal<std::string_view> m_submit_signal;
    };

} // namespace nandina::widgets