//
// Created by cvrain on 2026/4/28.
//

module;

#include <atomic>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.label;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.state;
import nandina.reactive.bindable_prop;
import nandina.text.nan_font;
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

    // ── 对齐枚举 ──────────────────────────────────────
    enum class TextAlign {
        Start, // 左对齐
        Center, // 居中对齐
        End // 右对齐
    };

    enum class TextVerticalAlign {
        Top,
        Center,
        Bottom
    };

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
    class Label : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Label>;

        ~Label() override = default;

        static auto create() -> Ptr {
            return Ptr(new Label());
        }

        // ── 属性设置 ──────────────────────────────────────
        auto set_text(std::string_view text) -> Label& {
            m_text.set(std::string{text});
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font_size(float size) -> Label& {
            m_font.size(size);
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font_family(std::string family) -> Label& {
            m_font.family(std::move(family));
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font_weight(text::NanFontWeight weight) -> Label& {
            m_font.weight(weight);
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        auto set_font(text::NanFont font) -> Label& {
            m_font = std::move(font);
            m_color.set(m_font.color());
            m_shape_cache_valid = false;
            mark_layout_dirty();
            return *this;
        }

        /// 只读访问内部 NanFont
        auto font() const -> const text::NanFont& {
            return m_font;
        }

        /// 局部更新字体属性；Fn: (NanFont&) -> void
        ///
        /// 示例：
        ///   label_ref->update_font([](auto& f){ f.color(NanColor::red()); });
        ///   label_ref->update_font([](auto& f){ f.size(f.size() + 2); });
        template <typename Fn>
            requires std::invocable<Fn, text::NanFont&>
        auto update_font(Fn&& fn) -> Label& {
            auto updated = m_font;
            std::invoke(std::forward<Fn>(fn), updated);
            return set_font(std::move(updated));
        }

        auto set_color(const nandina::NanColor& color) -> Label& {
            m_user_font_color = color;
            sync_resolved_style();
            mark_dirty();
            return *this;
        }

        auto set_align(TextAlign align) -> Label& {
            m_align = align;
            mark_dirty();
            return *this;
        }

        auto set_vertical_align(TextVerticalAlign valign) -> Label& {
            m_valign = valign;
            mark_dirty();
            return *this;
        }
        /// 禁用状态——禁用时文本色降至 70% 透明度（对应 shadcn peer-disabled:opacity-70）。
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
            m_shape_cache_valid = false;
            mark_dirty();
            mark_layout_dirty();
            return *this;
        }
        // ── 属性访问 ──────────────────────────────────────
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

        [[nodiscard]] auto disabled() const noexcept -> bool {
            return m_disabled;
        }

        [[nodiscard]] auto error() const noexcept -> bool {
            return m_error;
        }

        [[nodiscard]] auto required() const noexcept -> bool {
            return m_required;
        }

        // ── 首选尺寸 ──────────────────────────────────────
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto txt = display_text();
            if (txt.empty()) return {0.0f, 0.0f};

            if (m_font.is_loaded()) {
                const float text_w = m_font.estimate_text_width(txt);
                return {text_w, m_font.line_height()};
            }

            const float fs     = m_font.size();
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

            // 始终走 cached_shape 路径，保证 measure 和 on_draw 看到同一份布局
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

        /// 获取并重置 measure 路径统计（用于性能诊断）
        [[nodiscard]] static auto measure_diag() -> std::pair<int, int> {
            return {
                s_measure_fast_count.exchange(0, std::memory_order_relaxed),
                s_measure_slow_count.exchange(0, std::memory_order_relaxed)
            };
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto txt = display_text();
            if (txt.empty()) return;

            const auto bnds = bounds();

            // 使用 NanFont::shape() 布局文本
            // 优先使用 measure 阶段记录的可用宽度（measured_constraints().max_width()），
            // 与 measure() 共享同一份 shape 缓存，且避免在 relayout 前因 bnds.width()
            // 仅等于旧内容宽度而触发误溢出（例如 set_text 改成更宽字符时显示 "..."）。
            const float avail_w = measured_constraints().max_width();
            const float max_width = (avail_w > 0.0f && avail_w < geometry::NanConstraints::k_infinity)
                ? avail_w
                : bnds.width();
            const auto& layout    = cached_shape(txt, max_width > 0.0f ? max_width : 0.0f);

            if (layout.empty()) return;

            // 计算水平对齐偏移
            const float block_width = std::max(0.0f, layout.ink_right - layout.ink_left);
            float offset_x          = bnds.x();
            switch (m_align) {
            case TextAlign::Start:  break;
            case TextAlign::Center: offset_x += (bnds.width() - block_width) * 0.5f; break;
            case TextAlign::End:    offset_x += bnds.width() - block_width;           break;
            }
            offset_x -= layout.ink_left;

            // 计算垂直对齐偏移
            const float block_height = std::max(0.0f, layout.ink_bottom - layout.ink_top);
            float offset_y           = bnds.y();
            switch (m_valign) {
            case TextVerticalAlign::Top:    break;
            case TextVerticalAlign::Center: offset_y += (bnds.height() - block_height) * 0.5f; break;
            case TextVerticalAlign::Bottom: offset_y += bnds.height() - block_height;           break;
            }
            offset_y -= layout.ink_top;

            auto text_font = m_font;
            text_font.color(m_color.get());
            text_font.paint(canvas, layout, offset_x, offset_y);

            if (m_required) {
                auto suffix_font = m_font;
                suffix_font.color(required_indicator_color());

                const auto& raw_text = m_text.get();
                const auto raw_layout = raw_text.empty()
                    ? text::TextLayout{}
                    : cached_shape(raw_text, max_width > 0.0f ? max_width : 0.0f);

                const auto suffix = raw_text.empty() ? std::string{"*"} : std::string{" *"};
                const auto suffix_layout = suffix_font.shape(suffix, 0.0f);
                const float suffix_x = offset_x + raw_layout.total_width;
                const float suffix_y = offset_y + std::max(0.0f, layout.total_height - suffix_layout.total_height);
                suffix_font.paint(canvas, suffix_layout, suffix_x, suffix_y);
            }
        }

    private:
        Label() {
            const auto& style = theme::NanStylePrimitives::current().label;
            m_font.size(style.font_size)
                .weight(style.font_weight)
                .overflow(style.overflow)
                .single_line(style.single_line)
                .max_lines(style.max_lines);
            sync_resolved_style();
        }

        [[nodiscard]] auto display_text() const -> std::string {
            if (!m_required) {
                return m_text.get();
            }

            if (m_text.get().empty()) {
                return "*";
            }

            return m_text.get() + " *";
        }

        [[nodiscard]] auto resolved_text_color() const -> nandina::NanColor {
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

        auto sync_resolved_style() -> void {
            const auto resolved = resolved_text_color();
            m_color.set(resolved);
            m_font.color(resolved);
        }

        [[nodiscard]] auto cached_shape(const std::string& txt, float max_width) const -> const text::TextLayout& {
            const float quantized = std::round(max_width * 2.0f) * 0.5f;
            if (m_shape_cache_valid &&
                m_cached_shape_text == txt &&
                m_cached_shape_max_width == quantized) {
                return m_cached_shape_result;
            }
            m_cached_shape_result     = m_font.shape(txt, quantized > 0.0f ? quantized : 0.0f);
            m_cached_shape_text       = txt;
            m_cached_shape_max_width  = quantized;
            m_shape_cache_valid       = true;
            return m_cached_shape_result;
        }

        void mark_font_dirty() {
            m_shape_cache_valid = false;
        }

        reactive::BindableProp<std::string> m_text{""};
        reactive::BindableProp<nandina::NanColor> m_color{nandina::NanColor::from(nandina::NanRgb{220, 220, 240})};

        text::NanFont m_font;  // 字体（值类型，拷贝语义）

        TextAlign m_align{TextAlign::Start};
        TextVerticalAlign m_valign{TextVerticalAlign::Top};
        bool m_disabled{false};
        bool m_error{false};
        bool m_required{false};
        std::optional<nandina::NanColor> m_user_font_color;

        // ── shape 结果缓存 ────────────────────────────────────────
        mutable text::TextLayout m_cached_shape_result{};
        mutable std::string      m_cached_shape_text{};
        mutable float            m_cached_shape_max_width{-1.0f};
        mutable bool             m_shape_cache_valid{false};

        // ── measure 路径诊断计数器 ─────────────────────────────────
        inline static std::atomic<int> s_measure_fast_count{0};
        inline static std::atomic<int> s_measure_slow_count{0};
    };

} // namespace nandina::widgets
