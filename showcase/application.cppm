module;
#include <memory>
#include <thorvg-1/thorvg.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>

export module nandina.showcase;

import nandina.app.application;
import nandina.log;
import nandina.foundation.color;
import nandina.foundation.nan_point;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.layout.core;

export class MainComponent final : public nandina::app::NanComponent {
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

        // ── 背景 ──────────────────────────────────────────────
        auto *bg = tvg::Shape::gen();
        bg->appendRect(0, 0, w, h, 0, 0);
        bg->fill(bg_color.red(), bg_color.green(), bg_color.blue(), bg_color.alpha());
        canvas.add(bg);

        // ── 装饰圆 ──────────────────────────────────────────
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

        // ── 标题卡片背景 ──────────────────────────────────────
        const float card_w = w * 0.39f;
        const float card_h = h * 0.39f;
        const auto card_pos = nandina::geometry::NanPoint{
            (w - card_w) * 0.5f, (h - card_h) * 0.5f
        };
        auto *card = tvg::Shape::gen();
        card->appendRect(card_pos.x(), card_pos.y(), card_w, card_h, 16, 16);
        card->fill(30, 30, 38, 255);
        canvas.add(card);

        // ── Layout 演示区 ──────────────────────────────────────
        draw_layout_demo(canvas);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

private:
    // ── 绘制一个圆角矩形块 ──────────────────────────────────
    static void draw_rect(tvg::SwCanvas &canvas,
                          const float x, const float y, const float w, const float h,
                          const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
                          const float corner = 4.0f) {
        auto *shape = tvg::Shape::gen();
        shape->appendRect(x, y, w, h, corner, corner);
        shape->fill(r, g, b, a);
        canvas.add(shape);
    }

    // ── 绘制布局演示 ──────────────────────────────────────
    void draw_layout_demo(tvg::SwCanvas &canvas) const {
        using namespace nandina::layout;
        using namespace nandina::geometry;

        const float demo_x  = 50.0f;
        const float demo_y  = 50.0f;
        const float demo_w  = 400.0f;

        // ── 演示 1: Column 布局 ──────────────────────────────
        // 三个不同大小的矩形垂直排列，带 gap 和对齐
        {
            BasicLayoutBackend backend;

            LayoutRequest req;
            req.axis = LayoutAxis::column;
            req.container_bounds = {demo_x, demo_y, demo_x + demo_w, demo_y + 140.0f};
            req.gap = 6.0f;
            req.cross_alignment = LayoutAlignment::center;
            req.children = {
                {NanSize{100.0f, 30.0f}, 0},
                {NanSize{200.0f, 30.0f}, 0},
                {NanSize{150.0f, 30.0f}, 0},
            };

            // 背景框
            auto bounds = req.container_bounds;
            draw_rect(canvas, bounds.x(), bounds.y(), bounds.width(), bounds.height(),
                      60, 63, 81, 200, 8);

            auto frames = backend.compute(req);
            const auto colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
                {99, 102, 241},   // 靛蓝
                {95, 200, 130},   // 绿色
                {245, 158, 60},   // 橙色
            });

