module;

#include <array>
#include <memory>
#include <string_view>
#include <tuple>

export module nandina.showcase.stats_section;

import nandina.foundation.color;
import nandina.layout.core;
import nandina.log;
import nandina.runtime.nan_widget;
import nandina.widgets.card;
import nandina.widgets.label;
import nandina.widgets.pressable;

namespace {
static auto color4_to_nancolor(const uint8_t r, const uint8_t g, const uint8_t b,
    const uint8_t a = 255) -> nandina::NanColor {
    return nandina::NanColor::from(nandina::NanRgb{r, g, b, a});
}
}

export namespace nandina::showcase {

class StatsSection final : public runtime::NanWidget {
public:
    using Ptr = std::unique_ptr<StatsSection>;

    static auto create() -> Ptr {
        return Ptr{new StatsSection()};
    }

    auto set_bounds(const float x, const float y, const float w, const float h) noexcept -> NanWidget& override {
        NanWidget::set_bounds(x, y, w, h);

        using namespace nandina::layout;

        BasicLayoutBackend backend;
        LayoutRequest row;
        row.axis             = LayoutAxis::row;
        row.container_bounds = {x, y, x + w, y + h};
        row.gap              = 16.0f;
        row.cross_alignment  = LayoutAlignment::stretch;

        const float card_width = (w - row.gap * 3.0f) / 4.0f;
        for (size_t i = 0; i < 4; ++i) {
            LayoutChildSpec child;
            child.preferred_size = {card_width, h};
            child.flex_factor    = 1;
            row.children.push_back(child);
        }

        const auto frames = backend.compute(row);
        for (size_t i = 0; i < frames.size() && i < m_cards.size(); ++i) {
            const auto& frame = frames[i];
            if (m_cards[i]) {
                m_cards[i]->set_bounds(frame.x(), frame.y(), frame.width(), frame.height());
            }
            if (m_pressables[i]) {
                m_pressables[i]->set_bounds(frame.x(), frame.y(), frame.width(), frame.height());
            }
        }

        return *this;
    }

    [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
        return {0.0f, 100.0f};
    }

private:
    StatsSection() {
        using namespace nandina::widgets;

        const auto card_title_texts = std::to_array<std::string_view>({"Total Users", "Active Now", "Revenue", "Tasks"});
        const auto card_value_texts = std::to_array<std::string_view>({"2,847", "143", "$12.4k", "18/24"});
        constexpr auto card_colors = std::to_array<std::tuple<uint8_t, uint8_t, uint8_t>>({
            {99, 102, 241},
            {95, 200, 130},
            {245, 158, 60},
            {236, 110, 130},
        });

        for (size_t i = 0; i < 4; ++i) {
            const auto& [cr, cg, cb] = card_colors[i];

            auto card = Card::create();
            card->set_bg_color(color4_to_nancolor(50, 52, 72))
                .set_corner_radius(8.0f)
                .set_title(std::string{card_title_texts[i]})
                .set_title_color(color4_to_nancolor(110, 112, 130))
                .set_show_accent(true)
                .set_accent_color(color4_to_nancolor(cr, cg, cb));

            auto value = Label::create();
            value->set_text(std::string{card_value_texts[i]})
                .set_font_size(16.0f)
                .set_color(color4_to_nancolor(220, 220, 240));
            card->add_child(std::move(value));

            m_cards[i] = add_child(std::move(card));

            auto pressable = Pressable::create();
            pressable->on_click([this, i]() {
                m_log.info("Card {} clicked", i);
            });
            pressable->on_hover([this, i]() {
                m_log.debug("Card {} hovered", i);
            });
            m_pressables[i] = add_child(std::move(pressable));
        }
    }

    std::array<runtime::NanWidget*, 4> m_cards{};
    std::array<runtime::NanWidget*, 4> m_pressables{};
    decltype(nandina::log::get("")) m_log{nandina::log::get("showcase.stats")};
};

} // namespace nandina::showcase