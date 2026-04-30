module;

#include <memory>
#include <string>
#include <string_view>
#include <algorithm>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.progressbar;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.prop;

export namespace nandina::widgets {

    class ProgressBar : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<ProgressBar>;

        ~ProgressBar() override = default;

        static auto create() -> Ptr {
            return Ptr{new ProgressBar()};
        }

        auto set_progress(float p) -> ProgressBar& {
            m_progress.set(std::clamp(p, 0.0f, 1.0f));
            mark_dirty();
            return *this;
        }

        auto set_bar_color(const nandina::NanColor& color) -> ProgressBar& {
            m_bar_color.set(color);
            mark_dirty();
            return *this;
        }

        auto set_track_color(const nandina::NanColor& color) -> ProgressBar& {
            m_track_color.set(color);
            mark_dirty();
            return *this;
        }

        auto set_bar_height(float h) -> ProgressBar& {
            m_bar_height.set(h);
            mark_dirty();
            return *this;
        }

        auto set_corner_radius(float r) -> ProgressBar& {
            m_corner_radius.set(r);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto progress() const noexcept -> float {
            return m_progress.get();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return geometry::NanSize{0.0f, m_bar_height.get()};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect = bounds();
            const float bar_h = m_bar_height.get();
            const float radius = m_corner_radius.get();
            const float pct = m_progress.get();

            const float bar_y = rect.y() + (rect.height() - bar_h) * 0.5f;

            const auto& track = m_track_color.get();
            const auto track_rgb = track.to<nandina::NanRgb>();
            auto* bg = tvg::Shape::gen();
            bg->appendRect(rect.x(), bar_y, rect.width(), bar_h, radius, radius);
            bg->fill(track_rgb.red(), track_rgb.green(), track_rgb.blue(), track_rgb.alpha());
            canvas.add(bg);

            if (pct > 0.0f) {
                const float fill_w = rect.width() * pct;
                const auto& bar = m_bar_color.get();
                const auto bar_rgb = bar.to<nandina::NanRgb>();
                auto* fill = tvg::Shape::gen();
                fill->appendRect(rect.x(), bar_y, fill_w, bar_h, radius, radius);
                fill->fill(bar_rgb.red(), bar_rgb.green(), bar_rgb.blue(), bar_rgb.alpha());
                canvas.add(fill);
            }
        }

    private:
        ProgressBar() = default;

        reactive::Prop<float> m_progress{0.0f};
        reactive::Prop<float> m_bar_height{6.0f};
        reactive::Prop<float> m_corner_radius{3.0f};
        reactive::Prop<nandina::NanColor> m_bar_color{nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        reactive::Prop<nandina::NanColor> m_track_color{nandina::NanColor::from(nandina::NanRgb{42, 44, 62})};
    };

} // namespace nandina::widgets
