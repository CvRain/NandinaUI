module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <thorvg-1/thorvg.h>
#include <tuple>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.widgets;

import nandina.showcase.registry;

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

    auto build_sidebar(const nandina::app::NanRouter::Ptr& router) -> nandina::widgets::Sidebar::Ptr {
        auto sidebar = nandina::widgets::Sidebar::create();
        sidebar->set_header_title("NandinaUI")
            .set_user_name("dev")
            .set_user_role("showcase");

        std::vector<nandina::widgets::SidebarMenuButton*> nav_buttons;
        std::vector<std::string> nav_keys;

        for (const auto& page : router->pages()) {
            auto btn = nandina::widgets::SidebarMenuButton::create();
            btn->set_label(page->title())
                .set_icon_type(page->icon_type())
                .set_active(page->route_key() == router->current_key());

            nav_buttons.push_back(btn.get());
            nav_keys.emplace_back(page->route_key());

            btn->on_click([router, key = std::string{page->route_key()}] {
                router->navigate_to(key);
            });

            sidebar->add_menu_item(std::move(btn));
        }

        router->on_navigate([
            nav_buttons = std::move(nav_buttons),
            nav_keys = std::move(nav_keys)
        ](std::string_view new_key) {
            for (std::size_t i = 0; i < nav_buttons.size(); ++i) {
                nav_buttons[i]->set_active(nav_keys[i] == new_key);
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

} // namespace

export namespace nandina::showcase {

    auto create_showcase_shell_content() -> app::Node {
        using namespace nandina::app;

        auto router = nandina::app::NanRouter::create();
        nandina::showcase::register_default_pages(*router);

        return row(children(
            sized_box(adopt(build_sidebar(router)))
                .width(260.0f),
            expanded(adopt(std::make_unique<nandina::app::NanPageHost>(router)))
        )).align_items(nandina::layout::LayoutAlignment::stretch);
    }

} // namespace nandina::showcase

// ── MainShell — 应用 Shell 组件（sidebar + 内容区） ─────────────────────────
class MainShell final : public nandina::app::NanComponent {
public:
    explicit MainShell() {
        auto content = nandina::app::mount(nandina::showcase::create_showcase_shell_content());
        m_content = static_cast<nandina::app::NanComponent*>(add_child(std::move(content)));
    }

protected:
    auto measure(const nandina::geometry::NanConstraints& constraints) -> void override {
        if (!m_content) {
            const nandina::geometry::NanSize empty_size{};
            set_measured_layout_state(constraints, constraints.constrain(empty_size));
            return;
        }

        if (constraints.is_tight()) {
            set_measured_layout_state(
                constraints,
                nandina::geometry::NanSize{constraints.max_width(), constraints.max_height()});
            return;
        }

        m_content->measure(constraints);
        auto measured = m_content->measured_size();
        if (measured.width() <= 0.0f && measured.height() <= 0.0f) {
            measured = m_content->preferred_size();
        }
        set_measured_layout_state(constraints, constraints.constrain(measured));
    }

    auto layout() -> void override {
        if (m_content) {
            m_content->set_bounds(x(), y(), width(), height());
        }
        clear_layout_dirty();
    }

    auto on_draw(tvg::SwCanvas& canvas) -> void override {
        draw_rect(canvas, x(), y(), width(), height(), 21, 24, 32, 255);
    }

private:
    nandina::app::NanComponent* m_content{nullptr};
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
