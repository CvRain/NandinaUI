module;

#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>
#include <thorvg-1/thorvg.h>

export module nandina.showcase;

import nandina.app.application;
import nandina.log;
import nandina.foundation.color;
import nandina.foundation.nan_point;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.nan_insets;
import nandina.layout.core;
import nandina.widgets;
import nandina.runtime.nan_widget;
import nandina.reactive.animation;

// ── 辅助: 绘制圆角矩形 ──────────────────────────────────
static void draw_rect(tvg::SwCanvas& canvas,
    const float x, const float y, const float w, const float h,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
    const float corner = 4.0f) {
    auto* shape = tvg::Shape::gen();
    shape->appendRect(x, y, w, h, corner, corner);
    shape->fill(r, g, b, a);
    canvas.add(shape);
}

// ── 辅助: 绘制圆 ──────────────────────────────────────
static void draw_circle(tvg::SwCanvas& canvas,
    const float cx, const float cy, const float radius,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) {
    auto* shape = tvg::Shape::gen();
    shape->appendCircle(cx, cy, radius, radius);
    shape->fill(r, g, b, a);
    canvas.add(shape);
}

// ── 辅助: 路径 ────────────────────────────────────────
static void draw_line(tvg::SwCanvas& canvas,
    const float x1, const float y1, const float x2, const float y2,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
    const float width = 1.0f) {
    auto* shape = tvg::Shape::gen();
    shape->moveTo(x1, y1);
    shape->lineTo(x2, y2);
    shape->strokeWidth(width);
    shape->strokeFill(r, g, b, a);
    canvas.add(shape);
}

// ── 辅助: 绘制文字（用圆点模拟像素字）───────────────────
static void draw_text_dots(tvg::SwCanvas& canvas,
    float x, float y,
    const std::string_view text,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
    const float size = 10.0f) {
    const float dot_r   = size * 0.25f;
    const float spacing = size * 0.8f;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == ' ')
            continue;
        draw_circle(canvas, x + i * spacing, y, dot_r, r, g, b, a);
    }
}

// ── 辅助: 绘制图标（小方块图标）────────────────────────
static void draw_icon_rect(tvg::SwCanvas& canvas,
    const float cx, const float cy, const float size,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) {
    draw_rect(canvas, cx - size * 0.5f, cy - size * 0.5f, size, size, r, g, b, a, 2);
}

static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}

// ═══════════════════════════════════════════════════════════
//  MainComponent — 完整的 UI 布局 showcase
// ═══════════════════════════════════════════════════════════
export class MainComponent final : public nandina::app::NanComponent {
public:
    explicit MainComponent()
        : bg_color(nandina::NanColor::from(nandina::NanRgb{30, 30, 46})),
          sidebar_bg(42, 44, 62, 255),
          dock_bg(38, 40, 56, 255),
          card_bg(50, 52, 72, 255),
          accent(99, 102, 241),
          accent2(147, 150, 255),
          green(95, 200, 130),
          orange(245, 158, 60),
          pink(236, 110, 130),
          cyan(80, 200, 220),
          text_primary(220, 220, 240, 255),
          text_secondary(160, 162, 180, 255),
          text_dim(110, 112, 130, 255) {
        build_widget_tree();
    }

protected:
    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);
        m_layout_valid = false;
        return *this;
    }

    auto on_draw(tvg::SwCanvas& canvas) -> void override {
        const float w = width();
        const float h = height();

        if (!m_layout_valid || m_last_layout_w != w || m_last_layout_h != h) {
            layout_widgets(w, h);
            m_last_layout_w = w;
            m_last_layout_h = h;
            m_layout_valid  = true;
        }

        // ─ 1. 背景 ────────────────────────────────────
        const auto& _bg = bg_color.to<nandina::NanRgb>();
        draw_rect(canvas, 0, 0, w, h, _bg.red(), _bg.green(), _bg.blue(), 255);

        // ─ 2. 侧边栏 — 左侧导航 + 项目列表 ──────────
        constexpr float sidebar_w = 240.0f;
        draw_sidebar(canvas, sidebar_w, h);

        // ─ 3. 底部 Dock 栏 ────────────────────────────
        constexpr float dock_h = 56.0f;
        draw_dock(canvas, sidebar_w, h - dock_h, w - sidebar_w, dock_h);

        // ─ 4. 主内容区域（仅保留 ThorVG 无法替换的部分）──
        draw_content_bg_and_chart(canvas, sidebar_w, 0.0f, w - sidebar_w, h - dock_h);

        // ─ 5. 动画更新 ──────────────────────────────
        const auto now    = std::chrono::steady_clock::now();
        const float delta = std::chrono::duration<float>(now - m_last_frame_time).count();
        m_last_frame_time = now;
        m_anim_manager.update(delta);
    }

