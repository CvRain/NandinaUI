module;

#include <memory>

export module nandina.showcase;

import nandina.app.authoring;
import nandina.widgets;
import nandina.foundation.color;

import nandina.showcase.sandbox_page;
import nandina.showcase.main_page;

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
        // m_sandbox_page 作为成员，生命周期与 MainWindow 绑定；
        // build() 内的 [this]（SandboxPage*）和 Ref<Button> 引用因此保持有效
        //set_root(nandina::app::adopt(m_sandbox_page.build()));
        set_root(nandina::app::adopt(m_main_page.build()));
    }

private:
    //nandina::showcase::SandboxPage m_sandbox_page;
    nandina::showcase::MainPage m_main_page;
};
