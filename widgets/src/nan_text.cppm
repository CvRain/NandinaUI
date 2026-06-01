module;

#include <atomic>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.text;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.bindable_prop;
import nandina.text.nan_font;
import nandina.theme.nan_style;

export namespace nandina::widgets {

    enum class TextAlign {
        Start,
        Center,
        End
    };

    enum class TextVerticalAlign {
        Top,
        Center,
        Bottom
    };

    class Text : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Text>;

        ~Text() override = default;

        static auto create() -> Ptr {
            return Ptr(new Text());
        }

        auto set_text(std::string_view text) -> Text& {
            m_text.set(std::string{text});
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font_size(float size) -> Text& {
            m_typography_role.reset();
            m_font.size(size);
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font_family(std::string family) -> Text& {
            m_font.family(std::move(family));
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font_weight(text::NanFontWeight weight) -> Text& {
            m_typography_role.reset();
            m_font.weight(weight);
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font(text::NanFont font) -> Text& {
            m_typography_role.reset();
            m_font = std::move(font);
            m_user_font_color = m_font.has_explicit_color()
                ? std::optional<nandina::NanColor>{m_font.color()}
                : std::nullopt;
            m_shape_cache_valid = false;
            sync_resolved_style();
            mark_layout_dirty();
            return *this;
        }

        auto set_typography_role(const theme::NanTypographyRole role) -> Text& {
            if (m_typography_role.has_value() && *m_typography_role == role) {
                return *this;
            }

            m_typography_role = role;
            apply_typography_role();
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto font() const -> const text::NanFont& {
            return m_font;
        }

        template <typename Fn>
            requires std::invocable<Fn, text::NanFont&>
        auto update_font(Fn&& fn) -> Text& {
            auto updated = m_font;
            std::invoke(std::forward<Fn>(fn), updated);
            return set_font(std::move(updated));
        }

        auto set_color(const nandina::NanColor& color) -> Text& {
            m_user_font_color = color;
            sync_resolved_style();
            mark_dirty();
            return *this;
        }

        auto set_align(TextAlign align) -> Text& {
            m_align = align;
            mark_dirty();
            return *this;
        }

        auto set_vertical_align(TextVerticalAlign valign) -> Text& {
            m_valign = valign;
            mark_dirty();
            return *this;
        }

        auto set_align_to_ink_bounds(const bool value) -> Text& {
            if (m_align_to_ink_bounds == value) {
                return *this;
            }

            m_align_to_ink_bounds = value;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto text() const noexcept -> const std::string& {
            return m_text.get();
        }

        [[nodiscard]] auto font_size() const noexcept -> float {
            return m_font.size();
        }

        [[nodiscard]] auto font_family() const noexcept -> const std::string& {
            return m_font.family();
        }

        [[nodiscard]] auto font_weight() const noexcept -> text::NanFontWeight {
            return m_font.weight();
        }

        [[nodiscard]] auto color() const noexcept -> const nandina::NanColor& {
            return m_color.get();
        }

        [[nodiscard]] auto typography_role() const noexcept -> std::optional<theme::NanTypographyRole> {
            return m_typography_role;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto txt = display_text();
            if (txt.empty()) return {0.0f, 0.0f};

            if (m_font.is_loaded()) {
                const float text_w = m_font.estimate_text_width(txt);
                return {text_w, m_font.line_height()};
            }

            const float fs = m_font.size();
            const float text_w = static_cast<float>(txt.size()) * fs * 0.6f;
            return {text_w, fs * 1.4f};
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const auto txt = display_text();
            if (txt.empty()) {
                set_measured_layout_state(constraints, geometry::NanSize{});
                return;
            }

            const float max_width = constraints.max_width() != geometry::NanConstraints::k_infinity
                ? constraints.max_width()
                : 0.0f;

            const auto& layout = cached_shape(txt, max_width);
            const geometry::NanSize measured = layout.empty()
                ? geometry::NanSize{}
                : geometry::NanSize{layout.total_width, layout.total_height};

            if (measured.width() <= 0.0f && measured.height() <= 0.0f) {
                set_measured_layout_state(constraints, constraints.constrain(preferred_size()));
            } else {
                set_measured_layout_state(constraints, constraints.constrain(measured));
            }
        }

        [[nodiscard]] static auto measure_diag() -> std::pair<int, int> {
            return {
                s_measure_fast_count.exchange(0, std::memory_order_relaxed),
                s_measure_slow_count.exchange(0, std::memory_order_relaxed)
            };
        }

    protected:
        [[nodiscard]] virtual auto display_text() const -> std::string {
            return m_text.get();
        }

        [[nodiscard]] virtual auto resolved_text_color() const -> nandina::NanColor {
            const auto& style = theme::NanStylePrimitives::current().text;
            return m_user_font_color.value_or(style.font_color);
        }

        void on_draw(tvg::SwCanvas& canvas) override {
            const auto txt = display_text();
            if (txt.empty()) return;

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

            auto text_font = m_font;
            text_font.color(m_color.get());
            text_font.paint(canvas, layout, offset_x, offset_y);
        }

        Text() {
            const auto& style = theme::NanStylePrimitives::current().text;
            m_typography_role = theme::NanTypographyRole::body_medium;
            apply_typography_role();
            m_font.overflow(style.overflow)
                .single_line(style.single_line)
                .max_lines(style.max_lines);
            sync_resolved_style();
        }

    protected:
        auto apply_typography_role() -> void {
            if (!m_typography_role.has_value()) {
                return;
            }

            const auto& type_style = theme::resolve_typography_style(
                theme::NanStylePrimitives::current().typography,
                *m_typography_role);
            m_font.size(type_style.font_size)
                .weight(static_cast<text::NanFontWeight>(type_style.font_weight))
                .line_height(type_style.line_height)
                .letter_spacing(type_style.letter_spacing);
        }

        auto sync_resolved_style() -> void {
            const auto resolved = resolved_text_color();
            m_color.set(resolved);
            m_font.color(resolved);
        }

        auto invalidate_shape_cache() -> void {
            m_shape_cache_valid = false;
        }

        [[nodiscard]] auto cached_shape(const std::string& txt, float max_width) const -> const text::TextLayout& {
            const float quantized = std::round(max_width * 2.0f) * 0.5f;
            if (m_shape_cache_valid &&
                m_cached_shape_text == txt &&
                m_cached_shape_max_width == quantized) {
                return m_cached_shape_result;
            }
            m_cached_shape_result = m_font.shape(txt, quantized > 0.0f ? quantized : 0.0f);
            m_cached_shape_text = txt;
            m_cached_shape_max_width = quantized;
            m_shape_cache_valid = true;
            return m_cached_shape_result;
        }

        reactive::BindableProp<std::string> m_text{""};
        reactive::BindableProp<nandina::NanColor> m_color{nandina::NanColor::from(nandina::NanRgb{30, 30, 46})};
        text::NanFont m_font;
        TextAlign m_align{TextAlign::Start};
        TextVerticalAlign m_valign{TextVerticalAlign::Top};
        bool m_align_to_ink_bounds{true};
        std::optional<theme::NanTypographyRole> m_typography_role;
        std::optional<nandina::NanColor> m_user_font_color;

        mutable text::TextLayout m_cached_shape_result{};
        mutable std::string m_cached_shape_text{};
        mutable float m_cached_shape_max_width{-1.0f};
        mutable bool m_shape_cache_valid{false};

    private:
        inline static std::atomic<int> s_measure_fast_count{0};
        inline static std::atomic<int> s_measure_slow_count{0};
    };

} // namespace nandina::widgets