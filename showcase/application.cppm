module;
#include <memory>
#include <thorvg-1/thorvg.h>

export module nandina.showcase;

import nandina.app.application;
import nandina.log;
import nandina.foundation.color;

export class MainComponent final : public nandina::NanComponent {
protected:
    void on_draw(tvg::SwCanvas &canvas) override {
        logger.debug("MainComponent draw");

        const auto w = width();
        const auto h = height();

        constexpr auto crust_rgb = nandina::NanRgb{35u, 38u, 52u};
        const auto crust_from_rgb = nandina::NanColor::from(crust_rgb);
        const auto crust_oklch = crust_from_rgb.to<nandina::NanOklch>();
        const auto bg_color_rgb = crust_from_rgb.to<nandina::NanRgb>();

        auto *bg = tvg::Shape::gen();
        bg->appendRect(0, 0, w, h, 0, 0);
        bg->fill(bg_color_rgb.red(), bg_color_rgb.green(), bg_color_rgb.blue(), bg_color_rgb.alpha());
        logger.debug("MainComponent background color rgb={}, {}, {}",
                     bg_color_rgb.red(), bg_color_rgb.green(), bg_color_rgb.blue());
        logger.debug("MainComponent crust oklch raw=L:{} C:{} H:{}",
                     crust_oklch.lightness(), crust_oklch.chroma(), crust_oklch.hue());
        logger.debug("MainComponent crust oklch css-ish sample=L:{}% C:{}% H:{}deg (rounded page label)",
                     crust_oklch.lightness() * 100.0f,
                     crust_oklch.chroma() * 100.0f,
                     crust_oklch.hue());
        logger.flush();

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

private:
    nandina::log::Logger logger = nandina::log::get("MainComponent");
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
