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

/**
 * nandina.widgets.label
 *
 * Label — 文本显示组件（纯 ThorVG 模拟，M5 前占位）。
 *
 * 职责：
 * - 显示文本字符串
 * - 支持字体大小、颜色、水平/垂直对齐
 * - 首选尺寸基于文本近似长度计算
 *
 * 注意：
 * - M5 前使用「点阵模拟」绘制方式（与 showcase 中 draw_text_dots 相同）
 * - M5 后将替换为 freetype + harfbuzz 真实字体渲染
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

        // ── 首选尺寸 ──────────────────────────────────────
        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto& txt = m_text.get();
            const float fs = m_font_size.get();
            const float spacing = fs * 0.8f;
            const float dot_r = fs * 0.25f;

            const float text_width = static_cast<float>(txt.size()) * spacing + dot_r * 2.0f;
            const float text_height = fs + dot_r * 2.0f;

            return geometry::NanSize{text_width, text_height};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect = bounds();
            const auto& txt = m_text.get();
            const float fs = m_font_size.get();
            const auto& clr = m_color.get();
            const auto rgb = clr.to<nandina::NanRgb>();

            if (txt.empty()) return;

            // 计算文本的近似宽度
            const float spacing = fs * 0.8f;
            const float dot_r = fs * 0.25f;
            const float text_width = static_cast<float>(txt.size()) * spacing;
            const float text_height = fs;

            // 水平对齐
            float start_x = rect.x();
            switch (m_align) {
            case TextAlign::Start:
                start_x = rect.x() + dot_r;
                break;
            case TextAlign::Center:
                start_x = rect.x() + (rect.width() - text_width) * 0.5f;
                break;
            case TextAlign::End:
                start_x = rect.x() + rect.width() - text_width - dot_r;
                break;
            }

            // 垂直对齐
            float start_y = rect.y();
            switch (m_valign) {
            case TextVerticalAlign::Top:
                start_y = rect.y() + dot_r;
                break;
            case TextVerticalAlign::Center:
                start_y = rect.y() + (rect.height() - text_height) * 0.5f + dot_r;
                break;
            case TextVerticalAlign::Bottom:
                start_y = rect.y() + rect.height() - text_height - dot_r;
                break;
            }

            // 用点阵模拟绘制每个字符
            for (size_t i = 0; i < txt.size(); ++i) {
                if (txt[i] == ' ') continue;

                auto* dot = tvg::Shape::gen();
                const float cx = start_x + static_cast<float>(i) * spacing;
                const float cy = start_y;
                dot->appendCircle(cx, cy, dot_r, dot_r);
                dot->fill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
                canvas.add(dot);
            }
        }

    private:
        Label() = default;

        reactive::Prop<std::string> m_text{""};
        reactive::Prop<float> m_font_size{14.0f};
        reactive::Prop<nandina::NanColor> m_color{nandina::NanColor::from(nandina::NanRgb{220, 220, 240})};

        TextAlign m_align{TextAlign::Start};
        TextVerticalAlign m_valign{TextVerticalAlign::Top};
    };

} // namespace nandina::widgets