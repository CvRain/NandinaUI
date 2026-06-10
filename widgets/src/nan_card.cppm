//
// Created by cvrain on 2026/4/29.
//

module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <thorvg-1/thorvg.h>
#include <utility>

export module nandina.widgets.card;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_size;
import nandina.foundation.nan_rect;
import nandina.foundation.color;
import nandina.layout.container;
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
export namespace nandina::widgets
{
    enum class CardSize : std::uint8_t {
        default_size,
        sm,
    };

    class CardHeaderLayout final: public runtime::NanWidget {
    public:
        static auto Create() -> std::unique_ptr<CardHeaderLayout> {
            return std::unique_ptr<CardHeaderLayout>(new CardHeaderLayout());
        }

        auto set_text_widget(runtime::NanWidget::Ptr widget) -> runtime::NanWidget* {
            m_text = add_child(std::move(widget));
            return m_text;
        }

        auto set_action_slot(runtime::NanWidget::Ptr widget) -> runtime::NanWidget* {
            m_action_slot = add_child(std::move(widget));
            return m_action_slot;
        }

        auto set_section_gap(const float gap) -> CardHeaderLayout& {
            m_gap = gap;
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto text =
                m_text && m_text->visible() ? m_text->preferred_size() : geometry::NanSize {};
            const auto action = m_action_slot && m_action_slot->visible()
                ? m_action_slot->preferred_size()
                : geometry::NanSize {};

                if (text.height() <= 0.0f) {
                    return action;
                }
                if (action.height() <= 0.0f) {
                    return text;
                }

            return geometry::NanSize {
                text.width() + m_gap + action.width(),
                std::max(text.height(), action.height())
            };
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const bool has_text = m_text && m_text->visible();
            const bool has_action = m_action_slot && m_action_slot->visible();
            const float max_width = constraints.max_width();

            geometry::NanSize text_size {};
            geometry::NanSize action_size {};

                if (has_action) {
                    m_action_slot->measure(
                        geometry::NanConstraints {
                            0.0f,
                            max_width,
                            0.0f,
                            geometry::NanConstraints::k_infinity
                        }
                    );
                    action_size = measured_or_preferred(*m_action_slot);
                }

                if (has_text) {
                    const bool stack_action = should_stack(max_width, action_size.width());
                    m_stack_action = has_action && stack_action;

                    const float text_max_width =
                        (!has_action || m_stack_action
                         || max_width == geometry::NanConstraints::k_infinity)
                        ? max_width
                        : std::max(0.0f, max_width - action_size.width() - m_gap);

                    m_text->measure(
                        geometry::NanConstraints {
                            0.0f,
                            text_max_width,
                            0.0f,
                            geometry::NanConstraints::k_infinity
                        }
                    );
                    text_size = measured_or_preferred(*m_text);
                }
                else {
                    m_stack_action = false;
                }

            geometry::NanSize measured {};
                if (has_text && has_action) {
                        if (m_stack_action) {
                            measured = geometry::NanSize {
                                std::max(text_size.width(), action_size.width()),
                                text_size.height() + m_gap + action_size.height()
                            };
                        }
                        else {
                            measured = geometry::NanSize {
                                text_size.width() + m_gap + action_size.width(),
                                std::max(text_size.height(), action_size.height())
                            };
                        }
                }
                else if (has_text) {
                    measured = text_size;
                }
                else if (has_action) {
                    measured = action_size;
                }

            set_measured_layout_state(constraints, constraints.constrain(measured));
        }

