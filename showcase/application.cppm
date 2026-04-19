module;
#include <memory>
#include <thorvg-1/thorvg.h>

export module nandina.showcase;

import nandina.app.application;
import nandina.log;

export class MainComponent final : public nandina::NanComponent {
protected:
    void on_draw(tvg::SwCanvas &canvas) override {
        const auto w = width();
        const auto h = height();

        auto *bg = tvg::Shape::gen();
        bg->appendRect(0, 0, w, h, 0, 0);
        bg->fill(18, 18, 22, 255);
        canvas.add(bg);

        auto *circle_l = tvg::Shape::gen();
        circle_l->appendCircle(w * 0.125f, h * 0.28f, 120, 120);
        circle_l->fill(99, 102, 241, 160);
        canvas.add(circle_l);

        auto *circle_r = tvg::Shape::gen();
        circle_r->appendCircle(w * 0.86f, h * 0.73f, 90, 90);
        circle_r->fill(236, 72, 153, 140);
        canvas.add(circle_r);

        const float card_w = w * 0.39f;
        const float card_h = h * 0.39f;
        const float card_x = (w - card_w) * 0.5f;
        const float card_y = (h - card_h) * 0.5f;
        auto *card = tvg::Shape::gen();
        card->appendRect(card_x, card_y, card_w, card_h, 16, 16);
        card->fill(30, 30, 38, 255);
        canvas.add(card);
    }
};

export class MainWindow final : public nandina::NanAppWindow {
public:
    MainWindow() : NanAppWindow({
        .title = "NandinaUI — Component Showcase",
        .width = 1280,
        .height = 720,
        .resizable = true,
        .high_dpi = true
    }) {
        set_root_component(std::make_unique<MainComponent>());
    }

protected:
    void on_ready() override {
        m_log.info("MainWindow ready with root component");
    }

private:
    decltype(nandina::log::get("showcase.window")) m_log = nandina::log::get("showcase.window");
};
