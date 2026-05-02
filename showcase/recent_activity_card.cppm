module;

#include <array>
#include <cmath>
#include <memory>
#include <string_view>
#include <tuple>
#include <thorvg-1/thorvg.h>

export module nandina.showcase.recent_activity_card;

import nandina.foundation.color;
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

static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class RecentActivityCard final : public nandina::runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<RecentActivityCard>;

    static auto create() -> Ptr {
        return Ptr{new RecentActivityCard()};
    }

    auto set_pulse(float pulse) noexcept -> void {
        m_pulse = pulse;
        mark_dirty();
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_title) {
            m_title->set_bounds(x + 16.0f, y + 8.0f, 160.0f, 18.0f);
        }

        for (size_t i = 0; i < m_activity_texts.size(); ++i) {
            const float row_y = y + 36.0f + static_cast<float>(i) * 30.0f;
            if (m_activity_texts[i]) {
                m_activity_texts[i]->set_bounds(x + 36.0f, row_y + 2.0f, 200.0f, 16.0f);
            }
            if (m_activity_times[i]) {
                m_activity_times[i]->set_bounds(x + 36.0f, row_y + 16.0f, 120.0f, 12.0f);
            }
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

        constexpr auto colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
            std::tuple{95, 200, 130},
            std::tuple{99, 102, 241},
            std::tuple{236, 110, 130},
            std::tuple{245, 158, 60},
            std::tuple{80, 200, 220},
        });

        for (size_t i = 0; i < colors.size(); ++i) {
            const float row_y = rect.y() + 36.0f + static_cast<float>(i) * 30.0f;
            const auto& [r, g, b] = colors[i];
            const float pulse_offset = static_cast<float>(i) * 0.15f;
            const float pulse_alpha = 150.0f + 70.0f * (0.5f + 0.5f * std::sin(m_pulse * 6.2831853f + pulse_offset));
            draw_circle(canvas, rect.x() + 22.0f, row_y + 12.0f, 4.0f, r, g, b, static_cast<uint8_t>(pulse_alpha));
        }
    }

private:
    RecentActivityCard() {
        auto title = nandina::widgets::Label::create();
        title->set_text("Recent Activity")
            .set_font_size(10.0f)
            .set_color(color4_to_nancolor(220, 220, 240));
        m_title = add_child(std::move(title));

        constexpr auto texts = std::to_array<std::string_view>({
            "Updated layout-core", "Merged PR #42", "Fixed flex alignment", "Added Stack support", "Refactored backend"});
        constexpr auto times = std::to_array<std::string_view>({"2 min ago", "15 min ago", "1 hr ago", "3 hr ago", "6 hr ago"});

        for (size_t i = 0; i < m_activity_texts.size(); ++i) {
            auto text = nandina::widgets::Label::create();
            text->set_text(texts[i])
                .set_font_size(8.0f)
                .set_color(color4_to_nancolor(160, 162, 180));
            m_activity_texts[i] = add_child(std::move(text));

            auto time = nandina::widgets::Label::create();
            time->set_text(times[i])
                .set_font_size(6.0f)
                .set_color(color4_to_nancolor(110, 112, 130));
            m_activity_times[i] = add_child(std::move(time));
        }
    }

    nandina::runtime::NanWidget* m_title{nullptr};
    std::array<nandina::runtime::NanWidget*, 5> m_activity_texts{};
    std::array<nandina::runtime::NanWidget*, 5> m_activity_times{};
    float m_pulse{0.0f};
};

} // namespace nandina::showcase