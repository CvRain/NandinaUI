module;

#include <thorvg-1/thorvg.h>

export module nandina.showcase;

import nandina.app.application;
import nandina.runtime.nan_window;
import nandina.log;

export class ShowcaseApp final : public nandina::NanApplication {
protected:
    [[nodiscard]] auto configure() -> nandina::AppConfig override {
        return {
            .title = "NandinaUI — Showcase",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .high_dpi = true,
        };
    }

    auto on_ready() -> void override {
        m_log.info("ShowcaseApp ready");
    }

    auto on_resize(int w, int h) -> void override {
        m_log.debug("Window resized: {}x{}", w, h);
    }

    auto on_close_requested() -> void override {
        m_log.info("Close requested");
    }

    auto on_draw(tvg::SwCanvas &canvas) -> void override {
        const auto w = static_cast<float>(window_width());
        const auto h = static_cast<float>(window_height());

        // ── 背景 ────────────────────────────────────────────────
        auto *bg = tvg::Shape::gen();
        bg->appendRect(0, 0, w, h, 0, 0);
        bg->fill(18, 18, 22, 255);
        canvas.add(bg);

        // ── 装饰圆（左上）───────────────────────────────────────
        auto *circle_l = tvg::Shape::gen();
        circle_l->appendCircle(w * 0.125f, h * 0.28f, 120, 120);
        circle_l->fill(99, 102, 241, 160); // indigo, 半透明
        canvas.add(circle_l);

        // ── 装饰圆（右下）───────────────────────────────────────
        auto *circle_r = tvg::Shape::gen();
        circle_r->appendCircle(w * 0.86f, h * 0.73f, 90, 90);
        circle_r->fill(236, 72, 153, 140); // pink, 半透明
        canvas.add(circle_r);

        // ── 中央卡片（圆角矩形）─────────────────────────────────
        const float card_w = w * 0.39f;
        const float card_h = h * 0.39f;
        const float card_x = (w - card_w) * 0.5f;
        const float card_y = (h - card_h) * 0.5f;
        auto *card = tvg::Shape::gen();
        card->appendRect(card_x, card_y, card_w, card_h, 16, 16);
        card->fill(30, 30, 38, 255);
        canvas.add(card);
    }

private:
    decltype(nandina::log::get("showcase.app")) m_log = nandina::log::get("showcase.app");
};
