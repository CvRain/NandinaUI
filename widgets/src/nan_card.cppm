//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>
#include <utility>
#include <algorithm>
#include <cmath>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.card;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_size;
import nandina.foundation.nan_rect;
import nandina.foundation.color;
import nandina.reactive.prop;
import nandina.widgets.surface;
import nandina.widgets.label;

/**
 * nandina.widgets.card
 *
 * Card — 卡片组件。
 *
 * 职责：
 * - 继承 Surface，复用背景色/圆角/描边/内边距
 * - 阴影效果（Material Design elevation，多层半透明矩形模拟）
 * - 可选头部标题（Label 组件）+ 左侧装饰色条
 * - 标题栏与内容区分隔线
 *
 * 继承链：NanWidget → Surface → Card
 *
 * 用法：
 *   auto card = Card::create()
 *       .set_bg_color(NanColor::from(NanRgb{50, 52, 72}))
 *       .set_corner_radius(8.0f)
 *       .set_elevation(4.0f)
 *       .set_title("Statistics")
 *       .set_show_accent(true);
 *   card->add_child(std::move(content));
 */
export namespace nandina::widgets {

    class Card : public Surface {
    public:
        using Ptr = std::unique_ptr<Card>;

        ~Card() override = default;

        static auto create() -> Ptr {
            return Ptr{new Card()};
        }