private:
    // ── 构建 Widget 子树 ──────────────────────────────
    auto build_widget_tree() -> void {
        using namespace nandina::widgets;

        // ─ 4 张统计卡片 ──
        const auto card_title_texts = std::to_array<
            std::string_view>({"Total Users", "Active Now", "Revenue", "Tasks"});
        const auto card_value_texts = std::to_array<std::string_view>({"2,847", "143", "$12.4k", "18/24"});
        constexpr auto card_colors  = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
            {99, 102, 241},
            {95, 200, 130},
            {245, 158, 60},
            {236, 110, 130},
        });

        for (size_t i = 0; i < 4; ++i) {
            const auto& [cr, cg, cb] = card_colors[i];

            auto card = Card::create();
            card->set_bg_color(color4_to_nancolor(card_bg.r, card_bg.g, card_bg.b))
                .set_corner_radius(8.0f)
                .set_title(std::string{card_title_texts[i]})
                .set_title_color(color4_to_nancolor(text_dim.r, text_dim.g, text_dim.b))
                .set_show_accent(true)
                .set_accent_color(color4_to_nancolor(cr, cg, cb));

            auto val = Label::create();
            val->set_text(std::string{card_value_texts[i]})
                .set_font_size(16.0f)
                .set_color(color4_to_nancolor(text_primary.r, text_primary.g, text_primary.b));
            card->add_child(std::move(val));

            m_stat_cards[i] = add_child(std::move(card));
        }

        // ─ 顶部 Header 标题 ──
        auto hdr_title = Label::create();
        hdr_title->set_text("Dashboard / Overview")
            .set_font_size(10.0f)
            .set_color(color4_to_nancolor(text_secondary.r, text_secondary.g, text_secondary.b));
        m_header_title = add_child(std::move(hdr_title));

        // ─ "+" 按钮 ──
        {
            nandina::widgets::ButtonColors btn_colors;
            btn_colors.bg            = color4_to_nancolor(accent.r, accent.g, accent.b);
            btn_colors.bg_hover      = color4_to_nancolor(accent2.r, accent2.g, accent2.b);
            btn_colors.bg_pressed    = color4_to_nancolor(80, 82, 200);
            btn_colors.corner_radius = 6.0f;
            btn_colors.padding       = nandina::geometry::NanInsets{10.0f, 6.0f, 10.0f, 6.0f};

            auto btn = Button::create();
            btn->set_text("+")
                .set_colors(btn_colors);

            btn->on_click([this]() {
                m_log.info("button click");
            });

            m_add_button = add_child(std::move(btn));
        }

        // ─ Dock 时钟 ──
        auto clk = Label::create();
        clk->set_text("10:42")
            .set_font_size(9.0f)
            .set_color(color4_to_nancolor(text_secondary.r, text_secondary.g, text_secondary.b));
        m_clock = add_child(std::move(clk));

        // ─ 底部分隔区文字 ──
        auto footer = Label::create();
        footer->set_text("Built with NandinaUI · ThorVG rendering · C++26 modules")
            .set_font_size(7.0f)
            .set_color(color4_to_nancolor(text_dim.r, text_dim.g, text_dim.b));
        m_footer_text = add_child(std::move(footer));

        // ─ Stack 演示 ──
        {
            auto container = Surface::create();
            container->set_bg_color(color4_to_nancolor(card_bg.r, card_bg.g, card_bg.b))
                .set_corner_radius(8.0f);
            m_stack_container = add_child(std::move(container));

            auto st = Label::create();
            st->set_text("Stack Layout Demo")
                .set_font_size(9.0f)
                .set_color(color4_to_nancolor(text_primary.r, text_primary.g, text_primary.b));
            m_stack_title = add_child(std::move(st));

            const auto stack_box_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>({
                {99, 102, 241, 180},
                {147, 150, 255, 200},
                {200, 200, 255, 220},
            });
            for (size_t si = 0; si < 3; ++si) {
                const auto& [sr, sg, sb, sa] = stack_box_colors[si];
                auto box                     = Surface::create();
                box->set_bg_color(color4_to_nancolor(sr, sg, sb, sa))
                    .set_corner_radius(8.0f);
                m_stack_boxes[si] = add_child(std::move(box));
            }

            auto sf = Label::create();
            sf->set_text("Column · Row · Stack — all from BasicLayoutBackend")
                .set_font_size(6.0f)
                .set_color(color4_to_nancolor(text_dim.r, text_dim.g, text_dim.b));
            m_stack_footer = add_child(std::move(sf));
        }

        // ─ 4 行进度条 ──
        {
            const auto prog_data = std::to_array<std::tuple<std::string_view, float, uint8_t, uint8_t, uint8_t>>({
                {"Layout Engine", 0.85f, 99, 102, 241},
                {"Testing", 0.65f, 95, 200, 130},
                {"Documentation", 0.40f, 245, 158, 60},
                {"Performance", 0.30f, 236, 110, 130},
            });

            for (size_t pi = 0; pi < 4; ++pi) {
                const auto& [plabel, ppct, pr, pg, pb] = prog_data[pi];

                auto lbl = Label::create();
                lbl->set_text(plabel)
                    .set_font_size(7.0f)
                    .set_color(color4_to_nancolor(text_secondary.r, text_secondary.g, text_secondary.b));
                m_progress_rows[pi].label = add_child(std::move(lbl));

                auto bar = ProgressBar::create();
                bar->set_progress(ppct)
                    .set_bar_color(color4_to_nancolor(pr, pg, pb))
                    .set_track_color(color4_to_nancolor(42, 44, 62))
                    .set_bar_height(6.0f)
                    .set_corner_radius(3.0f);
                m_progress_rows[pi].bar = add_child(std::move(bar));

                char pct_buf[8];
                const int len = std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", static_cast<int>(ppct * 100.0f));
                auto pct      = Label::create();
                pct->set_text(std::string_view{pct_buf, static_cast<size_t>(len)})
                    .set_font_size(7.0f)
                    .set_color(color4_to_nancolor(text_dim.r, text_dim.g, text_dim.b));
                m_progress_rows[pi].pct = add_child(std::move(pct));
            }
        }

        // ─ 统计卡片的 Pressable 交互层 ──
        {
            for (size_t i = 0; i < 4; ++i) {
                auto pressable = Pressable::create();
                pressable->on_click([this, i] {
                    m_log.info("Card {} clicked", i);
                });
                pressable->on_hover([this, i] {
                    m_log.debug("Card {} hovered", i);
                });
                m_stat_card_pressables[i] = add_child(std::move(pressable));
            }
        }

        // ─ "+" 按钮交互 ──
        if (m_add_button) {
            auto* btn = dynamic_cast<nandina::widgets::Button*>(m_add_button);
            if (btn) {
                btn->on_click([this, btn] {
                    m_click_count++;
                    m_log.info("➕ Add clicked ({} times)", m_click_count);
                    btn->set_text("+" + std::to_string(m_click_count));
                });
            }
        }

        // ─ 活动列表脉冲动画 ──
        {
            auto pulse_anim = nandina::reactive::NanAnimation::create(2.0f);
            pulse_anim->set_loop(true)
                .set_easing(nandina::reactive::Easing::SineInOut)
                .on_update([this](float t) {
                    m_activity_pulse = t;
                });
            m_anim_manager.add(std::move(pulse_anim));
        }
    }

    // ── 布局计算 ──────────────────────────────────────
    auto layout_widgets(const float w, const float h) -> void {
        using namespace nandina::layout;

        constexpr float sidebar_w   = 240.0f;
        constexpr float dock_h      = 56.0f;
        constexpr float header_h    = 44.0f;
        constexpr float card_gap    = 16.0f;
        constexpr float content_pad = 20.0f;

        const float content_x   = sidebar_w;
        const float content_w   = w - sidebar_w;
        const float content_top = header_h + 20.0f;

        // ─ 顶部 Header 标题 ──
        {
            const float tx = content_x + 20.0f;
            const float ty = 16.0f;
            if (m_header_title) {
                m_header_title->set_bounds(tx, ty, 200.0f, 20.0f);
            }
        }

        // ─ Header "+" 按钮 ──
        {
            const float bx = content_x + content_w - 38.0f;
            const float by = 6.0f;
            if (m_add_button) {
                m_add_button->set_bounds(bx, by, 32.0f, 32.0f);
            }
        }

        // ─ 4 张统计卡片 ──
        {
            BasicLayoutBackend backend;
            LayoutRequest row1;
            row1.axis             = LayoutAxis::row;
            row1.container_bounds = {content_x + content_pad, content_top,
                                     content_x + content_w - content_pad, content_top + 100.0f};
            row1.gap             = card_gap;
            row1.cross_alignment = LayoutAlignment::stretch;

            const float card1_w = (row1.container_bounds.width() - card_gap * 3.0f) / 4.0f;
            for (int i = 0; i < 4; ++i) {
                LayoutChildSpec child;
                child.preferred_size = {card1_w, 100.0f};
                child.flex_factor    = 1;
                row1.children.push_back(child);
            }

            auto frames = backend.compute(row1);
            for (size_t i = 0; i < frames.size() && i < m_stat_cards.size(); ++i) {
                const auto& f = frames[i];
                if (m_stat_cards[i]) {
                    m_stat_cards[i]->set_bounds(f.x(), f.y(), f.width(), f.height());
                }
            }
        }

        // ─ Dock 时钟 ──
        {
            const float cx = content_x + content_w - 60.0f;
            const float cy = h - dock_h + (dock_h * 0.5f - 4.0f);
            if (m_clock) {
                m_clock->set_bounds(cx, cy, 50.0f, 14.0f);
            }
        }

        // ─ 底部分隔区文字 ──
        {
            const float row2_h   = 200.0f;
            const float stack_h  = 110.0f;
            const float footer_y = content_top + 116.0f + row2_h + 20.0f + stack_h + 20.0f;
            if (m_footer_text) {
                m_footer_text->set_bounds(content_x + content_pad, footer_y + 12.0f,
                    content_w - content_pad * 2.0f, 14.0f);
            }
        }

        // ─ Stack 演示 ──
        {
            const float stack_y     = content_top + 116.0f + 200.0f + 20.0f;
            const float stack_box_h = 110.0f;

            BasicLayoutBackend backend;
            LayoutRequest req;
            req.axis             = LayoutAxis::row;
            req.container_bounds = {content_x + content_pad, stack_y,
                                    content_x + content_w - content_pad, stack_y + stack_box_h};
            req.gap             = card_gap;
            req.cross_alignment = LayoutAlignment::stretch;

            const float item_w = (content_w - content_pad - content_pad - card_gap) * 0.5f; {
                LayoutChildSpec child;
                child.preferred_size = {item_w, 0.0f};
                child.flex_factor    = 1;
                req.children.push_back(child);
            } {
                LayoutChildSpec child;
                child.preferred_size = {item_w, 0.0f};
                child.flex_factor    = 1;
                req.children.push_back(child);
            }

            auto frames = backend.compute(req);
            if (!frames.empty()) {
                const auto& f = frames[0];

                // Stack 容器背景
                if (m_stack_container) {
                    m_stack_container->set_bounds(f.x(), f.y(), f.width(), f.height());
                }

                // Stack 标题
                if (m_stack_title) {
                    m_stack_title->set_bounds(f.x() + 14.0f, f.y() + 14.0f, 200.0f, 16.0f);
                }

                // 3 个重叠方框 — 使用 Stack 布局
                {
                    BasicLayoutBackend stack_backend;
                    LayoutRequest stack_req;
                    stack_req.axis             = LayoutAxis::stack;
                    stack_req.container_bounds = {f.x() + 20.0f, f.y() + 30.0f,
                                                  f.x() + f.width() - 20.0f, f.y() + f.height() - 16.0f};
                    stack_req.cross_alignment = LayoutAlignment::center;
                    stack_req.main_alignment  = LayoutAlignment::center; {
                        LayoutChildSpec child;
                        child.preferred_size = {140.0f, 60.0f};
                        child.flex_factor    = 0;
                        stack_req.children.push_back(child);
                    } {
                        LayoutChildSpec child;
                        child.preferred_size = {90.0f, 40.0f};
                        child.flex_factor    = 0;
                        stack_req.children.push_back(child);
                    } {
                        LayoutChildSpec child;
                        child.preferred_size = {50.0f, 20.0f};
                        child.flex_factor    = 0;
                        stack_req.children.push_back(child);
                    }

                    auto stack_frames = stack_backend.compute(stack_req);
                    for (size_t si = 0; si < stack_frames.size() && si < m_stack_boxes.size(); ++si) {
                        const auto& sf = stack_frames[si];
                        if (m_stack_boxes[si]) {
                            m_stack_boxes[si]->set_bounds(sf.x(), sf.y(), sf.width(), sf.height());
                        }
                    }
                }

                // Stack footer 文字
                if (m_stack_footer) {
                    m_stack_footer->set_bounds(f.x() + 14.0f, f.y() + f.height() - 14.0f,
                        f.width() - 28.0f, 12.0f);
                }

                // ─ "Project Progress" 标题由 ThorVG 在 draw_content_bg_and_chart 中绘制 ──
            }
        }

        // ─ 进度条子节点布局 ──
        {
            BasicLayoutBackend backend;
            LayoutRequest req;
            req.axis             = LayoutAxis::row;
            req.container_bounds = {content_x + content_pad, content_top + 116.0f + 200.0f + 20.0f,
                                    content_x + content_w - content_pad,
                                    content_top + 116.0f + 200.0f + 20.0f + 110.0f};
            req.gap             = card_gap;
            req.cross_alignment = LayoutAlignment::stretch;

            const float item_w = (content_w - content_pad - content_pad - card_gap) * 0.5f; {
                LayoutChildSpec child;
                child.preferred_size = {item_w, 0.0f};
                child.flex_factor    = 1;
                req.children.push_back(child);
            } {
                LayoutChildSpec child;
                child.preferred_size = {item_w, 0.0f};
                child.flex_factor    = 1;
                req.children.push_back(child);
            }

            auto frames = backend.compute(req);
            if (frames.size() >= 2) {
                const auto& f      = frames[1]; // 右侧进度卡片
                const float bar_x  = f.x() + 16.0f;
                const float pbar_x = f.x() + f.width() * 0.5f;
                const float pbar_w = f.width() * 0.42f;

                for (size_t pi = 0; pi < m_progress_rows.size(); ++pi) {
                    const float py = f.y() + 34.0f + static_cast<float>(pi) * 18.0f;

                    if (m_progress_rows[pi].label) {
                        m_progress_rows[pi].label->set_bounds(bar_x, py, 100.0f, 10.0f);
                    }
                    if (m_progress_rows[pi].bar) {
                        m_progress_rows[pi].bar->set_bounds(pbar_x, py + 1.0f, pbar_w, 6.0f);
                    }
                    if (m_progress_rows[pi].pct) {
                        m_progress_rows[pi].pct->set_bounds(pbar_x + pbar_w + 8.0f, py, 36.0f, 10.0f);
                    }
                }
            }
        }

        // ─ 统计卡片 Pressable ──
        {
            BasicLayoutBackend backend;
            LayoutRequest row1;
            row1.axis             = LayoutAxis::row;
            row1.container_bounds = {content_x + content_pad, content_top,
                                     content_x + content_w - content_pad, content_top + 100.0f};
            row1.gap             = card_gap;
            row1.cross_alignment = LayoutAlignment::stretch;

            const float card1_w = (row1.container_bounds.width() - card_gap * 3.0f) / 4.0f;
            for (int i = 0; i < 4; ++i) {
                LayoutChildSpec child;
                child.preferred_size = {card1_w, 100.0f};
                child.flex_factor    = 1;
                row1.children.push_back(child);
            }

            auto frames = backend.compute(row1);
            for (size_t i = 0; i < frames.size() && i < m_stat_card_pressables.size(); ++i) {
                const auto& f = frames[i];
                if (m_stat_card_pressables[i]) {
                    m_stat_card_pressables[i]->set_bounds(f.x(), f.y(), f.width(), f.height());
                }
            }
        }
    }

    // ── 侧边栏 ──────────────────────────────────────
    void draw_sidebar(tvg::SwCanvas& canvas, float w, float h) const {
        draw_rect(canvas, 0, 0, w, h, sidebar_bg.r, sidebar_bg.g, sidebar_bg.b, sidebar_bg.a);

        // Logo 区域
        const float logo_y = 20.0f;
        draw_circle(canvas, 32, logo_y + 14, 14, accent.r, accent.g, accent.b, 230);
        draw_circle(canvas, 32, logo_y + 14, 6, 255, 255, 255, 200);
        draw_text_dots(canvas, 55, logo_y + 8, "Nandina Studio", text_primary.r, text_primary.g, text_primary.b, 200,
            11);
        draw_line(canvas, 20, logo_y + 34, w - 20, logo_y + 34, 55, 57, 75, 150);

        // 导航分组
        draw_rect(canvas, 14, 60, 4, 14, accent.r, accent.g, accent.b, 220, 2);
        draw_text_dots(canvas, 26, 63, "Navigation", text_secondary.r, text_secondary.g, text_secondary.b, 180, 9);

        const auto nav_items = std::to_array<std::pair<std::string_view, bool>>({
            {"Dashboard", true},
            {"Projects", false},
            {"Analytics", false},
            {"Settings", false},
        });

        float nav_y = 85.0f;
        for (size_t i = 0; i < nav_items.size(); ++i) {
            const auto& [label, active] = nav_items[i];
            const float item_h          = 36.0f;

            if (active) {
                draw_rect(canvas, 0, nav_y - 2, w, item_h + 4, 40, 42, 60, 200);
                draw_rect(canvas, 0, nav_y - 2, 3, item_h + 4, accent.r, accent.g, accent.b, 255);
            }

            const auto& icon_c = active ? accent : text_dim;
            draw_icon_rect(canvas, 22, nav_y + item_h * 0.5f, 10, icon_c.r, icon_c.g, icon_c.b, icon_c.a);

            const auto& tc = active ? text_primary : text_secondary;
            draw_text_dots(canvas, 42, nav_y + 13, label, tc.r, tc.g, tc.b, tc.a, 9);
            nav_y += item_h + 2;
        }

        // 项目列表
        const float proj_y = nav_y + 20.0f;
        draw_rect(canvas, 14, proj_y, 4, 14, green.r, green.g, green.b, 220, 2);
        draw_text_dots(canvas, 26, proj_y + 3, "Recent Projects", text_secondary.r, text_secondary.g, text_secondary.b,
            180, 9);

        struct Project {
            std::string_view name;
            uint8_t pr, pg, pb;
        };
        const auto projects = std::to_array<Project>({
            {"nandina-ui", 99, 102, 241},
            {"layout-core", 95, 200, 130},
            {"flex-box", 245, 158, 60},
            {"render-test", 236, 110, 130},
        });

        float proj_item_y = proj_y + 25.0f;
        for (const auto& proj : projects) {
            draw_circle(canvas, 22, proj_item_y + 9, 5, proj.pr, proj.pg, proj.pb, 220);
            draw_text_dots(canvas, 36, proj_item_y + 3, proj.name,
                text_secondary.r, text_secondary.g, text_secondary.b, 200, 9);
            proj_item_y += 24.0f;
        }

        // 底部用户信息
        const float user_y = h - 60.0f;
        draw_line(canvas, 12, user_y - 8, w - 12, user_y - 8, 55, 57, 75, 150);
        draw_circle(canvas, 28, user_y + 22, 16, accent2.r, accent2.g, accent2.b, 220);
        draw_circle(canvas, 28, user_y + 18, 6, 255, 255, 255, 200);
        draw_circle(canvas, 28, user_y + 30, 8, 255, 255, 255, 100);
        draw_text_dots(canvas, 52, user_y + 14, "CvRain", text_primary.r, text_primary.g, text_primary.b, 200, 9);
        draw_text_dots(canvas, 52, user_y + 28, "Developer", text_dim.r, text_dim.g, text_dim.b, 160, 7);
    }

    // ── Dock 栏 ──────────────────────────────────────
    void draw_dock(tvg::SwCanvas& canvas, float x, float y, float w, float h) const {
        draw_rect(canvas, x, y, w, h, dock_bg.r, dock_bg.g, dock_bg.b, dock_bg.a);
        draw_line(canvas, x, y, x + w, y, 55, 57, 75, 120);

        using namespace nandina::layout;
        BasicLayoutBackend backend;

        LayoutRequest req;
        req.axis             = LayoutAxis::row;
        req.container_bounds = {x + 16.0f, y, x + w - 16.0f, y + h};
        req.cross_alignment  = LayoutAlignment::center;
        req.main_alignment   = LayoutAlignment::center;
        req.gap              = 12.0f;

        constexpr size_t dock_count = 7;
        const float icon_size       = 32.0f;
        for (size_t i = 0; i < dock_count; ++i) {
            nandina::layout::LayoutChildSpec child;
            child.preferred_size = {icon_size, icon_size};
            child.flex_factor    = 0;
            req.children.push_back(child);
        }

        auto frames = backend.compute(req);

        const auto dock_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
            std::tuple{99, 102, 241},
            std::tuple{95, 200, 130},
            std::tuple{245, 158, 60},
            std::tuple{236, 110, 130},
            std::tuple{80, 200, 220},
            std::tuple{180, 140, 240},
            std::tuple{160, 162, 180},
        });

        for (size_t i = 0; i < frames.size() && i < dock_colors.size(); ++i) {
            const auto f             = frames[i];
            const auto& [cr, cg, cb] = dock_colors[i];

            draw_rect(canvas, f.x(), f.y(), f.width(), f.height(), cr, cg, cb, 200, 7);

            if (i == 0) {
                draw_circle(canvas, f.center().x(), f.center().y() - 3, 5, 255, 255, 255, 200);
                draw_circle(canvas, f.center().x() - 4, f.center().y() - 5, 1.5f, 255, 255, 255, 200);
                draw_circle(canvas, f.center().x() + 4, f.center().y() - 5, 1.5f, 255, 255, 255, 200);
            } else if (i == 2) {
                auto* icon     = tvg::Shape::gen();
                const float cx = f.center().x(), cy = f.center().y();
                const float s  = 6.0f;
                icon->moveTo(cx - s * 0.6f, cy - s);
                icon->lineTo(cx + s * 0.6f, cy);
                icon->lineTo(cx - s * 0.6f, cy + s);
                icon->close();
                icon->fill(255, 255, 255, 200);
                canvas.add(icon);
            } else if (i == 3) {
                draw_rect(canvas, f.center().x() - 7, f.center().y() - 5, 14, 10, 255, 255, 255, 200, 2);
            } else if (i == 4) {
                auto* check = tvg::Shape::gen();
                check->moveTo(f.center().x() - 6, f.center().y());
                check->lineTo(f.center().x() - 2, f.center().y() + 5);
                check->lineTo(f.center().x() + 6, f.center().y() - 4);
                check->strokeWidth(2.5f);
                check->strokeFill(255, 255, 255, 200);
                canvas.add(check);
            } else if (i == 5) {
                auto* arrow    = tvg::Shape::gen();
                const float cx = f.center().x(), cy = f.center().y();
                arrow->moveTo(cx, cy - 6);
                arrow->lineTo(cx, cy + 6);
                arrow->moveTo(cx - 5, cy - 1);
                arrow->lineTo(cx, cy - 6);
                arrow->lineTo(cx + 5, cy - 1);
                arrow->strokeWidth(2.5f);
                arrow->strokeFill(255, 255, 255, 200);
                canvas.add(arrow);
            }

            if (i < 3) {
                draw_circle(canvas, f.center().x(), f.y() + f.height() + 4, 2, cr, cg, cb, 220);
            }
        }

        // 左侧窗口缩略图
        const float thumb_w = 120.0f;
        const float thumb_h = h - 12.0f;
        const float thumb_x = x + 12.0f;
        const float thumb_y = y + 6.0f;
        draw_rect(canvas, thumb_x, thumb_y, thumb_w, thumb_h, 55, 57, 75, 200, 4);
        draw_rect(canvas, thumb_x + 6, thumb_y + 6, thumb_w - 12, 6, 70, 72, 92, 200, 2);
        draw_rect(canvas, thumb_x + 6, thumb_y + 16, thumb_w - 12, 4, 65, 67, 85, 180, 2);
        draw_rect(canvas, thumb_x + 6, thumb_y + 24, thumb_w - 12, thumb_h - 30, 60, 62, 80, 150, 2);

        // 时钟文字已由 Label child 绘制
    }

    // ── 主内容区的 ThorVG 背景与不可替换部分 ──
    void draw_content_bg_and_chart(tvg::SwCanvas& canvas, float x, float y, float w, float /* h */) const {
        using namespace nandina::layout;

        // ── 顶部状态栏背景（按钮图标仍用 ThorVG）──
        const float header_h = 44.0f;
        draw_rect(canvas, x, y, w, header_h, 35, 37, 54, 240);
        draw_line(canvas, x, y + header_h, x + w, y + header_h, 50, 52, 70, 120);

        // 左侧两个工具栏图标按钮
        draw_rect(canvas, x + w - 100, y + 10, 28, 24, 55, 57, 75, 200, 4);
        draw_rect(canvas, x + w - 66, y + 10, 28, 24, 55, 57, 75, 200, 4);

        // ── 卡片网格 ────────────────────────────────
        const float content_top = y + header_h + 20.0f;
        const float card_gap    = 16.0f;

        // ─ 第二行：左侧图表 + 右侧活动列表 ──
        {
            const float row2_y = content_top + 116.0f;
            const float row2_h = 200.0f;

            // 左侧 — 图表卡片
            const float chart_w = (w - 20.0f - 20.0f - card_gap) * 0.6f;
            const float chart_x = x + 20.0f;
            draw_rect(canvas, chart_x, row2_y, chart_w, row2_h, card_bg.r, card_bg.g, card_bg.b, 255, 8);
            draw_text_dots(canvas, chart_x + 16, row2_y + 14, "Weekly Activity",
                text_primary.r, text_primary.g, text_primary.b, 220, 10);

            // 模拟折线图
            const float plot_x = chart_x + 20.0f;
            const float plot_y = row2_y + 36.0f;
            const float plot_w = chart_w - 40.0f;
            const float plot_h = row2_h - 56.0f;

            for (int gi = 0; gi < 4; ++gi) {
                const float gy = plot_y + plot_h * static_cast<float>(gi) / 3.0f;
                draw_line(canvas, plot_x, gy, plot_x + plot_w, gy, 55, 57, 75, 100, 1);
            }

            const std::array<float, 7> data = {0.3f, 0.6f, 0.45f, 0.8f, 0.65f, 0.9f, 0.7f};
            const float step                = plot_w / static_cast<float>(data.size() - 1);

            auto* area = tvg::Shape::gen();
            area->moveTo(plot_x, plot_y + plot_h);
            for (size_t di = 0; di < data.size(); ++di) {
                const float px = plot_x + static_cast<float>(di) * step;
                const float py = plot_y + plot_h * (1.0f - data[di] * 0.85f);
                area->lineTo(px, py);
            }
            area->lineTo(plot_x + plot_w, plot_y + plot_h);
            area->close();
            area->fill(accent.r, accent.g, accent.b, 40);
            canvas.add(area);

            auto* line = tvg::Shape::gen();
            for (size_t di = 0; di < data.size(); ++di) {
                const float px = plot_x + static_cast<float>(di) * step;
                const float py = plot_y + plot_h * (1.0f - data[di] * 0.85f);
                if (di == 0)
                    line->moveTo(px, py);
                else
                    line->lineTo(px, py);
            }
            line->strokeWidth(2.5f);
            line->strokeFill(accent.r, accent.g, accent.b, 220);
            canvas.add(line);

            for (size_t di = 0; di < data.size(); ++di) {
                const float px = plot_x + static_cast<float>(di) * step;
                const float py = plot_y + plot_h * (1.0f - data[di] * 0.85f);
                draw_circle(canvas, px, py, 3, accent.r, accent.g, accent.b, 220);
                draw_circle(canvas, px, py, 5, accent.r, accent.g, accent.b, 80);
            }

            const auto days = std::to_array<std::string_view>({"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"});
            for (size_t di = 0; di < days.size(); ++di) {
                const float lx = plot_x + static_cast<float>(di) * step;
                draw_text_dots(canvas, lx - 8, plot_y + plot_h + 8,
                    days[di], text_dim.r, text_dim.g, text_dim.b, 150, 7);
            }

            // ─ 右侧 — 活动列表 ──────────────
            const float list_x = chart_x + chart_w + card_gap;
            const float list_w = w - 20.0f - list_x;
            draw_rect(canvas, list_x, row2_y, list_w, row2_h, card_bg.r, card_bg.g, card_bg.b, 255, 8);
            draw_text_dots(canvas, list_x + 16, row2_y + 14, "Recent Activity",
                text_primary.r, text_primary.g, text_primary.b, 220, 10);

            const auto activities = std::to_array<std::pair<std::string_view, std::string_view>>({
                {"Updated layout-core", "2 min ago"},
                {"Merged PR #42", "15 min ago"},
                {"Fixed flex alignment", "1 hr ago"},
                {"Added Stack support", "3 hr ago"},
                {"Refactored backend", "6 hr ago"},
            });

            const auto act_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
                std::tuple{95, 200, 130},
                std::tuple{99, 102, 241},
                std::tuple{236, 110, 130},
                std::tuple{245, 158, 60},
                std::tuple{80, 200, 220},
            });

            for (size_t ai = 0; ai < activities.size(); ++ai) {
                const float ai_y         = row2_y + 36.0f + static_cast<float>(ai) * 30.0f;
                const auto& [cr, cg, cb] = act_colors[ai];

                const float pulse_offset = static_cast<float>(ai) * 0.15f;
                const float pulse_alpha  = 150.0f + 70.0f * (0.5f + 0.5f * std::sin(
                                                                m_activity_pulse * 6.2831853f + pulse_offset));
                draw_circle(canvas, list_x + 22, ai_y + 12, 4,
                    cr, cg, cb, static_cast<uint8_t>(pulse_alpha));

                draw_text_dots(canvas, list_x + 36, ai_y + 2, activities[ai].first,
                    text_secondary.r, text_secondary.g, text_secondary.b, 200, 8);
                draw_text_dots(canvas, list_x + 36, ai_y + 16, activities[ai].second,
                    text_dim.r, text_dim.g, text_dim.b, 140, 6);
            }
        }

        // ─ 第三行：进度卡片 ──
        {
            const float stack_y     = content_top + 116.0f + 200.0f + 20.0f;
            const float stack_box_h = 110.0f;

            BasicLayoutBackend backend;
            LayoutRequest req;
            req.axis             = LayoutAxis::row;
            req.container_bounds = {x + 20, stack_y, x + w - 20, stack_y + stack_box_h};
            req.gap              = card_gap;
            req.cross_alignment  = LayoutAlignment::stretch;

            const float item_w = (w - 20.0f - 20.0f - card_gap) * 0.5f; {
                LayoutChildSpec child;
                child.preferred_size = {item_w, 0.0f};
                child.flex_factor    = 1;
                req.children.push_back(child);
            } {
                LayoutChildSpec child;
                child.preferred_size = {item_w, 0.0f};
                child.flex_factor    = 1;
                req.children.push_back(child);
            }

            auto frames = backend.compute(req);

            // ─ 右侧：进度卡片背景与标题（进度条由 ProgressBar widget 绘制）──
            if (frames.size() >= 2) {
                const auto& f = frames[1];
                draw_rect(canvas, f.x(), f.y(), f.width(), f.height(), card_bg.r, card_bg.g, card_bg.b, 255, 8);
                draw_text_dots(canvas, f.x() + 14, f.y() + 14, "Project Progress",
                    text_primary.r, text_primary.g, text_primary.b, 220, 9);
            }
        }

        // ─ 底部分隔线 ──
        const float row2_h   = 200.0f;
        const float stack_h  = 110.0f;
        const float footer_y = content_top + 116.0f + row2_h + 20.0f + stack_h + 20.0f;
        draw_line(canvas, x + 20, footer_y, x + w - 20, footer_y, 55, 57, 75, 100);
    }

