module;

#include <memory>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.widgets;
import nandina.foundation.color;
import nandina.theme;

import nandina.showcase.sandbox_page;
import nandina.showcase.main_page;
import nandina.showcase.page.button;
import nandina.showcase.page.forms;
import nandina.showcase.page.checkbox;
import nandina.showcase.page.card;

export namespace nandina::showcase {
    [[nodiscard]] inline auto create_showcase_shell() -> nandina::app::Node {
        auto router = nandina::app::NanRouter::create();
        router->register_page(std::make_unique<nandina::showcase::MainPage>());
        router->register_page(std::make_unique<nandina::showcase::ButtonPage>());
        router->register_page(std::make_unique<nandina::showcase::FormsPage>());
        router->register_page(std::make_unique<nandina::showcase::CheckboxPage>());
        router->register_page(std::make_unique<nandina::showcase::CardPage>());
        router->register_page(std::make_unique<nandina::showcase::SandboxPage>());

        return nandina::app::create_shell(std::move(router), {
            .sidebar_width = 260.0f,
            .header_title = "NandinaUI Showcase",
        });
    }
}

// ── MainWindow — 唯一导出 ───────────────────────────────────────────────────
export class MainWindow final : public nandina::app::NanAppWindow {
public:
    MainWindow()
        : nandina::app::NanAppWindow({
            .title = "NandinaUI — Showcase",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .high_dpi = true,
            .bg_color = nandina::NanColor::from(nandina::NanRgb{"#0a0a0a"})
        }) {
        m_theme_connection = nandina::theme::ThemeManager::instance().on_changed(
            [this](const std::string& /*name*/) {
                m_pending_theme_rebuild = true;
            });
        set_root(nandina::showcase::create_showcase_shell());
    }

protected:
    void on_update(const double delta_seconds) override {
        nandina::app::NanAppWindow::on_update(delta_seconds);

        if (m_pending_theme_rebuild) {
            m_pending_theme_rebuild = false;
            set_root(nandina::showcase::create_showcase_shell());
        }
    }

private:
    nandina::theme::ThemeManager::Connection m_theme_connection;
    bool m_pending_theme_rebuild{false};
};
