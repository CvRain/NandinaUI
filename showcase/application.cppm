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

class ShowcaseContentColumn final : public nandina::runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ShowcaseContentColumn>;

    static auto create() -> Ptr {
        return Ptr{new ShowcaseContentColumn()};
    }

    auto set_top_gap(const float gap) noexcept -> ShowcaseContentColumn& {
        m_top_gap = std::max(0.0f, gap);
        return *this;
    }

    auto set_section_gap(const float gap) noexcept -> ShowcaseContentColumn& {
        m_section_gap = std::max(0.0f, gap);
        return *this;
    }

    auto set_stats_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_stats_section = child.get();
        add_child(std::move(child));
        return m_stats_section;
    }

    auto set_middle_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_middle_section = child.get();
        add_child(std::move(child));
        return m_middle_section;
    }

    auto set_bottom_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_bottom_section = child.get();
        add_child(std::move(child));
        return m_bottom_section;
    }

    auto set_footer_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_footer_section = child.get();
        add_child(std::move(child));
        return m_footer_section;
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        float cursor_y = y + m_top_gap;
        layout_section(m_stats_section, x, cursor_y, w);
        cursor_y += preferred_height(m_stats_section) + m_top_gap;

        layout_section(m_middle_section, x, cursor_y, w);
        cursor_y += preferred_height(m_middle_section) + m_section_gap;

        layout_section(m_bottom_section, x, cursor_y, w);
        cursor_y += preferred_height(m_bottom_section) + m_section_gap;

        layout_section(m_footer_section, x, cursor_y, w);

        return *this;
    }

private:
    ShowcaseContentColumn() = default;

    [[nodiscard]] static auto preferred_height(const nandina::runtime::NanWidget* widget) noexcept -> float {
        return widget ? widget->preferred_size().height() : 0.0f;
    }

    static auto layout_section(nandina::runtime::NanWidget* widget, const float x, const float y, const float w) -> void {
        if (!widget) {
            return;
        }
        widget->set_bounds(x, y, w, widget->preferred_size().height());
    }

    nandina::runtime::NanWidget* m_stats_section{nullptr};
    nandina::runtime::NanWidget* m_middle_section{nullptr};
    nandina::runtime::NanWidget* m_bottom_section{nullptr};
    nandina::runtime::NanWidget* m_footer_section{nullptr};
    float m_top_gap{20.0f};
    float m_section_gap{20.0f};
};

class ShowcaseShell final : public nandina::runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ShowcaseShell>;

    static auto create() -> Ptr {
        return Ptr{new ShowcaseShell()};
    }

    auto set_content_padding(const float padding) noexcept -> ShowcaseShell& {
        m_content_padding = std::max(0.0f, padding);
        return *this;
    }

    auto set_sidebar(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_sidebar = child.get();
        add_child(std::move(child));
        return m_sidebar;
    }

    auto set_header(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_header = child.get();
        add_child(std::move(child));
        return m_header;
    }

    auto set_content(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_content = child.get();
        add_child(std::move(child));
        return m_content;
    }

    auto set_dock(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_dock = child.get();
        add_child(std::move(child));
        return m_dock;
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        const float sidebar_w = preferred_width(m_sidebar);
        const float header_h = preferred_height(m_header);
        const float dock_h = preferred_height(m_dock);

        if (m_sidebar) {
            m_sidebar->set_bounds(x, y, sidebar_w, h);
        }

        const float main_x = x + sidebar_w;
        const float main_w = std::max(0.0f, w - sidebar_w);

        if (m_header) {
            m_header->set_bounds(main_x, y, main_w, header_h);
        }

        if (m_dock) {
            m_dock->set_bounds(main_x, y + h - dock_h, main_w, dock_h);
        }

        if (m_content) {
            m_content->set_bounds(
                main_x + m_content_padding,
                y + header_h,
                std::max(0.0f, main_w - m_content_padding * 2.0f),
                std::max(0.0f, h - header_h - dock_h));
        }

        return *this;
    }

private:
    ShowcaseShell() = default;

    [[nodiscard]] static auto preferred_width(const nandina::runtime::NanWidget* widget) noexcept -> float {
        return widget ? widget->preferred_size().width() : 0.0f;
    }

    [[nodiscard]] static auto preferred_height(const nandina::runtime::NanWidget* widget) noexcept -> float {
        return widget ? widget->preferred_size().height() : 0.0f;
    }

    nandina::runtime::NanWidget* m_sidebar{nullptr};
    nandina::runtime::NanWidget* m_header{nullptr};
    nandina::runtime::NanWidget* m_content{nullptr};
    nandina::runtime::NanWidget* m_dock{nullptr};
    float m_content_padding{20.0f};
};

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
        auto shell = ShowcaseShell::create();
        shell->set_content_padding(20.0f);
        m_shell = static_cast<ShowcaseShell*>(add_child(std::move(shell)));

        if (m_shell) {
            m_header = m_shell->set_header(nandina::showcase::HeaderBar::create());
            m_dock = m_shell->set_dock(nandina::showcase::DockBar::create());
            m_sidebar = m_shell->set_sidebar(nandina::showcase::SidebarSection::create());
        }

        auto content_column = ShowcaseContentColumn::create();
        content_column->set_top_gap(20.0f)
            .set_section_gap(20.0f);
        if (m_shell) {
            m_content_column = static_cast<ShowcaseContentColumn*>(
                m_shell->set_content(std::move(content_column)));
        }

        if (m_content_column) {
            m_stats_section = m_content_column->set_stats_section(nandina::showcase::StatsSection::create());
        }

        auto footer_section = nandina::showcase::FooterSection::create();
        m_footer_section = footer_section.get();
        if (m_content_column) {
            m_content_column->set_footer_section(std::move(footer_section));
        }

        auto middle_section = nandina::showcase::MiddleContentSection::create();
        m_middle_section = middle_section.get();
        if (m_content_column) {
            m_content_column->set_middle_section(std::move(middle_section));
        }

        auto bottom_section = nandina::showcase::BottomContentSection::create();
        m_bottom_section = bottom_section.get();
        if (m_content_column) {
            m_content_column->set_bottom_section(std::move(bottom_section));
        }

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

    }

    // ── 布局计算 ──────────────────────────────────────
    auto layout_widgets(const float w, const float h) -> void {
        if (m_shell) {
            m_shell->set_bounds(0.0f, 0.0f, w, h);
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
    ShowcaseShell* m_shell{nullptr};
    ShowcaseContentColumn* m_content_column{nullptr};

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
