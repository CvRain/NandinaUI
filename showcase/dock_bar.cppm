module;

#include <array>
#include <memory>
#include <tuple>
#include <thorvg-1/thorvg.h>

export module nandina.showcase.dock_bar;

import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.runtime.nan_widget;
import nandina.widgets.icon;
import nandina.widgets.label;
import nandina.widgets.surface;

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

class DockThumbnail final : public widgets::Surface {
public:
    using Ptr = std::unique_ptr<DockThumbnail>;

    static auto create() -> Ptr {
        return Ptr{new DockThumbnail()};
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {120.0f, 44.0f};
    }

protected:
    void on_draw(tvg::SwCanvas& canvas) override {
        Surface::on_draw(canvas);

        const auto rect = bounds();
        draw_rect(canvas, rect.x() + 6.0f, rect.y() + 6.0f, rect.width() - 12.0f, 6.0f, 70, 72, 92, 200, 2);
        draw_rect(canvas, rect.x() + 6.0f, rect.y() + 16.0f, rect.width() - 12.0f, 4.0f, 65, 67, 85, 180, 2);
        draw_rect(canvas, rect.x() + 6.0f, rect.y() + 24.0f, rect.width() - 12.0f, rect.height() - 30.0f, 60, 62, 80, 150, 2);
    }

private:
    DockThumbnail() {
        set_bg_color(color4_to_nancolor(55, 57, 75, 200))
            .set_corner_radius(4.0f);
    }
};

class DockIconItem final : public widgets::Surface {
public:
    using Ptr = std::unique_ptr<DockIconItem>;

    static auto create(const nandina::NanColor& bg_color,
        const nandina::widgets::IconType icon_type,
        const bool show_active_dot = false) -> Ptr {
        return Ptr{new DockIconItem(bg_color, icon_type, show_active_dot)};
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {32.0f, 32.0f};
    }

protected:
    void on_draw(tvg::SwCanvas& canvas) override {
        Surface::on_draw(canvas);

        if (!m_show_active_dot) {
            return;
        }

        const auto rect = bounds();
        const auto rgb = bg_color().to<nandina::NanRgb>();
        draw_circle(canvas, rect.center().x(), rect.y() + rect.height() + 4.0f, 2.0f, rgb.red(), rgb.green(), rgb.blue(), 220);
    }

private:
    DockIconItem(const nandina::NanColor& bg_color,
        const nandina::widgets::IconType icon_type,
        const bool show_active_dot)
        : m_show_active_dot(show_active_dot) {
        set_bg_color(bg_color)
            .set_corner_radius(7.0f);

        auto center = nandina::layout::Center::Create();
        auto icon = nandina::widgets::Icon::create();
        icon->set_type(icon_type)
            .set_size(16.0f)
            .set_color(color4_to_nancolor(255, 255, 255, 200));
        center->child(std::move(icon));
        add_child(std::move(center));
    }

    bool m_show_active_dot{false};
};

class DockBar final : public widgets::Surface {
public:
    using Ptr = std::unique_ptr<DockBar>;

    static auto create() -> Ptr {
        return Ptr{new DockBar()};
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 56.0f};
    }

protected:
    void on_draw(tvg::SwCanvas& canvas) override {
        Surface::on_draw(canvas);
        const auto rect = bounds();
        draw_line(canvas, rect.x(), rect.y(), rect.x() + rect.width(), rect.y(), 55, 57, 75, 120);
    }

private:
    DockBar() {
        set_bg_color(color4_to_nancolor(38, 40, 56, 255));

        auto overlay_row = nandina::layout::Row::Create();
        overlay_row->padding(12.0f, 6.0f, 12.0f, 6.0f)
            .align_items(nandina::layout::LayoutAlignment::center);
        overlay_row->add(DockThumbnail::create());
        overlay_row->add(nandina::layout::Spacer::Create());

        auto clock = nandina::widgets::Label::create();
        clock->set_text("10:42")
            .set_font_size(9.0f)
            .set_color(color4_to_nancolor(160, 162, 180));
        m_clock_label = static_cast<nandina::widgets::Label*>(clock.get());
        overlay_row->add(std::move(clock));

        auto icon_row = nandina::layout::Row::Create();
        icon_row->padding(16.0f, 0.0f, 16.0f, 0.0f)
            .align_items(nandina::layout::LayoutAlignment::center)
            .justify_content(nandina::layout::LayoutAlignment::center)
            .gap(12.0f);

        icon_row->add(DockIconItem::create(color4_to_nancolor(99, 102, 241), nandina::widgets::IconType::Circle, true));
        icon_row->add(DockIconItem::create(color4_to_nancolor(95, 200, 130), nandina::widgets::IconType::Dots, true));
        icon_row->add(DockIconItem::create(color4_to_nancolor(245, 158, 60), nandina::widgets::IconType::Triangle, true));
        icon_row->add(DockIconItem::create(color4_to_nancolor(236, 110, 130), nandina::widgets::IconType::Square));
        icon_row->add(DockIconItem::create(color4_to_nancolor(80, 200, 220), nandina::widgets::IconType::Check));
        icon_row->add(DockIconItem::create(color4_to_nancolor(180, 140, 240), nandina::widgets::IconType::ArrowUp));
        icon_row->add(DockIconItem::create(color4_to_nancolor(160, 162, 180), nandina::widgets::IconType::Dot));

        add_child(std::move(icon_row));
        add_child(std::move(overlay_row));
    }

    nandina::widgets::Label* m_clock_label{nullptr};
};

} // namespace nandina::showcase