            for (size_t i = 0; i < frames.size() && i < colors.size(); ++i) {
                const auto &[cr, cg, cb] = colors[i];
                draw_rect(canvas,
                          frames[i].x(), frames[i].y(),
                          frames[i].width(), frames[i].height(),
                          cr, cg, cb, 220, 6);
            }
        }

        // ── 演示 2: Row 布局（flex 分配）─────────────────────
        // 2 个固定 + 1 个 flex 水平排列
        {
            BasicLayoutBackend backend;

            LayoutRequest req;
            req.axis = LayoutAxis::row;
            req.container_bounds = {demo_x, demo_y + 155.0f, demo_x + demo_w, demo_y + 250.0f};
            req.gap = 8.0f;
            req.cross_alignment = LayoutAlignment::stretch;
            req.children = {
                {NanSize{60.0f, 0.0f}, 0},    // 固定 60px
                {NanSize{80.0f, 0.0f}, 0},    // 固定 80px
                {NanSize{0.0f, 0.0f}, 1},     // flex 1，填满剩余
            };

            auto bounds = req.container_bounds;
            draw_rect(canvas, bounds.x(), bounds.y(), bounds.width(), bounds.height(),
                      60, 63, 81, 200, 8);

            auto frames = backend.compute(req);
            const auto colors2 = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
                {99, 102, 241},
                {95, 200, 130},
                {236, 110, 130},  // 粉色
            });

            for (size_t i = 0; i < frames.size() && i < colors2.size(); ++i) {
                const auto &[cr, cg, cb] = colors2[i];
                draw_rect(canvas,
                          frames[i].x(), frames[i].y(),
                          frames[i].width(), frames[i].height(),
                          cr, cg, cb, 220, 6);
            }
        }

        // ── 演示 3: Stack 布局 ──────────────────────────────
        // 大矩形作为底，小矩形居中重叠
        {
            BasicLayoutBackend backend;

            LayoutRequest req;
            req.axis = LayoutAxis::stack;
            req.container_bounds = {demo_x + 420.0f, demo_y, demo_x + 560.0f, demo_y + 140.0f};
            req.cross_alignment = LayoutAlignment::center;
            req.main_alignment  = LayoutAlignment::center;
            req.children = {
                {NanSize{120.0f, 120.0f}, 0},
                {NanSize{80.0f, 80.0f}, 0},
                {NanSize{40.0f, 40.0f}, 0},
            };

            auto bounds = req.container_bounds;
            draw_rect(canvas, bounds.x(), bounds.y(), bounds.width(), bounds.height(),
                      60, 63, 81, 200, 8);

            auto frames = backend.compute(req);
            const auto colors3 = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
                {99, 102, 241},
                {95, 200, 130},
                {245, 158, 60},
            });

            for (size_t i = 0; i < frames.size() && i < colors3.size(); ++i) {
                const auto &[cr, cg, cb] = colors3[i];
                draw_rect(canvas,
                          frames[i].x(), frames[i].y(),
                          frames[i].width(), frames[i].height(),
                          cr, cg, cb, 200 - i * 30, 6);
            }
        }

        // ── 演示 4: 带 Padding 的 Column ────────────────────
        {
            BasicLayoutBackend backend;

            LayoutRequest req;
            req.axis = LayoutAxis::column;
            req.container_bounds = {demo_x + 420.0f, demo_y + 155.0f, demo_x + 560.0f, demo_y + 250.0f};
            req.padding = {8.0f, 8.0f, 8.0f, 8.0f};
            req.gap = 2.0f;
            req.cross_alignment = LayoutAlignment::stretch;
            req.children = {
                {NanSize{0.0f, 20.0f}, 0},
                {NanSize{0.0f, 20.0f}, 0},
                {NanSize{0.0f, 20.0f}, 0},
            };

            auto bounds = req.container_bounds;
            draw_rect(canvas, bounds.x(), bounds.y(), bounds.width(), bounds.height(),
                      60, 63, 81, 200, 8);

            auto frames = backend.compute(req);
            const auto colors4 = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
                {99, 102, 241},
                {95, 200, 130},
                {236, 110, 130},
            });

            for (size_t i = 0; i < frames.size() && i < colors4.size(); ++i) {
                const auto &[cr, cg, cb] = colors4[i];
                draw_rect(canvas,
                          frames[i].x(), frames[i].y(),
                          frames[i].width(), frames[i].height(),
                          cr, cg, cb, 220, 4);
            }
        }
    }

private:
    nandina::log::Logger logger = nandina::log::get("MainComponent");
    nandina::NanColor background_color;
    nandina::NanColor circle_color;
};

export class MainWindow final : public nandina::app::NanAppWindow {
public:
    MainWindow() : nandina::app::NanAppWindow({
        .title = "NandinaUI — Layout Showcase",
        .width = 1280,
        .height = 720,
        .resizable = true,
        .high_dpi = true
    }) {
        set_root_component(std::make_unique<MainComponent>());
    }

protected:
    void on_ready() override {
        m_log.info("MainWindow ready with layout demo");
    }

private:
    decltype(nandina::log::get("showcase.window")) m_log = nandina::log::get("showcase.window");
};