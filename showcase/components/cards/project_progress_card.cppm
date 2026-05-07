module;

#include <array>
#include <memory>
#include <string_view>
#include <tuple>

export module nandina.showcase.project_progress_card;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.layout.container;
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

class ProjectProgressRow final : public nandina::runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ProjectProgressRow>;

    static auto create(std::string_view label_text, std::string_view pct_text, float progress,
        nandina::NanColor label_color, nandina::NanColor pct_color, nandina::NanColor bar_color,
        nandina::NanColor track_color) -> Ptr {
        return Ptr{new ProjectProgressRow(label_text, pct_text, progress, label_color, pct_color, bar_color, track_color)};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        const float bar_x = x + w * 0.5f;
        const float bar_w = w * 0.42f;

        if (m_label) {
            m_label->set_bounds(x + 16.0f, y, 100.0f, h);
        }
        if (m_bar) {
            m_bar->set_bounds(bar_x, y + 1.0f, bar_w, 6.0f);
        }
        if (m_pct) {
            m_pct->set_bounds(bar_x + bar_w + 8.0f, y, 36.0f, h);
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 10.0f};
    }

private:
    ProjectProgressRow(std::string_view label_text, std::string_view pct_text, float progress,
        nandina::NanColor label_color, nandina::NanColor pct_color, nandina::NanColor bar_color,
        nandina::NanColor track_color) {
        using namespace nandina::widgets;

        auto label = Label::create();
        label->set_text(label_text)
            .set_font_size(7.0f)
            .set_color(label_color);
        m_label = add_child(std::move(label));

        auto bar = ProgressBar::create();
        bar->set_progress(progress)
            .set_bar_color(bar_color)
            .set_track_color(track_color)
            .set_bar_height(6.0f)
            .set_corner_radius(3.0f);
        m_bar = add_child(std::move(bar));

        auto pct = Label::create();
        pct->set_text(pct_text)
            .set_font_size(7.0f)
            .set_color(pct_color);
        m_pct = add_child(std::move(pct));
    }

    runtime::NanWidget* m_label{nullptr};
    runtime::NanWidget* m_bar{nullptr};
    runtime::NanWidget* m_pct{nullptr};
};

class ProjectProgressRows final : public nandina::runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ProjectProgressRows>;

    static auto create() -> Ptr {
        return Ptr{new ProjectProgressRows()};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_mounted_column) {
            m_mounted_column->set_bounds(x, y, w, h);
        }

        return *this;
    }

private:
    ProjectProgressRows() {
        auto nodes = nandina::app::children();

        const auto progress_data = std::to_array<std::tuple<std::string_view, float, uint8_t, uint8_t, uint8_t>>({
            {"Layout Engine", 0.85f, 99, 102, 241},
            {"Testing", 0.65f, 95, 200, 130},
            {"Documentation", 0.40f, 245, 158, 60},
            {"Performance", 0.30f, 236, 110, 130},
        });

        for (size_t i = 0; i < m_rows.size(); ++i) {
            const auto& [label_text, progress, r, g, b] = progress_data[i];
            char pct_buf[8];
            const int len = std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", static_cast<int>(progress * 100.0f));
            auto row = ProjectProgressRow::create(
                label_text,
                std::string_view{pct_buf, static_cast<size_t>(len)},
                progress,
                color4_to_nancolor(160, 162, 180),
                color4_to_nancolor(110, 112, 130),
                color4_to_nancolor(r, g, b),
                color4_to_nancolor(42, 44, 62));
            m_rows[i] = row.get();
            nodes.append(nandina::app::adopt(std::move(row)));
        }

        auto mounted = nandina::app::mount(
            nandina::app::column(std::move(nodes))
                .gap(8.0f)
                .align_items(nandina::layout::LayoutAlignment::stretch)
                .bind(m_column));
        m_mounted_column = mounted.get();
        add_child(std::move(mounted));
    }

    nandina::app::NanComponent* m_mounted_column{nullptr};
    nandina::app::Ref<nandina::layout::Column> m_column;
    std::array<nandina::runtime::NanWidget*, 4> m_rows{};
};

class ProjectProgressCard final : public runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<ProjectProgressCard>;

    static auto create() -> Ptr {
        return Ptr{new ProjectProgressCard()};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_mounted_content) {
            m_mounted_content->set_bounds(x, y, w, h);
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 110.0f};
    }

private:
    ProjectProgressCard() {
        auto rows = ProjectProgressRows::create();
        m_rows = rows.get();

        auto mounted = nandina::app::mount(
            nandina::app::stack(nandina::app::children(
                nandina::app::adopt(nandina::widgets::Surface::create())
                    .bg_color(color4_to_nancolor(50, 52, 72))
                    .corner_radius(8.0f),
                nandina::app::column(nandina::app::children(
                    nandina::app::padding(
                        nandina::app::sized_box(
                            nandina::app::label("Project Progress")
                                .font_size(9.0f)
                                .color(color4_to_nancolor(220, 220, 240))
                                .bind(m_title))
                            .width(160.0f)
                            .height(16.0f))
                        .padding(14.0f, 8.0f, 14.0f, 0.0f),
                    nandina::app::expanded(
                        nandina::app::adopt(std::move(rows)))))
                    .gap(10.0f)
                    .align_items(nandina::layout::LayoutAlignment::stretch)))
                .align_items(nandina::layout::LayoutAlignment::stretch)
                .justify_content(nandina::layout::LayoutAlignment::stretch));
        m_mounted_content = mounted.get();
        add_child(std::move(mounted));
    }

    nandina::app::NanComponent* m_mounted_content{nullptr};
    nandina::app::Ref<nandina::widgets::Label> m_title;
    ProjectProgressRows* m_rows{nullptr};
};

} // namespace nandina::showcase