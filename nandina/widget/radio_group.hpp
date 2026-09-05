//
// widget/radio_group - shared mutual-exclusion state for RadioButton members.
//
// A plain coordinating object (not a node): it tracks member registration order,
// enforces single selection, and drives arrow-key roving focus. Radios own the
// group via shared_ptr, so the group lives exactly as long as its members.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_RADIO_GROUP_HPP
#define NANDINA_EXPERIMENT_WIDGET_RADIO_GROUP_HPP

#include "../reactive/event.hpp"

#include <memory>
#include <vector>

namespace nandina::widget
{
    class RadioButton;

    class RadioGroup {
    public:
        RadioGroup() = default;

        [[nodiscard]] static auto create() -> std::shared_ptr<RadioGroup>;

        /// 成员注册（RadioButton 构造/析构时调用，保持注册顺序 = 视觉顺序）。
        void register_radio(RadioButton* radio);
        void unregister_radio(RadioButton* radio);

        /// 互斥选择：取消上一选中项，选中 `radio`（已选中则为 no-op）。
        void select(RadioButton* radio);
        [[nodiscard]] auto selected() const -> RadioButton*;
        /// 当前选中索引（-1 表示无选中）。
        [[nodiscard]] auto selected_index() const -> int;

        /// 方向键漫游：从 `from` 移动 focus+selection（direction -1 上一个 / +1 下一个，循环）。
        [[nodiscard]] auto move_focus(RadioButton* from, int direction) -> bool;

        /// 选中索引变化事件（用户或程序触发）。
        [[nodiscard]] auto selection_changed() const -> const reactive::Event<int>&;

    private:
        [[nodiscard]] auto index_of(const RadioButton* radio) const -> int;

        std::vector<RadioButton*> members_;
        RadioButton* selected_ = nullptr;
        reactive::Event<int> selection_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_RADIO_GROUP_HPP
