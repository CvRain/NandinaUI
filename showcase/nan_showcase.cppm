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

export namespace nandina::showcase {
    [[nodiscard]] inline auto create_showcase_shell() -> nandina::app::Node {
        auto router = nandina::app::NanRouter::create();
        router->register_page(std::make_unique<nandina::showcase::MainPage>());
        router->register_page(std::make_unique<nandina::showcase::ButtonPage>());
        router->register_page(std::make_unique<nandina::showcase::FormsPage>());
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
            .bg_color = nandina::NanColor::from(nandina::NanRgb{"#1e1e2e"})
        }) {
        ensure_themes_registered();
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
    static auto ensure_themes_registered() -> void {
        static bool done = false;
        if (done) return;
        done = true;

        auto rgb = [](std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255u) {
            return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
        };

        auto warm = nandina::theme::NanTheme{"showcase_warm"};
        warm.palette().set(nandina::theme::NanColorRole::primary,          rgb(181, 99, 36),  rgb(255, 183, 126));
        warm.palette().set(nandina::theme::NanColorRole::onPrimary,        rgb(255, 255, 255),rgb(78, 35, 0));
        warm.palette().set(nandina::theme::NanColorRole::primaryContainer, rgb(255, 220, 196),rgb(107, 56, 10));
        warm.palette().set(nandina::theme::NanColorRole::onPrimaryContainer,rgb(60, 22, 0),   rgb(255, 220, 196));
        warm.palette().set(nandina::theme::NanColorRole::surface,          rgb(250, 244, 236),rgb(35, 29, 26));
        warm.palette().set(nandina::theme::NanColorRole::onSurface,        rgb(43, 36, 33),  rgb(236, 225, 219));
        warm.palette().set(nandina::theme::NanColorRole::surfaceVariant,   rgb(231, 220, 211),rgb(80, 69, 63));
        warm.palette().set(nandina::theme::NanColorRole::onSurfaceVariant, rgb(79, 68, 61),  rgb(210, 196, 188));
        warm.palette().set(nandina::theme::NanColorRole::background,       rgb(255, 248, 242),rgb(27, 22, 20));
        warm.palette().set(nandina::theme::NanColorRole::onBackground,     rgb(32, 27, 24),  rgb(236, 225, 219));
        warm.palette().set(nandina::theme::NanColorRole::outline,          rgb(134, 114, 101),rgb(161, 141, 128));
        warm.palette().set(nandina::theme::NanColorRole::outlineVariant,   rgb(215, 201, 192),rgb(80, 69, 63));

        nandina::theme::ThemeManager::instance().register_theme(std::move(warm));
    }

    nandina::theme::ThemeManager::Connection m_theme_connection;
    bool m_pending_theme_rebuild{false};
};
