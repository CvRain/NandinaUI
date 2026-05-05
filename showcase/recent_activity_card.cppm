module;

#include <array>
#include <cmath>
#include <memory>
#include <string_view>
#include <tuple>
#include <thorvg-1/thorvg.h>

export module nandina.showcase.recent_activity_card;

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

static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class ActivityRow final : public nandina::runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ActivityRow>;

    static auto create(std::string_view text, std::string_view time, nandina::NanColor text_color,
        nandina::NanColor time_color, nandina::NanColor dot_color) -> Ptr {
        return Ptr{new ActivityRow(text, time, text_color, time_color, dot_color)};
    }

    auto set_pulse_phase(const float phase) noexcept -> void {
        m_pulse_phase = phase;
        mark_dirty();
    }

    auto set_pulse(float pulse) noexcept -> void {
        m_pulse = pulse;
        mark_dirty();
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_text) {
            m_text->set_bounds(x + 20.0f, y + 2.0f, 200.0f, 16.0f);
        }
        if (m_time) {
            m_time->set_bounds(x + 20.0f, y + 16.0f, 120.0f, 12.0f);
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 28.0f};
    }

protected:
    void on_draw(tvg::SwCanvas& canvas) override {
        const auto rect = bounds();
        const auto rgb = m_dot_color.to<nandina::NanRgb>();
        const float pulse_alpha = 150.0f + 70.0f * (0.5f + 0.5f * std::sin(m_pulse * 6.2831853f + m_pulse_phase));
        draw_circle(canvas, rect.x() + 6.0f, rect.y() + 12.0f, 4.0f,
            rgb.red(), rgb.green(), rgb.blue(), static_cast<uint8_t>(pulse_alpha));
    }

private:
    ActivityRow(std::string_view text, std::string_view time, nandina::NanColor text_color,
        nandina::NanColor time_color, nandina::NanColor dot_color)
        : m_dot_color(dot_color) {
        auto text_label = nandina::widgets::Label::create();
        text_label->set_text(text)
            .set_font_size(8.0f)
            .set_color(text_color);
        m_text = add_child(std::move(text_label));

        auto time_label = nandina::widgets::Label::create();
        time_label->set_text(time)
            .set_font_size(6.0f)
            .set_color(time_color);
        m_time = add_child(std::move(time_label));
    }

    nandina::runtime::NanWidget* m_text{nullptr};
    nandina::runtime::NanWidget* m_time{nullptr};
    nandina::NanColor m_dot_color{};
    float m_pulse{0.0f};
    float m_pulse_phase{0.0f};
};

class ActivityList final : public nandina::runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ActivityList>;

    static auto create() -> Ptr {
        return Ptr{new ActivityList()};
    }

    auto set_pulse(float pulse) noexcept -> void {
        for (auto* child : m_rows) {
            if (auto* row = dynamic_cast<ActivityRow*>(child)) {
                row->set_pulse(pulse);
            }
        }
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_column) {
            m_column->set_bounds(x, y, w, h);
        }

        return *this;
    }

private:
    ActivityList() {
        auto column = nandina::layout::Column::Create();
        column->gap(2.0f)
            .align_items(nandina::layout::LayoutAlignment::stretch);

        constexpr auto texts = std::to_array<std::string_view>({
            "Updated layout-core", "Merged PR #42", "Fixed flex alignment", "Added Stack support", "Refactored backend"});
        constexpr auto times = std::to_array<std::string_view>({"2 min ago", "15 min ago", "1 hr ago", "3 hr ago", "6 hr ago"});
        constexpr auto colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
            std::tuple{95, 200, 130},
            std::tuple{99, 102, 241},
            std::tuple{236, 110, 130},
            std::tuple{245, 158, 60},
            std::tuple{80, 200, 220},
        });

        for (size_t i = 0; i < m_rows.size(); ++i) {
            const auto& [r, g, b] = colors[i];
            auto row = ActivityRow::create(
                texts[i],
                times[i],
                color4_to_nancolor(160, 162, 180),
                color4_to_nancolor(110, 112, 130),
                color4_to_nancolor(r, g, b));
            row->set_pulse_phase(static_cast<float>(i) * 0.15f);
            m_rows[i] = row.get();
            column->add(std::move(row));
        }

        m_column = column.get();
        add_child(std::move(column));
    }

    nandina::layout::Column* m_column{nullptr};
    std::array<nandina::runtime::NanWidget*, 5> m_rows{};
};

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

        if (m_activity_list) {
            m_activity_list->set_bounds(x + 16.0f, y + 36.0f, w - 32.0f, h - 36.0f);
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
    }

private:
    RecentActivityCard() {
        auto title = nandina::widgets::Label::create();
        title->set_text("Recent Activity")
            .set_font_size(10.0f)
            .set_color(color4_to_nancolor(220, 220, 240));
        m_title = add_child(std::move(title));

        m_activity_list = add_child(ActivityList::create());
    }

    nandina::runtime::NanWidget* m_title{nullptr};
    nandina::runtime::NanWidget* m_activity_list{nullptr};
    float m_pulse{0.0f};
};

} // namespace nandina::showcase