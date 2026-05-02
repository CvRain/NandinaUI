module;

#include <array>
#include <memory>
#include <tuple>
#include <thorvg-1/thorvg.h>

export module nandina.showcase.dock_bar;

import nandina.foundation.color;
import nandina.layout.core;
import nandina.runtime.nan_widget;
import nandina.widgets.label;

namespace {
static void draw_rect(tvg::SwCanvas& canvas,
    const float x, const float y, const float w, const float h,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
    const float corner = 4.0f) {
    auto* shape = tvg::Shape::gen();
    shape->appendRect(x, y, w, h, corner, corner);
    shape->fill(r, g, b, a);
    canvas.add(shape);
}

static void draw_circle(tvg::SwCanvas& canvas,
    const float cx, const float cy, const float radius,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) {
    auto* shape = tvg::Shape::gen();
    shape->appendCircle(cx, cy, radius, radius);
    shape->fill(r, g, b, a);
    canvas.add(shape);
}

static void draw_line(tvg::SwCanvas& canvas,
    const float x1, const float y1, const float x2, const float y2,
    const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a,
    const float width = 1.0f) {
    auto* shape = tvg::Shape::gen();
    shape->moveTo(x1, y1);
    shape->lineTo(x2, y2);
    shape->strokeWidth(width);
    shape->strokeFill(r, g, b, a);
    canvas.add(shape);
}

static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class DockBar final : public runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<DockBar>;

    static auto create() -> Ptr {
        return Ptr{new DockBar()};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_clock_label) {
            m_clock_label->set_bounds(x + w - 60.0f, y + (h * 0.5f - 4.0f), 50.0f, 14.0f);
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 56.0f};
    }

protected:
    void on_draw(tvg::SwCanvas& canvas) override {
        const auto rect = bounds();
        draw_rect(canvas, rect.x(), rect.y(), rect.width(), rect.height(), 38, 40, 56, 255);
        draw_line(canvas, rect.x(), rect.y(), rect.x() + rect.width(), rect.y(), 55, 57, 75, 120);

        using namespace nandina::layout;

        BasicLayoutBackend backend;
        LayoutRequest req;
        req.axis             = LayoutAxis::row;
        req.container_bounds = {rect.x() + 16.0f, rect.y(), rect.x() + rect.width() - 16.0f, rect.y() + rect.height()};
        req.cross_alignment  = LayoutAlignment::center;
        req.main_alignment   = LayoutAlignment::center;
        req.gap              = 12.0f;

        constexpr size_t dock_count = 7;
        const float icon_size = 32.0f;
        for (size_t i = 0; i < dock_count; ++i) {
            LayoutChildSpec child;
            child.preferred_size = {icon_size, icon_size};
            child.flex_factor    = 0;
            req.children.push_back(child);
        }

        const auto frames = backend.compute(req);
        constexpr auto dock_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
            {99, 102, 241},
            {95, 200, 130},
            {245, 158, 60},
            {236, 110, 130},
            {80, 200, 220},
            {180, 140, 240},
            {160, 162, 180},
        });

        for (size_t i = 0; i < frames.size() && i < dock_colors.size(); ++i) {
            const auto frame = frames[i];
            const auto& [cr, cg, cb] = dock_colors[i];

            draw_rect(canvas, frame.x(), frame.y(), frame.width(), frame.height(), cr, cg, cb, 200, 7);

            if (i == 0) {
                draw_circle(canvas, frame.center().x(), frame.center().y() - 3.0f, 5.0f, 255, 255, 255, 200);
                draw_circle(canvas, frame.center().x() - 4.0f, frame.center().y() - 5.0f, 1.5f, 255, 255, 255, 200);
                draw_circle(canvas, frame.center().x() + 4.0f, frame.center().y() - 5.0f, 1.5f, 255, 255, 255, 200);
            } else if (i == 2) {
                auto* icon = tvg::Shape::gen();
                const float cx = frame.center().x();
                const float cy = frame.center().y();
                const float s = 6.0f;
                icon->moveTo(cx - s * 0.6f, cy - s);
                icon->lineTo(cx + s * 0.6f, cy);
                icon->lineTo(cx - s * 0.6f, cy + s);
                icon->close();
                icon->fill(255, 255, 255, 200);
                canvas.add(icon);
            } else if (i == 3) {
                draw_rect(canvas, frame.center().x() - 7.0f, frame.center().y() - 5.0f, 14.0f, 10.0f, 255, 255, 255, 200, 2.0f);
            } else if (i == 4) {
                auto* check = tvg::Shape::gen();
                check->moveTo(frame.center().x() - 6.0f, frame.center().y());
                check->lineTo(frame.center().x() - 2.0f, frame.center().y() + 5.0f);
                check->lineTo(frame.center().x() + 6.0f, frame.center().y() - 4.0f);
                check->strokeWidth(2.5f);
                check->strokeFill(255, 255, 255, 200);
                canvas.add(check);
            } else if (i == 5) {
                auto* arrow = tvg::Shape::gen();
                const float cx = frame.center().x();
                const float cy = frame.center().y();
                arrow->moveTo(cx, cy - 6.0f);
                arrow->lineTo(cx, cy + 6.0f);
                arrow->moveTo(cx - 5.0f, cy - 1.0f);
                arrow->lineTo(cx, cy - 6.0f);
                arrow->lineTo(cx + 5.0f, cy - 1.0f);
                arrow->strokeWidth(2.5f);
                arrow->strokeFill(255, 255, 255, 200);
                canvas.add(arrow);
            }

            if (i < 3) {
                draw_circle(canvas, frame.center().x(), frame.y() + frame.height() + 4.0f, 2.0f, cr, cg, cb, 220);
            }
        }

        const float thumb_w = 120.0f;
        const float thumb_h = rect.height() - 12.0f;
        const float thumb_x = rect.x() + 12.0f;
        const float thumb_y = rect.y() + 6.0f;
        draw_rect(canvas, thumb_x, thumb_y, thumb_w, thumb_h, 55, 57, 75, 200, 4);
        draw_rect(canvas, thumb_x + 6.0f, thumb_y + 6.0f, thumb_w - 12.0f, 6.0f, 70, 72, 92, 200, 2);
        draw_rect(canvas, thumb_x + 6.0f, thumb_y + 16.0f, thumb_w - 12.0f, 4.0f, 65, 67, 85, 180, 2);
        draw_rect(canvas, thumb_x + 6.0f, thumb_y + 24.0f, thumb_w - 12.0f, thumb_h - 30.0f, 60, 62, 80, 150, 2);
    }

private:
    DockBar() {
        auto clock = nandina::widgets::Label::create();
        clock->set_text("10:42")
            .set_font_size(9.0f)
            .set_color(color4_to_nancolor(160, 162, 180));
        m_clock_label = static_cast<nandina::widgets::Label*>(add_child(std::move(clock)));
    }

    nandina::widgets::Label* m_clock_label{nullptr};
};

} // namespace nandina::showcase