        // ── 阴影高度（elevation） ──────────────────────────
        auto set_elevation(float elevation) -> Card& {
            m_elevation.set(elevation);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto elevation() const noexcept -> float {
            return m_elevation.get();
        }

        // ── 从 Surface override 方法（返回 Card& 以保持链式） ──
        auto set_bg_color(const nandina::NanColor& color) -> Card& {
            Surface::set_bg_color(color);
            return *this;
        }

        auto set_corner_radius(float radius) -> Card& {
            Surface::set_corner_radius(radius);
            return *this;
        }

        auto set_padding(const geometry::NanInsets& insets) -> Card& {
            Surface::set_padding(insets);
            return *this;
        }

        auto set_border_color(const nandina::NanColor& color) -> Card& {
            Surface::set_border_color(color);
            return *this;
        }

        auto set_border_width(float width) -> Card& {
            Surface::set_border_width(width);
            return *this;
        }

        // ── 可选标题 ──────────────────────────────────────
        auto set_title(std::string title) -> Card& {
            m_title = std::move(title);
            // 如果已创建标题 Label，更新其文本
            if (m_title_label) {
                m_title_label->set_text(m_title);
            }
            mark_dirty();
            return *this;
        }

        auto set_title_color(const nandina::NanColor& color) -> Card& {
            m_title_color.set(color);
            if (m_title_label) {
                m_title_label->set_color(color);
            }
            mark_dirty();
            return *this;
        }

        auto set_title_font_size(float size) -> Card& {
            m_title_font_size.set(size);
            if (m_title_label) {
                m_title_label->set_font_size(size);
            }
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto title() const noexcept -> const std::string& {
            return m_title;
        }

        // ── 可选头部装饰色（左侧色条） ──────────────────
        auto set_accent_color(const nandina::NanColor& color) -> Card& {
            m_accent_color.set(color);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto accent_color() const noexcept -> const nandina::NanColor& {
            return m_accent_color.get();
        }

        auto set_show_accent(bool show) -> Card& {
            m_show_accent = show;
            mark_dirty();
            return *this;
        }

        // ── 布局 ──────────────────────────────────────────
        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            // 先调用 Surface 的 set_bounds 进行基本定位和 padding 收缩
            Surface::set_bounds(x, y, w, h);

            // 确保标题 Label 存在
            if (!m_title.empty() && !m_title_label) {
                ensure_title_label();
            }

            // 标题 Label 定位在头部区域
            if (m_title_label) {
                const auto b = bounds();
                const float header_off = title_header_height();
                const float text_start_x = b.x() + (m_show_accent ? 12.0f : 8.0f);
                const float text_y = b.y() + header_off * 0.15f;
                const float text_w = b.width() - 24.0f;
                const float text_h = header_off * 0.7f;
                m_title_label->set_bounds(text_start_x, text_y, text_w, text_h);
            }

            // 子节点定位在标题区域下方 + padding 内
            const auto& pad = m_padding.get();
            const float header_offset = title_header_height();
            const float child_x = x + pad.left();
            const float child_y = y + header_offset + pad.top();
            const float child_w = w - pad.left() - pad.right();
            const float child_h = h - header_offset - pad.top() - pad.bottom();

            for_each_child([&](runtime::NanWidget& child) {
                if (&child == m_title_label) return;
                child.set_bounds(child_x, child_y, child_w, child_h);
            });

            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize child_pref{0.0f, 0.0f};

            for_each_child([&](const runtime::NanWidget& child) {
                const auto cp = child.preferred_size();
                child_pref = geometry::NanSize{
                    std::max(child_pref.width(), cp.width()),
                    std::max(child_pref.height(), cp.height())
                };
            });

            const auto& pad = m_padding.get();
            const float header_h = title_header_height();
            const float min_h = header_h + pad.top() + pad.bottom();

            return geometry::NanSize{
                child_pref.width() + pad.left() + pad.right(),
                std::max(child_pref.height() + header_h + pad.top() + pad.bottom(), min_h)
            };
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect = bounds();
            const float radius = m_corner_radius.get();
            const float elev = m_elevation.get();
            const float header_off = title_header_height();

            // ── 1. 阴影绘制（在背景之前） ──────────────────
            if (elev > 0.0f) {
                const int shadow_layers = std::min(4, static_cast<int>(elev / 2.0f + 1.0f));
                for (int i = shadow_layers - 1; i >= 0; --i) {
                    const float offset = 1.0f + static_cast<float>(i) * 0.5f;
                    const float blur = 2.0f + static_cast<float>(i) * 1.5f;
                    const uint8_t alpha = static_cast<uint8_t>(30.0f * (1.0f - static_cast<float>(i) / static_cast<float>(shadow_layers)));

                    auto* shadow = tvg::Shape::gen();
                    shadow->appendRect(
                        rect.x() + offset,
                        rect.y() + offset + blur * 0.3f,
                        rect.width() - offset * 2.0f,
                        rect.height() - offset * 2.0f - blur * 0.3f,
                        radius, radius
                    );
                    shadow->fill(0, 0, 0, alpha);
                    canvas.add(shadow);
                }
            }

            // ── 2. 背景（复用 Surface 的 draw_background） ──
            draw_background(canvas);

            // ── 3. 头部装饰色条 ──────────────────────────
            if (!m_title.empty() || m_show_accent) {
                if (m_show_accent) {
                    const auto& acc = m_accent_color.get();
                    const auto acc_rgb = acc.to<nandina::NanRgb>();
                    const float bar_w = 3.0f;
                    auto* accent_bar = tvg::Shape::gen();
                    accent_bar->appendRect(
                        rect.x() + 1.0f,
                        rect.y() + header_off * 0.15f,
                        bar_w,
                        header_off * 0.7f,
                        1.5f, 1.5f
                    );
                    accent_bar->fill(acc_rgb.red(), acc_rgb.green(), acc_rgb.blue(), acc_rgb.alpha());
                    canvas.add(accent_bar);
                }
            }

            // ── 4. 标题栏与内容区分隔线 ──────────────────
            if (!m_title.empty() && header_off > 0.0f) {
                auto* divider = tvg::Shape::gen();
                const float div_y = rect.y() + header_off;
                divider->moveTo(rect.x() + 4.0f, div_y);
                divider->lineTo(rect.x() + rect.width() - 4.0f, div_y);
                divider->strokeWidth(1.0f);
                divider->strokeFill(55, 57, 75, 150);
                canvas.add(divider);
            }

            // 描边已由 draw_background() 处理
        }

    private:
        Card() : Surface() {
            // Card 默认值（覆盖 Surface 的默认白色背景）
            m_bg_color.set(nandina::NanColor::from(nandina::NanRgb{50, 52, 72}));
            m_corner_radius.set(8.0f);
        }

        auto ensure_title_label() -> void {
            if (m_title_label || m_title.empty()) return;
            auto label = Label::create();
            label->set_text(m_title)
                .set_font_size(m_title_font_size.get())
                .set_color(m_title_color.get());
            m_title_label = label.get();
            add_child(std::move(label));
        }

        [[nodiscard]] auto title_header_height() const noexcept -> float {
            if (m_title.empty()) return 0.0f;
            return m_title_font_size.get() * 1.6f;
        }

        // Card 独有的成员（bg_color, corner_radius, padding, border 从 Surface 继承）
        reactive::Prop<nandina::NanColor> m_title_color{
            nandina::NanColor::from(nandina::NanRgb{220, 220, 240})};
        reactive::Prop<nandina::NanColor> m_accent_color{
            nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        reactive::Prop<float> m_elevation{0.0f};
        reactive::Prop<float> m_title_font_size{12.0f};

        std::string m_title;
        bool m_show_accent{false};
        Label* m_title_label{nullptr};
    };

} // namespace nandina::widgets