private:
    nandina::NanColor bg_color;

    struct Color4 {
        uint8_t r, g, b, a{255};
    };

    Color4 sidebar_bg, dock_bg, card_bg;
    Color4 accent, accent2, green, orange, pink, cyan;
    Color4 text_primary, text_secondary, text_dim;

    // Widget 子节点
    nandina::runtime::NanWidget* m_header_title{nullptr};
    nandina::runtime::NanWidget* m_add_button{nullptr};
    nandina::runtime::NanWidget* m_clock{nullptr};
    nandina::runtime::NanWidget* m_footer_text{nullptr};
    std::array<nandina::runtime::NanWidget*, 4> m_stat_cards{};

    nandina::runtime::NanWidget* m_stack_container{nullptr};
    nandina::runtime::NanWidget* m_stack_title{nullptr};
    std::array<nandina::runtime::NanWidget*, 3> m_stack_boxes{};
    nandina::runtime::NanWidget* m_stack_footer{nullptr};

    // 进度条行（label + bar + percentage）
    struct ProgressRow {
        nandina::runtime::NanWidget* label{nullptr};
        nandina::runtime::NanWidget* bar{nullptr};
        nandina::runtime::NanWidget* pct{nullptr};
    };

    std::array<ProgressRow, 4> m_progress_rows{};

    // 统计卡片的 Pressable 交互层
    std::array<nandina::runtime::NanWidget*, 4> m_stat_card_pressables{};

    // 日志
    decltype(nandina::log::get("")) m_log{nandina::log::get("showcase")};

    // 动画
    nandina::reactive::AnimationManager m_anim_manager;
    std::chrono::steady_clock::time_point m_last_frame_time{std::chrono::steady_clock::now()};

    // 交互状态
    int m_click_count{0};

    // 活动列表脉冲动画进度
    float m_activity_pulse{0.0f};

    // 布局缓存
    float m_last_layout_w{0.0f};
    float m_last_layout_h{0.0f};
    bool m_layout_valid{false};
};

// ═══════════════════════════════════════════════════════════
//  MainWindow
// ═══════════════════════════════════════════════════════════
export class MainWindow final : public nandina::app::NanAppWindow {
public:
    MainWindow() : nandina::app::NanAppWindow({
        .title = "NandinaUI — Comprehensive UI Showcase",
        .width = 1280,
        .height = 720,
        .resizable = true,
        .high_dpi = true
    }) {
        set_root_component(std::make_unique<MainComponent>());
    }

protected:
    void on_ready() override {
        m_log.info("MainWindow ready — Dashboard UI showcase");
    }

private:
    decltype(nandina::log::get("showcase.window")) m_log = nandina::log::get("showcase.window");
};
