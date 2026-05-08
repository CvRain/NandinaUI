module;

#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <thorvg-1/thorvg.h>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.log;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
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
        if (m_column) {
            m_column->padding_top(m_top_gap);
        }
        return *this;
    }

    auto set_section_gap(const float gap) noexcept -> ShowcaseContentColumn& {
        m_section_gap = std::max(0.0f, gap);
        if (m_column) {
            m_column->gap(m_section_gap);
        }
        return *this;
    }

    auto set_stats_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_stats_section = child.get();
        if (m_column) {
            m_column->add(std::move(child));
        }
        return m_stats_section;
    }

    auto set_middle_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_middle_section = child.get();
        if (m_column) {
            m_column->add(std::move(child));
        }
        return m_middle_section;
    }

    auto set_bottom_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_bottom_section = child.get();
        if (m_column) {
            m_column->add(std::move(child));
        }
        return m_bottom_section;
    }

    auto set_footer_section(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_footer_section = child.get();
        if (m_column) {
            m_column->add(std::move(child));
        }
        return m_footer_section;
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_column) {
            m_column->set_bounds(x, y, w, h);
        }

        return *this;
    }

private:
    ShowcaseContentColumn() {
        auto column = nandina::layout::Column::Create();
        column->align_items(nandina::layout::LayoutAlignment::stretch)
            .padding_top(m_top_gap)
            .gap(m_section_gap);
        m_column = column.get();
        add_child(std::move(column));
    }

    nandina::layout::Column* m_column{nullptr};
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
        if (m_content_padding_node) {
            m_content_padding_node->padding(m_content_padding, 0.0f);
        }
        return *this;
    }

    auto set_sidebar(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_sidebar = child.get();
        if (m_sidebar_slot) {
            m_sidebar_slot->child(std::move(child));
        }
        return m_sidebar;
    }

    auto set_header(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_header = child.get();
        if (m_header_slot) {
            m_header_slot->child(std::move(child));
        }
        return m_header;
    }

    auto set_content(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_content = child.get();
        if (m_content_padding_node) {
            m_content_padding_node->child(std::move(child));
        }
        return m_content;
    }

    auto set_dock(std::unique_ptr<nandina::runtime::NanWidget> child) -> nandina::runtime::NanWidget* {
        m_dock = child.get();
        if (m_dock_slot) {
            m_dock_slot->child(std::move(child));
        }
        return m_dock;
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_root_row) {
            m_root_row->measure(nandina::geometry::NanConstraints::tight(w, h));
            m_root_row->set_bounds(x, y, w, h);
        }

        return *this;
    }

private:
    ShowcaseShell() {
        auto root_row = nandina::layout::Row::Create();
        root_row->align_items(nandina::layout::LayoutAlignment::stretch);
        m_root_row = root_row.get();
        add_child(std::move(root_row));

        auto sidebar_slot = nandina::layout::SizedBox::Create();
        m_sidebar_slot = sidebar_slot.get();
        m_root_row->add(std::move(sidebar_slot));

        auto main_expanded = nandina::layout::Expanded::Create();
        m_main_expanded = main_expanded.get();
        m_root_row->add(std::move(main_expanded));

        auto main_column = nandina::layout::Column::Create();
        main_column->align_items(nandina::layout::LayoutAlignment::stretch);
        m_main_column = main_column.get();
        m_main_expanded->child(std::move(main_column));

        auto header_slot = nandina::layout::SizedBox::Create();
        m_header_slot = header_slot.get();
        m_main_column->add(std::move(header_slot));

        auto content_slot = nandina::layout::Expanded::Create();
        m_content_slot = content_slot.get();
        m_main_column->add(std::move(content_slot));

        auto content_padding = nandina::layout::Padding::Create();
        content_padding->padding(m_content_padding, 0.0f);
        m_content_padding_node = content_padding.get();
        m_content_slot->child(std::move(content_padding));

        auto dock_slot = nandina::layout::SizedBox::Create();
        m_dock_slot = dock_slot.get();
        m_main_column->add(std::move(dock_slot));
    }

    nandina::runtime::NanWidget* m_sidebar{nullptr};
    nandina::runtime::NanWidget* m_header{nullptr};
    nandina::runtime::NanWidget* m_content{nullptr};
    nandina::runtime::NanWidget* m_dock{nullptr};
    nandina::layout::Row* m_root_row{nullptr};
    nandina::layout::SizedBox* m_sidebar_slot{nullptr};
    nandina::layout::Expanded* m_main_expanded{nullptr};
    nandina::layout::Column* m_main_column{nullptr};
    nandina::layout::SizedBox* m_header_slot{nullptr};
    nandina::layout::Expanded* m_content_slot{nullptr};
    nandina::layout::Padding* m_content_padding_node{nullptr};
    nandina::layout::SizedBox* m_dock_slot{nullptr};
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

        auto footer_section = nandina::showcase::FooterSection::create();
        m_footer_section = footer_section.get();
        if (m_content_column) {
            m_content_column->set_footer_section(std::move(footer_section));
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
        set_root(nandina::app::adopt(std::make_unique<MainComponent>()));
    }

protected:
    void on_ready() override {
        m_log.info("MainWindow ready — Dashboard UI showcase");
    }

private:
    decltype(nandina::log::get("showcase.window")) m_log = nandina::log::get("showcase.window");
};
