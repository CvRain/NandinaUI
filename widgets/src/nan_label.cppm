//
// Created by cvrain on 2026/4/28.
//

module;

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.label;

export import nandina.widgets.text;

import nandina.foundation.nan_constraints;
import nandina.foundation.nan_rect;
import nandina.foundation.color;
import nandina.theme.nan_style;

/**
 * nandina.widgets.label
 *
 * Label — 文本显示组件（基于 NanFont + HarfBuzz 真实字体渲染）。
 *
 * 职责：
 * - 显示文本字符串（UTF-8）
 * - 支持字体大小、颜色、水平/垂直对齐
 * - 首选尺寸基于 NanFont 真实度量计算
 * - 字体懒加载（第一次 on_draw 时自动获取系统默认字体或用户指定字体）
 */
export namespace nandina::widgets {

    /**
     * Label — 文本显示组件
     *
     * 用法：
     *   auto label = Label::create()
     *       .set_text("Hello, Nandina!")
     *       .set_font_size(14.0f)
     *       .set_color(NanColor::from(NanRgb{220, 220, 240}));
     *   parent->add_child(std::move(label));
     */
    class Label : public Text {
    public:
        using Ptr = std::unique_ptr<Label>;

        ~Label() override = default;

        static auto create() -> Ptr {
            return Ptr(new Label());
        }

        auto set_text(std::string_view text) -> Label& {
            Text::set_text(text);
            return *this;
        }

        auto set_font_size(float size) -> Label& {
            Text::set_font_size(size);
            return *this;
        }

        auto set_font_family(std::string family) -> Label& {
            Text::set_font_family(std::move(family));
            return *this;
        }

        auto set_font_weight(text::NanFontWeight weight) -> Label& {
            Text::set_font_weight(weight);
            return *this;
        }

        auto set_font(text::NanFont font) -> Label& {
            Text::set_font(std::move(font));
            return *this;
        }

        auto set_typography_role(const theme::NanTypographyRole role) -> Label& {
            Text::set_typography_role(role);
            return *this;
        }

        template <typename Fn>
            requires std::invocable<Fn, text::NanFont&>
        auto update_font(Fn&& fn) -> Label& {
            Text::update_font(std::forward<Fn>(fn));
            return *this;
        }

        auto set_color(const nandina::NanColor& color) -> Label& {
            Text::set_color(color);
            return *this;
        }

        auto set_align(TextAlign align) -> Label& {
            Text::set_align(align);
            return *this;
        }

        auto set_vertical_align(TextVerticalAlign valign) -> Label& {
            Text::set_vertical_align(valign);
            return *this;
        }

        auto set_align_to_ink_bounds(const bool value) -> Label& {
            Text::set_align_to_ink_bounds(value);
            return *this;
        }

        auto set_disabled(const bool value) -> Label& {
            if (m_disabled == value) {
                return *this;
            }
            m_disabled = value;
            sync_resolved_style();
            mark_dirty();
            return *this;
        }

        auto set_error(const bool value) -> Label& {
            if (m_error == value) {
                return *this;
            }

            m_error = value;
            sync_resolved_style();
            mark_dirty();
            return *this;
        }

        auto set_required(const bool value) -> Label& {
            if (m_required == value) {
                return *this;
            }

            m_required = value;
            invalidate_shape_cache();
            mark_dirty();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto disabled() const noexcept -> bool {
            return m_disabled;
        }

        [[nodiscard]] auto error() const noexcept -> bool {
            return m_error;
        }

        [[nodiscard]] auto required() const noexcept -> bool {
            return m_required;
        }

        [[nodiscard]] static auto measure_diag() -> std::pair<int, int> {
            return Text::measure_diag();
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            if (!m_required) {
                Text::on_draw(canvas);
                return;
            }

            const auto txt = display_text();
            if (txt.empty()) {
                return;
            }

            const auto bnds = bounds();
            const float avail_w = measured_constraints().max_width();
            const float max_width = (avail_w > 0.0f && avail_w < geometry::NanConstraints::k_infinity)
                ? avail_w
                : bnds.width();
            const auto& layout = cached_shape(txt, max_width > 0.0f ? max_width : 0.0f);

            if (layout.empty()) return;

            const float block_width = m_align_to_ink_bounds
                ? std::max(0.0f, layout.ink_right - layout.ink_left)
                : layout.total_width;
            float offset_x = bnds.x();
            switch (m_align) {
            case TextAlign::Start:  break;
            case TextAlign::Center: offset_x += (bnds.width() - block_width) * 0.5f; break;
            case TextAlign::End:    offset_x += bnds.width() - block_width; break;
            }
            if (m_align_to_ink_bounds) {
                offset_x -= layout.ink_left;
            }

            const float block_height = m_align_to_ink_bounds
                ? std::max(0.0f, layout.ink_bottom - layout.ink_top)
                : layout.total_height;
            float offset_y = bnds.y();
            switch (m_valign) {
            case TextVerticalAlign::Top: break;
            case TextVerticalAlign::Center: offset_y += (bnds.height() - block_height) * 0.5f; break;
            case TextVerticalAlign::Bottom: offset_y += bnds.height() - block_height; break;
            }
            if (m_align_to_ink_bounds) {
                offset_y -= layout.ink_top;
            }

            const auto& raw_text = m_text.get();
            if (!raw_text.empty()) {
                auto text_font = m_font;
                text_font.color(m_color.get());
                const auto raw_layout = text_font.shape(raw_text, max_width > 0.0f ? max_width : 0.0f);
                if (!raw_layout.empty()) {
                    text_font.paint(canvas, raw_layout, offset_x, offset_y);
                }
            }

            auto suffix_font = m_font;
            suffix_font.color(required_indicator_color());
            const auto suffix = raw_text.empty() ? std::string{"*"} : std::string{" *"};
            const auto suffix_layout = suffix_font.shape(suffix, 0.0f);
            const float raw_width = raw_text.empty()
                ? 0.0f
                : m_font.shape(raw_text, max_width > 0.0f ? max_width : 0.0f).total_width;
            const float suffix_x = offset_x + raw_width;
            const float suffix_y = offset_y + std::max(0.0f, layout.total_height - suffix_layout.total_height);
            suffix_font.paint(canvas, suffix_layout, suffix_x, suffix_y);
        }

    private:
        Label() {
            const auto& style = theme::NanStylePrimitives::current().label;
            m_font.overflow(style.overflow)
                .single_line(style.single_line)
                .max_lines(style.max_lines);
            sync_resolved_style();
        }

        [[nodiscard]] auto display_text() const -> std::string override {
            if (!m_required) {
                return m_text.get();
            }

            if (m_text.get().empty()) {
                return "*";
            }

            return m_text.get() + " *";
        }

        [[nodiscard]] auto resolved_text_color() const -> nandina::NanColor override {
            const auto& style = theme::NanStylePrimitives::current().label;

            if (m_error) {
                return style.error_font_color;
            }
            if (m_disabled) {
                return style.disabled_font_color;
            }

            return m_user_font_color.value_or(style.font_color);
        }

        [[nodiscard]] auto required_indicator_color() const -> nandina::NanColor {
            return theme::NanStylePrimitives::current().label.required_indicator_color;
        }

        bool m_disabled{false};
        bool m_error{false};
        bool m_required{false};
    };

} // namespace nandina::widgets
