module;

#include <memory>

export module nandina.showcase.bottom_content_section;

import nandina.widgets.split_row;
import nandina.showcase.project_progress_card;
import nandina.showcase.stack_demo_card;

export namespace nandina::showcase {

class BottomContentSection final : public nandina::widgets::SplitRow {
public:
    using Ptr = std::unique_ptr<BottomContentSection>;

    static auto create() -> Ptr {
        return Ptr{new BottomContentSection()};
    }

private:
    BottomContentSection() {
        set_gap(16.0f)
            .set_split_ratio(0.5f)
            .set_preferred_height(110.0f);

        auto stack_demo = StackDemoCard::create();
        m_stack_demo = stack_demo.get();
        set_leading(std::move(stack_demo));

        auto progress_card = ProjectProgressCard::create();
        m_progress_card = progress_card.get();
        set_trailing(std::move(progress_card));
    }

    StackDemoCard* m_stack_demo{nullptr};
    ProjectProgressCard* m_progress_card{nullptr};
};

} // namespace nandina::showcase