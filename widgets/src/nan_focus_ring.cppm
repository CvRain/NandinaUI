module;

#include <algorithm>
#include <memory>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.focus_ring;

import nandina.runtime.nan_widget;
import nandina.foundation.color;
import nandina.theme.nan_style;

export namespace nandina::widgets {

    class FocusRing final : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<FocusRing>;

        ~FocusRing() override = default;

        static auto create() -> Ptr {
            return Ptr{new FocusRing()};
        }

        auto set_active(const bool active) -> FocusRing& {
            if (m_active == active) {
                return *this;
            }

            m_active = active;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto active() const noexcept -> bool {
            return m_active;
        }

        auto set_color(const NanColor& color) -> FocusRing& {
            m_color = color;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto color() const noexcept -> const NanColor& {
            return m_color;
        }

        auto set_ring_width(const float width) -> FocusRing& {
            m_ring_width = std::max(0.0f, width);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto ring_width() const noexcept -> float {
            return m_ring_width;
        }

        auto set_offset(const float offset) -> FocusRing& {
            m_offset = std::max(0.0f, offset);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto offset() const noexcept -> float {
            return m_offset;
        }

        auto set_corner_radius(const float radius) -> FocusRing& {
            m_corner_radius = std::max(0.0f, radius);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto corner_radius() const noexcept -> float {
            return m_corner_radius;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return geometry::NanSize{};
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            set_measured_layout_state(constraints, constraints.constrain(geometry::NanSize{}));
        }

        auto layout() -> void override {
            clear_layout_dirty();
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            if (!m_active || m_ring_width <= 0.0f) {
                return;
            }

            const auto rgb = m_color.to<NanRgb>();
            const float ring_x = x() - m_offset;
            const float ring_y = y() - m_offset;
            const float ring_w = width() + m_offset * 2.0f;
            const float ring_h = height() + m_offset * 2.0f;

            auto* ring = tvg::Shape::gen();
            ring->appendRect(ring_x, ring_y, ring_w, ring_h, m_corner_radius, m_corner_radius);
            ring->strokeWidth(m_ring_width);
            ring->strokeFill(rgb.red(), rgb.green(), rgb.blue(), rgb.alpha());
            canvas.add(ring);
        }

    private:
        FocusRing() {
            const auto& style = theme::NanStylePrimitives::current().focus_ring;
            m_color = style.color;
            m_ring_width = style.width;
            m_offset = style.offset;
            set_hit_test_visible(false);
        }

        NanColor m_color{};
        float m_ring_width{2.0f};
        float m_offset{2.0f};
        float m_corner_radius{0.0f};
        bool m_active{false};
    };

} // namespace nandina::widgets