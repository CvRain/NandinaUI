module;

#include <array>
#include <memory>
#include <string_view>
#include <tuple>

export module nandina.showcase.project_progress_card;

import nandina.foundation.color;
import nandina.layout.core;
import nandina.runtime.nan_widget;
import nandina.widgets.label;
import nandina.widgets.progressbar;
import nandina.widgets.surface;

namespace {
static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class ProjectProgressCard final : public runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ProjectProgressCard>;

    static auto create() -> Ptr {
        return Ptr{new ProjectProgressCard()};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_container) {
            m_container->set_bounds(x, y, w, h);
        }
        if (m_title) {
            m_title->set_bounds(x + 14.0f, y + 8.0f, 160.0f, 16.0f);
        }

        const float label_x = x + 16.0f;
        const float bar_x = x + w * 0.5f;
        const float bar_w = w * 0.42f;

        for (size_t i = 0; i < m_rows.size(); ++i) {
            const float row_y = y + 34.0f + static_cast<float>(i) * 18.0f;
            if (m_rows[i].label) {
                m_rows[i].label->set_bounds(label_x, row_y, 100.0f, 10.0f);
            }
            if (m_rows[i].bar) {
                m_rows[i].bar->set_bounds(bar_x, row_y + 1.0f, bar_w, 6.0f);
            }
            if (m_rows[i].pct) {
                m_rows[i].pct->set_bounds(bar_x + bar_w + 8.0f, row_y, 36.0f, 10.0f);
            }
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 110.0f};
    }

private:
    struct ProgressRow {
        runtime::NanWidget* label{nullptr};
        runtime::NanWidget* bar{nullptr};
        runtime::NanWidget* pct{nullptr};
    };

    ProjectProgressCard() {
        using namespace nandina::widgets;

        auto container = Surface::create();
        container->set_bg_color(color4_to_nancolor(50, 52, 72))
            .set_corner_radius(8.0f);
        m_container = add_child(std::move(container));

        auto title = Label::create();
        title->set_text("Project Progress")
            .set_font_size(9.0f)
            .set_color(color4_to_nancolor(220, 220, 240));
        m_title = add_child(std::move(title));

        const auto progress_data = std::to_array<std::tuple<std::string_view, float, uint8_t, uint8_t, uint8_t>>({
            {"Layout Engine", 0.85f, 99, 102, 241},
            {"Testing", 0.65f, 95, 200, 130},
            {"Documentation", 0.40f, 245, 158, 60},
            {"Performance", 0.30f, 236, 110, 130},
        });

        for (size_t i = 0; i < m_rows.size(); ++i) {
            const auto& [label_text, progress, r, g, b] = progress_data[i];

            auto label = Label::create();
            label->set_text(label_text)
                .set_font_size(7.0f)
                .set_color(color4_to_nancolor(160, 162, 180));
            m_rows[i].label = add_child(std::move(label));

            auto bar = ProgressBar::create();
            bar->set_progress(progress)
                .set_bar_color(color4_to_nancolor(r, g, b))
                .set_track_color(color4_to_nancolor(42, 44, 62))
                .set_bar_height(6.0f)
                .set_corner_radius(3.0f);
            m_rows[i].bar = add_child(std::move(bar));

            char pct_buf[8];
            const int len = std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", static_cast<int>(progress * 100.0f));
            auto pct = Label::create();
            pct->set_text(std::string_view{pct_buf, static_cast<size_t>(len)})
                .set_font_size(7.0f)
                .set_color(color4_to_nancolor(110, 112, 130));
            m_rows[i].pct = add_child(std::move(pct));
        }
    }

    runtime::NanWidget* m_container{nullptr};
    runtime::NanWidget* m_title{nullptr};
    std::array<ProgressRow, 4> m_rows{};
};

} // namespace nandina::showcase