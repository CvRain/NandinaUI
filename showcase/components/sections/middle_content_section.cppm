module;

#include <memory>

export module nandina.showcase.middle_content_section;

import nandina.widgets.split_row;
import nandina.showcase.chart_card;
import nandina.showcase.recent_activity_card;

export namespace nandina::showcase {

class MiddleContentSection final : public nandina::widgets::SplitRow {
public:
    using Ptr = std::unique_ptr<MiddleContentSection>;

    static auto create() -> Ptr {
        return Ptr{new MiddleContentSection()};
    }

    auto set_activity_pulse(const float pulse) noexcept -> void {
        if (m_activity_card) {
            m_activity_card->set_pulse(pulse);
        }
    }

private:
    MiddleContentSection() {
        set_gap(16.0f)
            .set_split_ratio(0.6f)
            .set_preferred_height(200.0f);

        auto chart_card = ChartCard::create();
        m_chart_card = chart_card.get();
        set_leading(std::move(chart_card));

        auto activity_card = RecentActivityCard::create();
        m_activity_card = activity_card.get();
        set_trailing(std::move(activity_card));
    }

    ChartCard* m_chart_card{nullptr};
    RecentActivityCard* m_activity_card{nullptr};
};

} // namespace nandina::showcase