//
// widget/radio_group - shared mutual-exclusion state for RadioButton members.
//

#include "radio_group.hpp"

#include "radio_button.hpp"

#include <algorithm>

namespace nandina::widget
{
    auto RadioGroup::create() -> std::shared_ptr<RadioGroup> {
        return std::make_shared<RadioGroup>();
    }

    void RadioGroup::register_radio(RadioButton* radio) {
        if (radio == nullptr
            || std::ranges::find(members_, radio) != members_.end())
        {
            return;
        }
        members_.push_back(radio);
        focus_.sync(members_.size());
    }

    void RadioGroup::unregister_radio(RadioButton* radio) {
        if (radio == selected_) {
            selected_ = nullptr;
        }
        std::erase(members_, radio);
        focus_.sync(members_.size());
    }

    void RadioGroup::select(RadioButton* radio) {
        if (radio == nullptr || radio == selected_) {
            return;
        }
        auto* previous = selected_;
        selected_ = radio;
        if (previous != nullptr) {
            previous->set_checked(false);
        }
        radio->set_checked(true);
        selection_changed_.emit(selected_index());
    }

    auto RadioGroup::selected() const -> RadioButton* {
        return selected_;
    }

    auto RadioGroup::selected_index() const -> int {
        return index_of(selected_);
    }

    auto RadioGroup::move_focus(RadioButton* from, const int direction) -> bool {
        if (members_.empty() || direction == 0) {
            return false;
        }
        const int current = index_of(from);
        if (current < 0) {
            return false;
        }
        focus_.set_movement(RovingMovement::focus_and_selection);
        focus_.set_active_index(current);
        // 方向语义由共享设施统一（step 沿配置轴走一步），这里只负责把索引映射回成员
        // 并按 Intent 的两个独立决定落地：选中跟随 + 控件焦点移动。
        const auto intent = focus_.step(direction);
        if (!intent.has_value() || intent->index < 0) {
            return false;
        }
        auto* target = members_[static_cast<std::size_t>(intent->index)];
        if (intent->selection_follows_focus) {
            select(target);
        }
        if (intent->move_widget_focus) {
            target->request_focus();
        }
        return true;
    }

    auto RadioGroup::selection_changed() const -> const reactive::Event<int>& {
        return selection_changed_;
    }

    auto RadioGroup::index_of(const RadioButton* radio) const -> int {
        for (std::size_t i = 0; i < members_.size(); ++i) {
            if (members_[i] == radio) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
} // namespace nandina::widget
