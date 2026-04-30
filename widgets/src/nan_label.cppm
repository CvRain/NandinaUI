//
// Created by cvrain on 2026/4/28.
//

module;

#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.label;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.state;
import nandina.reactive.prop;
import nandina.text.nan_font;

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
        Start,   // 左对齐
        Center,  // 居中对齐
        End      // 右对齐
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
            return Ptr{new Label()};
        }

        // ── 属性设置 ──────────────────────────────────────
        auto set_text(std::string_view text) -> Label& {
            m_text.set(std::string{text});
            mark_dirty();
            return *this;
        }

        auto set_font_size(float size) -> Label& {
            m_font_size.set(size);
            m_font.reset();  // 字号变化 → 重置字体（下次懒加载新字号）
            mark_dirty();
            return *this;
        }

        /// 显式设置字体。接收 NanFont 后，font_size 由字体文件决定。
        auto set_font(text::NanFont::Ptr font) -> Label& {
            m_font = std::move(font);
            if (m_font) {
                m_font_size.set(m_font->size_pt());
            }
            mark_dirty();
            return *this;
        }

        auto set_color(const nandina::NanColor& color) -> Label& {
            m_color.set(color);
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

        // ── 属性访问 ──────────────────────────────────────
        [[nodiscard]] auto text() const noexcept -> const std::string& {
            return m_text.get();
        }

        [[nodiscard]] auto font_size() const noexcept -> float {
            return m_font_size.get();
        }

        [[nodiscard]] auto color() const noexcept -> const nandina::NanColor& {
            return m_color.get();
        }

        [[nodiscard]] auto font() const noexcept -> const text::NanFont::Ptr& {
            return m_font;
        }

        // ── 首选尺寸 ──────────────────────────────────────
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto& txt = m_text.get();
            if (txt.empty()) {
                return {0.0f, 0.0f};
            }

            // 如果有字体，使用字体度量计算真实宽度
            if (m_font) {
                const float text_w = m_font->estimate_text_width(txt);
                return {text_w, m_font->line_height()};
            }

            // 没有字体时的回退估算（首次 set_text 后可能还没有 font）
            const float fs = m_font_size.get();
            const float text_w = static_cast<float>(txt.size()) * fs * 0.6f;
            return {text_w, fs * 1.4f};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto& txt = m_text.get();
            if (txt.empty()) return;

            // 懒加载字体
            ensure_font();

            if (!m_font) {
                return;  // 字体加载失败时放弃绘制
            }

            const auto bnds = bounds();
            const auto& clr = m_color.get();

            // 使用 NanFont::shape() 布局文本（限制宽度 = bounds_width，支持自动换行）
            const float max_width = bnds.width();
            auto layout = m_font->shape(txt, max_width > 0.0f ? max_width : 0.0f, 0);

            if (layout.empty()) return;

            // 计算水平对齐偏移
            const float block_width = layout.total_width;
            float offset_x = bnds.x();

            switch (m_align) {
            case TextAlign::Start:
                // 左对齐（默认）
                break;
            case TextAlign::Center:
                offset_x += (bnds.width() - block_width) * 0.5f;
                break;
            case TextAlign::End:
                offset_x += bnds.width() - block_width;
                break;
            }

            // 计算垂直对齐偏移
            const float block_height = layout.total_height;
            float offset_y = bnds.y();

            switch (m_valign) {
            case TextVerticalAlign::Top:
                break;
            case TextVerticalAlign::Center:
                offset_y += (bnds.height() - block_height) * 0.5f;
                break;
            case TextVerticalAlign::Bottom:
                offset_y += bnds.height() - block_height;
                break;
            }

            // 使用 NanFont 绘制真实字体文本
            m_font->paint(canvas, layout, offset_x, offset_y, clr);
        }

    private:
        Label() = default;

        /// 懒加载字体：如果 m_font 为空，尝试加载系统默认字体。
        auto ensure_font() -> void {
            if (m_font) return;
            try {
                m_font = text::NanFont::load_system_default(m_font_size.get());
            } catch (const std::exception&) {
                // 字体加载失败——静默跳过绘制
                // 错误已由 NanFont 内部日志记录
            }
        }

        reactive::Prop<std::string>          m_text{""};
        reactive::Prop<float>                m_font_size{14.0f};
        reactive::Prop<nandina::NanColor>    m_color{nandina::NanColor::from(nandina::NanRgb{220, 220, 240})};

        text::NanFont::Ptr                   m_font;  // 懒加载

        TextAlign                            m_align{TextAlign::Start};
        TextVerticalAlign                    m_valign{TextVerticalAlign::Top};
    };

} // namespace nandina::widgets