module;

#include <array>
#include <functional>
#include <memory>
#include <string_view>
#include <thorvg-1/thorvg.h>
#include <tuple>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.widgets;

import nandina.showcase.overview_page;
import nandina.showcase.layout_page;
import nandina.showcase.widgets_page;
import nandina.showcase.authoring_page;

// ── 辅助函数 ────────────────────────────────────────────────────────────────
namespace {

    using nandina::NanColor;
    using nandina::NanRgb;

    auto draw_rect(tvg::SwCanvas& canvas,
        const float x, const float y, const float w, const float h,
        const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) -> void {
        auto* shape = tvg::Shape::gen();
        shape->appendRect(x, y, w, h, 0.0f, 0.0f);
        shape->fill(r, g, b, a);
        canvas.add(shape);
    }

    struct NavEntry {
        std::string_view   key;
        std::string_view   label;
        nandina::widgets::IconType icon;
    };

    constexpr auto k_nav_entries = std::array<NavEntry, 4>{{
        {"overview",  "Overview",  nandina::widgets::IconType::Circle},
        {"layout",    "Layout",    nandina::widgets::IconType::Square},
        {"widgets",   "Widgets",   nandina::widgets::IconType::Triangle},
        {"authoring", "Authoring", nandina::widgets::IconType::Dots},
    }};

} // namespace

// ── MainShell — 应用 Shell 组件（sidebar + 内容区） ─────────────────────────
class MainShell final : public nandina::app::NanComponent {
public:
    explicit MainShell() {
        build_shell();
    }

protected:
    auto layout() -> void override {
        if (m_root_row) {
            m_root_row->set_bounds(x(), y(), width(), height());
        }
        clear_layout_dirty();
    }

    auto on_draw(tvg::SwCanvas& canvas) -> void override {
        draw_rect(canvas, x(), y(), width(), height(), 21, 24, 32, 255);
    }

private:
    auto build_shell() -> void {
        // ── 创建路由器并注册页面 ──────────────────────────────
        m_router = nandina::app::NanRouter::create();
        m_router->register_page(std::make_unique<OverviewPage>());
        m_router->register_page(std::make_unique<LayoutPage>());
        m_router->register_page(std::make_unique<WidgetsPage>());
        m_router->register_page(std::make_unique<AuthoringPage>());

        // ── 根行布局 ─────────────────────────────────────────
        auto root_row = nandina::layout::Row::Create();
        root_row->align_items(nandina::layout::LayoutAlignment::stretch);
        m_root_row = root_row.get();
        add_child(std::move(root_row));

        // ── 侧边栏（固定宽度 260） ────────────────────────────
        auto sidebar_slot = nandina::layout::SizedBox::Create();
        sidebar_slot->width(260.0f).child(build_sidebar());
        m_root_row->add(std::move(sidebar_slot));

        // ── 内容区（Expanded → NanPageHost） ─────────────────
        auto page_host = std::make_unique<nandina::app::NanPageHost>(m_router);
        auto content_expanded = nandina::layout::Expanded::Create();
        content_expanded->child(std::move(page_host));
        m_root_row->add(std::move(content_expanded));
    }

    auto build_sidebar() -> nandina::widgets::Sidebar::Ptr {
        auto sidebar = nandina::widgets::Sidebar::create();
        sidebar->set_header_title("NandinaUI")
            .set_user_name("dev")
            .set_user_role("showcase");

        // 为每个导航项创建按钮，捕获原始指针以便后续更新 active 状态
        for (const auto& entry : k_nav_entries) {
            auto btn = nandina::widgets::SidebarMenuButton::create();
            btn->set_label(entry.label)
                .set_icon_type(entry.icon)
                .set_active(entry.key == m_router->current_key());

            // 保存原始指针
            m_nav_buttons.push_back(btn.get());

            // 绑定点击 → router 导航
            btn->on_click([router = m_router, key = std::string{entry.key}] {
                router->navigate_to(key);
            });

            sidebar->add_menu_item(std::move(btn));
        }

        // 订阅导航事件 → 更新 active 状态
        m_router->on_navigate([this](std::string_view new_key) {
            for (std::size_t i = 0; i < m_nav_buttons.size(); ++i) {
                m_nav_buttons[i]->set_active(k_nav_entries[i].key == new_key);
            }
        });

        sidebar->add_project_item([]{
            auto btn = nandina::widgets::SidebarMenuButton::create();
            btn->set_label("Issue 086").set_icon_type(nandina::widgets::IconType::Dot);
            return btn;
        }());
        sidebar->add_project_item([]{
            auto btn = nandina::widgets::SidebarMenuButton::create();
            btn->set_label("Router / PageHost").set_icon_type(nandina::widgets::IconType::ArrowUp);
            return btn;
        }());

        return sidebar;
    }

    nandina::app::NanRouter::Ptr                       m_router;
    nandina::layout::Row*                              m_root_row{nullptr};
    std::vector<nandina::widgets::SidebarMenuButton*>  m_nav_buttons;
};

// ── MainWindow — 导出给 main.cpp ─────────────────────────────────────────────
export class MainWindow final : public nandina::app::NanAppWindow {
public:
    MainWindow()
        : nandina::app::NanAppWindow({
            .title     = "NandinaUI — Showcase",
            .width     = 1280,
            .height    = 720,
            .resizable = true,
            .high_dpi  = true,
        }) {
        set_root(nandina::app::adopt(std::make_unique<MainShell>()));
    }
};
