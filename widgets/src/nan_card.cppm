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
import nandina.widgets.label;

/**
 * nandina.widgets.card
 *
 * Card — 卡片组件。
 *
 * 职责：
 * - 背景色 / 圆角 / 可选描边
 * - 阴影效果（通过多层半透明矩形模拟）
 * - 可选头部图标（小方块）和标题文本
 * - 内边距控制子节点位置
 *
 * 设计：
 * - Card 继承 NanWidget，不继承 Surface（因为需要独立的阴影计算逻辑）
 * - 阴影使用多层偏移的半透明矩形模拟（类似 Material Design 的 elevation）
 * - 标题和图标在标题区域渲染，子节点在内容区渲染
 * - 链式 API：Card::create().set_title("Stats").set_elevation(4.0f)
 */
export namespace nandina::widgets {

    /**
     * Card — 展示内容的卡片容器。
     *
     * 用法：
     *   auto card = Card::create()
     *       .set_bg_color(NanColor::from(NanRgb{50, 52, 72}))
     *       .set_corner_radius(8.0f)
     *       .set_elevation(4.0f)
     *       .set_padding(geometry::NanInsets{12.0f});
     *   card->add_child(std::move(content));
     */
    class Card : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Card>;

        ~Card() override = default;

        static auto create() -> Ptr {
            return Ptr{new Card()};
        }

        // ── 背景色 ────────────────────────────────────────
        auto set_bg_color(const nandina::NanColor& color) -> Card& {
            m_bg_color.set(color);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto bg_color() const noexcept -> const nandina::NanColor& {
            return m_bg_color.get();
        }

        // ── 圆角 ──────────────────────────────────────────
        auto set_corner_radius(float radius) -> Card& {
            m_corner_radius.set(radius);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto corner_radius() const noexcept -> float {
            return m_corner_radius.get();
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

        // ── 内边距 ────────────────────────────────────────
        auto set_padding(const geometry::NanInsets& insets) -> Card& {
            m_padding.set(insets);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto padding() const noexcept -> const geometry::NanInsets& {
            return m_padding.get();
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

        // ── 描边 ──────────────────────────────────────────
        auto set_border_color(const nandina::NanColor& color) -> Card& {
            m_border_color = color;
            mark_dirty();
            return *this;
        }

        auto set_border_width(float width) -> Card& {
            m_border_width = width;
            mark_dirty();
            return *this;
        }

        // ── 布局 ──────────────────────────────────────────
        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);

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

            const auto& pad = m_padding.get();
            const float header_offset = title_header_height();

            // 子节点定位在标题区域下方 + padding 内
            const float child_x = x + pad.left();
            const float child_y = y + header_offset + pad.top();
            const float child_w = w - pad.left() - pad.right();
            const float child_h = h - header_offset - pad.top() - pad.bottom();

            for_each_child([&](runtime::NanWidget& child) {
                // 跳过标题 Label（它由 Card 自身管理）
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

            // ── 1. 阴影绘制（多层半透明矩形模拟 elevation） ──
            if (elev > 0.0f) {
                const int shadow_layers = std::min(4, static_cast<int>(elev / 2.0f + 1.0f));
                for (int i = 0; i < shadow_layers; ++i) {
                    const float offset = 1.0f + static_cast<float>(i) * 0.5f;
                    const float blur = 2.0f + static_cast<float>(i) * 1.5f;
                    // 阴影 alpha 从外到内衰减
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

            // ── 2. 背景填充 ──────────────────────────────
            {
                const auto& bg = m_bg_color.get();
                const auto bg_rgb = bg.to<nandina::NanRgb>();
                auto* shape = tvg::Shape::gen();
                shape->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius);
                shape->fill(bg_rgb.red(), bg_rgb.green(), bg_rgb.blue(), bg_rgb.alpha());
                canvas.add(shape);
            }

            // ── 3. 头部区域（标题 + 装饰色条） ──────────
            if (!m_title.empty() || m_show_accent) {
                // 装饰色条（左侧竖线）
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
                // 标题文字由 m_title_label 子节点绘制
            }

            // ── 4. 标题栏与内容区分隔线（如果有标题） ──
            if (!m_title.empty() && header_off > 0.0f) {
                auto* divider = tvg::Shape::gen();
                const float div_y = rect.y() + header_off;
                divider->moveTo(rect.x() + 4.0f, div_y);
                divider->lineTo(rect.x() + rect.width() - 4.0f, div_y);
                divider->strokeWidth(1.0f);
                divider->strokeFill(55, 57, 75, 150);
                canvas.add(divider);
            }

            // ── 5. 描边（如果有） ──────────────────────────
            if (m_border_width > 0.0f) {
                const auto bc_rgb = m_border_color.to<nandina::NanRgb>();
                auto* border = tvg::Shape::gen();
                border->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius);
                border->strokeWidth(m_border_width);
                border->strokeFill(bc_rgb.red(), bc_rgb.green(), bc_rgb.blue(), bc_rgb.alpha());
                canvas.add(border);
            }
        }

    private:
        Card() = default;

        /** 确保标题 Label 子节点存在 */
        auto ensure_title_label() -> void {
            if (m_title_label || m_title.empty()) return;
            auto label = Label::create();
            label->set_text(m_title)
                .set_font_size(m_title_font_size.get())
                .set_color(m_title_color.get());
            m_title_label = label.get();
            add_child(std::move(label));
        }

        /** 计算标题区域高度 */
        [[nodiscard]] auto title_header_height() const noexcept -> float {
            if (m_title.empty()) return 0.0f;
            return m_title_font_size.get() * 1.6f;
        }

        reactive::Prop<nandina::NanColor> m_bg_color{nandina::NanColor::from(nandina::NanRgb{50, 52, 72})};
        reactive::Prop<nandina::NanColor> m_title_color{nandina::NanColor::from(nandina::NanRgb{220, 220, 240})};
        reactive::Prop<nandina::NanColor> m_accent_color{nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        reactive::Prop<float> m_corner_radius{8.0f};
        reactive::Prop<float> m_elevation{0.0f};
        reactive::Prop<float> m_title_font_size{12.0f};
        reactive::Prop<geometry::NanInsets> m_padding{geometry::NanInsets{}};

        nandina::NanColor m_border_color{nandina::NanColor::from(nandina::NanRgb{0, 0, 0})};
        float m_border_width{0.0f};

        std::string m_title;
        bool m_show_accent{false};

        // 标题 Label 子节点（由 Card 自身管理定位）
        Label* m_title_label{nullptr};
    };

} // namespace nandina::widgets