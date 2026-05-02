module;

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <thorvg-1/thorvg.h>

export module nandina.showcase;

import nandina.app.application;
import nandina.log;
import nandina.foundation.color;
import nandina.runtime.nan_widget;
import nandina.reactive.animation;
import nandina.showcase.bottom_content_section;
import nandina.showcase.dock_bar;
import nandina.showcase.footer_section;
import nandina.showcase.header_bar;
import nandina.showcase.middle_content_section;
import nandina.showcase.sidebar_section;
import nandina.showcase.stats_section;

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

        // ─ 2. 侧边栏 — 由 Sidebar widget 树自动绘制（在 draw() 中）──

        // ─ 3. 动画更新 ──────────────────────────────
        const auto now    = std::chrono::steady_clock::now();
        const float delta = std::chrono::duration<float>(now - m_last_frame_time).count();
        m_last_frame_time = now;
        m_anim_manager.update(delta);
    }

private:
    // ── 构建 Widget 子树 ──────────────────────────────
    auto build_widget_tree() -> void {
        m_stats_section = add_child(nandina::showcase::StatsSection::create());

        m_header = add_child(nandina::showcase::HeaderBar::create());

        // ─ Dock 栏 ──
        m_dock = add_child(nandina::showcase::DockBar::create());

        auto footer_section = nandina::showcase::FooterSection::create();
        m_footer_section = footer_section.get();
        add_child(std::move(footer_section));

        auto middle_section = nandina::showcase::MiddleContentSection::create();
        m_middle_section = middle_section.get();
        add_child(std::move(middle_section));

        auto bottom_section = nandina::showcase::BottomContentSection::create();
        m_bottom_section = bottom_section.get();
        add_child(std::move(bottom_section));

        // ─ 活动列表脉冲动画 ──
        {
            auto pulse_anim = nandina::reactive::NanAnimation::create(2.0f);
            pulse_anim->set_loop(true)
                .set_easing(nandina::reactive::Easing::SineInOut)
                .on_update([this](float t) {
                    if (m_middle_section) {
                        m_middle_section->set_activity_pulse(t);
                    }
                });
            m_anim_manager.add(std::move(pulse_anim));
        }

        m_sidebar = add_child(nandina::showcase::SidebarSection::create());

    }

    // ── 布局计算 ──────────────────────────────────────
    auto layout_widgets(const float w, const float h) -> void {
        constexpr float sidebar_w   = 240.0f;
        constexpr float dock_h      = 56.0f;
        constexpr float header_h    = 44.0f;
        constexpr float content_pad = 20.0f;

        const float content_x   = sidebar_w;
        const float content_w   = w - sidebar_w;
        const float content_top = header_h + 20.0f;

        if (m_header) {
            m_header->set_bounds(content_x, 0.0f, content_w, header_h);
        }

        if (m_stats_section) {
            m_stats_section->set_bounds(content_x + content_pad, content_top, content_w - content_pad * 2.0f, 100.0f);
        }

        if (m_dock) {
            m_dock->set_bounds(sidebar_w, h - dock_h, w - sidebar_w, dock_h);
        }

        {
            const float row2_h   = 200.0f;
            const float stack_h  = 110.0f;
            const float footer_y = content_top + 116.0f + row2_h + 20.0f + stack_h + 20.0f;
            if (m_footer_section) {
                m_footer_section->set_bounds(content_x + content_pad, footer_y,
                    content_w - content_pad * 2.0f, 26.0f);
            }
        }

        // ── Sidebar 组件定位 ──────────────────────────
        if (m_sidebar) {
            m_sidebar->set_bounds(0.0f, 0.0f, sidebar_w, h);
        }

        {
            const float row2_y = content_top + 116.0f;
            const float row2_h = 200.0f;
            if (m_middle_section) {
                m_middle_section->set_bounds(content_x + content_pad, row2_y, content_w - content_pad * 2.0f, row2_h);
            }
        }

        {
            const float stack_y     = content_top + 116.0f + 200.0f + 20.0f;
            const float stack_box_h = 110.0f;
            if (m_bottom_section) {
                m_bottom_section->set_bounds(content_x + content_pad, stack_y, content_w - content_pad * 2.0f, stack_box_h);
            }
        }
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
    nandina::runtime::NanWidget* m_header{nullptr};
    nandina::runtime::NanWidget* m_dock{nullptr};
    nandina::runtime::NanWidget* m_stats_section{nullptr};
    nandina::showcase::FooterSection* m_footer_section{nullptr};

    nandina::showcase::MiddleContentSection* m_middle_section{nullptr};
    nandina::showcase::BottomContentSection* m_bottom_section{nullptr};

    // ── Sidebar 组件 ────────────────────────────────
    nandina::runtime::NanWidget* m_sidebar{nullptr};

    // 日志
    decltype(nandina::log::get("")) m_log{nandina::log::get("showcase")};

    // 动画
    nandina::reactive::AnimationManager m_anim_manager;
    std::chrono::steady_clock::time_point m_last_frame_time{std::chrono::steady_clock::now()};

    // 交互状态

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
