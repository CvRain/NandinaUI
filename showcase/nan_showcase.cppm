module;

#include <memory>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.widgets;
import nandina.foundation.color;

import nandina.showcase.sandbox_page;

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
            .bg_color = nandina::NanColor::from(nandina::NanRgb{"#eff1f5"})
        }) {
        // 直接挂载页面内容，暂不走 Sidebar + PageHost 壳
        auto page = nandina::showcase::SandboxPage{};
        set_root(nandina::app::adopt(page.build()));

    }
};
