module;

#include <memory>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.widgets;
import nandina.foundation.color;

import nandina.showcase.sandbox_page;
import nandina.showcase.main_page;
import nandina.showcase.page.button;

export namespace nandina::showcase {
    [[nodiscard]] inline auto create_showcase_shell() -> nandina::app::Node {
        auto router = nandina::app::NanRouter::create();
        router->register_page(std::make_unique<nandina::showcase::MainPage>());
        router->register_page(std::make_unique<nandina::showcase::ButtonPage>());
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
        set_root(nandina::showcase::create_showcase_shell());
    }
};
