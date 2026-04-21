module;
#include <memory>
#include <thorvg-1/thorvg.h>
#include <cmath>
#include <chrono>
#include <thread>

export module nandina.showcase;

import nandina.app.application;
import nandina.log;
import nandina.foundation.color;

export class MainComponent final : public nandina::NanComponent {
public:
    explicit MainComponent()
        : background_color(nandina::NanColor::from(nandina::NanRgb{35, 38, 52})),
          circle_color(nandina::NanColor::from(nandina::NanRgb{210, 130, 132})) {
    }

protected:
    auto on_draw(tvg::SwCanvas &canvas) -> void override {
        const auto w = width();
        const auto h = height();
        const auto &bg_color = background_color.to<nandina::NanRgb>();

        auto *bg = tvg::Shape::gen();
        bg->appendRect(0, 0, w, h, 0, 0);
        bg->fill(bg_color.red(), bg_color.green(), bg_color.blue(), bg_color.alpha());
        canvas.add(bg);

        auto *circle_l = tvg::Shape::gen();
        circle_l->appendCircle(w * 0.125f, h * 0.28f, 120, 120);
        circle_l->fill(99, 102, 241, 160);
        canvas.add(circle_l);

        auto *circle_r = tvg::Shape::gen();
        circle_r->appendCircle(w * 0.86f, h * 0.73f, 90, 90);

        circle_color.transform<nandina::NanHsv>([&](auto hsv) {
            return nandina::NanHsv{
                hsv.hue() + 1,
                hsv.saturation(),
                hsv.value(),
                hsv.alpha()
            };
        });
        const auto circle_rgb = circle_color.to<nandina::NanRgb>();

        circle_r->fill(circle_rgb.red(), circle_rgb.green(), circle_rgb.blue(), circle_rgb.alpha());
        canvas.add(circle_r);

        const float card_w = w * 0.39f;
        const float card_h = h * 0.39f;
        const float card_x = (w - card_w) * 0.5f;
        const float card_y = (h - card_h) * 0.5f;
        auto *card = tvg::Shape::gen();
        card->appendRect(card_x, card_y, card_w, card_h, 16, 16);
        card->fill(30, 30, 38, 255);
        canvas.add(card);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

private:
    nandina::log::Logger logger = nandina::log::get("MainComponent");
    nandina::NanColor background_color;
    nandina::NanColor circle_color;
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
