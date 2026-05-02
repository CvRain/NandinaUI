module;

#include <array>
#include <memory>
#include <tuple>

export module nandina.showcase.stack_demo_card;

import nandina.foundation.color;
import nandina.layout.core;
import nandina.runtime.nan_widget;
import nandina.widgets.label;
import nandina.widgets.surface;

namespace {
static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class StackDemoCard final : public runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<StackDemoCard>;

    static auto create() -> Ptr {
        return Ptr{new StackDemoCard()};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        if (m_container) {
            m_container->set_bounds(x, y, w, h);
        }
        if (m_title) {
            m_title->set_bounds(x + 14.0f, y + 14.0f, 200.0f, 16.0f);
        }
        if (m_footer) {
            m_footer->set_bounds(x + 14.0f, y + h - 14.0f, w - 28.0f, 12.0f);
        }

        using namespace nandina::layout;

        BasicLayoutBackend stack_backend;
        LayoutRequest stack_req;
        stack_req.axis             = LayoutAxis::stack;
        stack_req.container_bounds = {x + 20.0f, y + 30.0f, x + w - 20.0f, y + h - 16.0f};
        stack_req.cross_alignment  = LayoutAlignment::center;
        stack_req.main_alignment   = LayoutAlignment::center;

        {
            LayoutChildSpec child;
            child.preferred_size = {140.0f, 60.0f};
            stack_req.children.push_back(child);
        }
        {
            LayoutChildSpec child;
            child.preferred_size = {90.0f, 40.0f};
            stack_req.children.push_back(child);
        }
        {
            LayoutChildSpec child;
            child.preferred_size = {50.0f, 20.0f};
            stack_req.children.push_back(child);
        }

        const auto frames = stack_backend.compute(stack_req);
        for (size_t i = 0; i < frames.size() && i < m_boxes.size(); ++i) {
            const auto& frame = frames[i];
            if (m_boxes[i]) {
                m_boxes[i]->set_bounds(frame.x(), frame.y(), frame.width(), frame.height());
            }
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 110.0f};
    }

private:
    StackDemoCard() {
        using namespace nandina::widgets;

        auto container = Surface::create();
        container->set_bg_color(color4_to_nancolor(50, 52, 72))
            .set_corner_radius(8.0f);
        m_container = add_child(std::move(container));

        auto title = Label::create();
        title->set_text("Stack Layout Demo")
            .set_font_size(9.0f)
            .set_color(color4_to_nancolor(220, 220, 240));
        m_title = add_child(std::move(title));

        constexpr auto stack_box_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>>({
            {99, 102, 241, 180},
            {147, 150, 255, 200},
            {200, 200, 255, 220},
        });
        for (size_t i = 0; i < m_boxes.size(); ++i) {
            const auto& [r, g, b, a] = stack_box_colors[i];
            auto box = Surface::create();
            box->set_bg_color(color4_to_nancolor(r, g, b, a))
                .set_corner_radius(8.0f);
            m_boxes[i] = add_child(std::move(box));
        }

        auto footer = Label::create();
        footer->set_text("Column · Row · Stack — all from BasicLayoutBackend")
            .set_font_size(6.0f)
            .set_color(color4_to_nancolor(110, 112, 130));
        m_footer = add_child(std::move(footer));
    }

    runtime::NanWidget* m_container{nullptr};
    runtime::NanWidget* m_title{nullptr};
    std::array<runtime::NanWidget*, 3> m_boxes{};
    runtime::NanWidget* m_footer{nullptr};
};

} // namespace nandina::showcase