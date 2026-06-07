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
import nandina.layout.flex_widgets;
import nandina.reactive.prop;
import nandina.widgets.surface;
import nandina.widgets.text;
import nandina.theme.nan_style;

/**
 * nandina.widgets.card
 *
 * Card — 卡片组件。
 *
 * 职责：
 * - 继承 Surface，复用背景色/圆角/描边/内边距
 * - 阴影效果（Material Design elevation，多层半透明矩形模拟）
 * - 可选头部标题（Text primitive）+ 左侧装饰色条
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
        auto set_bg_color(const nandina::NanColor &color) -> Card& override {
            Surface::set_bg_color(color);
            return *this;
        }

        auto set_corner_radius(float radius) -> Card& override {
            Surface::set_corner_radius(radius);
            return *this;
        }

        auto set_padding(const geometry::NanInsets &insets) -> Card& override {
            Surface::set_padding(insets);
            return *this;
        }

        auto set_border_color(const nandina::NanColor &color) -> Card& override {
            Surface::set_border_color(color);
            return *this;
        }

        auto set_border_width(const float width) -> Card& override {
            Surface::set_border_width(width);
            return *this;
        }

        // ── 可选标题 ──────────────────────────────────────
        auto set_title(std::string title) -> Card& {
            m_title = std::move(title);
            if (!m_title.empty() && !m_title_host) {
                ensure_title_label();
            }
            if (m_title_label) {
                m_title_label->set_text(m_title);
            }
            mark_layout_dirty();
            return *this;
        }

        auto set_title_color(const nandina::NanColor &color) -> Card& {
            m_title_color.set(color);
            if (m_title_label) {
                m_title_label->set_color(color);
            }
            mark_dirty();
            return *this;
        }

        auto set_title_font_size(float size) -> Card& {
            m_title_font_size.set(size);
            if (m_title_label) { m_title_label->set_font_size(size); }
            sync_title_host_style();
            mark_layout_dirty();
            return *this;
        }

        auto set_title_font_weight(text::NanFontWeight weight) -> Card& {
            m_title_font_weight = weight;
            if (m_title_label) { m_title_label->set_font_weight(weight); }
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto title() const noexcept -> const std::string& {
            return m_title;
        }

        // ── 可选头部装饰色（左侧色条） ──────────────────
        auto set_accent_color(const nandina::NanColor &color) -> Card& {
            m_accent_color.set(color);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto accent_color() const noexcept -> const nandina::NanColor& {
            return m_accent_color.get();
        }

        auto set_show_accent(bool show) -> Card& {
            m_show_accent = show;
            sync_title_host_style();
            mark_layout_dirty();
            return *this;
        }

        // ── 可选描述文本（标题下方副标题） ──────────────
        auto set_description(std::string desc) -> Card& {
            m_description = std::move(desc);
            if (!m_title_host) ensure_title_label();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto description() const noexcept -> const std::string& {
            return m_description;
        }

        // ── 可选 Footer 区域 ────────────────────────────
        auto set_footer(runtime::NanWidget::Ptr footer) -> Card& {
            auto wrapper = layout::Padding::Create();
            wrapper->padding(12.0f, 12.0f, 16.0f, 12.0f);
            wrapper->child(std::move(footer));
            m_footer = add_child(std::move(wrapper));
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto footer() const noexcept -> runtime::NanWidget* {
            return m_footer;
        }

        // ── 布局 ──────────────────────────────────────────
        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            runtime::NanWidget::set_bounds(x, y, w, h);
            return *this;
        }

        auto measure(const geometry::NanConstraints &constraints) -> void override {
            const auto &pad = padding();
            const float header_h = title_header_height();
            const float footer_h = footer_height();

            const float content_avail_w = constraints.max_width() == geometry::NanConstraints::k_infinity
                ? geometry::NanConstraints::k_infinity
                : std::max(0.0f, constraints.max_width() - pad.left() - pad.right());

            // 用无约束测量获取子节点真实首选宽度，再以此限制排版宽度。
            // 不能依赖 child.preferred_size() 的缓存，因为大幅缩放后缓存的
            // 是前一次紧凑测量下的换行宽度，而不是文本的自然宽度。
            float content_max_w = content_avail_w;
            if (content_avail_w != geometry::NanConstraints::k_infinity && content_avail_w > 0.0f) {
                geometry::NanSize natural_pref{0.0f, 0.0f};
                for_each_child([&](runtime::NanWidget &child) {
                    if (&child == m_title_host || &child == m_footer) return;
                    child.measure(geometry::NanConstraints{
                        0.0f, geometry::NanConstraints::k_infinity,
                        0.0f, geometry::NanConstraints::k_infinity
                    });
                    const auto m = child.measured_size();
                    const auto p = child.preferred_size();
                    const float child_w = m.width() > 0.0f ? m.width() : p.width();
                    const float child_h = m.height() > 0.0f ? m.height() : p.height();
                    natural_pref = geometry::NanSize{
                        std::max(natural_pref.width(), child_w),
                        std::max(natural_pref.height(), child_h)
                    };
                });
                if (natural_pref.width() > 0.0f && natural_pref.width() < content_max_w) {
                    content_max_w = natural_pref.width();
                }
            }

            geometry::NanSize child_measured{0.0f, 0.0f};
            for_each_child([&](runtime::NanWidget &child) {
                if (&child == m_title_host || &child == m_footer) return;

                const geometry::NanConstraints cc{
                    std::max(0.0f, constraints.min_width() - pad.left() - pad.right()),
                    content_max_w,
                    std::max(0.0f, constraints.min_height() - pad.top() - pad.bottom() - header_h - footer_h),
                    constraints.max_height() == geometry::NanConstraints::k_infinity
                        ? geometry::NanConstraints::k_infinity
                        : std::max(0.0f, constraints.max_height() - pad.top() - pad.bottom() - header_h - footer_h),
                };

                child.measure(geometry::NanConstraints{
                    cc.min_width(), cc.max_width(),
                    0.0f, geometry::NanConstraints::k_infinity
                });
                const auto m = child.measured_size();
                const auto p = child.preferred_size();
                child_measured = geometry::NanSize{
                    std::max(child_measured.width(), m.width() > 0.0f ? m.width() : p.width()),
                    std::max(child_measured.height(), m.height() > 0.0f ? m.height() : p.height())
                };
            });

            if (m_footer) {
                const geometry::NanConstraints fc{
                    std::max(0.0f, constraints.min_width() - pad.left() - pad.right()),
                    content_avail_w,
                    0.0f, geometry::NanConstraints::k_infinity,
                };
                m_footer->measure(geometry::NanConstraints{
                    fc.min_width(), fc.max_width(),
                    0.0f, geometry::NanConstraints::k_infinity
                });
            }

            set_measured_layout_state(constraints, constraints.constrain(geometry::NanSize{
                child_measured.width() + pad.left() + pad.right(),
                child_measured.height() + header_h + footer_h + pad.top() + pad.bottom()
            }));
        }

        auto layout() -> void override {
            const auto& pad = padding();
            const float header_h = title_header_height();

            // 计算 footer 高度（基于上次 measure）
            float footer_h = 0.0f;
            if (m_footer) {
                const auto ms = m_footer->measured_size();
                const auto ps = m_footer->preferred_size();
                footer_h = std::max(ms.height() > 0.0f ? ms.height() : ps.height(), 0.0f);
            }

            // 标题区域
            if (m_title_host) {
                m_title_host->set_bounds(x(), y(), width(), header_h);
                m_title_host->layout();
            }

            // Content 区域
            {
                const float content_y = y() + header_h + pad.top();
                const float content_h = std::max(0.0f,
                    height() - header_h - footer_h - pad.top() - pad.bottom());
                for_each_child([&](runtime::NanWidget& child) {
                    if (&child == m_title_host || &child == m_footer) return;
                    child.set_bounds(x() + pad.left(), content_y,
                        width() - pad.left() - pad.right(), content_h);
                    child.layout();
                });
            }

            // Footer 区域 — 底部对齐
            if (m_footer && footer_h > 0.0f) {
                const float fy = y() + height() - footer_h;
                m_footer->set_bounds(x() + pad.left(), fy,
                    width() - pad.left() - pad.right(), footer_h);
                m_footer->layout();
            }

            clear_layout_dirty();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto child_pref = measure_content_preferred_size(m_title_host);
            const auto &pad = padding();
            const float header_h = title_header_height();
            const float footer_h = footer_height();

            return geometry::NanSize{
                child_pref.width() + pad.left() + pad.right(),
                std::max(child_pref.height() + header_h + footer_h + pad.top() + pad.bottom(),
                         header_h + footer_h + pad.top() + pad.bottom())
            };
        }

    protected:
        void on_draw(tvg::SwCanvas &canvas) override {
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
                    const uint8_t alpha = static_cast<uint8_t>(
                        30.0f * (1.0f - static_cast<float>(i) / static_cast<float>(shadow_layers)));

                    auto *shadow = tvg::Shape::gen();
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
                    const auto &acc = m_accent_color.get();
                    const auto acc_rgb = acc.to<nandina::NanRgb>();
                    const float bar_w = 3.0f;
                    auto *accent_bar = tvg::Shape::gen();
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
                auto *divider = tvg::Shape::gen();
                const float div_y = rect.y() + header_off;
                divider->moveTo(rect.x() + 4.0f, div_y);
                divider->lineTo(rect.x() + rect.width() - 4.0f, div_y);
                divider->strokeWidth(1.0f);
                divider->strokeFill(55, 57, 75, 150);
                canvas.add(divider);
            }

            // ── 5. Footer 分隔线 ──────────────────────────
            if (m_footer) {
                float fh = 0.0f;
                const auto ms = m_footer->measured_size();
                const auto ps = m_footer->preferred_size();
                fh = std::max(ms.height() > 0.0f ? ms.height() : ps.height(), 0.0f);
                if (fh > 0.0f) {
                    auto *divider = tvg::Shape::gen();
                    const float div_y = rect.y() + rect.height() - fh;
                    divider->moveTo(rect.x() + 16.0f, div_y);
                    divider->lineTo(rect.x() + rect.width() - 16.0f, div_y);
                    divider->strokeWidth(1.0f);
                    divider->strokeFill(55, 57, 75, 150);
                    canvas.add(divider);
                }
            }

            // 描边已由 draw_background() 处理
        }

    private:
        Card() : Surface() {
            // 从 theme token 读取默认样式（背景/圆角/边框/内边距/标题）
            const auto &style = nandina::theme::NanStylePrimitives::current().card;
            m_bg_color.set(style.bg);
            m_corner_radius.set(style.corner_radius);
            m_border_color = style.border;
            m_border_width = style.border_width;
            set_padding(style.padding);

            m_title_font_size.set(style.title_font_size);
            m_title_color.set(style.title_font_color);
        }

        auto ensure_title_label() -> void {
            if (m_title_host || m_title.empty())
                return;

            auto title_host = layout::Padding::Create();
            m_title_host = title_host.get();

            auto label = Text::create();
            label->set_text(m_title)
                    .set_typography_role(theme::NanTypographyRole::title_medium)
                    .set_font_size(m_title_font_size.get())
                    .set_font_weight(m_title_font_weight)
                    .set_color(m_title_color.get());
            m_title_label = label.get();
            title_host->child(std::move(label));
            sync_title_host_style();
            add_child(std::move(title_host));
        }

        auto sync_title_host_style() -> void {
            if (!m_title_host) {
                return;
            }

            const float header_off = title_header_height();
            const float left = m_show_accent ? 12.0f : 8.0f;
            const float right = m_show_accent ? 12.0f : 16.0f;
            const float vertical = header_off * 0.15f;
            m_title_host->padding(left, vertical, right, vertical);
        }

        [[nodiscard]] auto title_header_height() const noexcept -> float {
            if (m_title.empty()) return 0.0f;
            float h = m_title_font_size.get() * 1.6f;
            if (!m_description.empty()) h += m_title_font_size.get() * 0.9f;
            return h;
        }

        [[nodiscard]] auto footer_height() const noexcept -> float {
            if (!m_footer) return 0.0f;
            const auto fp = m_footer->preferred_size();
            return fp.height() > 0.0f ? fp.height() : 36.0f;
        }

        // Card 独有的成员（bg_color, corner_radius, padding, border 从 Surface 继承）
        reactive::Prop<nandina::NanColor> m_title_color{
            nandina::NanColor::from(nandina::NanRgb{220, 220, 240})
        };
        reactive::Prop<nandina::NanColor> m_accent_color{
            nandina::NanColor::from(nandina::NanRgb{99, 102, 241})
        };
        reactive::Prop<float> m_elevation{0.0f};
        reactive::Prop<float> m_title_font_size{12.0f};
        text::NanFontWeight m_title_font_weight{text::NanFontWeight::semiBold};

        std::string m_title;
        std::string m_description;
        bool m_show_accent{false};
        layout::Padding *m_title_host{nullptr};
        Text *m_title_label{nullptr};
        runtime::NanWidget *m_footer{nullptr};
    };
} // namespace nandina::widgets