        auto layout() -> void override {
            const bool has_text = m_text && m_text->visible();
            const bool has_action = m_action_slot && m_action_slot->visible();

                if (has_text) {
                    const auto text_size = measured_or_preferred(*m_text);
                        if (has_action && m_stack_action) {
                            const float text_h = std::min(text_size.height(), height());
                            m_text->set_bounds(x(), y(), width(), text_h);
                            m_text->layout();
                        }
                        else {
                            const float action_w = has_action
                                ? std::min(measured_or_preferred(*m_action_slot).width(), width())
                                : 0.0f;
                            const float text_w =
                                has_action ? std::max(0.0f, width() - action_w - m_gap) : width();
                            m_text->set_bounds(x(), y(), text_w, height());
                            m_text->layout();
                        }
                }

                if (has_action) {
                    const auto action_size = measured_or_preferred(*m_action_slot);
                    const float action_w = std::min(action_size.width(), width());
                    const float action_h = std::min(action_size.height(), height());
                    const float action_x = x() + std::max(0.0f, width() - action_w);
                    const float action_y = has_text && m_stack_action
                        ? y() + measured_or_preferred(*m_text).height() + m_gap
                        : y();
                    m_action_slot->set_bounds(action_x, action_y, action_w, action_h);
                    m_action_slot->layout();
                }

            clear_layout_dirty();
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept
            -> runtime::NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);
            return *this;
        }

    private:
        CardHeaderLayout() noexcept = default;

        [[nodiscard]] static auto measured_or_preferred(const runtime::NanWidget& widget) noexcept
            -> geometry::NanSize {
            const auto measured = widget.measured_size();
            const auto preferred = widget.preferred_size();
            return geometry::NanSize {
                measured.width() > 0.0f ? measured.width() : preferred.width(),
                measured.height() > 0.0f ? measured.height() : preferred.height()
            };
        }

        [[nodiscard]] auto
        should_stack(const float max_width, const float action_width) const noexcept -> bool {
                if (max_width == geometry::NanConstraints::k_infinity || action_width <= 0.0f) {
                    return false;
                }

            constexpr float min_inline_header_width = 280.0f;
            constexpr float min_inline_text_width = 168.0f;
            return max_width < min_inline_header_width
                || (max_width - action_width - m_gap) < min_inline_text_width
                || action_width > max_width * 0.4f;
        }

