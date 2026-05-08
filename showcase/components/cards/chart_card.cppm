module;

#include <array>
#include <memory>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.showcase.chart_card;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.layout.container;
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

    class ChartDayStrip final : public nandina::runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<ChartDayStrip>;

        static auto create() -> Ptr {
            return Ptr{new ChartDayStrip()};
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);

            const float step = w / 6.0f;
            for (size_t i = 0; i < m_day_labels.size(); ++i) {
                if (m_day_labels[i]) {
                    const float label_x = x + static_cast<float>(i) * step - 10.0f;
                    m_day_labels[i]->set_bounds(label_x, y, 24.0f, h);
                }
            }

            return *this;
        }

    private:
        ChartDayStrip() {
            constexpr auto days = std::to_array<std::string_view>({"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"});
            for (size_t i = 0; i < m_day_labels.size(); ++i) {
                auto label = nandina::widgets::Label::create();
                label->set_text(days[i])
                    .set_font_size(7.0f)
                    .set_color(color4_to_nancolor(110, 112, 130));
                m_day_labels[i] = add_child(std::move(label));
            }
        }

        std::array<nandina::runtime::NanWidget*, 7> m_day_labels{};
    };

    class ChartCard final : public nandina::runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<ChartCard>;

        static auto create() -> Ptr {
            return Ptr{new ChartCard()};
        }

        auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);

            if (m_mounted_content) {
                m_mounted_content->set_bounds(x, y, w, h);
            }

            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return {0.0f, 200.0f};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect = bounds();
            draw_rect(canvas, rect.x(), rect.y(), rect.width(), rect.height(), 50, 52, 72, 255, 8.0f);

            const float plot_x = rect.x() + 20.0f;
            const float plot_y = rect.y() + 36.0f;
            const float plot_w = rect.width() - 40.0f;
            const float plot_h = rect.height() - 56.0f;

            for (int i = 0; i < 4; ++i) {
                const float gy = plot_y + plot_h * static_cast<float>(i) / 3.0f;
                draw_line(canvas, plot_x, gy, plot_x + plot_w, gy, 55, 57, 75, 100, 1.0f);
            }

            constexpr std::array<float, 7> data = {0.3f, 0.6f, 0.45f, 0.8f, 0.65f, 0.9f, 0.7f};
            const float step                    = plot_w / static_cast<float>(data.size() - 1);

            auto* area = tvg::Shape::gen();
            area->moveTo(plot_x, plot_y + plot_h);
            for (size_t i = 0; i < data.size(); ++i) {
                const float px = plot_x + static_cast<float>(i) * step;
                const float py = plot_y + plot_h * (1.0f - data[i] * 0.85f);
                area->lineTo(px, py);
            }
            area->lineTo(plot_x + plot_w, plot_y + plot_h);
            area->close();
            area->fill(99, 102, 241, 40);
            canvas.add(area);

            auto* line = tvg::Shape::gen();
            for (size_t i = 0; i < data.size(); ++i) {
                const float px = plot_x + static_cast<float>(i) * step;
                const float py = plot_y + plot_h * (1.0f - data[i] * 0.85f);
                if (i == 0) {
                    line->moveTo(px, py);
                } else {
                    line->lineTo(px, py);
                }
            }
            line->strokeWidth(2.5f);
            line->strokeFill(99, 102, 241, 220);
            canvas.add(line);

            for (size_t i = 0; i < data.size(); ++i) {
                const float px = plot_x + static_cast<float>(i) * step;
                const float py = plot_y + plot_h * (1.0f - data[i] * 0.85f);
                draw_circle(canvas, px, py, 3.0f, 99, 102, 241, 220);
                draw_circle(canvas, px, py, 5.0f, 99, 102, 241, 80);
            }
        }

    private:
        ChartCard() {
            auto day_strip = ChartDayStrip::create();
            m_day_strip    = day_strip.get();

            auto mounted = nandina::app::mount(
                nandina::app::stack(nandina::app::children(
                    nandina::app::padding(
                        nandina::app::sized_box(
                            nandina::app::label("Weekly Activity")
                            .font_size(10.0f)
                            .color(color4_to_nancolor(220, 220, 240))
                            .bind(m_title))
                        .width(160.0f)
                        .height(18.0f))
                    .padding(16.0f, 8.0f, 16.0f, 0.0f),
                    nandina::app::padding(
                        nandina::app::column(nandina::app::children(
                            nandina::app::spacer(),
                            nandina::app::sized_box(
                                nandina::app::adopt(std::move(day_strip)))
                            .height(14.0f)))
                        .align_items(nandina::layout::LayoutAlignment::stretch))
                    .padding(20.0f, 0.0f, 20.0f, 2.0f)))
                .align_items(nandina::layout::LayoutAlignment::stretch)
                .justify_content(nandina::layout::LayoutAlignment::stretch));
            m_mounted_content = mounted.get();
            add_child(std::move(mounted));
        }

        nandina::app::NanComponent* m_mounted_content{nullptr};
        nandina::app::Ref<nandina::widgets::Label> m_title;
        ChartDayStrip* m_day_strip{nullptr};
    };

} // namespace nandina::showcase