        runtime::NanWidget* m_text {nullptr};
        runtime::NanWidget* m_action_slot {nullptr};
        float m_gap {8.0f};
        bool m_stack_action {false};
    };

    class Card: public Surface {
    public:
        using Ptr = std::unique_ptr<Card>;

        ~Card() override = default;

        static auto create() -> Ptr {
            return Ptr {new Card()};
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

        auto set_size(const CardSize size) -> Card& {
            m_size = size;
            apply_size_style();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto size() const noexcept -> CardSize {
            return m_size;
        }

        auto set_card_spacing(const float spacing) -> Card& {
            m_card_spacing.set(std::max(8.0f, spacing));
            m_spacing_overridden = true;
            sync_title_host_style();
            sync_footer_style();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto card_spacing() const noexcept -> float {
            return m_card_spacing.get();
        }

        // ── 从 Surface override 方法（返回 Card& 以保持链式） ──
        auto set_bg_color(const nandina::NanColor& color) -> Card& override {
            Surface::set_bg_color(color);
            return *this;
        }

        auto set_corner_radius(float radius) -> Card& override {
            Surface::set_corner_radius(radius);
            return *this;
        }

        auto set_padding(const geometry::NanInsets& insets) -> Card& override {
            Surface::set_padding(insets);
            return *this;
        }

        auto set_border_color(const nandina::NanColor& color) -> Card& override {
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
                if ((!m_title.empty() || !m_description.empty()) && !m_title_host) {
                    ensure_title_label();
                }
                if (m_title_label) {
                    m_title_label->set_text(m_title);
                    m_title_label->set_visible(!m_title.empty());
                }
            sync_header_visibility();
            mark_layout_dirty();
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
                if (m_description_label) {
                    m_description_label->set_font_size(std::max(11.0f, size - 1.0f));
                }
            sync_title_host_style();
            mark_layout_dirty();
            return *this;
        }

        auto set_title_font_weight(text::NanFontWeight weight) -> Card& {
            m_title_font_weight = weight;
                if (m_title_label) {
                    m_title_label->set_font_weight(weight);
                }
            mark_layout_dirty();
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
            sync_title_host_style();
            mark_layout_dirty();
            return *this;
        }

        // ── 可选描述文本（标题下方副标题） ──────────────
        auto set_description(std::string desc) -> Card& {
            m_description = std::move(desc);
                if ((!m_title.empty() || !m_description.empty()) && !m_title_host) {
                    ensure_title_label();
                }
                if (m_description_label) {
                    m_description_label->set_text(m_description);
                    m_description_label->set_visible(!m_description.empty());
                }
            sync_header_visibility();
            mark_layout_dirty();
            return *this;
        }

        auto set_description_color(const nandina::NanColor& color) -> Card& {
            m_description_color.set(color);
                if (m_description_label) {
                    m_description_label->set_color(color);
                }
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto description_color() const noexcept -> const nandina::NanColor& {
            return m_description_color.get();
        }

        [[nodiscard]] auto description() const noexcept -> const std::string& {
            return m_description;
        }

        // ── 可选 Header Action 区域 ───────────────────────
        auto set_header_action(runtime::NanWidget::Ptr action) -> Card& {
            m_has_header_action = action != nullptr;
            ensure_title_label();
                if (!m_header_action_slot) {
                    return *this;
                }

                if (m_header_action) {
                    m_header_action->set_visible(false);
                    m_header_action->set_hit_test_visible(false);
                }

            m_header_action_slot->clear_children();
            m_header_action = nullptr;

                if (action) {
                    m_header_action = m_header_action_slot->add_child(std::move(action));
                    m_header_action_slot->set_visible(true);
                }
                else {
                    m_header_action_slot->set_visible(false);
                }

            sync_header_visibility();
            sync_title_host_style();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto header_action() const noexcept -> runtime::NanWidget* {
            return m_header_action;
        }

        // ── 可选 Footer 区域 ────────────────────────────
        auto set_footer(runtime::NanWidget::Ptr footer) -> Card& {
                if (m_footer) {
                    m_footer->set_visible(false);
                    m_footer->set_hit_test_visible(false);
                }
            auto wrapper = layout::Padding::Create();
            wrapper->padding(
                resolved_section_spacing(),
                resolved_section_spacing(),
                padding().right(),
                resolved_section_spacing()
            );
            wrapper->child(std::move(footer));
            m_footer = NanWidget::add_child(std::move(wrapper));
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto footer() const noexcept -> runtime::NanWidget* {
            return m_footer;
        }

        // ── 布局 ──────────────────────────────────────────
        auto set_bounds(const float x, const float y, const float w, const float h) noexcept
            -> NanWidget& override {
            runtime::NanWidget::set_bounds(x, y, w, h);
            return *this;
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            const auto& pad = padding();

            const float content_avail_w =
                constraints.max_width() == geometry::NanConstraints::k_infinity
                ? geometry::NanConstraints::k_infinity
                : std::max(0.0f, constraints.max_width() - pad.left() - pad.right());

                if (m_title_host) {
                    m_title_host->measure(
                        geometry::NanConstraints {
                            0.0f,
                            constraints.max_width(),
                            0.0f,
                            geometry::NanConstraints::k_infinity
                        }
                    );
                }

            const float header_h = title_header_height();
            const float footer_h = footer_height();

            // 用 preferred_size() 获取 body 子节点的自然宽度，避免一次完整 measure
            // 遍历。Text::preferred_size() 现在与 measure(unconstrained) 结果一致。
            float content_max_w = content_avail_w;
                if (content_avail_w != geometry::NanConstraints::k_infinity
                    && content_avail_w > 0.0f)
                {
                    float max_natural_w = 0.0f;
                    for_each_child([&](const runtime::NanWidget& child) {
                        if (!child.visible() || &child == m_title_host || &child == m_footer)
                            return;
                        const auto p = child.preferred_size();
                            if (p.width() > max_natural_w) {
                                max_natural_w = p.width();
                            }
                    });
                        if (max_natural_w > 8.0f && max_natural_w < content_max_w) {
                            content_max_w = max_natural_w;
                        }
                }

            geometry::NanSize child_measured {0.0f, 0.0f};
            for_each_child([&](runtime::NanWidget& child) {
                if (!child.visible() || &child == m_title_host || &child == m_footer)
                    return;

                const geometry::NanConstraints cc {
                    std::max(0.0f, constraints.min_width() - pad.left() - pad.right()),
                    std::max(
                        content_max_w,
                        std::max(0.0f, constraints.min_width() - pad.left() - pad.right())
                    ),
                    std::max(
                        0.0f,
                        constraints.min_height() - pad.top() - pad.bottom() - header_h - footer_h
                    ),
                    constraints.max_height() == geometry::NanConstraints::k_infinity
                        ? geometry::NanConstraints::k_infinity
                        : std::max(
                              0.0f,
                              constraints.max_height() - pad.top() - pad.bottom() - header_h
                                  - footer_h
                          ),
                };

                child.measure(
                    geometry::NanConstraints {
                        cc.min_width(),
                        cc.max_width(),
                        0.0f,
                        geometry::NanConstraints::k_infinity
                    }
                );
                const auto m = child.measured_size();
                const auto p = child.preferred_size();
                child_measured = geometry::NanSize {
                    std::max(child_measured.width(), m.width() > 0.0f ? m.width() : p.width()),
                    std::max(child_measured.height(), m.height() > 0.0f ? m.height() : p.height())
                };
            });

                if (m_footer) {
                    const geometry::NanConstraints fc {
                        std::max(0.0f, constraints.min_width() - pad.left() - pad.right()),
                        content_avail_w,
                        0.0f,
                        geometry::NanConstraints::k_infinity,
                    };
                    m_footer->measure(
                        geometry::NanConstraints {
                            fc.min_width(),
                            fc.max_width(),
                            0.0f,
                            geometry::NanConstraints::k_infinity
                        }
                    );
                }

            set_measured_layout_state(
                constraints,
                constraints.constrain(
                    geometry::NanSize {
                        child_measured.width() + pad.left() + pad.right(),
                        child_measured.height() + header_h + footer_h + pad.top() + pad.bottom()
                    }
                )
            );
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

            const float footer_y = (m_footer && footer_h > 0.0f) ? y() + height() - footer_h
                                                                 : y() + height() - pad.bottom();

            // Content 区域（body 子节点，位于 header 和 footer 之间）
            {
                const float content_y = y() + header_h + pad.top();
                const float content_bottom = std::max(content_y, footer_y);
                const float content_h = std::max(0.0f, content_bottom - content_y);
                for_each_child([&](runtime::NanWidget& child) {
                    if (!child.visible() || &child == m_title_host || &child == m_footer)
                        return;
                    child.set_bounds(
                        x() + pad.left(),
                        content_y,
                        width() - pad.left() - pad.right(),
                        content_h
                    );
                    child.layout();
                });
            }

                // Footer 区域 — 底部对齐
                if (m_footer && footer_h > 0.0f) {
                    m_footer->set_bounds(
                        x() + pad.left(),
                        footer_y,
                        width() - pad.left() - pad.right(),
                        footer_h
                    );
                    m_footer->layout();
                }

            clear_layout_dirty();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            geometry::NanSize child_pref {0.0f, 0.0f};
            for_each_child([&](const runtime::NanWidget& child) {
                if (!child.visible() || &child == m_title_host || &child == m_footer) {
                    return;
                }
            const auto cp = child.preferred_size();
            child_pref = geometry::NanSize {
                std::max(child_pref.width(), cp.width()),
                std::max(child_pref.height(), cp.height())
            };
            });
            const auto& pad = padding();
            const float header_h = title_header_height();
            const float footer_h = footer_height();

            return geometry::NanSize {
                child_pref.width() + pad.left() + pad.right(),
                std::max(
                    child_pref.height() + header_h + footer_h + pad.top() + pad.bottom(),
                    header_h + footer_h + pad.top() + pad.bottom()
                )
            };
        }

        // ── 绘制覆盖 ──────────────────────────────────────────
        // 手动管理 body 子节点的 clip scope：header 和 footer 不受 clip 影响，
        // 而 body 子节点在 clip stack 保护下绘制，避免越界侵入相邻 section。
        void draw(tvg::SwCanvas& canvas) override {
                if (!visible()) {
                    return;
                }

            // 1. 绘制卡面自身（阴影、背景、装饰色条、分隔线）
            on_draw(canvas);

                // 2. Header 区域 — 无 clip
                if (m_title_host && m_title_host->visible()) {
                    m_title_host->draw(canvas);
                }

            // 3. Body 区域 — 带 clip
            {
                const auto& pad = padding();
                const float header_h = title_header_height();
                float footer_h = 0.0f;
                    if (m_footer) {
                        const auto ms = m_footer->measured_size();
                        const auto ps = m_footer->preferred_size();
                        footer_h = std::max(ms.height() > 0.0f ? ms.height() : ps.height(), 0.0f);
                    }
                const float footer_y = (m_footer && footer_h > 0.0f)
                    ? y() + height() - footer_h
                    : y() + height() - pad.bottom();
                const float content_y = y() + header_h + pad.top();
                const float content_bottom = std::max(content_y, footer_y);
                const float content_h = std::max(0.0f, content_bottom - content_y);

                auto clip_guard = nandina::runtime::ScopedDrawClip {
                    geometry::NanRect {
                        geometry::NanPoint {x() + pad.left(), content_y},
                        geometry::NanSize {width() - pad.left() - pad.right(), content_h}
                    },
                    m_corner_radius.get(),
                    m_corner_radius.get()
                };

                for_each_child([&](runtime::NanWidget& child) {
                    if (!child.visible() || &child == m_title_host || &child == m_footer) {
                        return;
                    }
                child.draw(canvas);
                });
            }

                // 4. Footer 区域 — 无 clip
                if (m_footer && m_footer->visible()) {
                    m_footer->draw(canvas);
                }
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
                            const uint8_t alpha = static_cast<uint8_t>(
                                30.0f
                                * (1.0f - static_cast<float>(i) / static_cast<float>(shadow_layers))
                            );

                            auto* shadow = tvg::Shape::gen();
                            shadow->appendRect(
                                rect.x() + offset,
                                rect.y() + offset + blur * 0.3f,
                                rect.width() - offset * 2.0f,
                                rect.height() - offset * 2.0f - blur * 0.3f,
                                radius,
                                radius
                            );
                            shadow->fill(0, 0, 0, alpha);
                            nandina::runtime::paint_with_draw_clip(canvas, shadow);
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
                                1.5f,
                                1.5f
                            );
                            accent_bar->fill(
                                acc_rgb.red(),
                                acc_rgb.green(),
                                acc_rgb.blue(),
                                acc_rgb.alpha()
                            );
                            nandina::runtime::paint_with_draw_clip(canvas, accent_bar);
                        }
                }

                // ── 4. 标题栏与内容区分隔线 ──────────────────
                if (header_off > 0.0f
                    && (!m_title.empty() || !m_description.empty() || m_header_action != nullptr))
                {
                    auto* divider = tvg::Shape::gen();
                    const float div_y = rect.y() + header_off;
                    divider->moveTo(rect.x() + 4.0f, div_y);
                    divider->lineTo(rect.x() + rect.width() - 4.0f, div_y);
                    divider->strokeWidth(1.0f);
                    divider->strokeFill(55, 57, 75, 150);
                    nandina::runtime::paint_with_draw_clip(canvas, divider);
                }

                // ── 5. Footer 分隔线 ──────────────────────────
                if (m_footer) {
                    float fh = 0.0f;
                    const auto ms = m_footer->measured_size();
                    const auto ps = m_footer->preferred_size();
                    fh = std::max(ms.height() > 0.0f ? ms.height() : ps.height(), 0.0f);
                        if (fh > 0.0f) {
                            auto* divider = tvg::Shape::gen();
                            const float div_y = rect.y() + rect.height() - fh;
                            divider->moveTo(rect.x() + 16.0f, div_y);
                            divider->lineTo(rect.x() + rect.width() - 16.0f, div_y);
                            divider->strokeWidth(1.0f);
                            divider->strokeFill(55, 57, 75, 150);
                            nandina::runtime::paint_with_draw_clip(canvas, divider);
                        }
                }

            // 描边已由 draw_background() 处理
        }

    private:
        Card(): Surface() {
            // 从 theme token 读取默认样式（背景/圆角/边框/内边距/标题）
            const auto& style = nandina::theme::NanStylePrimitives::current().card;
            m_bg_color.set(style.bg);
            m_corner_radius.set(style.corner_radius);
            m_border_color = style.border;
            m_border_width = style.border_width;
            m_title_color.set(style.title_font_color);
            m_title_font_size.set(style.title_font_size);
            m_card_spacing.set(style.section_spacing);
            apply_size_style();
        }

        auto ensure_title_label() -> void {
            if (m_title_host || (m_title.empty() && m_description.empty() && !m_has_header_action))
                return;

            auto title_host = layout::Padding::Create();
            m_title_host = title_host.get();

            auto header_layout = CardHeaderLayout::Create();
            m_header_layout = header_layout.get();

            auto header_column = layout::Column::Create();
            header_column->gap(4.0f).align_items(layout::LayoutAlignment::stretch);
            m_header_column = header_column.get();

            auto title_label = Text::create();
            title_label->set_text(m_title)
                .set_typography_role(theme::NanTypographyRole::title_medium)
                .set_font_size(m_title_font_size.get())
                .set_font_weight(m_title_font_weight)
                .set_color(m_title_color.get())
                .set_wrap_policy(text::TextWrapPolicy::break_word)
                .set_single_line(false);
            title_label->set_visible(!m_title.empty());
            m_title_label = title_label.get();
            header_column->add(std::move(title_label));

            auto description_label = Text::create();
            description_label->set_text(m_description)
                .set_typography_role(theme::NanTypographyRole::body_small)
                .set_font_size(std::max(11.0f, m_title_font_size.get() - 1.0f))
                .set_font_weight(text::NanFontWeight::regular)
                .set_color(m_description_color.get())
                .set_wrap_policy(text::TextWrapPolicy::break_word)
                .set_single_line(false);
            description_label->set_visible(!m_description.empty());
            m_description_label = description_label.get();
            header_column->add(std::move(description_label));

            header_layout->set_text_widget(std::move(header_column));

            auto header_action_slot = layout::SizedBox::Create();
            header_action_slot->set_visible(false);
            m_header_action_slot = header_action_slot.get();
            header_layout->set_action_slot(std::move(header_action_slot));

            title_host->child(std::move(header_layout));
            sync_title_host_style();
            sync_header_visibility();
            NanWidget::add_child(std::move(title_host));
        }

        auto sync_title_host_style() -> void {
                if (!m_title_host) {
                    return;
                }

            const float left = m_show_accent ? 12.0f : 8.0f;
            const float right = m_show_accent ? 12.0f : padding().right();
            const float vertical = std::max(6.0f, resolved_section_spacing() * 0.5f);
            m_title_host->padding(left, vertical, right, vertical);
                if (m_header_column) {
                    m_header_column->gap(
                        m_title.empty() || m_description.empty()
                            ? 0.0f
                            : std::max(4.0f, resolved_section_spacing() * 0.25f)
                    );
                }
                if (m_header_layout) {
                    m_header_layout->set_section_gap(
                        m_header_action ? std::max(8.0f, resolved_section_spacing() * 0.5f) : 0.0f
                    );
                }
        }

        auto sync_header_visibility() -> void {
                if (!m_title_host) {
                    return;
                }
            const bool has_header_text = !m_title.empty() || !m_description.empty();
            const bool has_header = has_header_text || m_header_action != nullptr;
            m_title_host->set_visible(has_header);
                if (m_header_column) {
                    m_header_column->set_visible(has_header_text);
                }
                if (m_header_action_slot) {
                    m_header_action_slot->set_visible(m_header_action != nullptr);
                }
        }

        auto sync_footer_style() -> void {
                if (auto* footer_padding = dynamic_cast<layout::Padding*>(m_footer)) {
                    footer_padding->padding(
                        resolved_section_spacing(),
                        resolved_section_spacing(),
                        padding().right(),
                        resolved_section_spacing()
                    );
                }
        }

        auto apply_size_style() -> void {
            const auto& style = nandina::theme::NanStylePrimitives::current().card;
                switch (m_size) {
                    case CardSize::default_size:
                        Surface::set_padding(style.padding);
                        m_title_font_size.set(style.title_font_size);
                            if (!m_spacing_overridden) {
                                m_card_spacing.set(style.section_spacing);
                            }
                        break;
                    case CardSize::sm:
                        Surface::set_padding(style.padding_sm);
                        m_title_font_size.set(style.title_font_size_sm);
                            if (!m_spacing_overridden) {
                                m_card_spacing.set(style.section_spacing_sm);
                            }
                        break;
                }

                if (m_title_label) {
                    m_title_label->set_font_size(m_title_font_size.get());
                }
                if (m_description_label) {
                    m_description_label->set_font_size(
                        std::max(11.0f, m_title_font_size.get() - 1.0f)
                    );
                }
            sync_title_host_style();
            sync_footer_style();
        }

        [[nodiscard]] auto resolved_section_spacing() const noexcept -> float {
            return m_card_spacing.get();
        }

        [[nodiscard]] auto title_header_height() const noexcept -> float {
                if (!m_title_host) {
                    return 0.0f;
                }
            const auto measured = m_title_host->measured_size();
                if (measured.height() > 0.0f) {
                    return measured.height();
                }
            const auto pref = m_title_host->preferred_size();
            return pref.height();
        }

        [[nodiscard]] auto footer_height() const noexcept -> float {
            if (!m_footer)
                return 0.0f;
            const auto fp = m_footer->preferred_size();
            return fp.height() > 0.0f ? fp.height() : 36.0f;
        }

        // Card 独有的成员（bg_color, corner_radius, padding, border 从 Surface 继承）
        reactive::Prop<nandina::NanColor> m_title_color {
            nandina::NanColor::from(nandina::NanRgb {220, 220, 240})
        };
        reactive::Prop<nandina::NanColor> m_description_color {
            nandina::NanColor::from(nandina::NanRgb {107, 114, 128})
        };
        reactive::Prop<nandina::NanColor> m_accent_color {
            nandina::NanColor::from(nandina::NanRgb {99, 102, 241})
        };
        reactive::Prop<float> m_elevation {0.0f};
        reactive::Prop<float> m_title_font_size {12.0f};
        reactive::Prop<float> m_card_spacing {16.0f};
        text::NanFontWeight m_title_font_weight {text::NanFontWeight::semiBold};
        CardSize m_size {CardSize::default_size};
        bool m_spacing_overridden {false};

        std::string m_title;
        std::string m_description;
        bool m_show_accent {false};
        bool m_has_header_action {false};
        layout::Padding* m_title_host {nullptr};
        CardHeaderLayout* m_header_layout {nullptr};
        layout::Column* m_header_column {nullptr};
        layout::SizedBox* m_header_action_slot {nullptr};
        Text* m_title_label {nullptr};
        Text* m_description_label {nullptr};
        runtime::NanWidget* m_header_action {nullptr};
        runtime::NanWidget* m_footer {nullptr};
    };
} // namespace nandina::widgets
